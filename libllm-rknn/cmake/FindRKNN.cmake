# Find RKNN runtime library
#
# This module defines:
#  RKNN_FOUND - True if RKNN is found
#  RKNN_INCLUDE_DIRS - Include directories for RKNN
#  RKNN_LIBRARIES - Libraries to link against

# Try to find RKNN in common locations
find_path(RKNN_INCLUDE_DIR
    NAMES rknn_api.h
    PATHS
        /usr/include
        /usr/local/include
        ${CMAKE_SOURCE_DIR}/../rknpu2/runtime/Linux/librknn_api/include
        ${CMAKE_SOURCE_DIR}/../../rknpu2/runtime/Linux/librknn_api/include
        ENV RKNN_INCLUDE_DIR
    PATH_SUFFIXES rknn
)

# Determine architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(RKNN_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(RKNN_ARCH "armhf")
else()
    set(RKNN_ARCH "x86_64")
endif()

find_library(RKNN_LIBRARY
    NAMES rknnrt librknnrt
    PATHS
        /usr/lib
        /usr/local/lib
        /usr/lib/${RKNN_ARCH}-linux-gnu
        ${CMAKE_SOURCE_DIR}/../rknpu2/runtime/Linux/librknn_api/${RKNN_ARCH}
        ${CMAKE_SOURCE_DIR}/../../rknpu2/runtime/Linux/librknn_api/${RKNN_ARCH}
        ENV RKNN_LIBRARY_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RKNN
    REQUIRED_VARS RKNN_LIBRARY RKNN_INCLUDE_DIR
)

if(RKNN_FOUND)
    set(RKNN_LIBRARIES ${RKNN_LIBRARY})
    set(RKNN_INCLUDE_DIRS ${RKNN_INCLUDE_DIR})
    
    message(STATUS "Found RKNN:")
    message(STATUS "  Include: ${RKNN_INCLUDE_DIRS}")
    message(STATUS "  Library: ${RKNN_LIBRARIES}")
endif()

mark_as_advanced(RKNN_INCLUDE_DIR RKNN_LIBRARY)
