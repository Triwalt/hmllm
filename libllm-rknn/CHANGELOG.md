# libllm-rknn Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-10-09

### Added
- Initial release of libllm-rknn
- High-level C/C++ API for LLM inference on Rockchip NPUs
- RKNN Runtime backend integration
- SentencePiece tokenizer support
- Auto-regressive inference loop implementation
- KV cache management system
- Multiple sampling methods:
  - Greedy search
  - Top-K sampling
  - Top-P (Nucleus) sampling
  - Combined Top-K + Top-P sampling
- Streaming token generation with callback interface
- Extended callback with metadata (log probabilities)
- Performance profiling and statistics
- Conversation reset functionality
- Interactive demo application (`llm_demo`)
- Simple test application (`llm_simple_test`)
- Comprehensive documentation
- CMake build system
- Cross-compilation support
- Apache 2.0 license

### Features
- Temperature-based sampling control
- Repetition penalty support
- Configurable NPU core mask
- Multi-threaded preprocessing
- Special token handling (BOS/EOS/PAD)
- Error handling with descriptive messages
- Thread-safe implementation

### Documentation
- Complete API documentation
- Usage examples and tutorials
- Model preparation guide
- Performance tuning guidelines
- Troubleshooting section
- Build instructions for native and cross-compilation

### Supported Platforms
- RK3588/RK3588S
- RK3576
- RK3566/RK3568
- RV1106/RV1103
- RK2118
- Other RKNPU2-compatible devices

## [Unreleased]

### Planned Features
- Batch inference support
- Dynamic KV cache management
- INT4/INT8 quantization-aware inference
- Multi-language tokenizer support (GPT, BERT, etc.)
- Python bindings
- Beam search sampling
- Memory-mapped model loading
- WebSocket/REST API server example
- Benchmark suite
- Unit tests
- CI/CD pipeline

---

## Version History

### Version Numbering
- **MAJOR**: Incompatible API changes
- **MINOR**: Backward-compatible functionality additions
- **PATCH**: Backward-compatible bug fixes
