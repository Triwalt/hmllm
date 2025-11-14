# Kylin Messenger 测试策略

## 概述

本文档描述了Kylin Messenger轻量级架构重构后的全面测试策略，涵盖单元测试、集成测试、性能基准测试和手动测试流程。

## 新组件架构

### 1. MicroKernel (微内核)
- **职责**: 轻量级服务管理和事件分发
- **关键特性**: Pimpl模式、线程安全、事件驱动的服务架构
- **测试重点**: 服务生命周期、事件分发、线程安全

### 2. OpenCV NSFW Detector
- **职责**: 轻量级NSFW内容检测（无Python依赖）
- **关键特性**: OpenCV DNN模块、27MB ONNX模型、<50ms推理时间
- **测试重点**: 检测准确性、性能、边界情况

### 3. LightweightDiscovery
- **职责**: 智能网络设备发现和用户管理
- **关键特性**: 自适应心跳(5-60s)、IPMSG协议、网络感知
- **测试重点**: 设备发现、心跳机制、网络状态变化

### 4. ConcurrentFileTransfer
- **职责**: 多线程并发文件传输
- **关键特性**: 4工作线程、256KB块、断点续传
- **测试重点**: 传输速度、断点续传、错误恢复

## Phase 1: 单元测试 (Unit Testing)

### 目标
- 验证每个组件的内部逻辑正确性
- 实现高代码覆盖率(目标: >80%)
- 快速反馈开发中的问题

### MicroKernel 单元测试

```cpp
// tests/test_micro_kernel.cpp

TEST(MicroKernelTest, ServiceRegistration) {
    MicroKernel kernel;
    auto testService = std::make_unique<TestService>();

    EXPECT_TRUE(kernel.registerService("test", std::move(testService)));
    EXPECT_FALSE(kernel.registerService("test", std::make_unique<TestService>())); // Duplicate
}

TEST(MicroKernelTest, EventDistribution) {
    MicroKernel kernel;
    auto testService = std::make_unique<TestService>();
    TestService* servicePtr = testService.get();

    kernel.registerService("test", std::move(testService));

    Event testEvent("test.event", Event::Type::Custom);
    testEvent.data = "test data";

    kernel.publishEvent(testEvent);

    // 验证事件被正确处理
    EXPECT_TRUE(servicePtr->eventReceived);
}

TEST(MicroKernelTest, ThreadSafety) {
    MicroKernel kernel;
    const int numThreads = 10;
    const int operationsPerThread = 100;

    std::vector<std::thread> threads;

    // 多线程注册服务
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&kernel, i, operationsPerThread]() {
            for (int j = 0; j < operationsPerThread; ++j) {
                auto service = std::make_unique<TestService>();
                std::string name = "service_" + std::to_string(i) + "_" + std::to_string(j);
                kernel.registerService(name, std::move(service));
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证所有服务都被注册
    EXPECT_EQ(kernel.getServiceCount(), numThreads * operationsPerThread);
}
```

### OpenCV NSFW Detector 单元测试

```cpp
// tests/test_opencv_nsfw_detector.cpp

class NSFWDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = std::make_unique<OpenCVNSFWDetector>(
            "models/nsfw_model.onnx",
            0.8f  // 阈值
        );
    }

    std::unique_ptr<OpenCVNSFWDetector> detector;
};

TEST_F(NSFWDetectorTest, SafeImageDetection) {
    QImage safeImage("tests/data/safe_image.jpg");
    ASSERT_FALSE(safeImage.isNull());

    auto result = detector->predict(safeImage);

    EXPECT_FALSE(result.isUnsafe);
    EXPECT_LT(result.confidence, 0.8f);
}

TEST_F(NSFWDetectorTest, UnsafeImageDetection) {
    QImage unsafeImage("tests/data/unsafe_image.jpg");
    ASSERT_FALSE(unsafeImage.isNull());

    auto auto result = detector->predict(unsafeImage);

    EXPECT_TRUE(result.isUnsafe);
    EXPECT_GT(result.confidence, 0.8f);
}

TEST_F(NSFWDetectorTest, InvalidModelPath) {
    OpenCVNSFWDetector invalidDetector("nonexistent_model.onnx", 0.8f);

    QImage testImage(100, 100, QImage::Format_RGB888);
    testImage.fill(Qt::red);

    auto result = invalidDetector.predict(testImage);

    // 应该返回安全但带错误标志
    EXPECT_FALSE(result.isUnsafe);
    EXPECT_TRUE(result.hasError);
}

TEST_F(NSFWDetectorTest, PerformanceBenchmark) {
    QImage testImage("tests/data/test_image.jpg");

    const int iterations = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        detector->predict(testImage);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 平均推理时间应小于100ms
    EXPECT_LT(duration.count() / iterations, 100);
}
```

