## Toolchain file for cross-compiling to RK3566 (aarch64)
## Edit SYSROOT to point to your target sysroot path (where target /usr and /lib are mirrored)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compilers (assumes aarch64-linux-gnu toolchain installed)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Adjust this to your sysroot location (e.g. /opt/rk3566-sysroot)
set(SYSROOT "${CMAKE_SOURCE_DIR}/sysroot" CACHE PATH "Path to target sysroot")

if(NOT IS_DIRECTORY ${SYSROOT})
  message(WARNING "Sysroot '${SYSROOT}' not found. Please create or set SYSROOT to your RK3566 sysroot before cross-compiling.")
endif()

set(CMAKE_SYSROOT ${SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${SYSROOT})

# Search for libraries and headers in the sysroot first
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Tell Qt CMake where to look if you installed Qt into the sysroot
# Example usage when invoking cmake:
# cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-rk3566.cmake \
#       -DQT_SYSROOT=/path/to/sysroot/usr/local/Qt6 ..

if(DEFINED QT_SYSROOT)
  # Prefer Qt from sysroot
  set(CMAKE_PREFIX_PATH "${QT_SYSROOT}" ${CMAKE_PREFIX_PATH})
endif()
