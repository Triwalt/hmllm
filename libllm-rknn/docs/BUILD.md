# Building and Running libllm-rknn

This guide provides detailed instructions for building, installing, and using libllm-rknn on Rockchip platforms.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Building on Target Device](#building-on-target-device)
3. [Cross-Compilation](#cross-compilation)
4. [Installation](#installation)
5. [Running Examples](#running-examples)
6. [Integration into Your Project](#integration-into-your-project)

---

## Prerequisites

### Required Dependencies

1. **RKNN Runtime** (librknnrt.so)
   - Version: 2.0.0 or newer
   - Usually provided with the board's SDK

2. **SentencePiece Library**
   ```bash
   # On Debian/Ubuntu
   sudo apt-get update
   sudo apt-get install libsentencepiece-dev
   
   # Or build from source
   git clone https://github.com/google/sentencepiece.git
   cd sentencepiece
   mkdir build && cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   make -j$(nproc)
   sudo make install
   sudo ldconfig
   ```

3. **Build Tools**
   ```bash
   sudo apt-get install build-essential cmake git
   ```

### Verifying RKNN Runtime

Check if RKNN runtime is installed:

```bash
# Check library
ldconfig -p | grep rknnrt

# Check header
find /usr -name "rknn_api.h" 2>/dev/null

# Check driver
dmesg | grep rknpu
```

If not found, install from your board's SDK:

```bash
cd /path/to/rknpu2/runtime/Linux/librknn_api

# For RK3588 (aarch64)
sudo cp aarch64/librknnrt.so /usr/lib/
sudo cp include/rknn_api.h /usr/include/
sudo ldconfig
```

---

## Building on Target Device

### Quick Build (Recommended)

Use the provided build script:

```bash
# Clone repository
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# Make script executable
chmod +x build.sh

# Build (defaults: Release, shared library, with examples)
./build.sh

# Or with custom options
./build.sh --debug --no-examples
./build.sh --static --prefix /opt/llm-rknn
```

### Manual Build

If you prefer manual control:

```bash
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

### Build Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release, Debug | Release | Optimization level |
| `BUILD_SHARED_LIBS` | ON, OFF | ON | Build shared (.so) or static (.a) |
| `BUILD_EXAMPLES` | ON, OFF | ON | Build example applications |
| `CMAKE_INSTALL_PREFIX` | path | /usr/local | Installation directory |

---

## Cross-Compilation

For building on an x86/x64 host for ARM target:

### Setup Cross-Compiler

```bash
# Install ARM64 cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### Prepare Dependencies

You need ARM versions of dependencies on your host:

```bash
# Create sysroot directory
mkdir -p ~/arm64-sysroot/usr/{lib,include}

# Copy from target device (or extract from SDK)
scp user@target:/usr/lib/librknnrt.so ~/arm64-sysroot/usr/lib/
scp user@target:/usr/include/rknn_api.h ~/arm64-sysroot/usr/include/
scp -r user@target:/usr/include/sentencepiece ~/arm64-sysroot/usr/include/
scp user@target:/usr/lib/libsentencepiece.so ~/arm64-sysroot/usr/lib/
```

### Create Toolchain File

Create `cmake/aarch64-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross-compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Sysroot
set(CMAKE_SYSROOT ~/arm64-sysroot)
set(CMAKE_FIND_ROOT_PATH ~/arm64-sysroot)

# Search paths
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### Cross-Compile

```bash
mkdir build-arm64 && cd build-arm64

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)

# Copy to target device
scp lib/libllm-rknn.so user@target:/usr/local/lib/
scp bin/llm_demo user@target:/usr/local/bin/
```

---

## Installation

### System-wide Installation

```bash
cd build
sudo make install
sudo ldconfig

# Verify installation
ldconfig -p | grep llm-rknn
ls -l /usr/local/include/llm_rknn.h
```

### Custom Location Installation

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/libllm-rknn
make install

# Update library path
export LD_LIBRARY_PATH=/opt/libllm-rknn/lib:$LD_LIBRARY_PATH

# Or add to /etc/ld.so.conf.d/
echo "/opt/libllm-rknn/lib" | sudo tee /etc/ld.so.conf.d/libllm-rknn.conf
sudo ldconfig
```

---

## Running Examples

### Prepare Test Files

You need:
1. A `.rknn` model file
2. A tokenizer model file (e.g., `tokenizer.model` for SentencePiece)

### Run Interactive Demo

```bash
cd build/bin

./llm_demo \
    -m /path/to/model.rknn \
    -t /path/to/tokenizer.model
```

Interactive commands:
- Type your prompt and press Enter
- Type `reset` to clear conversation
- Type `exit` or `quit` to quit
- Press Ctrl+C to interrupt generation

### Run with Single Prompt

```bash
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -p "Write a poem about nature"
```

### Advanced Usage

```bash
# Top-K sampling with temperature
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topk \
    -T 0.8 \
    -k 40 \
    -n 256

# Top-P (nucleus) sampling
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topp \
    -P 0.9 \
    -T 0.7

# Combined Top-K + Top-P
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topk_topp \
    -k 50 \
    -P 0.95 \
    -T 0.8
```

### Simple Test

```bash
./llm_simple_test model.rknn tokenizer.model
```

---

## Integration into Your Project

### Using CMake

Add to your `CMakeLists.txt`:

```cmake
find_package(llm-rknn REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app llm-rknn::llm-rknn)
```

### Using pkg-config

```bash
# Compile
g++ -o my_app main.cpp $(pkg-config --cflags --libs llm-rknn)
```

### Manual Linking

```bash
g++ -o my_app main.cpp \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lllm-rknn \
    -lrknnrt \
    -lsentencepiece \
    -lpthread
```

### Minimal Example

Create `my_app.cpp`:

```cpp
#include "llm_rknn.h"
#include <stdio.h>

int callback(const char* text, int32_t id, void* data) {
    printf("%s", text);
    fflush(stdout);
    return 0;
}

int main() {
    auto handle = llm_rknn_init("model.rknn", "tokenizer.model", nullptr);
    if (!handle) return 1;
    
    llm_rknn_generate(handle, "Hello", callback, nullptr);
    
    llm_rknn_release(handle);
    return 0;
}
```

Compile and run:

```bash
g++ -o my_app my_app.cpp -lllm-rknn
./my_app
```

---

## Troubleshooting

### Build Errors

**Error: "rknn_api.h: No such file or directory"**

Solution:
```bash
# Find RKNN headers
find /usr -name "rknn_api.h" 2>/dev/null

# If found, add to CMake:
cmake .. -DRKNN_INCLUDE_DIR=/path/to/include
```

**Error: "cannot find -lrknnrt"**

Solution:
```bash
# Find library
find /usr -name "librknnrt.so" 2>/dev/null

# If found, add to CMake:
cmake .. -DRKNN_LIBRARY=/path/to/librknnrt.so
```

### Runtime Errors

**Error: "error while loading shared libraries: libllm-rknn.so"**

Solution:
```bash
# Add to library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or run ldconfig
sudo ldconfig
```

**Error: "Failed to init RKNN context"**

Possible causes:
- Model file corrupted or wrong architecture
- RKNN runtime version mismatch
- NPU driver not loaded

Check:
```bash
# Verify NPU driver
dmesg | grep rknpu

# Check RKNN version
strings /usr/lib/librknnrt.so | grep version
```

---

## Performance Optimization

### Compiler Flags

For maximum performance:

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

### NPU Core Selection

In your code:

```c
llm_rknn_config_t config;
llm_rknn_init_default_config(&config);
config.rknn_core_mask = 3;  // Use all available cores
```

### Memory Settings

Reduce memory usage:

```c
config.max_context_length = 1024;  // Reduce from 2048
config.enable_kv_cache = true;     // Keep enabled for speed
```

---

## Next Steps

- Read the [API Documentation](../README.md#api-documentation)
- Check [Performance Tuning](../README.md#performance-tuning)
- Explore [Model Preparation](../README.md#model-preparation)
- Join community discussions

---

For more help, open an issue on GitHub or consult the README.md file.
