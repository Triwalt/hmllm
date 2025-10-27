# libllm-rknn Project Structure

```
libllm-rknn/
│
├── README.md                   # Main project documentation
├── LICENSE                     # Apache 2.0 license
├── CHANGELOG.md               # Version history
├── .gitignore                 # Git ignore rules
├── build.sh                   # Build automation script
├── CMakeLists.txt             # Root CMake configuration
│
├── include/                   # Public API headers
│   └── llm_rknn.h            # Main public API header
│
├── src/                       # Library implementation
│   ├── llm_rknn.cpp          # Public API implementation
│   ├── llm_context.cpp       # Core LLM context and inference loop
│   ├── sampler.cpp           # Token sampling algorithms
│   ├── tokenizer_sentencepiece.cpp  # SentencePiece tokenizer
│   ├── llm_rknn_internal.h  # Internal implementation headers
│   └── tokenizer_sentencepiece.h    # Tokenizer interface
│
├── examples/                  # Example applications
│   ├── CMakeLists.txt        # Examples build configuration
│   ├── llm_demo.cpp          # Interactive chatbot demo
│   └── simple_test.cpp       # Minimal usage example
│
├── cmake/                     # CMake modules and helpers
│   ├── FindRKNN.cmake        # Find RKNN runtime module
│   └── llm-rknn-config.cmake.in  # Package config template
│
├── docs/                      # Additional documentation
│   └── BUILD.md              # Detailed build instructions
│
└── third_party/              # Third-party dependencies (optional)
    └── sentencepiece/        # Bundled SentencePiece (if needed)
```

## Key Components

### Public API (`include/llm_rknn.h`)
- Clean C interface for use from C and C++
- Opaque handle-based design
- Callback-based streaming output
- Comprehensive error handling
- Performance statistics API

### Core Implementation (`src/`)

#### `llm_rknn.cpp`
- Public API entry points
- Handle management
- Error code to string conversion
- Version information

#### `llm_context.cpp`
- Main LLM inference engine
- RKNN runtime integration
- Auto-regressive generation loop
- KV cache management
- Performance tracking
- Thread safety

#### `sampler.cpp`
- Greedy sampling
- Top-K sampling
- Top-P (nucleus) sampling
- Combined Top-K + Top-P
- Temperature scaling
- Softmax computation

#### `tokenizer_sentencepiece.cpp`
- Text encoding to token IDs
- Token IDs decoding to text
- Special token handling
- SentencePiece integration

### Examples (`examples/`)

#### `llm_demo.cpp`
- Full-featured interactive chatbot
- Command-line argument parsing
- Streaming token output
- Performance statistics display
- Conversation reset capability
- Signal handling (Ctrl+C)

#### `simple_test.cpp`
- Minimal usage example
- Quick functionality verification
- Template for integration

### Build System (`cmake/`)

#### `CMakeLists.txt`
- Library build configuration
- Dependency management
- Installation rules
- Package export

#### `FindRKNN.cmake`
- Locates RKNN runtime
- Cross-platform path detection
- Version checking

## Build Outputs

After building, the structure becomes:

```
build/
├── lib/
│   ├── libllm-rknn.so        # Shared library (or .a for static)
│   └── cmake/llm-rknn/       # CMake package files
├── bin/
│   ├── llm_demo              # Interactive demo executable
│   └── llm_simple_test       # Simple test executable
└── [build artifacts]
```

## Installation Layout

After `make install`:

```
/usr/local/
├── include/
│   └── llm_rknn.h            # Public header
├── lib/
│   ├── libllm-rknn.so        # Shared library
│   └── cmake/llm-rknn/       # CMake config files
└── bin/
    ├── llm_demo              # Demo application
    └── llm_simple_test       # Test application
```

## Dependencies

### Required at Build Time
- CMake 3.10+
- GCC 7.5+ or Clang 9.0+
- RKNN Runtime headers (rknn_api.h)
- SentencePiece headers

### Required at Runtime
- RKNN Runtime library (librknnrt.so)
- SentencePiece library (libsentencepiece.so)
- pthread (usually system-provided)

### Optional
- RKNN-Toolkit2 (for model conversion, Python)

## Usage Flow

```
User Application
    ↓
llm_rknn_init()
    ↓
[Load Model] → RKNN Runtime
[Load Tokenizer] → SentencePiece
[Initialize Sampler]
[Allocate KV Cache]
    ↓
llm_rknn_generate()
    ↓
[Encode Prompt] → Tokenizer
[Prefill Phase] → RKNN Inference
[Decode Loop]
    ├─→ [NPU Inference] → RKNN Runtime
    ├─→ [Sample Token] → Sampler
    ├─→ [Decode Token] → Tokenizer
    └─→ [User Callback] → User Code
    ↓
llm_rknn_get_perf_stats()
    ↓
llm_rknn_release()
```

## Design Principles

1. **Simplicity**: Easy-to-use C API
2. **Performance**: Optimized inference loop with KV cache
3. **Flexibility**: Configurable sampling and generation parameters
4. **Portability**: Cross-platform Linux support
5. **Extensibility**: Modular design for future enhancements
6. **Robustness**: Comprehensive error handling
7. **Documentation**: Well-commented code and examples

## Future Enhancements

Planned features (see CHANGELOG.md):
- Batch inference
- Additional tokenizer backends
- Python bindings
- More sampling methods
- Memory optimizations
- Extended model architecture support
