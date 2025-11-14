# 快速开始测试指南

## 立即执行的测试步骤

### Step 1: 验证构建系统 (2分钟)

首先验证CMake配置和编译是否正常：

```bash
cd e:\Project\hmllm\kylin-messenger
mkdir build && cd build
cmake ..
```

**预期输出：**
- 找到OpenCV库
- 找到Threads库
- 找到Qt5或Qt6
- 配置摘要显示所有新组件（MicroKernel、NSFW Detector等）

**关键检查点：**
```
-- 找到OpenCV: 4.x.x
-- 找到Threads: TRUE
-- 项目: kylin-messenger v0.0.0
-- AI功能: ON
-- 构建测试: ON
```

### Step 2: 编译项目 (5-10分钟)

```bash
cd build
cmake --build . --parallel
```

**预期结果：**
- 无编译错误
- 生成 kylin-messenger 可执行文件
- 生成 kylin-messenger-core 库

**如果出错：**
- 检查OpenCV是否已安装：`pkg-config --modversion opencv4`
- 检查Qt版本：`qmake --version`
- 查看详细错误：`cmake --build . --verbose`

### Step 3: 运行基本功能测试 (3分钟)

先确保应用可以启动：

```bash
cd build
./kylin-messenger --version
```

**预期输出：**显示版本号和构建信息

测试网络通信：
```bash
# 开启详细日志
export QT_LOGGING_RULES="kylin.*=true"
./kylin-messenger
```

检查是否可以发现本地网络中的其他用户

### Step 4: 创建测试数据 (2分钟)

```bash
cd e:\Project\hmllm\kylin-messenger
mkdir -p tests/data models

# 创建测试文件
dd if=/dev/urandom of=tests/data/small_file.bin bs=1M count=10
dd if=/dev/urandom of=tests/data/medium_file.bin bs=1M count=50
dd if=/dev/urandom of=tests/data/large_file.bin bs=1M count=100

# 创建简单的测试图片（使用Python）
python3 -c "
from PIL import Image
import numpy as np

# 创建一张测试图片
data = np.random.randint(0, 255, (224, 224, 3), dtype=np.uint8)
img = Image.fromarray(data)
img.save('tests/data/test_image.jpg')
"
```

### Step 5: 单元测试 (5分钟)

编译测试框架（如果没有Google Test，先安装）：

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev libbenchmark-dev

# 然后重新配置CMake
cd build
cmake ..
cmake --build . --target test_micro_kernel
```

运行单元测试：

```bash
# 运行所有测试
ctest --output-on-failure

# 或者单独运行测试（待实现）
# ./tests/test_micro_kernel
# ./tests/test_opencv_nsfw_detector
```

**当前状态：**
由于四个核心组件(MicroKernel、NSFW Detector、LightweightDiscovery、ConcurrentFileTransfer)刚刚实现，相关的单元测试代码需要补充。建议优先编写以下测试：

1. **test_micro_kernel.cpp** - 验证服务注册和事件分发
2. **test_opencv_nsfw_detector.cpp** - 验证NSFW检测基本功能
3. **test_lightweight_discovery.cpp** - 验证用户发现和心跳机制
4. **test_concurrent_file_transfer.cpp** - 验证文件传输基本流程

### Step 6: 集成测试 (5分钟)

测试完整的用户流程：

#### 测试1: 点对点消息
1. 在两台计算机上运行应用
2. 计算机A：查看是否发现计算机B
3. 计算机A：向B发送消息
4. 计算机B：检查是否收到消息
5. 计算机B：回复消息

**检查日志输出：**
```
用户 "ComputerB" 已上线
发送消息到 192.168.1.x:1337
收到消息: "Hello"
```

#### 测试2: 文件传输
1. 准备测试文件（10MB以上）
2. 计算机A：向B发送文件
3. 观察传输进度和速度
4. 在B上验证文件完整性

```bash
# 验证文件完整性
md5sum tests/data/test_file.bin
# 与接收的文件比较
```

#### 测试3: NSFW检测（如有NSFW模型）

```bash
# 下载NSFW模型（可选）
wget -O models/nsfw_model.onnx \
  https://github.com/GantMan/nsfw_model/releases/download/1.2.0/nsfw_shuffle.onnx

# 发送一张测试图片
# 查看是否被正确标记为安全/不安全
```

### Step 7: 压力测试 (可选，5分钟)

测试高负载情况：

```bash
# 同时传输多个文件
for i in {1..5}; do
  cp tests/data/medium_file.bin tests/data/file_$i.bin &
done

# 发送所有文件
# 监控系统资源
htop  # 查看CPU和内存使用
```

## 创建缺失的测试文件（立即需要）

### 文件1: tests/test_micro_kernel.cpp

```cpp
#include <gtest/gtest.h>
#include "core/micro_kernel.h"
#include <QCoreApplication>
#include <QThread>

// 模拟服务
class TestService : public LightweightService {
public:
    bool receivedEvent = false;
    QString lastEventData;

    void handleEvent(const Event& event) override {
        receivedEvent = true;
        lastEventData = event.data.toString();
    }
};

TEST(MicroKernelBasicTest, ServiceRegistration) {
    MicroKernel kernel;
    auto service = std::make_unique<TestService>();

    EXPECT_TRUE(kernel.registerService("test", std::move(service)));
}

