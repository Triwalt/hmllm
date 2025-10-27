# libllm-rknn

**一个用于在瑞芯微NPU上运行大语言模型(LLM)的高级C/C++库，基于RKNN运行时。**

[![许可证](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![版本](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/yourusername/libllm-rknn)

---

## 📋 目录

- [概述](#概述)
- [特性](#特性)
- [系统架构](#系统架构)
- [系统要求](#系统要求)
- [安装](#安装)
  - [依赖项](#依赖项)
  - [从源码编译](#从源码编译)
- [使用方法](#使用方法)
  - [快速开始](#快速开始)
  - [API文档](#api文档)
  - [示例应用](#示例应用)
- [模型准备](#模型准备)
- [性能调优](#性能调优)
- [故障排除](#故障排除)
- [贡献](#贡献)
- [许可证](#许可证)

---

## 🎯 概述

**libllm-rknn** 是一个生产就绪的软件库，使开发者能够轻松地在瑞芯微SoC上部署对话式大语言模型，并利用NPU加速。它提供了一个类似于瑞芯微RKLLM库的简单、高级C/C++ API，但可以使用标准的`.rknn`模型，为您提供更广泛的硬件支持和灵活性。

### 为什么选择libllm-rknn？

- **简单的API**: 仅需几行代码即可部署LLM
- **基于RKNN**: 使用广泛支持的RKNN运行时作为后端
- **生产就绪**: 包含KV缓存优化、多种采样方法和流式输出
- **灵活**: 可用于任何能转换为RKNN格式的LLM模型
- **开源**: 完全控制推理流程

---

## ✨ 特性

### 核心功能

- ✅ **自回归文本生成**: 完整实现LLM推理循环
- ✅ **流式输出**: 逐token回调接口，实现实时生成
- ✅ **KV缓存管理**: 高效内存管理，实现快速推理
- ✅ **多种采样方法**: 贪婪、Top-K、Top-P(核采样)和组合采样
- ✅ **分词**: 集成SentencePiece分词器支持
- ✅ **性能分析**: 内置统计信息，包括token/秒、延迟跟踪
- ✅ **对话管理**: 重置和多轮对话支持

### 技术特性

- **NPU加速**: 通过RKNN运行时充分利用瑞芯微NPU核心
- **多线程**: 优化的CPU预处理，可配置线程数
- **温度缩放**: 可调节的生成随机性
- **重复惩罚**: 减少重复输出
- **特殊Token处理**: 自动BOS/EOS/PAD token管理

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                    用户应用程序                              │
│                  (llm_demo, 您的应用)                       │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ 公共C API
                         │ (llm_rknn.h)
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                    libllm-rknn.so                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   分词器     │  │   采样器     │  │  KV缓存      │     │
│  │ (SentPiece)  │  │(贪婪/TopK)   │  │  管理器      │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│  ┌─────────────────────────────────────────────────────┐   │
│  │         自回归推理循环                               │   │
│  │    (预填充 → 解码 → 采样 → 重复)                    │   │
│  └─────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────┘
                         │
                         │ RKNN运行时API
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                 librknnrt.so (RKNN运行时)                   │
│                                                             │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                 │
│  │ NPU核心  │  │ NPU核心  │  │ NPU核心  │  ...            │
│  │    0     │  │    1     │  │    2     │                 │
│  └──────────┘  └──────────┘  └──────────┘                 │
└─────────────────────────────────────────────────────────────┘
                         │
                         ▼
                   .rknn 模型文件
```

---

## 📦 系统要求

### 硬件
- 支持NPU的瑞芯微SoC:
  - RK3588/RK3588S
  - RK3576
  - RK3566/RK3568
  - RV1106/RV1103
  - RK2118
  - 或任何其他RKNPU2兼容芯片

### 软件
- **操作系统**: Linux (ARM64 或 ARM32)
  - Debian 11+ / Ubuntu 20.04+
  - 基于Buildroot的系统
- **编译器**: GCC 7.5+ 或 Clang 9.0+
- **CMake**: 3.10 或更新版本
- **RKNN运行时**: librknnrt.so (v2.0.0 或更新版本)
- **SentencePiece**: 用于分词

### 可选
- **RKNN-Toolkit2**: 用于模型转换(Python，可在x86主机上运行)

---

## 🚀 安装

### 依赖项

#### 1. 安装RKNN运行时

RKNN运行时由瑞芯微提供。如果尚未安装:

```bash
# 以RK3588为例
cd rknpu2/runtime/Linux/librknn_api
sudo cp aarch64/librknnrt.so /usr/lib/
sudo cp include/rknn_api.h /usr/include/
sudo ldconfig
```

#### 2. 安装SentencePiece

```bash
# 从包管理器安装(如果可用)
sudo apt-get install libsentencepiece-dev

# 或从源码编译
git clone https://github.com/google/sentencepiece.git
cd sentencepiece
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 从源码编译

```bash
# 克隆仓库
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# 创建构建目录
mkdir build && cd build

# 配置
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_EXAMPLES=ON

# 编译
make -j$(nproc)

# 安装(可选)
sudo make install
sudo ldconfig
```

### 交叉编译

从x86主机交叉编译:

```bash
mkdir build-arm64 && cd build-arm64

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DRKNN_INCLUDE_DIR=/path/to/rknpu2/include \
    -DRKNN_LIBRARY=/path/to/rknpu2/aarch64/librknnrt.so

make -j$(nproc)
```

---

## 💡 使用方法

### 快速开始

这是一个最小示例:

```c
#include "llm_rknn.h"
#include <stdio.h>

// 回调函数接收生成的token
int callback(const char* token_text, int32_t token_id, void* user_data) {
    printf("%s", token_text);
    fflush(stdout);
    return 0;  // 继续生成
}

int main() {
    // 1. 初始化库
    llm_rknn_handle_t handle = llm_rknn_init(
        "model.rknn",           // RKNN模型路径
        "tokenizer.model",      // 分词器路径
        NULL                    // 使用默认配置
    );
    
    if (!handle) {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }
    
    // 2. 生成文本
    const char* prompt = "从前有一个";
    int ret = llm_rknn_generate(handle, prompt, callback, NULL);
    
    if (ret != LLM_RKNN_SUCCESS) {
        fprintf(stderr, "生成失败: %s\n", 
                llm_rknn_get_error_string(ret));
    }
    
    // 3. 清理
    llm_rknn_release(handle);
    
    return 0;
}
```

编译并运行:

```bash
gcc -o my_app my_app.c -lllm-rknn
./my_app
```

### 高级配置

```c
#include "llm_rknn.h"

int main() {
    // 创建自定义配置
    llm_rknn_config_t config;
    llm_rknn_init_default_config(&config);
    
    // 自定义参数
    config.max_new_tokens = 256;
    config.sampling_method = LLM_RKNN_SAMPLING_TOP_K;
    config.temperature = 0.8f;
    config.top_k = 40;
    config.top_p = 0.95f;
    config.rknn_core_mask = 3;  // 使用NPU核心0和1
    
    // 使用自定义配置初始化
    llm_rknn_handle_t handle = llm_rknn_init(
        "model.rknn",
        "tokenizer.model",
        &config
    );
    
    // ... 使用handle ...
    
    llm_rknn_release(handle);
    return 0;
}
```

### API文档

#### 初始化

```c
llm_rknn_handle_t llm_rknn_init(
    const char* rknn_model_path,
    const char* tokenizer_path,
    llm_rknn_config_t* config  // NULL表示使用默认值
);
```

加载RKNN模型和分词器，初始化NPU上下文。

**参数:**
- `rknn_model_path`: `.rknn`模型文件路径
- `tokenizer_path`: 分词器模型路径(例如SentencePiece的`tokenizer.model`)
- `config`: 配置结构体(或NULL表示使用默认值)

**返回值:** LLM实例句柄，失败时返回NULL

#### 生成

```c
int llm_rknn_generate(
    llm_rknn_handle_t handle,
    const char* prompt,
    llm_rknn_callback_t callback,
    void* user_data
);
```

从提示词生成文本，支持流式输出。

**回调签名:**
```c
int callback(const char* token_text, int32_t token_id, void* user_data);
// 返回0继续，非零值停止生成
```

#### 性能统计

```c
int llm_rknn_get_perf_stats(
    llm_rknn_handle_t handle,
    llm_rknn_perf_stats_t* stats
);
```

获取上次生成的性能指标:
- `total_tokens_generated`: 生成的token数量
- `prefill_time_ms`: 提示词处理时间
- `decode_time_ms`: token生成时间
- `tokens_per_second`: 生成速度

#### 清理

```c
void llm_rknn_release(llm_rknn_handle_t handle);
```

释放所有资源。调用后句柄失效。

### 示例应用

#### 交互式聊天机器人

```bash
cd build/bin
./llm_demo -m /path/to/model.rknn -t /path/to/tokenizer.model
```

这将启动一个交互式REPL，您可以与模型对话。

#### 单次提示

```bash
./llm_demo -m model.rknn -t tokenizer.model -p "解释量子计算"
```

#### 自定义采样

```bash
./llm_demo -m model.rknn -t tokenizer.model \
    -s topk_topp \
    -T 0.7 \
    -k 50 \
    -P 0.9 \
    -n 200
```

---

## 🔧 模型准备

### 将模型转换为RKNN格式

您需要使用**RKNN-Toolkit2**(在x86/x64上运行)将LLM转换为RKNN格式:

```python
from rknn.api import RKNN

# 初始化RKNN
rknn = RKNN(verbose=True)

# 配置LLM
rknn.config(
    target_platform='rk3588',
    optimization_level=3,
    quantized_dtype='int8'  # 或 'fp16'
)

# 加载ONNX/PyTorch模型
rknn.load_onnx(model='llm_model.onnx')

# 构建RKNN模型
rknn.build(do_quantization=True, dataset='calibration_data.txt')

# 导出
rknn.export_rknn('llm_model.rknn')

rknn.release()
```

### 分词器准备

对于SentencePiece分词器，从您的模型导出:

```python
# 对于Hugging Face模型
from transformers import AutoTokenizer

tokenizer = AutoTokenizer.from_pretrained("model_name")
tokenizer.save_pretrained("./tokenizer")
# 使用tokenizer.model文件
```

### 模型要求

您的RKNN模型应该:
- 接受token ID作为输入(int32)
- 输出词汇表上的logits(float32)
- 可选地支持KV缓存输入/输出以获得最佳性能

---

## ⚡ 性能调优

### 采样方法选择

- **贪婪(Greedy)**: 最快，确定性，但可能重复
- **Top-K**: 速度和质量的良好平衡
- **Top-P**: 更多样化，稍慢
- **Top-K + Top-P**: 最佳质量，最慢

### 温度

- `temperature = 0.1`: 非常专注，确定性
- `temperature = 1.0`: 平衡(默认)
- `temperature = 1.5`: 更有创意，多样化

### NPU核心使用

```c
config.rknn_core_mask = 0;  // 自动(推荐)
config.rknn_core_mask = 1;  // 仅核心0
config.rknn_core_mask = 2;  // 仅核心1
config.rknn_core_mask = 3;  // 两个核心(RK3588)
```

### 内存优化

如果遇到内存问题，减少`max_context_length`:

```c
config.max_context_length = 1024;  // 而不是2048
```

---

## 🐛 故障排除

### 常见问题

**问题**: `加载模型失败`
- **解决方案**: 确保模型路径正确且文件可读
- 检查模型是否为正确的目标平台构建

**问题**: `分词器错误`
- **解决方案**: 验证tokenizer.model文件是有效的SentencePiece格式
- 尝试从源模型转换分词器

**问题**: `NPU运行时错误`
- **解决方案**: 确保RKNN运行时版本与模型版本匹配
- 检查NPU驱动是否正确安装: `dmesg | grep rknpu`

**问题**: 性能慢
- **解决方案**: 
  - 启用KV缓存: `config.enable_kv_cache = true`
  - 转换模型时使用int8量化
  - 减少max_new_tokens
  - 使用贪婪采样

### 调试模式

使用调试符号编译:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

---

## 🤝 贡献

欢迎贡献！请:

1. Fork仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m '添加惊人特性'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启Pull Request

### 代码规范

- 遵循现有代码风格(C++17, Google C++风格指南)
- 为复杂逻辑添加注释
- 为新特性包含单元测试

---

## 📄 许可证

本项目采用Apache License 2.0许可证 - 详见[LICENSE](LICENSE)文件。

---

## 🙏 致谢

- **瑞芯微** 提供RKNN运行时和NPU硬件
- **Google** 提供SentencePiece分词器
- 开源AI社区

---

## 📞 联系与支持

- **问题**: [GitHub Issues](https://github.com/yourusername/libllm-rknn/issues)
- **讨论**: [GitHub Discussions](https://github.com/yourusername/libllm-rknn/discussions)

---

**为瑞芯微NPU社区用❤️制作**
