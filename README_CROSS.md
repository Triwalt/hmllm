RK3566 Cross-compilation Notes
==============================

This document contains notes and a minimal workflow to cross-compile `kylin-messenger` for RK3566 (aarch64).

1) Toolchain

 - Use `/usr/bin/aarch64-linux-gnu-gcc` and `aarch64-linux-gnu-g++` (installed on host via apt).
 - A CMake toolchain file is provided at `cmake/toolchains/aarch64-rk3566.cmake`.

2) Sysroot

 - Obtain a target sysroot from your RK3566 board. On the board:

   sudo tar -C / --numeric-owner -czf /tmp/sysroot.tar.gz \
       usr lib bin include opt

   Copy `sysroot.tar.gz` to host and extract to e.g. `/opt/rk3566-sysroot`.

 - Make sure the sysroot contains Qt6 development files if you want to link Qt6 on target.
   Easiest: install Qt6 on the RK3566 via its package manager (if available) and then export `/usr` from the board.

3) Qt6

 - Ubuntu 20.04 host may not have Qt6 in apt. Options:
   - Install Qt6 on host via the official Qt online installer and point CMake to that via `-DCMAKE_PREFIX_PATH=/home/<user>/Qt/6.x.x/gcc_64`.
   - Preferable for cross-build: install Qt6 on the RK3566 target and copy the headers/libs to sysroot.

4) Example cross build command

 cmake -S . -B build-rk3566 \
   -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-rk3566.cmake \
   -DSYSROOT=/opt/rk3566-sysroot \
   -DQT_SYSROOT=/opt/rk3566-sysroot/usr/local/Qt6 \
   -DCMAKE_BUILD_TYPE=Release

5) Notes

 - RKNN runtime (`librknnrt.so`) must exist in the sysroot (`/usr/lib` or `/usr/local/lib`) for AI features.
 - If you want a faster iteration loop, build and test on the board natively and use host only for model conversion.