### LightweightDiscovery 单元测试

```cpp
// tests/test_lightweight_discovery.cpp

class LightweightDiscoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        discovery = std::make_unique<LightweightDiscovery>(1337);
    }

    std::unique_ptr<LightweightDiscovery> discovery;
};

TEST_F(LightweightDiscoveryTest, StartStop) {
    EXPECT_TRUE(discovery->start());
    EXPECT_TRUE(discovery->isRunning());

    discovery->stop();
    EXPECT_FALSE(discovery->isRunning());
}

TEST_F(LightweightDiscoveryTest, UserDetection) {
    discovery->start();

    // 模拟接收到用户广播
    UserInfo testUser;
    testUser.username = "test_user";
    testUser.hostname = "test_host";
    testUser.ipAddress = "192.168.1.100";

    discovery->onUserDiscovered(testUser);

    auto users = discovery->getOnlineUsers();
    EXPECT_EQ(users.size(), 1);
    EXPECT_EQ(users[0].username, "test_user");
}

TEST_F(LightweightDiscoveryTest, HeartbeatAdaptation) {
    discovery->start();

    // 初始心跳应为5秒
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 5000);

    // 模拟高网络活动
    for (int i = 0; i < 10; ++i) {
        UserInfo user;
        user.username = "user_" + std::to_string(i);
        discovery->onUserDiscovered(user);
    }

    // 心跳应增加至最大60秒
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 60000);
}

TEST_F(LightweightDiscoveryTest, NetworkStateChange) {
    discovery->start();

    // 模拟网络状态变化
    discovery->onNetworkStateChanged(NetworkState::WiFi);
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 30000);

    discovery->onNetworkStateChanged(NetworkState::Ethernet);
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 60000);

    discovery->onNetworkStateChanged(NetworkState::Mobile);
    EXPECT_EQ(discovery->getCurrentHeartbeatInterval(), 5000);
}
```

### ConcurrentFileTransfer 单元测试

