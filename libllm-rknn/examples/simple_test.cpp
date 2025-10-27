/**
 * @file simple_test.cpp
 * @brief Simple test program for libllm-rknn
 */

#include "llm_rknn.h"
#include <stdio.h>
#include <string>

int simple_callback(const char* token_text, int32_t token_id, void* user_data) {
    printf("%s", token_text);
    fflush(stdout);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <model.rknn> <tokenizer.model>\n", argv[0]);
        return 1;
    }
    
    const char* model_path = argv[1];
    const char* tokenizer_path = argv[2];
    
    printf("LLM-RKNN Simple Test\n");
    printf("====================\n\n");
    
    // Initialize with default config
    printf("Initializing...\n");
    llm_rknn_handle_t handle = llm_rknn_init(model_path, tokenizer_path, nullptr);
    
    if (!handle) {
        printf("Failed to initialize!\n");
        return 1;
    }
    
    printf("Initialization successful!\n\n");
    
    // Test prompt
    const char* prompt = "Once upon a time";
    printf("Prompt: %s\n", prompt);
    printf("Response: ");
    
    int ret = llm_rknn_generate(handle, prompt, simple_callback, nullptr);
    
    printf("\n\n");
    
    if (ret == LLM_RKNN_SUCCESS) {
        llm_rknn_perf_stats_t stats;
        llm_rknn_get_perf_stats(handle, &stats);
        
        printf("Statistics:\n");
        printf("  Tokens: %u\n", stats.total_tokens_generated);
        printf("  Speed:  %.2f tok/s\n", stats.tokens_per_second);
    } else {
        printf("Error: %s\n", llm_rknn_get_error_string(ret));
    }
    
    llm_rknn_release(handle);
    
    return 0;
}
