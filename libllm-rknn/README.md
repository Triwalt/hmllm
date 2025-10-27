# libllm-rknn

**A high-level C/C++ library for running Large Language Models (LLMs) on Rockchip NPUs using the RKNN Runtime.**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/yourusername/libllm-rknn)

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [Building from Source](#building-from-source)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [API Documentation](#api-documentation)
  - [Example Applications](#example-applications)
- [Model Preparation](#model-preparation)
- [Performance Tuning](#performance-tuning)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## 🎯 Overview

**libllm-rknn** is a production-ready software library that enables developers to easily deploy conversational Large Language Models on Rockchip SoCs with NPU acceleration. It provides a simple, high-level C/C++ API similar to Rockchip's RKLLM library but works with standard `.rknn` models, giving you broader hardware support and flexibility.

### Why libllm-rknn?

- **Simple API**: Deploy LLMs with just a few lines of code
- **RKNN-Based**: Uses the widely-supported RKNN Runtime as backend
- **Production-Ready**: Includes KV cache optimization, multiple sampling methods, and streaming output
- **Flexible**: Works with any LLM model that can be converted to RKNN format
- **Open Source**: Full control over the inference pipeline

---

## ✨ Features

### Core Capabilities

- ✅ **Auto-regressive Text Generation**: Complete implementation of the LLM inference loop
- ✅ **Streaming Output**: Token-by-token callback interface for real-time generation
- ✅ **KV Cache Management**: Efficient memory management for fast inference
- ✅ **Multiple Sampling Methods**: Greedy, Top-K, Top-P (Nucleus), and combined sampling
- ✅ **Tokenization**: Integrated SentencePiece tokenizer support
- ✅ **Performance Profiling**: Built-in statistics for tokens/second, latency tracking
- ✅ **Conversation Management**: Reset and multi-turn conversation support

### Technical Features

- **NPU Acceleration**: Full utilization of Rockchip NPU cores via RKNN Runtime
- **Multi-threading**: Optimized CPU preprocessing with configurable thread count
- **Temperature Scaling**: Adjustable randomness in generation
- **Repetition Penalty**: Reduce repetitive outputs
- **Special Token Handling**: Automatic BOS/EOS/PAD token management

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    User Application                         │
│                  (llm_demo, your app)                       │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ Public C API
                         │ (llm_rknn.h)
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    libllm-rknn.so                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   Tokenizer  │  │   Sampler    │  │  KV Cache    │     │
│  │ (SentPiece)  │  │(Greedy/TopK) │  │  Manager     │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         Auto-Regressive Inference Loop              │   │
│  │    (Prefill → Decode → Sample → Repeat)            │   │
│  └─────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ RKNN Runtime API
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                 librknnrt.so (RKNN Runtime)                 │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                 │
│  │ NPU Core │  │ NPU Core │  │ NPU Core │  ...            │
│  │    0     │  │    1     │  │    2     │                 │
│  └──────────┘  └──────────┘  └──────────┘                 │
└─────────────────────────────────────────────────────────────┘
                         │
                         ▼
                   .rknn Model File
```

---

## 📦 Requirements

### Hardware
- Rockchip SoC with NPU support:
  - RK3588/RK3588S
  - RK3576
  - RK3566/RK3568
  - RV1106/RV1103
  - RK2118
  - Or any other RKNPU2-compatible chip

### Software
- **OS**: Linux (ARM64 or ARM32)
  - Debian 11+ / Ubuntu 20.04+
  - Buildroot-based systems
- **Compiler**: GCC 7.5+ or Clang 9.0+
- **CMake**: 3.10 or newer
- **RKNN Runtime**: librknnrt.so (v2.0.0 or newer)
- **SentencePiece**: For tokenization

### Optional
- **RKNN-Toolkit2**: For model conversion (Python, can run on x86 host)

---

## 🚀 Installation

### Dependencies

#### 1. Install RKNN Runtime

The RKNN runtime is provided by Rockchip. If not already installed:

```bash
# For RK3588 (example)
cd rknpu2/runtime/Linux/librknn_api
sudo cp aarch64/librknnrt.so /usr/lib/
sudo cp include/rknn_api.h /usr/include/
sudo ldconfig
```

#### 2. Install SentencePiece

```bash
# From package manager (if available)
sudo apt-get install libsentencepiece-dev

# Or build from source
git clone https://github.com/google/sentencepiece.git
cd sentencepiece
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# Create build directory
mkdir build && cd build

# Configure
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_EXAMPLES=ON

# Build
make -j$(nproc)

# Install (optional)
sudo make install
sudo ldconfig
```

### Cross-Compilation

For cross-compiling from an x86 host:

```bash
mkdir build-arm64 && cd build-arm64

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DRKNN_INCLUDE_DIR=/path/to/rknpu2/include \
    -DRKNN_LIBRARY=/path/to/rknpu2/aarch64/librknnrt.so

make -j$(nproc)
```

---

## 💡 Usage

### Quick Start

Here's a minimal example to get started:

```c
#include "llm_rknn.h"
#include <stdio.h>

// Callback function to receive generated tokens
int callback(const char* token_text, int32_t token_id, void* user_data) {
    printf("%s", token_text);
    fflush(stdout);
    return 0;  // Continue generation
}

int main() {
    // 1. Initialize the library
    llm_rknn_handle_t handle = llm_rknn_init(
        "model.rknn",           // Path to RKNN model
        "tokenizer.model",      // Path to tokenizer
        NULL                    // Use default config
    );
    
    if (!handle) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }
    
    // 2. Generate text
    const char* prompt = "Once upon a time";
    int ret = llm_rknn_generate(handle, prompt, callback, NULL);
    
    if (ret != LLM_RKNN_SUCCESS) {
        fprintf(stderr, "Generation failed: %s\n", 
                llm_rknn_get_error_string(ret));
    }
    
    // 3. Cleanup
    llm_rknn_release(handle);
    
    return 0;
}
```

Compile and run:

```bash
gcc -o my_app my_app.c -lllm-rknn
./my_app
```

### Advanced Configuration

```c
#include "llm_rknn.h"

int main() {
    // Create custom configuration
    llm_rknn_config_t config;
    llm_rknn_init_default_config(&config);
    
    // Customize parameters
    config.max_new_tokens = 256;
    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K;
    config.temperature = 0.8f;
    config.top_k = 40;
    config.top_p = 0.95f;
    config.rknn_core_mask = 3;  // Use NPU cores 0 and 1
    
    // Initialize with custom config
    llm_rknn_handle_t handle = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        &config
    );
    
    // ... use handle ...
    
    llm_rknn_release(handle);
    return 0;
}
```

### API Documentation

#### Initialization

```c
llm_rknn_handle_t llm_rknn_init(
    const char* rknn_model_path,
    const char* tokenizer_path,
    llm_rknn_config_t* config  // NULL for defaults
);
```

Loads the RKNN model and tokenizer, initializes NPU context.

**Parameters:**
- `rknn_model_path`: Path to `.rknn` model file
- `tokenizer_path`: Path to tokenizer model (e.g., `tokenizer.model` for SentencePiece)
- `config`: Configuration struct (or NULL for defaults)

**Returns:** Handle to LLM instance, or NULL on failure

#### Generation

```c
int llm_rknn_generate(
    llm_rknn_handle_t handle,
    const char* prompt,
    llm_rknn_callback_t callback,
    void* user_data
);
```

Generates text from a prompt with streaming output.

**Callback signature:**
```c
int callback(const char* token_text, int32_t token_id, void* user_data);
// Return 0 to continue, non-zero to stop generation
```

#### Performance Statistics

```c
int llm_rknn_get_perf_stats(
    llm_rknn_handle_t handle,
    llm_rknn_perf_stats_t* stats
);
```

Retrieves performance metrics from the last generation:
- `total_tokens_generated`: Number of tokens produced
- `prefill_time_ms`: Time for prompt processing
- `decode_time_ms`: Time for token generation
- `tokens_per_second`: Generation speed

#### Cleanup

```c
void llm_rknn_release(llm_rknn_handle_t handle);
```

Frees all resources. Handle becomes invalid after this call.

### Example Applications

#### Interactive Chatbot

```bash
cd build/bin
./llm_demo -m /path/to/model.rknn -t /path/to/tokenizer.model
```

This launches an interactive REPL where you can chat with the model.

#### Single Prompt

```bash
./llm_demo -m model.rknn -t tokenizer.model -p "Explain quantum computing"
```

#### Custom Sampling

```bash
./llm_demo -m model.rknn -t tokenizer.model \
    -s topk_topp \
    -T 0.7 \
    -k 50 \
    -P 0.9 \
    -n 200
```

---

## 🔧 Model Preparation

### Converting Models to RKNN

You need to convert your LLM to RKNN format using **RKNN-Toolkit2** (runs on x86/x64):

```python
from rknn.api import RKNN

# Initialize RKNN
rknn = RKNN(verbose=True)

# Configure for LLM
rknn.config(
    target_platform='rk3588',
    optimization_level=3,
    quantized_dtype='int8'  # or 'fp16'
)

# Load ONNX/PyTorch model
rknn.load_onnx(model='llm_model.onnx')

# Build RKNN model
rknn.build(do_quantization=True, dataset='calibration_data.txt')

# Export
rknn.export_rknn('llm_model.rknn')

rknn.release()
```

### Tokenizer Preparation

For SentencePiece tokenizers, export from your model:

```python
# For Hugging Face models
from transformers import AutoTokenizer

tokenizer = AutoTokenizer.from_pretrained("model_name")
tokenizer.save_pretrained("./tokenizer")
# Use tokenizer.model file
```

### Model Requirements

Your RKNN model should:
- Accept token IDs as input (int32)
- Output logits over vocabulary (float32)
- Optionally support KV cache inputs/outputs for optimal performance

---

## ⚡ Performance Tuning

### Sampling Method Selection

- **Greedy**: Fastest, deterministic, but may be repetitive
- **Top-K**: Good balance of speed and quality
- **Top-P**: More diverse, slightly slower
- **Top-K + Top-P**: Best quality, slowest

### Temperature

- `temperature = 0.1`: Very focused, deterministic
- `temperature = 1.0`: Balanced (default)
- `temperature = 1.5`: More creative, diverse

### NPU Core Usage

```c
config.rknn_core_mask = 0;  // Auto (recommended)
config.rknn_core_mask = 1;  // Core 0 only
config.rknn_core_mask = 2;  // Core 1 only
config.rknn_core_mask = 3;  // Both cores (RK3588)
```

### Memory Optimization

Reduce `max_context_length` if you encounter memory issues:

```c
config.max_context_length = 1024;  // Instead of 2048
```

---

## 🐛 Troubleshooting

### Common Issues

**Problem**: `Failed to load model`
- **Solution**: Ensure model path is correct and file is readable
- Check model was built for correct target platform

**Problem**: `Tokenizer error`
- **Solution**: Verify tokenizer.model file is valid SentencePiece format
- Try converting tokenizer from source model

**Problem**: `NPU runtime error`
- **Solution**: Ensure RKNN runtime version matches model version
- Check NPU drivers are properly installed: `dmesg | grep rknpu`

**Problem**: Slow performance
- **Solution**: 
  - Enable KV cache: `config.enable_kv_cache = true`
  - Use int8 quantization when converting model
  - Reduce max_new_tokens
  - Use greedy sampling

### Debug Mode

Build with debug symbols:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

Enable verbose logging by modifying source or setting environment:

```bash
export LLM_RKNN_VERBOSE=1
./llm_demo ...
```

---

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Follow existing code style (C++17, Google C++ Style Guide)
- Add comments for complex logic
- Include unit tests for new features

---

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **Rockchip** for RKNN Runtime and NPU hardware
- **Google** for SentencePiece tokenizer
- The open-source AI community

---

## 📞 Contact & Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/libllm-rknn/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/libllm-rknn/discussions)
- **Email**: your.email@example.com

---

**Made with ❤️ for the Rockchip NPU community**