```cpp
// tests/test_concurrent_file_transfer.cpp

class ConcurrentFileTransferTest : public ::testing::Test {
protected:
    void SetUp() override {
        transfer = std::make_unique<ConcurrentFileTransfer>();
    }

    std::unique_ptr<ConcurrentFileTransfer> transfer;
    QString testFilePath = "tests/data/large_file.bin";
};

TEST_F(ConcurrentFileTransferTest, SendFile) {
    bool completed = false;
    QString errorMessage;

    transfer->onProgress([&completed, &errorMessage](const TransferProgress& progress) {
        if (progress.status == TransferStatus::Completed) {
            completed = true;
        } else if (progress.status == TransferStatus::Failed) {
            errorMessage = progress.errorMessage;
        }
    });

    transfer->sendFile(testFilePath, "192.168.1.100", 1337);

    // 等待传输完成（异步）
    QCoreApplication::processEvents();

    if (!completed && errorMessage.isEmpty()) {
        // 等待最多5秒
        QElapsedTimer timer;
        timer.start();
        while (!completed && timer.elapsed() < 5000) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
    }

    EXPECT_TRUE(completed);
    EXPECT_TRUE(errorMessage.isEmpty());
}

TEST_F(ConcurrentFileTransferTest, ResumeTransfer) {
    // 第一次传输（中断）
    transfer->sendFile(testFilePath, "192.168.1.100", 1337);

    // 模拟中断
    transfer->pause();
    auto progress = transfer->getProgress();
    EXPECT_EQ(progress.status, TransferStatus::Paused);

    // 恢复传输
    transfer->resume();
    progress = transfer->getProgress();
    EXPECT_EQ(progress.status, TransferStatus::InProgress);
}

TEST_F(ConcurrentFileTransferTest, MultipleConcurrentTransfers) {
    const int numTransfers = 5;
    std::vector<std::unique_ptr<ConcurrentFileTransfer>> transfers;

    for (int i = 0; i < numTransfers; ++i) {
        transfers.push_back(std::make_unique<ConcurrentFileTransfer>());
    }

    // 同时启动多个传输
    for (int i = 0; i < numTransfers; ++i) {
        transfers[i]->sendFile(testFilePath, "192.168.1.10" + QString::number(i), 1337);
    }

    // 验证所有传输都在进行
    int activeTransfers = 0;
    for (const auto& t : transfers) {
        if (t->getProgress().status == TransferStatus::InProgress) {
            activeTransfers++;
        }
    }

    EXPECT_EQ(activeTransfers, numTransfers);
}

TEST_F(ConcurrentFileTransferTest, PerformanceBenchmark) {
    QElapsedTimer timer;
    timer.start();

    transfer->sendFile(testFilePath, "127.0.0.1", 1337);

    // 等待传输完成
    while (transfer->getProgress().status == TransferStatus::InProgress) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }

    auto elapsed = timer.elapsed();
    auto fileSize = QFileInfo(testFilePath).size();

    // 计算传输速度（MB/s）
    double speed = (fileSize / 1024.0 / 1024.0) / (elapsed / 1000.0);

    // 期望速度至少10MB/s（本地回环）
    EXPECT_GT(speed, 10.0);
}
```

## Phase 2: 集成测试 (Integration Testing)

### MicroKernel 与 NSFW Detector 集成

```cpp
// tests/integration/test_kernel_nsfw_integration.cpp

TEST(KernelNSFWIntegrationTest, NSFWDetectionAsService) {
    MicroKernel kernel;

    // 注册NSFW检测器为服务
    auto nsfwDetector = std::make_unique<OpenCVNSFWDetector>(
        "models/nsfw_model.onnx",
        0.8f
    );

    kernel.registerService("nsfw_detector", std::move(nsfwDetector));

    // 通过事件触发检测
    Event detectionRequest("nsfw.detect", Event::Type::Custom);
    detectionRequest.data = "tests/data/test_image.jpg";

    // 订阅检测结果
    DetectionResult result;
    kernel.subscribe("nsfw.result", [&result](const Event& e) {
        result = e.data.value<DetectionResult>();
    });

    kernel.publishEvent(detectionRequest);

    // 等待处理
    QCoreApplication::processEvents();

    // 验证结果
    EXPECT_FALSE(result.hasError);
    EXPECT_GT(result.confidence, 0.0f);
    EXPECT_LT(result.confidence, 1.0f);
}
```

### LightweightDiscovery 与 MicroKernel 集成

```cpp
// tests/integration/test_discovery_kernel_integration.cpp

class MockChatService : public LightweightService {
public:
    void handleEvent(const Event& event) override {
        if (event.name == "user.online") {
            onlineUsers.append(event.data.value<UserInfo>());
        } else if (event.name == "user.offline") {
            offlineUsers.append(event.data.value<UserInfo>());
        }
    }

    QVector<UserInfo> onlineUsers;
    QVector<UserInfo> offlineUsers;
};

TEST(DiscoveryKernelIntegrationTest, UserStatusBroadcast) {
    MicroKernel kernel;
    LightweightDiscovery discovery(1337);

    // 注册Mock聊天服务
    auto mockService = std::make_unique<MockChatService>();
    MockChatService* servicePtr = mockService.get();

    kernel.registerService("chat", std::move(mockService));

    // 连接发现器和内核
    discovery.onUserDiscovered([&kernel](const UserInfo& user) {
        Event onlineEvent("user.online", Event::Type::Broadcast);
        onlineEvent.data = QVariant::fromValue(user);
        kernel.publishEvent(onlineEvent);
    });

    discovery.onUserOffline([&kernel](const UserInfo& user) {
        Event offlineEvent("user.offline", Event::Type::Broadcast);
        offlineEvent.data = QVariant::fromValue(user);
        kernel.publishEvent(offlineEvent);
    });

    // 模拟用户上线
    UserInfo testUser;
    testUser.username = "test_user";
    discovery.onUserDiscovered(testUser);

    QCoreApplication::processEvents();

    EXPECT_EQ(servicePtr->onlineUsers.size(), 1);
    EXPECT_EQ(servicePtr->onlineUsers[0].username, "test_user");
}
```

