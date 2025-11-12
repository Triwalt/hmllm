# Contributing to Kylin Messenger

Thank you for your interest in contributing! This project powers a cross-platform LAN messenger with AI enhancements. The following guidelines help keep contributions consistent and maintainable.

## Getting Started

1. **Fork the repository** and create a feature branch from `main`.
2. **Install prerequisites**:
   - CMake 3.16+
   - Ninja or Make
   - Qt 6.5+ (Widgets, Network, Test modules)
   - A C++17 compiler (MSVC 2019+/Clang 13+/GCC 11+)
   - Python 3.8+（用于本地 NSFW 审核）
   - 可选：RKNN Runtime 2.x 与 RKNN Toolkit2（RK3566/RK3588 平台）
3. **Configure the project**
   ```bash
   cmake -S kylin-messenger -B build -GNinja -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```
4. **Install runtime assets**
   ```bash
   pip install nsfw-detector
   curl -L -o models/nsfw_mobilenet2.224x224.h5 \
     https://github.com/GantMan/nsfw_model/releases/download/1.1.0/nsfw_mobilenet2.224x224.h5
   ```
   Set environment variables when running the app:
   ```bash
   export KYLIN_NSFW_MODEL=/absolute/path/to/nsfw_mobilenet2.224x224.h5
   export KYLIN_NSFW_PYTHON=python3        # optional, override interpreter
   # export KYLIN_NSFW_BACKEND=rknn        # future RKNN backend
   ```
   When working on RK3566 with RKNN2 acceleration, install the official Toolkit2, copy
   `librknnrt.so` to the sysroot, and ensure `ENABLE_AI_FEATURES=ON` at configure time.

5. **Run tests** before submitting
   ```bash
   ctest --test-dir build
   ```

## Coding Standards

- Follow the `.clang-format` and `.clang-tidy` configurations in the repository.
- Prefer **modern C++17** constructs (smart pointers, `std::optional`, `std::chrono`, structured bindings).
- Use **Qt idioms** consistently (`QString`, `QVector`, signals/slots). Avoid mixing raw pointers with Qt ownership patterns.
- Place public headers under `include/` and implementation files under `src/`. Keep headers free of heavy dependencies when possible.
- Document classes and complex functions with Doxygen-style comments.
- Favor **small, focused PRs** with descriptive commit messages.

## Branch & Commit Conventions

- Branch names: `feature/<summary>` or `fix/<issue>`.
- Commit messages: `Type: short summary` (e.g. `UI: add dark theme toggle`).
- Reference issues with `Fixes #123` when applicable.

## Pull Request Checklist

- [ ] Code builds on Windows, Linux, and macOS (CI will verify).
- [ ] All unit and integration tests pass.
- [ ] New features include tests and documentation updates.
- [ ] UI changes include before/after screenshots or screencasts when relevant.
- [ ] No secrets, API keys, or proprietary assets added.
- [ ] `CHANGELOG.md` updated (if applicable).

## Reporting Issues

- Check existing issues before filing a new one.
- Provide reproduction steps, logs, and platform details.
- For security issues, please contact the maintainers privately.

## Licensing

By contributing, you agree that your contributions will be licensed under the project’s Apache 2.0 License.

We appreciate your help in making Kylin Messenger better!