TEST(MicroKernelBasicTest, EventPublishing) {
    MicroKernel kernel;
    auto service = std::make_unique<TestService>();
    TestService* servicePtr = service.get();

    kernel.registerService("test", std::move(service));

    Event event("test.event", Event::Type::Custom);
    event.data = "test data";

    kernel.publishEvent(event);

    // 给事件处理一些时间
    QCoreApplication::processEvents();
    QThread::msleep(10);

    EXPECT_TRUE(servicePtr->receivedEvent);
    EXPECT_EQ(servicePtr->lastEventData, "test data");
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 文件2: tests/test_lightweight_discovery.cpp

```cpp
#include <gtest/gtest.h>
#include "network/lightweight_discovery.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

class LightweightDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        discovery = std::make_unique<LightweightDiscovery>(1337);
    }

    std::unique_ptr<LightweightDiscovery> discovery;
};

TEST_F(LightweightDiscoveryTest, Initialization) {
    EXPECT_FALSE(discovery->isRunning());
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 5000); // 初始5秒
}

TEST_F(LightweightDiscoveryTest, StartStop) {
    EXPECT_TRUE(discovery->start());
    EXPECT_TRUE(discovery->isRunning());

    QThread::msleep(100); // 等待启动

    discovery->stop();
    EXPECT_FALSE(discovery->isRunning());
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

## 快速检查清单

### 立即检查项（5分钟内验证）

- [ ] CMake配置无错误
- [ ] OpenCV库已找到
- [ ] Threads库已找到
- [ ] 项目成功编译
- [ ] 应用可以启动
- [ ] 可以在两台计算机间发现对方
- [ ] 可以发送文本消息
- [ ] 可以传输文件（小文件测试）

### 需要补充的测试

创建以下文件：
- [ ] `tests/test_micro_kernel.cpp` - 已提供模板
- [ ] `tests/test_opencv_nsfw_detector.cpp` - 需要实现
- [ ] `tests/test_lightweight_discovery.cpp` - 已提供模板
- [ ] `tests/test_concurrent_file_transfer.cpp` - 需要实现
- [ ] `tests/integration/test_*.cpp` - 集成测试
- [ ] `tests/performance_benchmark.cpp` - 已存在，需解除注释

### 下一步行动计划

**今天完成：**
1. 使用上面的模板创建基本单元测试
2. 运行构建验证所有组件编译正常
3. 在两台计算机上测试基本功能（消息+文件传输）

**本周完成：**
1. 完善所有单元测试
2. 实现集成测试
3. 运行性能基准测试
4. 手动测试所有场景

**持续进行：**
1. 每次修改后运行单元测试
2. 每次提交前进行集成测试
3. 每周运行性能测试跟踪性能趋势

## 遇到问题时的排查步骤

### 问题1: 找不到OpenCV

```bash
# 检查OpenCV是否安装
pkg-config --modversion opencv4

# 如果没有，安装OpenCV
sudo apt-get install libopencv-dev

# 如果已安装但CMake找不到，指定路径
cmake .. -DOpenCV_DIR=/usr/lib/x86_64-linux-gnu/cmake/opencv4
```

### 问题2: 编译错误

```bash
# 查看详细错误
cmake --build . --verbose

# 常见错误：缺少Qt头文件
sudo apt-get install qt6-base-dev
# 或
sudo apt-get install qtbase5-dev
```

### 问题3: 运行时崩溃

```bash
# 启用Qt调试输出
export QT_LOGGING_RULES="kylin.*=true"
./kylin-messenger 2>&1 | tee debug.log

# 使用GDB调试
gdb ./kylin-messenger
(gdb) run
(gdb) bt
```

### 问题4: 文件传输失败

- 检查防火墙：`sudo ufw status`
- 检查端口：确保1337端口开放
- 检查权限：读写权限是否正常
- 查看日志：启用详细日志查看原因

### 问题5: 网络发现失败

- 检查两台计算机在同一局域网
- 检查广播地址是否正确
- 使用tcpdump抓包：`tcpdump -i any port 1337`
- 检查是否有其他应用占用端口：`netstat -tulpn | grep 1337`

## 立即行动建议

按优先级顺序：

1. **现在做：**
   - 编译项目验证构建
   - 在两台计算机上运行测试基本消息功能
   - 创建测试数据目录

2. **今天做：**
   - 创建test_micro_kernel.cpp和test_lightweight_discovery.cpp
   - 测试文件传输功能
   - 检查日志输出是否正常

3. **明天做：**
   - 完善所有单元测试
   - 编写NSFW测试（如有模型）
   - 进行集成测试

4. **本周完成：**
   - 完成全套自动化测试
   - 性能基准测试
   - 长时间的稳定性测试

## 获取帮助

如果测试过程中遇到问题：

1. 查看详细日志：`export QT_LOGGING_RULES="*.*=true"`
2. 检查编译输出：使用 `--verbose` 标志
3. 查看测试数据：确保测试文件存在
4. 检查网络：确保两台计算机在同一网络

---

**最后更新**: 2025-11-12
**版本**: 1.0
**状态**: 活跃维护