## Phase 3: 性能基准测试 (Performance Benchmarking)

### 使用 Google Benchmark 框架

```cpp
// tests/performance_benchmark.cpp

#include <benchmark/benchmark.h>
#include "core/micro_kernel.h"
#include "ai/opencv_nsfw_detector.h"
#include "network/lightweight_discovery.h"
#include "transfer/concurrent_file_transfer.h"

// MicroKernel 性能测试
static void BM_EventPublishing(benchmark::State& state) {
    MicroKernel kernel;
    auto testService = std::make_unique<TestService>();
    kernel.registerService("test", std::move(testService));

    Event testEvent("test.event", Event::Type::Custom);
    testEvent.data = "benchmark data";

    for (auto _ : state) {
        kernel.publishEvent(testEvent);
    }
}
BENCHMARK(BM_EventPublishing)
    ->Range(8, 8<<10)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->UseRealTime();

// NSFW 检测性能测试
static void BM_NSFWDetection(benchmark::State& state) {
    OpenCVNSFWDetector detector("models/nsfw_model.onnx", 0.8f);
    QImage testImage("tests/data/benchmark_image.jpg");

    for (auto _ : state) {
        auto result = detector.predict(testImage);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_NSFWDetection)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100);

// LightweightDiscovery 心跳自适应测试
static void BM_HeartbeatAdaptation(benchmark::State& state) {
    LightweightDiscovery discovery(1337);

    for (auto _ : state) {
        // 模拟网络变化
        discovery.onNetworkStateChanged(NetworkState::WiFi);
        benchmark::DoNotOptimize(discovery.getCurrentHeartbeatInterval());
    }
}
BENCHMARK(BM_HeartbeatAdaptation);

// 文件传输性能测试
static void BM_FileTransfer(benchmark::State& state) {
    ConcurrentFileTransfer transfer;

    for (auto _ : state) {
        transfer.sendFile("tests/data/large_file.bin", "127.0.0.1", 1337);

        // 等待传输完成
        while (transfer.getProgress().status == TransferStatus::InProgress) {
            QCoreApplication::processEvents();
        }
    }
}
BENCHMARK(BM_FileTransfer)
    ->Unit(benchmark::kSecond)
    ->Iterations(10);

BENCHMARK_MAIN();
```

### 性能测试执行

```bash
# 编译性能测试
cd build
make performance_benchmark

# 运行基准测试
./tests/performance_benchmark --benchmark_out=benchmark_results.json

# 生成报告
python3 scripts/generate_benchmark_report.py benchmark_results.json
```

### 性能目标

| 组件 | 指标 | 目标值 | 当前值 |
|------|------|--------|--------|
| MicroKernel | 事件分发延迟 | < 1ms | - |
| NSFW Detector | 推理时间 | < 50ms | - |
| NSFW Detector | CPU使用率 | < 20% | - |
| LightweightDiscovery | 心跳自适应响应 | < 1s | - |
| FileTransfer | 传输速度 | > 10MB/s | - |
| FileTransfer | 并发传输数 | 5+ | - |

## Phase 4: 集成和系统测试 (Integration & System Testing)

### 系统级功能测试清单

#### 1. 消息传递流程
- [ ] 用户上线后能被其他用户发现
- [ ] 文本消息能正确发送和接收
- [ ] 消息历史记录正确保存
- [ ] 离线消息在重新上线后送达

#### 2. 文件传输流程
- [ ] 小文件(< 1MB)传输成功
- [ ] 大文件(> 100MB)传输成功
- [ ] 传输中断后能继续传输
- [ ] 多文件并发传输
- [ ] 文件完整性校验通过

