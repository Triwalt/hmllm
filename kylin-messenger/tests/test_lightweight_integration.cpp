/**
 * @file test_lightweight_integration.cpp
 * @brief 轻量级组件集成测试
 * @version 1.0.0
 */

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <chrono>
#include <iostream>

#include "core/micro_kernel.h"
#include "ai/opencv_nsfw_detector.h"
#include "network/lightweight_discovery.h"
#include "transfer/concurrent_file_transfer.h"

using namespace KylinMessenger;

class TestService : public Core::LightweightService {
public:
    bool initialize() override {
        qDebug() << "[TestService] 初始化";
        return true;
    }

    void processEvent(const Core::Event& event) override {
        qDebug() << "[TestService] 处理事件类型:" << event.type();
        eventCount_++;
    }

    void shutdown() override {
        qDebug() << "[TestService] 关闭，事件计数:" << eventCount_;
    }

    std::string getName() const override { return "TestService"; }
    bool isAvailable() const override { return true; }

private:
    int eventCount_ = 0;
};

class LightweightIntegrationTest {
public:
    static bool testMicroKernel() {
        qDebug() << "=== 测试微内核 ===";

        Core::MicroKernel kernel;

        // 加载测试服务
        auto testService = std::make_unique<TestService>();
        kernel.loadService(std::move(testService), "TestService");

        // 启动内核
        if (!kernel.start()) {
            qCritical() << "微内核启动失败";
            return false;
        }

        // 发布测试事件
        Core::Event testEvent(Core::Event::UserInteraction);
        testEvent.setSource("IntegrationTest");
        kernel.publishEvent(testEvent);

        // 等待事件处理
        QCoreApplication::processEvents();
        QThread::msleep(100);

        // 关闭内核
        kernel.shutdown();

        qDebug() << "微内核测试完成";
        return true;
    }

    static bool testNSFWDetector() {
        qDebug() << "=== 测试NSFW检测器 ===";

        // 检查模型文件是否存在
        QString modelPath = "models/mobilenetv2_nsfw.onnx";
        if (!QFile::exists(modelPath)) {
            qWarning() << "NSFW模型文件不存在，跳过检测器测试";
            return true; // 不是失败，只是跳过
        }

        try {
            AI::LightweightNSFWDetector detector(modelPath.toStdString(), 0.75f);

            if (!detector.isAvailable()) {
                qWarning() << "NSFW检测器不可用";
                return false;
            }

            qDebug() << "NSFW检测器创建成功";
            qDebug() << "模型路径:" << QString::fromStdString(detector.getModelPath());
            qDebug() << "检测阈值:" << detector.getThreshold();

            // 创建测试图像
            QImage testImage(224, 224, QImage::Format_RGB888);
            testImage.fill(Qt::blue);

            // 进行分类测试
            auto result = detector.classifyImage(testImage);

            if (result.success) {
                qDebug() << "NSFW分类成功";
                qDebug() << "NSFW概率:" << result.nsfwProbability;
                qDebug() << "SFW概率:" << result.sfwProbability;
                qDebug() << "是否为NSFW:" << result.isNSFW();
            } else {
                qWarning() << "NSFW分类失败:" << QString::fromStdString(result.errorMessage);
                return false;
            }

        } catch (const std::exception& e) {
            qCritical() << "NSFW检测器异常:" << e.what();
            return false;
        } catch (...) {
            qCritical() << "NSFW检测器未知异常";
            return false;
        }

        qDebug() << "NSFW检测器测试完成";
        return true;
    }

    static bool testNetworkDiscovery() {
        qDebug() << "=== 测试网络发现服务 ===";

        // 创建模拟的QCoreApplication
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char argv[] = "test";
            static QCoreApplication app(argc, &argv);
        }

        Network::LightweightDiscovery discovery;

        if (!discovery.initialize()) {
            qCritical() << "网络发现服务初始化失败";
            return false;
        }

        // 创建测试用户
        Core::UserInfo testUser;
        testUser.user_id = "test_user_123";
        testUser.username = "TestUser";
        testUser.hostname = "TestHost";
        testUser.status = Core::UserStatus::Online;
        testUser.status_text = "测试中";

        discovery.updateLocalUser(testUser);

        // 等待一段时间让服务运行
        QCoreApplication::processEvents();
        QThread::msleep(500);

        // 获取在线用户
        auto onlineUsers = discovery.getOnlineUsers();
        qDebug() << "在线用户数量:" << onlineUsers.size();

        // 关闭服务
        discovery.shutdown();

