/**
 * @file llm_rknn_internal.h
 * @brief Internal implementation header for LLM-RKNN library
 * 
 * This header contains internal structures and declarations not exposed
 * to the public API.
 */

#ifndef LLM_RKNN_INTERNAL_H
#define LLM_RKNN_INTERNAL_H

#include "llm_rknn.h"
#include "rknn_api.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

namespace llm_rknn {

/**
 * @brief KV Cache for a single layer
 */
struct KVCache {
    std::vector<float> key_cache;      // Shape: [max_seq_len, num_heads, head_dim]
    std::vector<float> value_cache;    // Shape: [max_seq_len, num_heads, head_dim]
    uint32_t current_length;           // Current cached sequence length
    uint32_t max_length;               // Maximum cache capacity
    uint32_t num_heads;
    uint32_t head_dim;
    
    KVCache() : current_length(0), max_length(0), num_heads(0), head_dim(0) {}
    
    void init(uint32_t max_len, uint32_t n_heads, uint32_t h_dim);
    void reset();
    void append(const float* key_data, const float* value_data, uint32_t seq_len);
};

/**
 * @brief Tokenizer interface
 */
class ITokenizer {
public:
    virtual ~ITokenizer() = default;
    
    /**
     * @brief Encode text to token IDs
     */
    virtual std::vector<int32_t> encode(const std::string& text, bool add_bos = true) = 0;
    
    /**
     * @brief Decode token IDs to text
     */
    virtual std::string decode(const std::vector<int32_t>& tokens) = 0;
    
    /**
     * @brief Decode a single token
     */
    virtual std::string decode_token(int32_t token_id) = 0;
    
    /**
     * @brief Get vocabulary size
     */
    virtual int32_t vocab_size() const = 0;
    
    /**
     * @brief Get special token IDs
     */
    virtual int32_t bos_token_id() const = 0;
    virtual int32_t eos_token_id() const = 0;
    virtual int32_t pad_token_id() const = 0;
};

/**
 * @brief Sampler for token selection
 */
class Sampler {
public:
    explicit Sampler(const llm_rknn_config_t& config);
    
    /**
     * @brief Sample next token from logits
     * 
     * @param logits Probability distribution over vocabulary
     * @param vocab_size Size of vocabulary
     * @return Selected token ID
     */
    int32_t sample(const float* logits, int32_t vocab_size);
    
    /**
     * @brief Get log probability of last sampled token
     */
    float get_last_logprob() const { return last_logprob_; }
    
private:
    llm_rknn_sampling_method_t method_;
    float temperature_;
    uint32_t top_k_;
    float top_p_;
    float repetition_penalty_;
    float last_logprob_;
    
    // Helper functions
    int32_t sample_greedy(const float* logits, int32_t vocab_size);
    int32_t sample_top_k(const float* logits, int32_t vocab_size);
    int32_t sample_top_p(const float* logits, int32_t vocab_size);
    int32_t sample_top_k_top_p(const float* logits, int32_t vocab_size);
    
    void apply_temperature(std::vector<float>& logits);
    void apply_softmax(std::vector<float>& logits);
};

/**
 * @brief Main LLM context implementation
 */
class LLMContext {
public:
    LLMContext();
    ~LLMContext();
    
    /**
     * @brief Initialize the LLM context
     */
    int init(const char* rknn_model_path, 
             const char* tokenizer_path, 
             llm_rknn_config_t* config);
    
    /**
     * @brief Generate text from prompt
     */
    int generate(const char* prompt, 
                 llm_rknn_callback_t callback,
                 void* user_data);
    
    /**
     * @brief Generate with extended callback
     */
    int generate_ex(const char* prompt,
                    llm_rknn_callback_ex_t callback,
                    void* user_data);
    
    /**
     * @brief Get performance statistics
     */
    void get_perf_stats(llm_rknn_perf_stats_t* stats);
    
    /**
     * @brief Reset conversation state
     */
    void reset();
    
    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }
    
private:
    // RKNN runtime context
    rknn_context rknn_ctx_;
    rknn_input_output_num io_num_;
    rknn_tensor_attr* input_attrs_;
    rknn_tensor_attr* output_attrs_;
    
    // Tokenizer
    std::unique_ptr<ITokenizer> tokenizer_;
    
    // Sampler
    std::unique_ptr<Sampler> sampler_;
    
    // KV Cache (one per transformer layer)
    std::vector<KVCache> kv_caches_;
    
    // Configuration
    llm_rknn_config_t config_;
    
    // State
    bool initialized_;
    uint32_t current_seq_length_;
    std::vector<int32_t> generated_tokens_;
    
    // Performance tracking
    llm_rknn_perf_stats_t perf_stats_;
    
    // Thread safety
    std::mutex mutex_;
    
    // Private methods
    int load_rknn_model(const char* model_path);
    int init_tokenizer(const char* tokenizer_path);
    int init_kv_cache();
    
    int prefill(const std::vector<int32_t>& prompt_tokens);
    int decode_step(int32_t token_id, float* output_logits);
    
    int run_inference(const std::vector<int32_t>& input_tokens,
                      float* output_logits,
                      bool is_prefill);
    
    void update_perf_stats(double time_ms, bool is_prefill);
};

} // namespace llm_rknn

/**
 * @brief C++ to C handle conversion
 */
inline llm_rknn::LLMContext* to_context(llm_rknn_handle_t handle) {
    return static_cast<llm_rknn::LLMContext*>(handle);
}

#endif // LLM_RKNN_INTERNAL_H
