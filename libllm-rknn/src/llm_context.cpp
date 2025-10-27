/**
 * @file llm_context.cpp
 * @brief LLM context implementation
 */

#include "llm_rknn_internal.h"
#include "tokenizer_sentencepiece.h"
#include <cstring>
#include <cstdio>
#include <chrono>
#include <algorithm>

namespace llm_rknn {

LLMContext::LLMContext()
    : rknn_ctx_(0)
    , input_attrs_(nullptr)
    , output_attrs_(nullptr)
    , initialized_(false)
    , current_seq_length_(0) {
    memset(&io_num_, 0, sizeof(io_num_));
    memset(&config_, 0, sizeof(config_));
    memset(&perf_stats_, 0, sizeof(perf_stats_));
}

LLMContext::~LLMContext() {
    if (rknn_ctx_) {
        rknn_destroy(rknn_ctx_);
    }
    
    if (input_attrs_) {
        delete[] input_attrs_;
    }
    
    if (output_attrs_) {
        delete[] output_attrs_;
    }
}

int LLMContext::init(const char* rknn_model_path, 
                      const char* tokenizer_path, 
                      llm_rknn_config_t* config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        fprintf(stderr, "[LLM-RKNN] Already initialized\n");
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    // Use default config if not provided
    if (config == nullptr) {
        llm_rknn_init_default_config(&config_);
    } else {
        memcpy(&config_, config, sizeof(llm_rknn_config_t));
    }
    
    printf("[LLM-RKNN] Initializing LLM-RKNN v%s\n", llm_rknn_get_version());
    printf("[LLM-RKNN] Model: %s\n", rknn_model_path);
    printf("[LLM-RKNN] Tokenizer: %s\n", tokenizer_path);
    
    // Load RKNN model
    int ret = load_rknn_model(rknn_model_path);
    if (ret != LLM_RKNN_SUCCESS) {
        return ret;
    }
    
    // Initialize tokenizer
    ret = init_tokenizer(tokenizer_path);
    if (ret != LLM_RKNN_SUCCESS) {
        return ret;
    }
    
    // Initialize sampler
    sampler_ = std::make_unique<Sampler>(config_);
    
    // Initialize KV cache if enabled
    if (config_.enable_kv_cache) {
        ret = init_kv_cache();
        if (ret != LLM_RKNN_SUCCESS) {
            return ret;
        }
    }
    
    initialized_ = true;
    printf("[LLM-RKNN] Initialization complete\n");
    
    return LLM_RKNN_SUCCESS;
}

int LLMContext::load_rknn_model(const char* model_path) {
    printf("[LLM-RKNN] Loading RKNN model...\n");
    
    // Load model from file
    FILE* fp = fopen(model_path, "rb");
    if (fp == nullptr) {
        fprintf(stderr, "[LLM-RKNN] Failed to open model file: %s\n", model_path);
        return LLM_RKNN_ERROR_MODEL_LOAD;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t model_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    void* model_data = malloc(model_size);
    if (model_data == nullptr) {
        fclose(fp);
        fprintf(stderr, "[LLM-RKNN] Failed to allocate memory for model\n");
        return LLM_RKNN_ERROR_MEMORY;
    }
    
    size_t read_size = fread(model_data, 1, model_size, fp);
    fclose(fp);
    
    if (read_size != model_size) {
        free(model_data);
        fprintf(stderr, "[LLM-RKNN] Failed to read model file\n");
        return LLM_RKNN_ERROR_MODEL_LOAD;
    }
    
    // Initialize RKNN context
    int ret = rknn_init(&rknn_ctx_, model_data, model_size, 0, nullptr);
    free(model_data);
    
    if (ret != RKNN_SUCC) {
        fprintf(stderr, "[LLM-RKNN] Failed to init RKNN context, error=%d\n", ret);
        return LLM_RKNN_ERROR_NPU;
    }
    
    // Set core mask if specified
    if (config_.rknn_core_mask > 0) {
        rknn_core_mask core_mask = (rknn_core_mask)config_.rknn_core_mask;
        ret = rknn_set_core_mask(rknn_ctx_, core_mask);
        if (ret != RKNN_SUCC) {
            fprintf(stderr, "[LLM-RKNN] Warning: Failed to set core mask\n");
        }
    }
    
    // Query model I/O info
    ret = rknn_query(rknn_ctx_, RKNN_QUERY_IN_OUT_NUM, &io_num_, sizeof(io_num_));
    if (ret != RKNN_SUCC) {
        fprintf(stderr, "[LLM-RKNN] Failed to query I/O number\n");
        return LLM_RKNN_ERROR_NPU;
    }
    
    printf("[LLM-RKNN] Model inputs: %d, outputs: %d\n", io_num_.n_input, io_num_.n_output);
    
    // Query input attributes
    input_attrs_ = new rknn_tensor_attr[io_num_.n_input];
    for (uint32_t i = 0; i < io_num_.n_input; i++) {
        input_attrs_[i].index = i;
        ret = rknn_query(rknn_ctx_, RKNN_QUERY_INPUT_ATTR, &input_attrs_[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            fprintf(stderr, "[LLM-RKNN] Failed to query input %d attributes\n", i);
            return LLM_RKNN_ERROR_NPU;
        }
    }
    
    // Query output attributes
    output_attrs_ = new rknn_tensor_attr[io_num_.n_output];
    for (uint32_t i = 0; i < io_num_.n_output; i++) {
        output_attrs_[i].index = i;
        ret = rknn_query(rknn_ctx_, RKNN_QUERY_OUTPUT_ATTR, &output_attrs_[i], sizeof(rknn_tensor_attr));
        if (ret != RKNN_SUCC) {
            fprintf(stderr, "[LLM-RKNN] Failed to query output %d attributes\n", i);
            return LLM_RKNN_ERROR_NPU;
        }
    }
    
    return LLM_RKNN_SUCCESS;
}

int LLMContext::init_tokenizer(const char* tokenizer_path) {
    printf("[LLM-RKNN] Loading tokenizer...\n");
    
    // Create SentencePiece tokenizer
    auto sp_tokenizer = std::make_unique<SentencePieceTokenizer>();
    
    if (!sp_tokenizer->load(tokenizer_path)) {
        fprintf(stderr, "[LLM-RKNN] Failed to load tokenizer\n");
        return LLM_RKNN_ERROR_TOKENIZER;
    }
    
    tokenizer_ = std::move(sp_tokenizer);
    
    // Update special token IDs if not set in config
    if (config_.bos_token_id < 0) {
        config_.bos_token_id = tokenizer_->bos_token_id();
    }
    if (config_.eos_token_id < 0) {
        config_.eos_token_id = tokenizer_->eos_token_id();
    }
    if (config_.pad_token_id < 0) {
        config_.pad_token_id = tokenizer_->pad_token_id();
    }
    
    printf("[LLM-RKNN] Tokenizer loaded, vocab_size=%d\n", tokenizer_->vocab_size());
    printf("[LLM-RKNN] BOS=%d, EOS=%d, PAD=%d\n", 
           config_.bos_token_id, config_.eos_token_id, config_.pad_token_id);
    
    return LLM_RKNN_SUCCESS;
}

int LLMContext::init_kv_cache() {
    printf("[LLM-RKNN] Initializing KV cache...\n");
    
    // For now, we'll create a simple placeholder
    // In a real implementation, this would be based on model architecture
    // Typically: num_layers * 2 (key + value) cache buffers
    
    // This is a simplified version - actual implementation depends on model
    uint32_t num_layers = 32;  // Should be detected from model
    uint32_t num_heads = 32;   // Should be detected from model
    uint32_t head_dim = 128;   // Should be detected from model
    
    kv_caches_.resize(num_layers);
    for (auto& cache : kv_caches_) {
        cache.init(config_.max_context_length, num_heads, head_dim);
    }
    
    printf("[LLM-RKNN] KV cache initialized for %d layers\n", num_layers);
    
    return LLM_RKNN_SUCCESS;
}

int LLMContext::generate(const char* prompt, 
                          llm_rknn_callback_t callback,
                          void* user_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Reset performance stats
    memset(&perf_stats_, 0, sizeof(perf_stats_));
    
    // Encode prompt
    printf("[LLM-RKNN] Encoding prompt...\n");
    std::vector<int32_t> prompt_tokens = tokenizer_->encode(prompt, true);
    printf("[LLM-RKNN] Prompt tokens: %zu\n", prompt_tokens.size());
    
    // Prefill phase
    printf("[LLM-RKNN] Running prefill...\n");
    auto prefill_start = std::chrono::high_resolution_clock::now();
    
    int ret = prefill(prompt_tokens);
    if (ret != LLM_RKNN_SUCCESS) {
        return ret;
    }
    
    auto prefill_end = std::chrono::high_resolution_clock::now();
    perf_stats_.prefill_time_ms = std::chrono::duration<double, std::milli>(prefill_end - prefill_start).count();
    
    // Generation loop
    printf("[LLM-RKNN] Starting token generation...\n");
    auto decode_start = std::chrono::high_resolution_clock::now();
    
    generated_tokens_.clear();
    int32_t vocab_size = tokenizer_->vocab_size();
    std::vector<float> output_logits(vocab_size);
    
    for (uint32_t i = 0; i < config_.max_new_tokens; i++) {
        // Get next token (use last generated or last prompt token)
        int32_t input_token = (i == 0 && generated_tokens_.empty()) 
                              ? prompt_tokens.back() 
                              : generated_tokens_.back();
        
        // Run single decode step
        ret = decode_step(input_token, output_logits.data());
        if (ret != LLM_RKNN_SUCCESS) {
            return ret;
        }
        
        // Sample next token
        int32_t next_token = sampler_->sample(output_logits.data(), vocab_size);
        generated_tokens_.push_back(next_token);
        perf_stats_.total_tokens_generated++;
        
        // Decode token to text
        std::string token_text = tokenizer_->decode_token(next_token);
        
        // Call user callback
        int cb_ret = callback(token_text.c_str(), next_token, user_data);
        
        // Check for EOS or early stopping
        if (next_token == config_.eos_token_id || cb_ret != 0) {
            printf("[LLM-RKNN] Generation stopped (EOS or callback)\n");
            break;
        }
    }
    
    auto decode_end = std::chrono::high_resolution_clock::now();
    perf_stats_.decode_time_ms = std::chrono::duration<double, std::milli>(decode_end - decode_start).count();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    perf_stats_.total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    perf_stats_.tokens_per_second = (perf_stats_.total_tokens_generated * 1000.0) / perf_stats_.decode_time_ms;
    
    printf("[LLM-RKNN] Generation complete: %d tokens in %.2f ms (%.2f tok/s)\n",
           perf_stats_.total_tokens_generated, 
           perf_stats_.total_time_ms,
           perf_stats_.tokens_per_second);
    
    return LLM_RKNN_SUCCESS;
}

int LLMContext::generate_ex(const char* prompt,
                             llm_rknn_callback_ex_t callback,
                             void* user_data) {
    // Wrapper to convert extended callback to standard callback
    struct CallbackWrapper {
        llm_rknn_callback_ex_t callback;
        void* user_data;
        Sampler* sampler;
        int32_t eos_token_id;
        uint32_t token_count;
        uint32_t max_tokens;
    };
    
    CallbackWrapper wrapper{callback, user_data, sampler_.get(), 
                           config_.eos_token_id, 0, config_.max_new_tokens};
    
    auto simple_callback = [](const char* token_text, int32_t token_id, void* user_data) -> int {
        CallbackWrapper* w = static_cast<CallbackWrapper*>(user_data);
        float logprob = w->sampler->get_last_logprob();
        bool is_final = (token_id == w->eos_token_id) || (w->token_count >= w->max_tokens - 1);
        w->token_count++;
        return w->callback(token_text, token_id, logprob, is_final, w->user_data);
    };
    
    return generate(prompt, simple_callback, &wrapper);
}

int LLMContext::prefill(const std::vector<int32_t>& prompt_tokens) {
    // For simplicity, we run inference on the entire prompt
    // In production, this might be chunked for long sequences
    
    std::vector<float> dummy_output(tokenizer_->vocab_size());
    return run_inference(prompt_tokens, dummy_output.data(), true);
}

int LLMContext::decode_step(int32_t token_id, float* output_logits) {
    std::vector<int32_t> input_tokens = {token_id};
    return run_inference(input_tokens, output_logits, false);
}

int LLMContext::run_inference(const std::vector<int32_t>& input_tokens,
                               float* output_logits,
                               bool is_prefill) {
    // Prepare input tensors
    std::vector<rknn_input> inputs(io_num_.n_input);
    
    // For LLM, typically we have:
    // - input_ids (token IDs)
    // - attention_mask (optional)
    // - position_ids (optional)
    // - past_key_values (KV cache, optional)
    
    // Simplified: assume single input tensor for token IDs
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_INT32;
    inputs[0].size = input_tokens.size() * sizeof(int32_t);
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = (void*)input_tokens.data();
    
    int ret = rknn_inputs_set(rknn_ctx_, io_num_.n_input, inputs.data());
    if (ret != RKNN_SUCC) {
        fprintf(stderr, "[LLM-RKNN] Failed to set inputs, error=%d\n", ret);
        return LLM_RKNN_ERROR_NPU;
    }
    
    // Run inference
    ret = rknn_run(rknn_ctx_, nullptr);
    if (ret != RKNN_SUCC) {
        fprintf(stderr, "[LLM-RKNN] Failed to run inference, error=%d\n", ret);
        return LLM_RKNN_ERROR_INFERENCE;
    }
    
    // Get outputs
    std::vector<rknn_output> outputs(io_num_.n_output);
    for (uint32_t i = 0; i < io_num_.n_output; i++) {
        outputs[i].index = i;
        outputs[i].want_float = 1;  // Request float output
    }
    
    ret = rknn_outputs_get(rknn_ctx_, io_num_.n_output, outputs.data(), nullptr);
    if (ret != RKNN_SUCC) {
        fprintf(stderr, "[LLM-RKNN] Failed to get outputs, error=%d\n", ret);
        return LLM_RKNN_ERROR_INFERENCE;
    }
    
    // Copy logits (assume first output is the logits)
    if (outputs[0].buf && outputs[0].size >= tokenizer_->vocab_size() * sizeof(float)) {
        memcpy(output_logits, outputs[0].buf, tokenizer_->vocab_size() * sizeof(float));
    }
    
    // Release outputs
    rknn_outputs_release(rknn_ctx_, io_num_.n_output, outputs.data());
    
    return LLM_RKNN_SUCCESS;
}

void LLMContext::get_perf_stats(llm_rknn_perf_stats_t* stats) {
    if (stats) {
        memcpy(stats, &perf_stats_, sizeof(llm_rknn_perf_stats_t));
    }
}

void LLMContext::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    current_seq_length_ = 0;
    generated_tokens_.clear();
    
    // Reset KV caches
    for (auto& cache : kv_caches_) {
        cache.reset();
    }
    
    memset(&perf_stats_, 0, sizeof(perf_stats_));
}

// KVCache implementation
void KVCache::init(uint32_t max_len, uint32_t n_heads, uint32_t h_dim) {
    max_length = max_len;
    num_heads = n_heads;
    head_dim = h_dim;
    current_length = 0;
    
    size_t cache_size = max_length * num_heads * head_dim;
    key_cache.resize(cache_size, 0.0f);
    value_cache.resize(cache_size, 0.0f);
}

void KVCache::reset() {
    current_length = 0;
    std::fill(key_cache.begin(), key_cache.end(), 0.0f);
    std::fill(value_cache.begin(), value_cache.end(), 0.0f);
}

void KVCache::append(const float* key_data, const float* value_data, uint32_t seq_len) {
    if (current_length + seq_len > max_length) {
        fprintf(stderr, "[LLM-RKNN] Warning: KV cache overflow\n");
        return;
    }
    
    size_t elem_per_token = num_heads * head_dim;
    size_t offset = current_length * elem_per_token;
    size_t copy_size = seq_len * elem_per_token;
    
    memcpy(&key_cache[offset], key_data, copy_size * sizeof(float));
    memcpy(&value_cache[offset], value_data, copy_size * sizeof(float));
    
    current_length += seq_len;
}

} // namespace llm_rknn
