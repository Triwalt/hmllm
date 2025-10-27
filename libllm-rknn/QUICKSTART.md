# Quick Start Guide - libllm-rknn

Get up and running with libllm-rknn in 5 minutes!

## Prerequisites

You'll need:
- A Rockchip board with NPU (RK3588, RK3576, etc.)
- Linux running on the board
- RKNN Runtime installed
- A converted `.rknn` model file
- A tokenizer model file

## Step 1: Install Dependencies

```bash
# Install build tools
sudo apt-get update
sudo apt-get install build-essential cmake git

# Install SentencePiece
sudo apt-get install libsentencepiece-dev

# Verify RKNN runtime is installed
ldconfig -p | grep rknnrt
```

## Step 2: Build the Library

```bash
# Clone repository
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# Build
chmod +x build.sh
./build.sh

# Install
cd build
sudo make install
sudo ldconfig
```

## Step 3: Get a Model

You need an RKNN-converted LLM model. If you don't have one:

```bash
# Example: Download a pre-converted model (if available)
# Or convert your own using RKNN-Toolkit2 on a host PC

# You should have:
# - model.rknn (the converted model)
# - tokenizer.model (SentencePiece tokenizer)
```

## Step 4: Run Your First Inference

```bash
# Interactive mode
./build/bin/llm_demo -m /path/to/model.rknn -t /path/to/tokenizer.model

# Single prompt mode
./build/bin/llm_demo \
    -m /path/to/model.rknn \
    -t /path/to/tokenizer.model \
    -p "Tell me a joke"
```

## Step 5: Write Your Own Application

Create `my_app.cpp`:

```cpp
#include "llm_rknn.h"
#include <stdio.h>

// Callback receives each generated token
int token_callback(const char* text, int32_t id, void* data) {
    printf("%s", text);
    fflush(stdout);
    return 0;  // Return 0 to continue, non-zero to stop
}

int main() {
    // 1. Initialize
    llm_rknn_handle_t llm = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        NULL  // Use defaults
    );
    
    if (!llm) {
        fprintf(stderr, "Failed to initialize!\n");
        return 1;
    }
    
    printf("Initialized successfully!\n\n");
    
    // 2. Generate text
    printf("Assistant: ");
    int ret = llm_rknn_generate(
        llm,
        "What is artificial intelligence?",
        token_callback,
        NULL
    );
    
    printf("\n\n");
    
    // 3. Check results
    if (ret == LLM_RKNN_SUCCESS) {
        llm_rknn_perf_stats_t stats;
        llm_rknn_get_perf_stats(llm, &stats);
        printf("Generated %u tokens in %.2f ms (%.2f tok/s)\n",
               stats.total_tokens_generated,
               stats.total_time_ms,
               stats.tokens_per_second);
    } else {
        fprintf(stderr, "Error: %s\n", llm_rknn_get_error_string(ret));
    }
    
    // 4. Cleanup
    llm_rknn_release(llm);
    
    return 0;
}
```

Compile and run:

```bash
# Compile
g++ -o my_app my_app.cpp -lllm-rknn

# Run
./my_app
```

## Advanced Example: Custom Configuration

```cpp
#include "llm_rknn.h"
#include <stdio.h>

int callback(const char* text, int32_t id, void* data) {
    printf("%s", text);
    fflush(stdout);
    return 0;
}

int main() {
    // Create custom configuration
    llm_rknn_config_t config;
    llm_rknn_init_default_config(&config);
    
    // Customize generation parameters
    config.max_new_tokens = 200;              // Generate up to 200 tokens
    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K;  // Use Top-K
    config.temperature = 0.8f;                // More creative
    config.top_k = 40;                        // Top-40 sampling
    config.rknn_core_mask = 3;                // Use all NPU cores
    
    // Initialize with custom config
    llm_rknn_handle_t llm = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        &config
    );
    
    if (!llm) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }
    
    // Generate with creative settings
    llm_rknn_generate(llm, "Write a creative story:", callback, NULL);
    
    llm_rknn_release(llm);
    return 0;
}
```

## Common Sampling Configurations

### For Factual Q&A (Deterministic)
```c
config.sampling_method = LLM_RKNN_SAMPLING_GREEDY;
config.temperature = 1.0f;
```

### For Creative Writing (Diverse)
```c
config.sampling_method = LLM_RKNN_SAMPLING_TOP_K_TOP_P;
config.temperature = 0.8f;
config.top_k = 40;
config.top_p = 0.95f;
```

### For Balanced Output
```c
config.sampling_method = LLM_RKNN_SAMPLING_TOP_P;
config.temperature = 0.7f;
config.top_p = 0.9f;
```

## Troubleshooting

**Problem**: Library not found when running
```bash
# Solution:
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
# Or install properly with ldconfig
```

**Problem**: RKNN runtime error
```bash
# Check NPU driver is loaded:
dmesg | grep rknpu

# Check RKNN library:
ldconfig -p | grep rknnrt
```

**Problem**: Slow inference
- Enable KV cache: `config.enable_kv_cache = true;`
- Use greedy sampling for speed
- Ensure model is quantized (int8 or fp16)

## Next Steps

- **Full Documentation**: Read [README.md](README.md)
- **Build Guide**: See [docs/BUILD.md](docs/BUILD.md)
- **API Reference**: Check [include/llm_rknn.h](include/llm_rknn.h)
- **Examples**: Explore [examples/](examples/)

## Need Help?

- Open an issue on GitHub
- Check existing documentation
- Review example code

---

**Happy coding! 🚀**
