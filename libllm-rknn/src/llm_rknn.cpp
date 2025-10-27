/**
 * @file llm_rknn.cpp
 * @brief Main implementation of LLM-RKNN public API
 */

#include "llm_rknn.h"
#include "llm_rknn_internal.h"
#include <cstring>
#include <cstdio>

// Default configuration values
static const llm_rknn_config_t DEFAULT_CONFIG = {
    .max_context_length = 2048,
    .max_new_tokens = 512,
    .sampling_method = LLM_RKNN_SAMPLING_GREEDY,
    .temperature = 1.0f,
    .top_k = 50,
    .top_p = 0.9f,
    .repetition_penalty = 1.0f,
    .num_threads = 4,
    .enable_profiling = false,
    .bos_token_id = -1,
    .eos_token_id = -1,
    .pad_token_id = -1,
    .rknn_core_mask = 0,
    .enable_kv_cache = true
};

int llm_rknn_init_default_config(llm_rknn_config_t* config) {
    if (config == nullptr) {
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    memcpy(config, &DEFAULT_CONFIG, sizeof(llm_rknn_config_t));
    return LLM_RKNN_SUCCESS;
}

llm_rknn_handle_t llm_rknn_init(const char* rknn_model_path, 
                                 const char* tokenizer_path, 
                                 llm_rknn_config_t* config) {
    if (rknn_model_path == nullptr || tokenizer_path == nullptr) {
        fprintf(stderr, "[LLM-RKNN] Error: NULL path provided\n");
        return nullptr;
    }
    
    // Create context
    llm_rknn::LLMContext* ctx = new llm_rknn::LLMContext();
    
    // Initialize with config
    int ret = ctx->init(rknn_model_path, tokenizer_path, config);
    if (ret != LLM_RKNN_SUCCESS) {
        delete ctx;
        return nullptr;
    }
    
    return static_cast<llm_rknn_handle_t>(ctx);
}

int llm_rknn_generate(llm_rknn_handle_t handle, 
                       const char* prompt, 
                       llm_rknn_callback_t callback,
                       void* user_data) {
    if (handle == nullptr || prompt == nullptr || callback == nullptr) {
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    llm_rknn::LLMContext* ctx = to_context(handle);
    if (!ctx->is_initialized()) {
        return LLM_RKNN_ERROR_NOT_INITIALIZED;
    }
    
    return ctx->generate(prompt, callback, user_data);
}

int llm_rknn_generate_ex(llm_rknn_handle_t handle,
                          const char* prompt,
                          llm_rknn_callback_ex_t callback,
                          void* user_data) {
    if (handle == nullptr || prompt == nullptr || callback == nullptr) {
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    llm_rknn::LLMContext* ctx = to_context(handle);
    if (!ctx->is_initialized()) {
        return LLM_RKNN_ERROR_NOT_INITIALIZED;
    }
    
    return ctx->generate_ex(prompt, callback, user_data);
}

int llm_rknn_get_perf_stats(llm_rknn_handle_t handle, llm_rknn_perf_stats_t* stats) {
    if (handle == nullptr || stats == nullptr) {
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    llm_rknn::LLMContext* ctx = to_context(handle);
    if (!ctx->is_initialized()) {
        return LLM_RKNN_ERROR_NOT_INITIALIZED;
    }
    
    ctx->get_perf_stats(stats);
    return LLM_RKNN_SUCCESS;
}

int llm_rknn_reset(llm_rknn_handle_t handle) {
    if (handle == nullptr) {
        return LLM_RKNN_ERROR_INVALID_PARAM;
    }
    
    llm_rknn::LLMContext* ctx = to_context(handle);
    if (!ctx->is_initialized()) {
        return LLM_RKNN_ERROR_NOT_INITIALIZED;
    }
    
    ctx->reset();
    return LLM_RKNN_SUCCESS;
}

void llm_rknn_release(llm_rknn_handle_t handle) {
    if (handle == nullptr) {
        return;
    }
    
    llm_rknn::LLMContext* ctx = to_context(handle);
    delete ctx;
}

const char* llm_rknn_get_error_string(int error_code) {
    switch (error_code) {
        case LLM_RKNN_SUCCESS:
            return "Success";
        case LLM_RKNN_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case LLM_RKNN_ERROR_MODEL_LOAD:
            return "Failed to load model";
        case LLM_RKNN_ERROR_TOKENIZER:
            return "Tokenizer error";
        case LLM_RKNN_ERROR_MEMORY:
            return "Memory allocation failed";
        case LLM_RKNN_ERROR_NPU:
            return "NPU/RKNN runtime error";
        case LLM_RKNN_ERROR_INFERENCE:
            return "Inference execution error";
        case LLM_RKNN_ERROR_INVALID_HANDLE:
            return "Invalid handle";
        case LLM_RKNN_ERROR_NOT_INITIALIZED:
            return "System not initialized";
        default:
            return "Unknown error";
    }
}

const char* llm_rknn_get_version(void) {
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d", 
             LLM_RKNN_VERSION_MAJOR, 
             LLM_RKNN_VERSION_MINOR, 
             LLM_RKNN_VERSION_PATCH);
    return version;
}