#### 3. NSFW检测流程
- [ ] 安全图片通过检测
- [ ] 不安全图片被正确标记
- [ ] 检测不影响消息发送速度
- [ ] GPU加速检测可用（如有GPU）

#### 4. 网络适应性测试
- [ ] WiFi网络下心跳正常
- [ ] 有线网络下心跳正常
- [ ] 网络切换时自动调整
- [ ] 网络断开/重连处理

#### 5. 性能和压力测试
- [ ] 100+用户同时在线
- [ ] 高频率消息发送(1000+ msg/min)
- [ ] 多文件同时传输(5+ files)
- [ ] 长时间运行(24+ hours)稳定性

## Phase 5: 手动测试流程

### 测试环境准备

```bash
# 1. 编译项目
cd kylin-messenger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 2. 准备测试数据
mkdir -p ../tests/data
# 放置测试图片、文件等

# 3. 下载NSFW模型
wget -O models/nsfw_model.onnx https://github.com/path/to/model.onnx

# 4. 运行应用
./kylin-messenger
```

### 手动测试步骤

#### 测试场景 1: 基本消息功能

1. 在两台计算机上启动应用
2. 验证两台计算机能互相发现
3. 从计算机A发送消息到计算机B
4. 验证消息在计算机B上正确显示
5. 从计算机B回复消息
6. 验证回复在计算机A上正确显示

**预期结果**: 消息正确传递，无丢失、无乱序

#### 测试场景 2: 文件传输

1. 准备测试文件（1MB、10MB、100MB）
2. 从计算机A向计算机B发送小文件
3. 验证文件完整性和内容正确性
4. 发送大文件，中途断开网络
5. 恢复网络，验证传输能从断点继续
6. 同时发送3个文件

**预期结果**: 文件完整传输，断点续传工作正常，并发传输无冲突

#### 测试场景 3: NSFW 检测

1. 准备安全和不安全的测试图片
2. 发送安全图片，验证未被阻止
3. 发送不安全图片，验证被正确标记
4. 发送大量图片，验证性能影响
5. 测试不同格式的图片（JPG、PNG、GIF）

**预期结果**: 检测准确率>90%，平均处理时间<50ms

#### 测试场景 4: 网络环境变化

1. 在WiFi环境下启动应用
2. 切换到有线网络
3. 验证应用自动适应新网络
4. 断开网络连接
5. 验证用户显示为离线
6. 重连网络，验证用户重新上线

**预期结果**: 网络切换平滑，用户状态正确更新

#### 测试场景 5: 性能和稳定性

1. 启动应用，连续运行24小时
2. 期间进行正常操作（发送消息、传输文件）
3. 监控系统资源使用（CPU、内存）
4. 验证无内存泄漏
5. 验证响应速度没有明显下降

**预期结果**: 内存使用稳定，CPU使用率<30%，响应时间无明显增长

## Phase 6: 测试自动化和CI/CD集成

### GitHub Actions 工作流

```yaml
# .github/workflows/test.yml
name: Build and Test

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        cxx: [g++, clang++]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3

    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y \
          build-essential \
          cmake \
          qt6-base-dev \
          libopencv-dev \
          libgtest-dev \
          libbenchmark-dev

    - name: Configure CMake
      run: |
        mkdir build && cd build
        cmake .. \
          -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
          -DCMAKE_CXX_COMPILER=${{ matrix.cxx }} \
          -DBUILD_TESTS=ON

    - name: Build
      run: cd build && make -j$(nproc)

    - name: Run Unit Tests
      run: cd build && ctest --output-on-failure

    - name: Run Performance Tests
      run: |
        cd build
        ./tests/performance_benchmark \
          --benchmark_out=benchmark_results.json

    - name: Upload Benchmark Results
      uses: actions/upload-artifact@v3
      with:
        name: benchmark-results-${{ matrix.cxx }}-${{ matrix.build_type }}
        path: build/benchmark_results.json

    - name: Generate Coverage Report
      if: matrix.build_type == 'Debug'
      run: |
        cd build
        make coverage
        bash <(curl -s https://codecov.io/bash)
```

## 测试数据准备

### 测试文件
在 `tests/data/` 目录准备以下文件:

```bash
tests/data/
├── safe_image.jpg          # 安全图片（风景、物品等）
├── unsafe_image.jpg        # 不安全图片
├── large_file.bin          # 100MB测试文件
├── medium_file.bin         # 10MB测试文件
└── small_file.bin          # 1MB测试文件
```

### 模型文件
```bash
mkdir -p models/
wget -O models/nsfw_model.onnx \
  https://github.com/notAI-tech/NudeNet/releases/download/v0/deploy.onnx
```

## 测试报告模板

### 单元测试报告

**测试覆盖率:**
- 行覆盖率: XX%
- 分支覆盖率: XX%
- 函数覆盖率: XX%

**测试结果:**
- MicroKernel: X/X 测试通过
- NSFW Detector: X/X 测试通过
- LightweightDiscovery: X/X 测试通过
- ConcurrentFileTransfer: X/X 测试通过

### 性能测试报告

**MicroKernel:**
- 事件分发延迟: X ms (avg), Y ms (p99)

**NSFW Detector:**
- 推理时间: X ms (avg), Y ms (max)
- CPU使用率: X%

**LightweightDiscovery:**
- 心跳适应时间: X s
- 发现延迟: X ms

**ConcurrentFileTransfer:**
- 传输速度: X MB/s
- 并发传输数: X

### 集成测试报告

**消息传递:** 通过/失败
**文件传输:** 通过/失败
**NSFW检测:** 通过/失败
**网络适应性:** 通过/失败

## 测试执行计划

### 第一阶段: 单元测试
**时间**: 2天
**负责人**: 开发工程师
**目标**: 所有单元测试通过，覆盖率>80%

### 第二阶段: 集成测试
**时间**: 2天
**负责人**: 开发工程师
**目标**: 所有集成测试通过

### 第三阶段: 性能测试
**时间**: 1天
**负责人**: 性能工程师
**目标**: 达到性能目标

### 第四阶段: 手动测试
**时间**: 3天
**负责人**: QA工程师
**目标**: 所有手动测试场景通过

### 第五阶段: 回归测试
**时间**: 持续
**负责人**: CI/CD系统
**目标**: 每次提交自动运行测试

## 风险评估

| 风险 | 可能性 | 影响 | 缓解措施 |
|------|--------|------|----------|
| NSFW模型文件过大 | 中 | 中 | 使用模型压缩和量化 |
| OpenCV依赖安装困难 | 中 | 中 | 提供详细安装文档和脚本 |
| 网络环境复杂 | 高 | 中 | 增加网络模拟测试 |
| 并发文件传输冲突 | 中 | 高 | 完善锁机制和错误处理 |
| 内存泄漏 | 低 | 高 | 使用AddressSanitizer和Valgrind测试 |

## 测试环境要求

### 硬件要求
- CPU: 4核以上
- 内存: 8GB以上
- 存储: 50GB可用空间
- 网络: 支持有线和WiFi

### 软件要求
- OS: Ubuntu 20.04+ / Windows 10+
- Qt: 5.15+ 或 6.0+
- OpenCV: 4.5+
- CMake: 3.16+
- Compiler: GCC 9+, Clang 11+, or MSVC 2019+

## 附录

### 常用测试命令

```bash
# 运行所有测试
cd build && ctest

# 运行指定测试
./tests/test_micro_kernel
./tests/test_opencv_nsfw_detector

# 运行性能测试
./tests/performance_benchmark

# 生成覆盖率报告
make coverage
firefox coverage/index.html

# 内存泄漏检测
valgrind --leak-check=full ./tests/test_micro_kernel

# AddressSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined"
make
ASAN_OPTIONS=detect_leaks=1 ./tests/test_micro_kernel
```

### 调试技巧

```bash
# GDB调试
gdb ./kylin-messenger
(gdb) run
(gdb) bt  # 查看堆栈

# Qt日志
export QT_LOGGING_RULES="*.debug=true"
./kylin-messenger

# 网络调试
tcpdump -i any port 1337 -w network_capture.pcap
```

---

**文档版本**: 1.0
**最后更新**: 2025-11-12
**维护人**: Kylin Messenger Team
