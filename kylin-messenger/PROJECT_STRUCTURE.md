# Kylin Messenger 项目结构

本文档描述 `hmllm` 仓库内各工程的组织方式，并重点说明 `kylin-messenger` 子项目的目录布局与核心模块。

最后更新：2025-11-13

## 仓库概览

```
hmllm/
├── kylin-messenger/     # 桌面端主程序（Qt + 微内核架构）
├── libllm-rknn/         # RKNN 推理适配层
├── rknn-toolkit2/       # 上游 RKNN 工具链镜像
└── scripts/             # 统一脚本与安装器
```

以下章节围绕 `kylin-messenger/` 展开。

## `kylin-messenger/` 顶层布局

```
kylin-messenger/
├── CMakeLists.txt        # 顶层构建入口
├── VERSION               # 应用版本号
├── README.md             # 使用说明
├── CHANGELOG.md          # 版本变更记录
├── CONTRIBUTING.md       # 贡献指南
├── FILE_MANIFEST.md      # 文件清单（需持续维护）
├── IMPLEMENTATION_SUMMARY.md
├── OPTIMIZATION_COMPLETE.md
├── PROJECT_STRUCTURE.md  # 本文档
├── QUICKSTART*.md, TESTING_*.md 等辅助文档
├── cmake/                # CMake 模块与工具链
├── docs/                 # 深度设计/调优文档
├── include/              # 公共头文件
├── src/                  # 源码实现
├── resources/            # Qt 资源（icons/emojis/qss/scripts）
├── models/               # 推理模型与权重
├── scripts/              # 构建、打包、诊断脚本
├── python/               # 可选 Python 工具
├── tests/                # 单元/集成/性能测试
├── desktop/              # 桌面快捷方式与 manifest
├── debian/               # Debian 打包配置
├── build*/               # 各平台/配置的构建输出（忽略）
└── WORK_IN_PROGRESS.md 等状态记录
```

## 头文件目录 `include/`

头文件按模块拆分，但仍保留部分历史兼容文件在根目录。当前布局如下：

```
include/
├── core/
│   ├── di/                   # 依赖注入容器（ServiceLocator）
│   ├── repositories/         # 仓储接口定义
│   ├── logging.h             # 日志设施
│   ├── micro_kernel.h        # 微内核总线
│   └── models.h              # 领域模型
├── network/
│   ├── async_transport.h
│   ├── constants.h
│   ├── gateway_client.h
│   ├── group_hub.h           # 预留群组管理接口
│   ├── ipmsg.h
│   ├── lightweight_discovery.h
│   └── payload_tags.h
├── transfer/
│   └── concurrent_file_transfer.h
├── ui/
│   ├── contact_list_page.h
│   ├── file_transfer_dialog.h
│   ├── group_list_page.h
│   └── user_list_page.h
├── ai/
│   └── opencv_nsfw_detector.h
├── utils/
│   └── qt_compat.h
├── ai_service.h / ai_echo_service.h / ai_service_factory.h
├── chat_window.h / main_window.h
├── compliance_service.h / compliance_stub_service.h
├── network_manager.h / network_protocol.h
├── nsfw_compliance_service.h / rknn_nsfw_compliance_service.h
└── version_info.h.in
```

> 说明：`compliance_*`, `network_manager.h` 等文件仍位于根目录，后续会按新架构划分到 `services/` 与 `network/` 子目录。

## 源码目录 `src/`

```
src/
├── core/
│   ├── di/service_locator.cpp
│   ├── logging.cpp
│   ├── micro_kernel.cpp
│   ├── models.cpp
│   └── repositories/
│       ├── contact_repository.cpp
│       └── in_memory_message_repository.cpp
├── network/
│   ├── async_transport.cpp
│   ├── gateway_client.cpp
│   ├── ipmsg.cpp
│   ├── lightweight_discovery.cpp
│   └── network_manager.cpp
├── transfer/
│   └── concurrent_file_transfer.cpp
├── services/
│   ├── compliance_stub_service.cpp
│   ├── nsfw_compliance_service.cpp
│   └── rknn_nsfw_compliance_service.cpp
├── ui/
│   ├── chat_window.cpp
│   ├── main_window.cpp
│   ├── user_list_page.cpp
│   ├── contact_list_page.cpp
│   ├── group_list_page.cpp
│   └── file_transfer_dialog.cpp
├── ai/
│   └── opencv_nsfw_detector.cpp
├── ai_echo_service.cpp
├── ai_service_factory.cpp
├── utils/ (预留占位)
└── main.cpp
```

与 `include/` 一样，部分旧有实现仍放在根目录，后续可移动到对应子目录。

## 测试与调试

- `tests/`：包含 `CMakeLists.txt` 与多条目测试（微内核、NSFW 检测、性能基准等）。
- `build/` 与 `build-win*/`：按平台生成的构建输出。`build-verify/` 用于验证脚本。
- `cmake/toolchains/`：面向 RK3566 等目标板的交叉编译工具链文件。

## 核心模块概览

- **微内核 `Core::MicroKernel`**：`include/core/micro_kernel.h` 与 `src/core/micro_kernel.cpp`，负责轻量级服务装载、事件分发、生命周期管理。
- **网络发现 `Network::LightweightDiscovery`**：`include/network/lightweight_discovery.h` 与 `src/network/lightweight_discovery.cpp`，完成自适应心跳与用户在线状态维护。
- **并发传输 `Transfer::ConcurrentFileTransfer`**：`include/transfer/concurrent_file_transfer.h` 与 `src/transfer/concurrent_file_transfer.cpp`，提供多线程分块传输能力。
- **UI 主窗口 `MainWindow`**：`include/main_window.h` 与 `src/ui/main_window.cpp`，封装三大页面与系统托盘。
- **合规服务**：`include/compliance_stub_service.h` 等头文件与 `src/services/` 实现，提供默认占位和 RKNN/Python 后端。

## 资源与模型

- `resources/icons/`、`resources/emojis/`、`resources/qss/`、`resources/scripts/`：对应的 `.qrc` 汇编在 `resources/kylin-messenger` 下，构建时统一打包。
- `models/`：当前包含 `mobilenetv2_nsfw.onnx`，供 OpenCV 轻量检测器使用。

## 维护建议

1. 在引入新模块时，同时更新 `PROJECT_STRUCTURE.md` 与 `FILE_MANIFEST.md`，保持目录描述与实际一致。
2. 按照微内核分层逐步整理遗留的顶层头文件与实现文件，建议移入 `include/services/`、`src/services/legacy/` 等分组目录。
3. 若新增资源，务必同步更新对应 `.qrc` 与打包脚本。

如需更细致的依赖与构建信息，可参考 `IMPLEMENTATION_SUMMARY.md` 与 `TESTING_STRATEGY.md`。