/**
 * @file performance_benchmark.cpp
 * @brief 性能基准测试
 */

#include <benchmark/benchmark.h>
#include <QCoreApplication>
#include <QThread>
#include <chrono>
#include "core/micro_kernel.h"
#include "ai/opencv_nsfw_detector.h"
#include "network/lightweight_discovery.h"
#include "transfer/concurrent_file_transfer.h"

using namespace KylinMessenger;

// 微内核性能基准
static void BM_MicroKernelEventPublish(benchmark::State& state) {
    Core::MicroKernel kernel;
    
    class BenchmarkService : public Core::LightweightService {
    public:
        bool initialize() override { return true; }
        void processEvent(const Core::Event& event) override { eventCount_++; }
        void shutdown() override {}
        std::string getName() const override { return "BenchmarkService"; }
        bool isAvailable() const override { return true; }
        
        int eventCount() const { return eventCount_; }
        
    private:
        std::atomic<int> eventCount_{0};
    };
    
    auto service = std::make_unique<BenchmarkService>();
    auto* servicePtr = service.get();
    
    kernel.loadService(std::move(service), "BenchmarkService");
    kernel.start();
    
    Core::Event event(Core::Event::UserInteraction);
    event.setSource("Benchmark");
    
    for (auto _ : state) {
        kernel.publishEvent(event);
    }
    
    // 等待事件处理完成
    QThread::msleep(100);
    
    state.counters["EventsProcessed"] = servicePtr->eventCount();
    
    kernel.shutdown();
}
BENCHMARK(BM_MicroKernelEventPublish)->Range(100, 10000);

// NSFW检测器性能基准
static void BM_NSFWDetection(benchmark::State& state) {
    // 注意：需要实际的模型文件
    // AI::LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // QImage testImage(224, 224, QImage::Format_RGB888);
    // testImage.fill(Qt::blue);
    // 
    // for (auto _ : state) {
    //     auto result = detector.classifyImage(testImage);
    //     benchmark::DoNotOptimize(result);
    // }
}
// BENCHMARK(BM_NSFWDetection)->Unit(benchmark::kMillisecond);

// 文件传输性能基准
static void BM_FileTransferSmall(benchmark::State& state) {
    // Transfer::ConcurrentFileTransfer transfer;
    // transfer.initialize();
    // 
    // // 创建测试文件
    // QFile testFile("test_small.dat");
    // testFile.open(QIODevice::WriteOnly);
    // testFile.write(QByteArray(1024 * 1024, 'A')); // 1MB
    // testFile.close();
    // 
    // for (auto _ : state) {
    //     QString taskId = transfer.sendFile(
    //         "test_small.dat",
    //         QHostAddress::LocalHost,
    //         2425,
    //         "test_peer"
    //     );
    //     benchmark::DoNotOptimize(taskId);
    // }
    // 
    // QFile::remove("test_small.dat");
}
// BENCHMARK(BM_FileTransferSmall)->Unit(benchmark::kMillisecond);

// 网络发现性能基准
static void BM_NetworkDiscovery(benchmark::State& state) {
    // QCoreApplication app(argc, argv); // 需要QApplication
    // 
    // Network::LightweightDiscovery discovery;
    // discovery.initialize();
    // 
    // UserInfo localUser;
    // localUser.user_id = "benchmark_user";
    // localUser.username = "Benchmark";
    // localUser.status = UserStatus::Online;
    // discovery.updateLocalUser(localUser);
    // 
    // for (auto _ : state) {
    //     discovery.broadcastPresence();
    //     benchmark::ClobberMemory();
    // }
}
// BENCHMARK(BM_NetworkDiscovery)->Unit(benchmark::kMillisecond);

// 内存使用基准
static void BM_MemoryUsage(benchmark::State& state) {
    for (auto _ : state) {
        Core::MicroKernel kernel;
        
        // 加载多个服务
        for (int i = 0; i < 10; ++i) {
            class DummyService : public Core::LightweightService {
            public:
                bool initialize() override { return true; }
                void processEvent(const Core::Event& event) override {}
                void shutdown() override {}
                std::string getName() const override { return "DummyService"; }
                bool isAvailable() const override { return true; }
            };
            
            auto service = std::make_unique<DummyService>();
            kernel.loadService(std::move(service), "Dummy" + std::to_string(i));
        }
        
        kernel.start();
        
        // 发布一些事件
        for (int i = 0; i < 100; ++i) {
            Core::Event event(Core::Event::UserInteraction);
            kernel.publishEvent(event);
        }
        
        QThread::msleep(10);
        kernel.shutdown();
    }
}
BENCHMARK(BM_MemoryUsage)->Unit(benchmark::kMillisecond);

// 并发性能基准
static void BM_ConcurrentTransfers(benchmark::State& state) {
    // Transfer::ConcurrentFileTransfer transfer;
    // transfer.initialize();
    // 
    // // 创建多个测试文件
    // std::vector<QString> testFiles;
    // for (int i = 0; i < 5; ++i) {
    //     QString filename = QString("test_concurrent_%1.dat").arg(i);
    //     QFile file(filename);
    //     file.open(QIODevice::WriteOnly);
    //     file.write(QByteArray(5 * 1024 * 1024, 'B')); // 5MB each
    //     file.close();
    //     testFiles.push_back(filename);
    // }
    // 
    // for (auto _ : state) {
    //     std::vector<QString> taskIds;
    //     for (const auto& file : testFiles) {
    //         QString taskId = transfer.sendFile(
    //             file,
    //             QHostAddress::LocalHost,
    //             2425,
    //             "test_peer"
    //         );
    //         taskIds.push_back(taskId);
    //     }
    //     benchmark::DoNotOptimize(taskIds);
    // }
    // 
    // // 清理
    // for (const auto& file : testFiles) {
    //     QFile::remove(file);
    // }
}
// BENCHMARK(BM_ConcurrentTransfers)->Unit(benchmark::kMillisecond);

// 主函数
BENCHMARK_MAIN();