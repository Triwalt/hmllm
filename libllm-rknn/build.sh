#!/bin/bash

# Build script for libllm-rknn
# This script automates the build process for different configurations

set -e  # Exit on error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
BUILD_SHARED="ON"
BUILD_EXAMPLES="ON"
JOBS=$(nproc)

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --static)
            BUILD_SHARED="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --jobs|-j)
            JOBS="$2"
            shift 2
            ;;
        --clean)
            print_info "Cleaning build directory..."
            rm -rf ${BUILD_DIR}
            print_info "Clean complete"
            exit 0
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug           Build in Debug mode (default: Release)"
            echo "  --static          Build static library (default: shared)"
            echo "  --no-examples     Don't build examples"
            echo "  --prefix PATH     Installation prefix (default: /usr/local)"
            echo "  --jobs N, -j N    Number of parallel jobs (default: $(nproc))"
            echo "  --clean           Clean build directory"
            echo "  --help, -h        Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                          # Build with defaults"
            echo "  $0 --debug --no-examples    # Debug build without examples"
            echo "  $0 --static --prefix /opt   # Static lib, install to /opt"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Print configuration
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  libllm-rknn Build Configuration"
echo "═══════════════════════════════════════════════════════════"
echo "  Build type:       ${BUILD_TYPE}"
echo "  Library type:     $([ "$BUILD_SHARED" == "ON" ] && echo "Shared" || echo "Static")"
echo "  Build examples:   ${BUILD_EXAMPLES}"
echo "  Install prefix:   ${INSTALL_PREFIX}"
echo "  Parallel jobs:    ${JOBS}"
echo "  Build directory:  ${BUILD_DIR}"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Check dependencies
print_info "Checking dependencies..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install CMake 3.10 or newer."
    exit 1
fi

# Check for compiler
if ! command -v g++ &> /dev/null; then
    print_error "g++ not found. Please install GCC."
    exit 1
fi

# Check for RKNN runtime
RKNN_FOUND=0
for path in /usr/lib /usr/local/lib /usr/lib/aarch64-linux-gnu; do
    if [ -f "$path/librknnrt.so" ]; then
        RKNN_FOUND=1
        print_info "Found RKNN runtime: $path/librknnrt.so"
        break
    fi
done

if [ $RKNN_FOUND -eq 0 ]; then
    print_warn "RKNN runtime not found in standard locations"
    print_warn "Make sure librknnrt.so is available or set RKNN_LIBRARY_DIR"
fi

# Check for SentencePiece
if pkg-config --exists sentencepiece; then
    print_info "Found SentencePiece: $(pkg-config --modversion sentencepiece)"
elif ldconfig -p | grep -q libsentencepiece; then
    print_info "Found SentencePiece (via ldconfig)"
else
    print_warn "SentencePiece not found in standard locations"
    print_warn "Build may fail if SentencePiece is not installed"
fi

# Create build directory
print_info "Creating build directory..."
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure
print_info "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
    -DBUILD_SHARED_LIBS=${BUILD_SHARED} \
    -DBUILD_EXAMPLES=${BUILD_EXAMPLES}

# Build
print_info "Building..."
make -j${JOBS}

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    print_info "Build completed successfully!"
    echo ""
    echo "Output files:"
    if [ -f "lib/libllm-rknn.so" ]; then
        echo "  Library: $(pwd)/lib/libllm-rknn.so"
    elif [ -f "lib/libllm-rknn.a" ]; then
        echo "  Library: $(pwd)/lib/libllm-rknn.a"
    fi
    
    if [ -f "bin/llm_demo" ]; then
        echo "  Demo:    $(pwd)/bin/llm_demo"
    fi
    
    echo ""
    echo "To install, run:"
    echo "  cd ${BUILD_DIR} && sudo make install"
    echo ""
    echo "To run the demo:"
    echo "  ${BUILD_DIR}/bin/llm_demo -m model.rknn -t tokenizer.model"
    echo ""
else
    print_error "Build failed!"
    exit 1
fi
