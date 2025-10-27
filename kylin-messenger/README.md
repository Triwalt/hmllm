# Kylin Messenger

**一个现代化的P2P局域网通讯应用，集成NPU加速的AI功能**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/Platform-Kylin%20Linux-orange.svg)](https://www.kylinos.cn/)

---

## 📋 目录

- [项目概述](#项目概述)
- [核心特性](#核心特性)
- [系统架构](#系统架构)
- [技术栈](#技术栈)
- [构建指南](#构建指南)
- [安装说明](#安装说明)
- [使用指南](#使用指南)
- [AI功能](#ai功能)
- [开发文档](#开发文档)
- [贡献指南](#贡献指南)

---

## 🎯 项目概述

**Kylin Messenger** 是一个专为瑞芯微开发板和麒麟操作系统设计的现代化P2P局域网通讯应用。它复刻了经典飞秋软件的核心功能，同时创新性地集成了基于NPU加速的AI特性。

### 设计目标

- **无服务器架构**: 完全P2P通信，无需中央服务器
- **自动发现**: UDP广播自动发现局域网内在线用户
- **可靠传输**: TCP保证消息和文件的可靠传输
- **AI增强**: NPU加速的智能助手、图像标注、智能回复等
- **易用性**: 简洁直观的Qt6界面设计
- **可扩展性**: 模块化AI服务接口，轻松添加新功能

---

## ✨ 核心特性

### 网络通信

- ✅ **自动用户发现**: UDP广播机制自动发现在线用户
- ✅ **即时消息**: 支持单聊、群聊和广播消息
- ✅ **文件传输**: 点对点文件传输，支持断点续传
- ✅ **状态指示**: 实时打字状态、消息已读回执
- ✅ **群组管理**: 本地群组创建和成员管理
- ✅ **表情支持**: 内置表情包，支持自定义表情

### AI功能（基于RKNN NPU加速）

- 🤖 **AI聊天助手**: 集成libllm-rknn，本地LLM对话
- 🏷️ **智能图像标注**: YOLO目标检测自动标注截图
- 💬 **智能回复建议**: 上下文感知的快速回复建议
- 🛡️ **内容过滤**: 客户端实时内容安全检测
- 🎨 **可扩展框架**: 抽象AI服务接口，易于添加新模型

### 用户体验

- 🖥️ **现代UI**: Qt6 Material Design风格界面
- 📸 **截图工具**: 集成屏幕截图，直接发送
- 🔍 **快速搜索**: 按用户名、组名、IP搜索
- 📁 **拖放支持**: 拖放文件直接发送
- 🌙 **主题支持**: 明亮/暗黑主题切换

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Kylin Messenger                        │
│                      (Qt 6 Application)                     │
└─────────────────────────────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   UI Layer   │    │ Network Layer│    │   AI Layer   │
│   (Qt6 GUI)  │    │  (P2P Comm)  │    │ (NPU Accel)  │
└──────────────┘    └──────────────┘    └──────────────┘
        │                    │                    │
        │            ┌───────┴───────┐            │
        │            ▼               ▼            │
        │    ┌──────────┐    ┌──────────┐        │
        │    │   UDP    │    │   TCP    │        │
        │    │Broadcast │    │ Reliable │        │
        │    └──────────┘    └──────────┘        │
        │                                         │
        │                    ┌────────────────────┤
        │                    │                    │
        │                    ▼                    ▼
        │            ┌──────────────┐    ┌──────────────┐
        │            │ User         │    │ AI Service   │
        │            │ Discovery    │    │ Interface    │
        │            └──────────────┘    └──────────────┘
        │                                         │
        └─────────────────────────────────────────┤
                                                  │
                    ┌─────────────────────────────┴──────┐
                    │                                    │
                    ▼                                    ▼
            ┌──────────────┐                    ┌──────────────┐
            │ LLM Chat     │                    │ Image Tagger │
            │ (libllm-rknn)│                    │  (YOLO)      │
            └──────────────┘                    └──────────────┘
                    │                                    │
                    │           RKNN Runtime             │
                    │        (librknnrt.so)              │
                    │                                    │
                    └────────────────┬───────────────────┘
                                     │
                                     ▼
                              ┌────────────┐
                              │ NPU        │
                              │ Hardware   │
                              └────────────┘
```

---

## 🔧 技术栈

### 核心框架
- **Qt 6**: 跨平台GUI框架
- **C++17**: 现代C++标准
- **CMake 3.16+**: 构建系统

### 网络层
- **QUdpSocket**: UDP广播用户发现
- **QTcpServer/QTcpSocket**: TCP可靠传输
- **自定义协议**: 二进制协议，CRC32校验

### AI层
- **RKNN Runtime**: 瑞芯微NPU运行时
- **libllm-rknn**: LLM推理库
- **抽象接口**: 模块化AI服务设计

### 依赖库
- Qt6Core, Qt6Widgets, Qt6Network
- librknnrt.so (RKNN运行时)
- libllm-rknn.so (可选，用于LLM功能)

---

## 🚀 构建指南

### 前提条件

#### 硬件
- 瑞芯微开发板（RK3588/RK3576/RK3568等）
- 至少2GB RAM
- 支持NPU的SoC

#### 软件
- 麒麟操作系统 (Debian-based)
- Qt 6.2+
- CMake 3.16+
- GCC 9.0+ 或 Clang 10.0+
- RKNN Runtime 2.0+

### 安装依赖

```bash
# 更新系统
sudo apt update

# 安装Qt6开发包
sudo apt install qt6-base-dev qt6-tools-dev qt6-tools-dev-tools

# 安装构建工具
sudo apt install build-essential cmake git

# 安装RKNN运行时（如果尚未安装）
# 请参考瑞芯微官方文档
```

### 编译项目

```bash
# 克隆仓库
git clone https://github.com/yourusername/kylin-messenger.git
cd kylin-messenger

# 创建构建目录
mkdir build && cd build

# 配置
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DENABLE_AI_FEATURES=ON

# 编译
make -j$(nproc)

# 运行测试
make test

# 安装
sudo make install
```

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `CMAKE_BUILD_TYPE` | Release | 构建类型 (Debug/Release) |
| `ENABLE_AI_FEATURES` | ON | 启用AI功能 |
| `BUILD_TESTS` | ON | 构建单元测试 |
| `BUILD_DOCS` | OFF | 构建API文档 |

---

## 📦 安装说明

### 从DEB包安装（推荐）

```bash
# 构建DEB包
cd build
make package

# 安装
sudo dpkg -i kylin-messenger_1.0.0_arm64.deb

# 解决依赖
sudo apt -f install
```

### 从源码安装

```bash
cd build
sudo make install
sudo ldconfig
```

### 卸载

```bash
# DEB包
sudo apt remove kylin-messenger

# 源码安装
cd build
sudo make uninstall
```

---

## 📖 使用指南

### 首次运行

1. 启动应用：从应用菜单或运行 `kylin-messenger`
2. 设置用户信息：首次运行会提示设置用户名和状态
3. 自动发现：应用会自动发现局域网内的其他用户

### 基本操作

#### 发送消息
1. 在用户列表中选择用户
2. 双击或右键点击"发送消息"
3. 在聊天窗口输入消息并发送

#### 发送文件
- 方法1: 右键用户 → "发送文件"
- 方法2: 拖放文件到聊天窗口
- 方法3: 点击聊天窗口的文件按钮

#### 发送截图
1. 点击截图按钮或按 `Ctrl+Alt+A`
2. 拖动选择截图区域
3. 截图自动发送到当前聊天

#### 群组管理
1. 右键用户 → "添加到组"
2. 创建新组或选择现有组
3. 对组发送消息：右键组 → "发送组消息"

### AI功能使用

#### AI聊天助手
1. 在用户列表中找到"AI助手"
2. 双击打开聊天窗口
3. 向AI提问，获得本地化LLM回复

#### 智能回复
1. 接收消息时，窗口底部显示智能回复建议
2. 点击建议按钮快速回复

#### 图像标注
1. 发送截图或图片时
2. AI自动检测并标注图像内容
3. 标签显示在图片下方

---

## 🤖 AI功能

### AI服务架构

Kylin Messenger使用抽象的`IAIService`接口，所有AI功能都是该接口的实现。这使得添加新的AI模型变得非常简单。

#### 接口定义

```cpp
class IAIService {
public:
    virtual bool initialize(const std::string& model_path) = 0;
    virtual AIResult processText(const std::string& input) = 0;
    virtual AIResult processImage(const QImage& image) = 0;
    virtual AIResult generateSmartReplies(
        const std::vector<std::string>& history) = 0;
    // ... 更多方法
};
```

### 内置AI服务

#### 1. LLM聊天助手 (`LLMChatService`)

基于libllm-rknn实现，提供本地化大语言模型对话。

**特性**:
- 完全本地运行，无需联网
- NPU加速，低延迟响应
- 支持多轮对话上下文
- 流式输出，实时显示

**使用**:
```cpp
auto llm = AIServiceFactory::createService("llm_chat");
llm->initialize("/path/to/model.rknn");
auto result = llm->processText("你好，请介绍一下自己");
```

#### 2. 图像标注服务 (`ImageTaggerService`)

使用YOLO模型进行目标检测和标注。

**特性**:
- 实时目标检测
- 自动生成描述性标签
- 支持80+类别物体识别

**使用**:
```cpp
auto tagger = AIServiceFactory::createService("image_tagger");
tagger->initialize("/path/to/yolo.rknn");
auto result = tagger->processImage(screenshot);
// result.metadata["tags"] = ["window", "code", "terminal"]
```

#### 3. 智能回复服务 (`SmartReplyService`)

基于文本分类模型生成快速回复建议。

**特性**:
- 上下文感知
- 3个智能建议
- 支持中英文

**使用**:
```cpp
auto reply = AIServiceFactory::createService("smart_reply");
reply->initialize("/path/to/classifier.rknn");
std::vector<std::string> history = {"你在吗？", "在的"};
auto result = reply->generateSmartReplies(history);
// result.suggestions = ["好的", "明白了", "收到"]
```

### 添加自定义AI服务

1. **创建服务类**:

```cpp
class MyAIService : public IAIService {
public:
    bool initialize(const std::string& model_path) override {
        // 加载RKNN模型
        rknn_init(&ctx, model_data, model_size, 0, nullptr);
        return true;
    }
    
    AIResult processText(const std::string& input) override {
        // 实现您的处理逻辑
        AIResult result;
        result.success = true;
        result.text_output = "处理结果";
        return result;
    }
    
    // 实现其他必需方法...
};
```

2. **注册到工厂**:

```cpp
AIServiceFactory::registerService("my_service", 
    []() { return std::make_unique<MyAIService>(); });
```

3. **使用服务**:

```cpp
auto service = AIServiceFactory::createService("my_service");
```

---

## 📚 开发文档

### 项目结构

```
kylin-messenger/
├── src/                    # 源代码
│   ├── main.cpp           # 应用入口
│   ├── network/           # 网络层实现
│   ├── ui/                # UI组件
│   ├── ai/                # AI服务实现
│   └── utils/             # 工具类
├── include/               # 头文件
│   ├── ai_service.h       # AI接口定义
│   ├── network_protocol.h # 网络协议
│   └── network_manager.h  # 网络管理器
├── ui/                    # Qt UI文件
│   ├── mainwindow.ui
│   ├── chatwindow.ui
│   └── settingsdialog.ui
├── resources/             # 资源文件
│   ├── icons/            # 图标
│   ├── emojis/           # 表情包
│   └── qss/              # 样式表
├── debian/               # Debian打包文件
│   ├── control
│   ├── rules
│   └── copyright
├── cmake/                # CMake模块
├── docs/                 # 文档
├── tests/                # 单元测试
├── CMakeLists.txt        # 根CMake文件
└── README.md             # 本文件
```

### API文档

完整的API文档可以使用Doxygen生成：

```bash
cd build
make docs
# 文档位于 build/docs/html/index.html
```

---

## 🤝 贡献指南

欢迎贡献！请遵循以下步骤：

1. Fork本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启Pull Request

### 代码规范

- 遵循C++17标准
- 使用Qt代码风格
- 所有公共API必须有Doxygen注释
- 新功能需要包含单元测试

---

## 📄 许可证

本项目采用 Apache License 2.0 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 🙏 致谢

- Qt Company - Qt框架
- 瑞芯微 - RKNN运行时和NPU支持
- 麒麟操作系统团队
- 所有贡献者

---

## � 联系方式

- 项目主页: https://github.com/yourusername/kylin-messenger
- 问题反馈: https://github.com/yourusername/kylin-messenger/issues
- 邮箱: your.email@example.com

---

**用 ❤️ 为麒麟生态打造**

