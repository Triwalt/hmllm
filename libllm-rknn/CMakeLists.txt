cmake_minimum_required(VERSION 3.10)
project(libllm-rknn VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Build options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build unit tests" OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# RKNN Runtime (required)
find_package(RKNN REQUIRED)

# SentencePiece (required for tokenizer)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SENTENCEPIECE sentencepiece)
endif()

if(NOT SENTENCEPIECE_FOUND)
    # Try to find manually
    find_path(SENTENCEPIECE_INCLUDE_DIR
        NAMES sentencepiece_processor.h
        PATHS /usr/include /usr/local/include
        PATH_SUFFIXES sentencepiece
    )
    
    find_library(SENTENCEPIECE_LIBRARY
        NAMES sentencepiece
        PATHS /usr/lib /usr/local/lib
    )
    
    if(SENTENCEPIECE_INCLUDE_DIR AND SENTENCEPIECE_LIBRARY)
        set(SENTENCEPIECE_FOUND TRUE)
        set(SENTENCEPIECE_INCLUDE_DIRS ${SENTENCEPIECE_INCLUDE_DIR})
        set(SENTENCEPIECE_LIBRARIES ${SENTENCEPIECE_LIBRARY})
    else()
        message(WARNING "SentencePiece not found, will use bundled version if available")
        # Add path to third_party sentencepiece if bundled
        set(SENTENCEPIECE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sentencepiece/src)
        set(SENTENCEPIECE_LIBRARIES sentencepiece)
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RKNN_INCLUDE_DIRS}
    ${SENTENCEPIECE_INCLUDE_DIRS}
)

# Library source files
set(LLM_RKNN_SOURCES
    src/llm_rknn.cpp
    src/llm_context.cpp
    src/sampler.cpp
    src/tokenizer_sentencepiece.cpp
)

set(LLM_RKNN_HEADERS
    include/llm_rknn.h
    src/llm_rknn_internal.h
    src/tokenizer_sentencepiece.h
)

# Create library
if(BUILD_SHARED_LIBS)
    add_library(llm-rknn SHARED ${LLM_RKNN_SOURCES} ${LLM_RKNN_HEADERS})
    set_target_properties(llm-rknn PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    add_library(llm-rknn STATIC ${LLM_RKNN_SOURCES} ${LLM_RKNN_HEADERS})
endif()

# Link libraries
target_link_libraries(llm-rknn
    PUBLIC
        ${RKNN_LIBRARIES}
    PRIVATE
        ${SENTENCEPIECE_LIBRARIES}
        pthread
)

# Set include directories for the library
target_include_directories(llm-rknn
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler flags
target_compile_options(llm-rknn PRIVATE
    -Wall
    -Wextra
    -O3
    -march=native
)

# Build examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
install(TARGETS llm-rknn
    EXPORT llm-rknn-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES include/llm_rknn.h
    DESTINATION include
)

# Export configuration
install(EXPORT llm-rknn-targets
    FILE llm-rknn-targets.cmake
    NAMESPACE llm-rknn::
    DESTINATION lib/cmake/llm-rknn
)

# Create config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/llm-rknn-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/llm-rknn-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/llm-rknn-config.cmake"
    INSTALL_DESTINATION lib/cmake/llm-rknn
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/llm-rknn-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/llm-rknn-config-version.cmake"
    DESTINATION lib/cmake/llm-rknn
)

# Print configuration summary
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  libllm-rknn Configuration Summary")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library:   ${BUILD_SHARED_LIBS}")
message(STATUS "  Build examples:   ${BUILD_EXAMPLES}")
message(STATUS "  C++ compiler:     ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "  Dependencies:")
message(STATUS "    RKNN:           ${RKNN_LIBRARIES}")
message(STATUS "    SentencePiece:  ${SENTENCEPIECE_LIBRARIES}")
message(STATUS "")
message(STATUS "  Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")
