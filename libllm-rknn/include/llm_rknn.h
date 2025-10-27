/**
 * @file llm_rknn.h
 * @brief High-level C/C++ API for LLM inference on Rockchip NPUs using RKNN Runtime
 * 
 * This library provides a simple, high-level interface for running conversational
 * Large Language Models on Rockchip SoCs with NPU acceleration. It abstracts away
 * the complexities of tokenization, KV cache management, and auto-regressive
 * inference loops.
 * 
 * @version 1.0.0
 * @date 2025-10-09
 * 
 * @copyright Copyright (c) 2025
 * Licensed under the Apache License, Version 2.0
 */

#ifndef LLM_RKNN_H
#define LLM_RKNN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Library version information
 */
#define LLM_RKNN_VERSION_MAJOR 1
#define LLM_RKNN_VERSION_MINOR 0
#define LLM_RKNN_VERSION_PATCH 0

/**
 * @brief Error codes returned by API functions
 */
typedef enum {
    LLM_RKNN_SUCCESS = 0,              ///< Operation completed successfully
    LLM_RKNN_ERROR_INVALID_PARAM = -1, ///< Invalid parameter provided
    LLM_RKNN_ERROR_MODEL_LOAD = -2,    ///< Failed to load RKNN model
    LLM_RKNN_ERROR_TOKENIZER = -3,     ///< Tokenizer initialization/operation failed
    LLM_RKNN_ERROR_MEMORY = -4,        ///< Memory allocation failed
    LLM_RKNN_ERROR_NPU = -5,           ///< NPU/RKNN runtime error
    LLM_RKNN_ERROR_INFERENCE = -6,     ///< Inference execution error
    LLM_RKNN_ERROR_INVALID_HANDLE = -7,///< Invalid handle provided
    LLM_RKNN_ERROR_NOT_INITIALIZED = -8,///< System not initialized
    LLM_RKNN_ERROR_UNKNOWN = -99       ///< Unknown error occurred
} llm_rknn_error_t;

/**
 * @brief Sampling methods for token generation
 */
typedef enum {
    LLM_RKNN_SAMPLING_GREEDY = 0,   ///< Always select the most probable token
    LLM_RKNN_SAMPLING_TOP_K = 1,    ///< Sample from top-k most probable tokens
    LLM_RKNN_SAMPLING_TOP_P = 2,    ///< Nucleus sampling (top-p)
    LLM_RKNN_SAMPLING_TOP_K_TOP_P = 3 ///< Combined top-k and top-p sampling
} llm_rknn_sampling_method_t;

/**
 * @brief Opaque handle to an LLM-RKNN instance
 */
typedef void* llm_rknn_handle_t;

/**
 * @brief Configuration structure for LLM initialization
 */
typedef struct {
    // Generation parameters
    uint32_t max_context_length;      ///< Maximum context length (tokens), default: 2048
    uint32_t max_new_tokens;          ///< Maximum number of new tokens to generate, default: 512
    
    // Sampling parameters
    llm_rknn_sampling_method_t sampling_method; ///< Sampling method, default: GREEDY
    float temperature;                ///< Temperature for sampling (0.0 to 2.0), default: 1.0
    uint32_t top_k;                   ///< Top-k value for sampling, default: 50
    float top_p;                      ///< Top-p value for nucleus sampling, default: 0.9
    float repetition_penalty;         ///< Penalty for token repetition, default: 1.0
    
    // System parameters
    uint32_t num_threads;             ///< Number of CPU threads for preprocessing, default: 4
    bool enable_profiling;            ///< Enable performance profiling, default: false
    
    // Special tokens
    int32_t bos_token_id;             ///< Beginning of sequence token ID, -1 for auto-detect
    int32_t eos_token_id;             ///< End of sequence token ID, -1 for auto-detect
    int32_t pad_token_id;             ///< Padding token ID, -1 for auto-detect
    
    // Advanced RKNN parameters
    uint32_t rknn_core_mask;          ///< NPU core mask (0 = auto, 1 = core0, 2 = core1, 3 = both)
    bool enable_kv_cache;             ///< Enable KV cache optimization, default: true
} llm_rknn_config_t;

/**
 * @brief Callback function type for streaming token generation
 * 
 * This callback is invoked for each generated token during the inference process.
 * 
 * @param token_text The decoded text of the generated token
 * @param token_id The token ID
 * @param user_data User-provided data pointer passed during generation
 * @return 0 to continue generation, non-zero to stop generation early
 */
typedef int (*llm_rknn_callback_t)(const char* token_text, int32_t token_id, void* user_data);

