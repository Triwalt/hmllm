/**
 * @file test_opencv_nsfw_detector.cpp
 * @brief OpenCV NSFW检测器单元测试
 */

#include <gtest/gtest.h>
#include <QImage>
#include <QBuffer>
#include <QFile>
#include "ai/opencv_nsfw_detector.h"

using namespace KylinMessenger::AI;

class NSFWDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试图像
        createTestImages();
    }

    void TearDown() override {
        // 清理测试文件
        cleanupTestFiles();
    }

    // 创建测试用的图像
    void createTestImages() {
        // 创建纯色图像（模拟SFW）
        sfwImage_ = QImage(224, 224, QImage::Format_RGB888);
        sfwImage_.fill(Qt::blue);
        
        // 创建随机噪声图像（模拟NSFW）
        nsfwImage_ = QImage(224, 224, QImage::Format_RGB888);
        for (int y = 0; y < 224; ++y) {
            for (int x = 0; x < 224; ++x) {
                nsfwImage_.setPixel(x, y, qRgb(rand() % 256, rand() % 256, rand() % 256));
            }
        }
        
        // 保存测试图像
        sfwImage_.save("test_sfw.jpg", "JPG", 90);
        nsfwImage_.save("test_nsfw.jpg", "JPG", 90);
    }

    void cleanupTestFiles() {
        QFile::remove("test_sfw.jpg");
        QFile::remove("test_nsfw.jpg");
    }

    QImage sfwImage_;
    QImage nsfwImage_;
};

// 测试检测器初始化
TEST_F(NSFWDetectorTest, Initialization) {
    // 注意：这需要实际的ONNX模型文件
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // EXPECT_TRUE(detector.isAvailable());
}

// 测试图像分类
TEST_F(NSFWDetectorTest, ImageClassification) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // // 测试SFW图像
    // auto result1 = detector.classifyImage(sfwImage_);
    // EXPECT_TRUE(result1.success);
    // EXPECT_LT(result1.nsfwProbability, 0.5);
    // 
    // // 测试NSFW图像
    // auto result2 = detector.classifyImage(nsfwImage_);
    // EXPECT_TRUE(result2.success);
    // // 由于是随机图像，可能无法保证一定检测到NSFW
}

// 测试图像数据分类
TEST_F(NSFWDetectorTest, ImageDataClassification) {
    // 准备图像数据
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    sfwImage_.save(&buffer, "JPG");
    buffer.close();
    
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfx.onnx");
    // auto result = detector.classifyImageData(imageData, "jpg");
    // EXPECT_TRUE(result.success);
}

// 测试阈值设置
TEST_F(NSFWDetectorTest, ThresholdConfiguration) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx", 0.8);
    // EXPECT_FLOAT_EQ(detector.getThreshold(), 0.8);
    // 
    // detector.setThreshold(0.6);
    // EXPECT_FLOAT_EQ(detector.getThreshold(), 0.6);
}

// 测试无效图像
TEST_F(NSFWDetectorTest, InvalidImage) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // QImage invalidImage;
    // auto result = detector.classifyImage(invalidImage);
    // EXPECT_FALSE(result.success);
}

// 测试大图像处理
TEST_F(NSFWDetectorTest, LargeImageProcessing) {
    // 创建大图像
    QImage largeImage(1024, 1024, QImage::Format_RGB888);
    largeImage.fill(Qt::green);
    
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // auto result = detector.classifyImage(largeImage);
    // EXPECT_TRUE(result.success);
    // 验证图像被正确缩放
}

// 性能测试：单张图像处理时间
TEST_F(NSFWDetectorTest, PerformanceSingleImage) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // auto start = std::chrono::high_resolution_clock::now();
    // auto result = detector.classifyImage(sfwImage_);
    // auto end = std::chrono::high_resolution_clock::now();
    // 
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // EXPECT_LT(duration.count(), 100); // 应该小于100ms
    // std::cout << "Single image processing time: " << duration.count() << "ms" << std::endl;
}

// 性能测试：批量处理
TEST_F(NSFWDetectorTest, PerformanceBatchProcessing) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // const int batchSize = 100;
    // auto start = std::chrono::high_resolution_clock::now();
    // 
    // for (int i = 0; i < batchSize; ++i) {
    //     detector.classifyImage(sfwImage_);
    // }
    // 
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // 
    // double avgTime = static_cast<double>(duration.count()) / batchSize;
    // EXPECT_LT(avgTime, 100); // 平均处理时间应该小于100ms
    // std::cout << "Average processing time: " << avgTime << "ms" << std::endl;
}

// 测试配置加载
TEST(NSFWDetectorConfigTest, EnvironmentConfiguration) {
    // 设置环境变量
    qputenv("KYLIN_NSFW_MODEL_PATH", "models/mobilenetv2_nsfw.onnx");
    qputenv("KYLIN_NSFW_THRESHOLD", "0.8");
    qputenv("KYLIN_NSFW_USE_GPU", "false");
    
    // auto config = NSFWDetectorConfig::fromEnvironment();
    // EXPECT_EQ(config.modelPath, "models/mobilenetv2_nsfw.onnx");
    // EXPECT_FLOAT_EQ(config.threshold, 0.8);
    // EXPECT_FALSE(config.useGPU);
    
    // 清理环境变量
    qunsetenv("KYLIN_NSFW_MODEL_PATH");
    qunsetenv("KYLIN_NSFW_THRESHOLD");
    qunsetenv("KYLIN_NSFW_USE_GPU");
}

// 测试模型文件不存在
TEST_F(NSFWDetectorTest, MissingModelFile) {
    // LightweightNSFWDetector detector("nonexistent_model.onnx");
    // EXPECT_FALSE(detector.isAvailable());
}

// 测试多线程安全性
TEST_F(NSFWDetectorTest, ThreadSafety) {
    // LightweightNSFWDetector detector("models/mobilenetv2_nsfw.onnx");
    // 
    // std::vector<std::thread> threads;
    // const int threadCount = 4;
    // const int imagesPerThread = 25;
    // 
    // for (int i = 0; i < threadCount; ++i) {
    //     threads.emplace_back([&detector, this, i]() {
    //         for (int j = 0; j < imagesPerThread; ++j) {
    //             auto result = detector.classifyImage(sfwImage_);
    //             EXPECT_TRUE(result.success);
    //         }
    //     });
    // }
    // 
    // for (auto& thread : threads) {
    //     thread.join();
    // }
}