        qDebug() << "网络发现服务测试完成";
        return true;
    }

    static bool testFileTransfer() {
        qDebug() << "=== 测试文件传输服务 ===";

        Transfer::ConcurrentFileTransfer transfer;

        if (!transfer.initialize()) {
            qCritical() << "文件传输服务初始化失败";
            return false;
        }

        // 创建测试文件
        QString testFile = "test_transfer.dat";
        QFile file(testFile);
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray testData(1024 * 1024, 'T'); // 1MB测试数据
            file.write(testData);
            file.close();
        }

        // 测试传输任务创建
        QString taskId = transfer.sendFile(
            testFile,
            QHostAddress::LocalHost,
            2425,
            "test_peer"
        );

        if (taskId.isEmpty()) {
            qWarning() << "传输任务创建失败";
            QFile::remove(testFile);
            return false;
        }

        qDebug() << "传输任务创建成功，任务ID:" << taskId;

        // 获取任务信息
        auto task = transfer.getTask(taskId);
        if (task.has_value()) {
            qDebug() << "任务文件名:" << task->fileName;
            qDebug() << "任务总大小:" << task->totalSize;
            qDebug() << "任务状态:" << static_cast<int>(task->status);
        }

        // 清理
        transfer.shutdown();
        QFile::remove(testFile);

        qDebug() << "文件传输服务测试完成";
        return true;
    }

    static bool runAllTests() {
        qDebug() << "\n========== 轻量级组件集成测试开始 ==========";

        auto start = std::chrono::steady_clock::now();

        bool allPassed = true;

        allPassed &= testMicroKernel();
        allPassed &= testNSFWDetector();
        allPassed &= testNetworkDiscovery();
        allPassed &= testFileTransfer();

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        qDebug() << "\n========== 测试结果 ==========";
        qDebug() << "测试总用时:" << duration << "ms";
        qDebug() << "所有测试:" << (allPassed ? "通过" : "失败");
        qDebug() << "==============================\n";

        return allPassed;
    }
};

// 主函数
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // 设置日志级别
    QLoggingCategory::setFilterRules("default.debug=true\nkylin.debug=true");

    std::cout << "Kylin Messenger 轻量级组件集成测试" << std::endl;
    std::cout << "====================================" << std::endl;

    bool result = LightweightIntegrationTest::runAllTests();

    return result ? 0 : 1;
}

#ifdef _MSC_VER
#pragma comment(lib, "opencv_core.lib")
#pragma comment(lib, "opencv_imgproc.lib")
#pragma comment(lib, "opencv_dnn.lib")
#endif

// 测试宏定义
#define LIGHTWEIGHT_TEST_ENABLED
#define LIGHTWEIGHT_INTEGRATION_TEST

// 性能测试辅助函数
namespace TestUtils {
    template<typename Func>
    double measureExecutionTime(Func func, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            func();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        return static_cast<double>(duration) / iterations;
    }

    QString formatBytes(qint64 bytes) {
        const qint64 KB = 1024;
        const qint64 MB = KB * 1024;
        const qint64 GB = MB * 1024;

        if (bytes >= GB) {
            return QString("%1 GB").arg(bytes / double(GB), 0, 'f', 2);
        } else if (bytes >= MB) {
            return QString("%1 MB").arg(bytes / double(MB), 0, 'f', 2);
        } else if (bytes >= KB) {
            return QString("%1 KB").arg(bytes / double(KB), 0, 'f', 2);
        } else {
            return QString("%1 B").arg(bytes);
        }
    }

    QString formatDuration(double microseconds) {
        if (microseconds < 1000) {
            return QString("%1 μs").arg(microseconds, 0, 'f', 2);
        } else if (microseconds < 1000000) {
            return QString("%1 ms").arg(microseconds / 1000.0, 0, 'f', 2);
        } else {
            return QString("%1 s").arg(microseconds / 1000000.0, 0, 'f', 2);
        }
    }
}

// 内存使用监控
class MemoryMonitor {
public:
    MemoryMonitor(const QString& name) : name_(name) {
#ifdef Q_OS_LINUX
        initialMemory_ = getCurrentMemoryUsage();
#endif
    }

    ~MemoryMonitor() {
#ifdef Q_OS_LINUX
        qint64 finalMemory = getCurrentMemoryUsage();
        qint64 delta = finalMemory - initialMemory_;
        qDebug() << "[" << name_ << "] 内存使用变化:" << TestUtils::formatBytes(delta);
#endif
    }

private:
    QString name_;
    qint64 initialMemory_ = 0;

#ifdef Q_OS_LINUX
    qint64 getCurrentMemoryUsage() {
        QFile file("/proc/self/status");
        if (!file.open(QIODevice::ReadOnly)) {
            return 0;
        }

        QTextStream stream(&file);
        QString line;
        while (!stream.atEnd()) {
            line = stream.readLine();
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegExp("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toLongLong() * 1024; // KB to bytes
                }
            }
        }
        return 0;
    }
#endif
};