/**
 * @brief Extended callback function type with additional metadata
 * 
 * @param token_text The decoded text of the generated token
 * @param token_id The token ID
 * @param logprob Log probability of the token
 * @param is_final Whether this is the final token (EOS reached)
 * @param user_data User-provided data pointer
 * @return 0 to continue generation, non-zero to stop
 */
typedef int (*llm_rknn_callback_ex_t)(const char* token_text, int32_t token_id, 
                                       float logprob, bool is_final, void* user_data);

/**
 * @brief Performance statistics structure
 */
typedef struct {
    uint32_t total_tokens_generated;   ///< Total number of tokens generated
    double prefill_time_ms;            ///< Time spent on prompt processing (ms)
    double decode_time_ms;             ///< Time spent on token generation (ms)
    double total_time_ms;              ///< Total inference time (ms)
    double tokens_per_second;          ///< Generation speed (tokens/second)
    uint64_t peak_memory_usage_bytes;  ///< Peak memory usage in bytes
} llm_rknn_perf_stats_t;

/**
 * @brief Initialize default configuration
 * 
 * Fills the configuration structure with sensible default values.
 * 
 * @param config Pointer to configuration structure to initialize
 * @return LLM_RKNN_SUCCESS on success, error code otherwise
 */
int llm_rknn_init_default_config(llm_rknn_config_t* config);

/**
 * @brief Initialize LLM-RKNN runtime
 * 
 * Loads the RKNN model and tokenizer, initializes the NPU context,
 * and prepares all necessary resources for LLM inference.
 * 
 * @param rknn_model_path Path to the .rknn model file
 * @param tokenizer_path Path to the tokenizer model file (e.g., tokenizer.model for SentencePiece)
 * @param config Configuration structure (NULL for defaults)
 * @return Handle to the initialized LLM instance, or NULL on failure
 */
llm_rknn_handle_t llm_rknn_init(const char* rknn_model_path, 
                                 const char* tokenizer_path, 
                                 llm_rknn_config_t* config);

/**
 * @brief Generate text from a prompt
 * 
 * Performs auto-regressive text generation using the loaded model.
 * Tokens are streamed back to the caller via the callback function.
 * 
 * @param handle Handle to initialized LLM instance
 * @param prompt Input text prompt
 * @param callback Callback function for receiving generated tokens
 * @param user_data User data to pass to callback
 * @return LLM_RKNN_SUCCESS on success, error code otherwise
 */
int llm_rknn_generate(llm_rknn_handle_t handle, 
                       const char* prompt, 
                       llm_rknn_callback_t callback,
                       void* user_data);

/**
 * @brief Generate text with extended callback
 * 
 * Similar to llm_rknn_generate but uses extended callback with more metadata.
 * 
 * @param handle Handle to initialized LLM instance
 * @param prompt Input text prompt
 * @param callback Extended callback function
 * @param user_data User data to pass to callback
 * @return LLM_RKNN_SUCCESS on success, error code otherwise
 */
int llm_rknn_generate_ex(llm_rknn_handle_t handle,
                          const char* prompt,
                          llm_rknn_callback_ex_t callback,
                          void* user_data);

/**
 * @brief Get performance statistics from last generation
 * 
 * @param handle Handle to initialized LLM instance
 * @param stats Pointer to structure to receive statistics
 * @return LLM_RKNN_SUCCESS on success, error code otherwise
 */
int llm_rknn_get_perf_stats(llm_rknn_handle_t handle, llm_rknn_perf_stats_t* stats);

/**
 * @brief Reset conversation history
 * 
 * Clears the KV cache and resets the conversation state.
 * 
 * @param handle Handle to initialized LLM instance
 * @return LLM_RKNN_SUCCESS on success, error code otherwise
 */
int llm_rknn_reset(llm_rknn_handle_t handle);

/**
 * @brief Release LLM-RKNN resources
 * 
 * Frees all allocated resources including NPU context, memory caches,
 * and tokenizer. The handle becomes invalid after this call.
 * 
 * @param handle Handle to initialized LLM instance
 */
void llm_rknn_release(llm_rknn_handle_t handle);

/**
 * @brief Get error string for error code
 * 
 * @param error_code Error code
 * @return Human-readable error string
 */
const char* llm_rknn_get_error_string(int error_code);

/**
 * @brief Get library version string
 * 
 * @return Version string in format "major.minor.patch"
 */
const char* llm_rknn_get_version(void);

#ifdef __cplusplus
}
#endif

#endif // LLM_RKNN_H
