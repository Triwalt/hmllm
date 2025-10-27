# 构建和运行 libllm-rknn

本指南提供了在瑞芯微平台上构建、安装和使用libllm-rknn的详细说明。

## 目录

1. [前提条件](#前提条件)
2. [在目标设备上构建](#在目标设备上构建)
3. [交叉编译](#交叉编译)
4. [安装](#安装)
5. [运行示例](#运行示例)
6. [集成到您的项目](#集成到您的项目)

---

## 前提条件

### 必需依赖

1. **RKNN运行时** (librknnrt.so)
   - 版本: 2.0.0或更新
   - 通常随开发板SDK提供

2. **SentencePiece库**

   ```bash
   # 在Debian/Ubuntu上
   sudo apt-get update
   sudo apt-get install libsentencepiece-dev
   
   # 或从源码编译
   git clone https://github.com/google/sentencepiece.git
   cd sentencepiece
   mkdir build && cd build
   cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
   make -j$(nproc)
   sudo make install
   sudo ldconfig
   ```

3. **构建工具**

   ```bash
   sudo apt-get install build-essential cmake git
   ```

### 验证RKNN运行时

检查RKNN运行时是否已安装:

```bash
# 检查库
ldconfig -p | grep rknnrt

# 检查头文件
find /usr -name "rknn_api.h" 2>/dev/null

# 检查驱动
dmesg | grep rknpu
```

如果未找到，从开发板SDK安装:

```bash
cd /path/to/rknpu2/runtime/Linux/librknn_api

# 对于RK3588 (aarch64)
sudo cp aarch64/librknnrt.so /usr/lib/
sudo cp include/rknn_api.h /usr/include/
sudo ldconfig
```

---

## 在目标设备上构建

### 快速构建(推荐)

使用提供的构建脚本:

```bash
# 克隆仓库
git clone https://github.com/yourusername/libllm-rknn.git
cd libllm-rknn

# 使脚本可执行
chmod +x build.sh

# 构建(默认: Release、共享库、包含示例)
./build.sh

# 或使用自定义选项
./build.sh --debug --no-examples
./build.sh --static --prefix /opt/llm-rknn
```

### 手动构建

如果您更喜欢手动控制:

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_EXAMPLES=ON

# 构建
make -j$(nproc)

# 安装(可选)
sudo make install
sudo ldconfig
```

### 构建选项

| 选项 | 值 | 默认 | 描述 |
|------|-----|------|------|
| `CMAKE_BUILD_TYPE` | Release, Debug | Release | 优化级别 |
| `BUILD_SHARED_LIBS` | ON, OFF | ON | 构建共享(.so)或静态(.a)库 |
| `BUILD_EXAMPLES` | ON, OFF | ON | 构建示例应用 |
| `CMAKE_INSTALL_PREFIX` | 路径 | /usr/local | 安装目录 |

---

## 交叉编译

在x86/x64主机上为ARM目标构建:

### 设置交叉编译器

```bash
# 安装ARM64交叉编译器
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### 准备依赖

您需要在主机上准备ARM版本的依赖:

```bash
# 创建sysroot目录
mkdir -p ~/arm64-sysroot/usr/{lib,include}

# 从目标设备复制(或从SDK提取)
scp user@target:/usr/lib/librknnrt.so ~/arm64-sysroot/usr/lib/
scp user@target:/usr/include/rknn_api.h ~/arm64-sysroot/usr/include/
scp -r user@target:/usr/include/sentencepiece ~/arm64-sysroot/usr/include/
scp user@target:/usr/lib/libsentencepiece.so ~/arm64-sysroot/usr/lib/
```

### 创建工具链文件

创建 `cmake/aarch64-toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 交叉编译器
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Sysroot
set(CMAKE_SYSROOT ~/arm64-sysroot)
set(CMAKE_FIND_ROOT_PATH ~/arm64-sysroot)

# 搜索路径
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### 交叉编译

```bash
mkdir build-arm64 && cd build-arm64

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)

# 复制到目标设备
scp lib/libllm-rknn.so user@target:/usr/local/lib/
scp bin/llm_demo user@target:/usr/local/bin/
```

---

## 安装

### 系统范围安装

```bash
cd build
sudo make install
sudo ldconfig

# 验证安装
ldconfig -p | grep llm-rknn
ls -l /usr/local/include/llm_rknn.h
```

### 自定义位置安装

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/libllm-rknn
make install

# 更新库路径
export LD_LIBRARY_PATH=/opt/libllm-rknn/lib:$LD_LIBRARY_PATH

# 或添加到 /etc/ld.so.conf.d/
echo "/opt/libllm-rknn/lib" | sudo tee /etc/ld.so.conf.d/libllm-rknn.conf
sudo ldconfig
```

---

## 运行示例

### 准备测试文件

您需要:
1. 一个`.rknn`模型文件
2. 一个分词器模型文件(例如SentencePiece的`tokenizer.model`)

### 运行交互式演示

```bash
cd build/bin

./llm_demo \
    -m /path/to/model.rknn \
    -t /path/to/tokenizer.model
```

交互式命令:
- 输入提示词并按Enter
- 输入`reset`清除对话
- 输入`exit`或`quit`退出
- 按Ctrl+C中断生成

### 使用单次提示运行

```bash
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -p "写一首关于自然的诗"
```

### 高级用法

```bash
# 带温度的Top-K采样
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topk \
    -T 0.8 \
    -k 40 \
    -n 256

# Top-P(核)采样
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topp \
    -P 0.9 \
    -T 0.7

# 组合Top-K + Top-P
./llm_demo \
    -m model.rknn \
    -t tokenizer.model \
    -s topk_topp \
    -k 50 \
    -P 0.95 \
    -T 0.8
```

### 简单测试

```bash
./llm_simple_test model.rknn tokenizer.model
```

---

## 集成到您的项目

### 使用CMake

添加到您的`CMakeLists.txt`:

```cmake
find_package(llm-rknn REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app llm-rknn::llm-rknn)
```

### 使用pkg-config

```bash
# 编译
g++ -o my_app main.cpp $(pkg-config --cflags --libs llm-rknn)
```

### 手动链接

```bash
g++ -o my_app main.cpp \
    -I/usr/local/include \
    -L/usr/local/lib \
    -lllm-rknn \
    -lrknnrt \
    -lsentencepiece \
    -lpthread
```

### 最小示例

创建 `my_app.cpp`:

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
    
    llm_rknn_generate(handle, "你好", callback, nullptr);
    
    llm_rknn_release(handle);
    return 0;
}
```

编译并运行:

```bash
g++ -o my_app my_app.cpp -lllm-rknn
./my_app
```

---

## 故障排除

### 构建错误

**错误: "rknn_api.h: 没有那个文件或目录"**

解决方案:

```bash
# 查找RKNN头文件
find /usr -name "rknn_api.h" 2>/dev/null

# 如果找到，添加到CMake:
cmake .. -DRKNN_INCLUDE_DIR=/path/to/include
```

**错误: "找不到 -lrknnrt"**

解决方案:

```bash
# 查找库
find /usr -name "librknnrt.so" 2>/dev/null

# 如果找到，添加到CMake:
cmake .. -DRKNN_LIBRARY=/path/to/librknnrt.so
```

### 运行时错误

**错误: "加载共享库时出错: libllm-rknn.so"**

解决方案:

```bash
# 添加到库路径
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# 或运行ldconfig
sudo ldconfig
```

**错误: "初始化RKNN上下文失败"**

可能原因:
- 模型文件损坏或架构错误
- RKNN运行时版本不匹配
- NPU驱动未加载

检查:

```bash
# 验证NPU驱动
dmesg | grep rknpu

# 检查RKNN版本
strings /usr/lib/librknnrt.so | grep version
```

---

## 性能优化

### 编译器标志

获得最大性能:

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

### NPU核心选择

在您的代码中:

```c
llm_rknn_config_t config;
llm_rknn_init_default_config(&config);
config.rknn_core_mask = 3;  // 使用所有可用核心
```

### 内存设置

减少内存使用:

```c
config.max_context_length = 1024;  // 从2048减少
config.enable_kv_cache = true;     // 保持启用以提高速度
```

---

## 下一步

- 阅读[API文档](../README_CN.md#api文档)
- 查看[性能调优](../README_CN.md#性能调优)
- 探索[模型准备](../README_CN.md#模型准备)
- 加入社区讨论

---

如需更多帮助，请在GitHub上提出issue或查阅README_CN.md文件。
