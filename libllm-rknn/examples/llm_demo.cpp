/**
 * @file llm_demo.cpp
 * @brief Demo application showcasing libllm-rknn usage
 * 
 * This demo creates a simple command-line chatbot interface that uses
 * the libllm-rknn library to run LLM inference on Rockchip NPUs.
 */

#include "llm_rknn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Global flag for interrupt handling
static volatile bool g_interrupted = false;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        printf("\n[Demo] Interrupt received, stopping...\n");
        g_interrupted = true;
    }
}

// Callback function for receiving generated tokens
int token_callback(const char* token_text, int32_t token_id, void* user_data) {
    // Print token immediately (streaming output)
    printf("%s", token_text);
    fflush(stdout);
    
    // Check for interrupt
    if (g_interrupted) {
        return 1;  // Non-zero to stop generation
    }
    
    return 0;  // Continue generation
}

// Extended callback with metadata
int token_callback_ex(const char* token_text, int32_t token_id, 
                      float logprob, bool is_final, void* user_data) {
    printf("%s", token_text);
    fflush(stdout);
    
    if (is_final) {
        printf("\n[Token ID: %d, LogProb: %.4f]\n", token_id, logprob);
    }
    
    if (g_interrupted) {
        return 1;
    }
    
    return 0;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -m MODEL      Path to .rknn model file (required)\n");
    printf("  -t TOKENIZER  Path to tokenizer model file (required)\n");
    printf("  -p PROMPT     Initial prompt (optional, interactive mode if not provided)\n");
    printf("  -n MAX_TOKENS Maximum tokens to generate (default: 512)\n");
    printf("  -s METHOD     Sampling method: greedy|topk|topp|topk_topp (default: greedy)\n");
    printf("  -T TEMP       Temperature (default: 1.0)\n");
    printf("  -k TOP_K      Top-k value (default: 50)\n");
    printf("  -P TOP_P      Top-p value (default: 0.9)\n");
    printf("  -e            Use extended callback with metadata\n");
    printf("  -h            Show this help message\n");
    printf("\nExample:\n");
    printf("  %s -m model.rknn -t tokenizer.model -p \"Hello, how are you?\"\n", program_name);
    printf("  %s -m model.rknn -t tokenizer.model -s topk -T 0.8 -k 40\n", program_name);
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    const char* model_path = nullptr;
    const char* tokenizer_path = nullptr;
    const char* initial_prompt = nullptr;
    bool use_extended_callback = false;
    
    llm_rknn_config_t config;
    llm_rknn_init_default_config(&config);
    
    int opt;
    while ((opt = getopt(argc, argv, "m:t:p:n:s:T:k:P:eh")) != -1) {
        switch (opt) {
            case 'm':
                model_path = optarg;
                break;
            case 't':
                tokenizer_path = optarg;
                break;
            case 'p':
                initial_prompt = optarg;
                break;
            case 'n':
                config.max_new_tokens = atoi(optarg);
                break;
            case 's':
                if (strcmp(optarg, "greedy") == 0) {
                    config.sampling_method = LLM_RKNN_SAMPLING_GREEDY;
                } else if (strcmp(optarg, "topk") == 0) {
                    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K;
                } else if (strcmp(optarg, "topp") == 0) {
                    config.sampling_method = LLM_RKNN_SAMPLING_TOP_P;
                } else if (strcmp(optarg, "topk_topp") == 0) {
                    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K_TOP_P;
                } else {
                    fprintf(stderr, "Unknown sampling method: %s\n", optarg);
                    return 1;
                }
                break;
            case 'T':
                config.temperature = atof(optarg);
                break;
            case 'k':
                config.top_k = atoi(optarg);
                break;
            case 'P':
                config.top_p = atof(optarg);
                break;
            case 'e':
                use_extended_callback = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Validate required arguments
    if (!model_path || !tokenizer_path) {
        fprintf(stderr, "Error: Model and tokenizer paths are required\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Print banner
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  LLM-RKNN Demo - LLM Inference on Rockchip NPU\n");
    printf("  Version: %s\n", llm_rknn_get_version());
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Initialize LLM
    printf("[Demo] Initializing LLM-RKNN...\n");
    llm_rknn_handle_t handle = llm_rknn_init(model_path, tokenizer_path, &config);
    
    if (!handle) {
        fprintf(stderr, "[Demo] Failed to initialize LLM-RKNN\n");
        return 1;
    }
    
    printf("[Demo] Initialization successful!\n\n");
    
    // Single prompt mode
    if (initial_prompt) {
        printf("Prompt: %s\n", initial_prompt);
        printf("Response: ");
        
        int ret;
        if (use_extended_callback) {
            ret = llm_rknn_generate_ex(handle, initial_prompt, token_callback_ex, nullptr);
        } else {
            ret = llm_rknn_generate(handle, initial_prompt, token_callback, nullptr);
        }
        
        printf("\n");
        
        if (ret != LLM_RKNN_SUCCESS) {
            fprintf(stderr, "\n[Demo] Generation failed: %s\n", llm_rknn_get_error_string(ret));
        } else {
            // Print performance statistics
            llm_rknn_perf_stats_t stats;
            llm_rknn_get_perf_stats(handle, &stats);
            
            printf("\n");
            printf("───────────────────────────────────────────────────────────────\n");
            printf("Performance Statistics:\n");
            printf("  Tokens generated: %u\n", stats.total_tokens_generated);
            printf("  Prefill time:     %.2f ms\n", stats.prefill_time_ms);
            printf("  Decode time:      %.2f ms\n", stats.decode_time_ms);
            printf("  Total time:       %.2f ms\n", stats.total_time_ms);
            printf("  Speed:            %.2f tokens/sec\n", stats.tokens_per_second);
            printf("───────────────────────────────────────────────────────────────\n");
        }
    }
    // Interactive mode
    else {
        printf("[Demo] Entering interactive mode (Ctrl+C to exit)\n");
        printf("[Demo] Type 'exit' or 'quit' to end the session\n");
        printf("[Demo] Type 'reset' to clear conversation history\n\n");
        
        char prompt[4096];
        
        while (!g_interrupted) {
            printf("You: ");
            fflush(stdout);
            
            if (!fgets(prompt, sizeof(prompt), stdin)) {
                break;
            }
            
            // Remove trailing newline
            size_t len = strlen(prompt);
            if (len > 0 && prompt[len-1] == '\n') {
                prompt[len-1] = '\0';
            }
            
            // Check for commands
            if (strcmp(prompt, "exit") == 0 || strcmp(prompt, "quit") == 0) {
                break;
            }
            
            if (strcmp(prompt, "reset") == 0) {
                llm_rknn_reset(handle);
                printf("[Demo] Conversation reset\n\n");
                continue;
            }
            
            if (strlen(prompt) == 0) {
                continue;
            }
            
            printf("Assistant: ");
            
            int ret;
            if (use_extended_callback) {
                ret = llm_rknn_generate_ex(handle, prompt, token_callback_ex, nullptr);
            } else {
                ret = llm_rknn_generate(handle, prompt, token_callback, nullptr);
            }
            
            printf("\n\n");
            
            if (ret != LLM_RKNN_SUCCESS) {
                fprintf(stderr, "[Demo] Generation failed: %s\n\n", llm_rknn_get_error_string(ret));
            }
        }
    }
    
    // Cleanup
    printf("\n[Demo] Cleaning up...\n");
    llm_rknn_release(handle);
    
    printf("[Demo] Goodbye!\n");
    
    return 0;
}
