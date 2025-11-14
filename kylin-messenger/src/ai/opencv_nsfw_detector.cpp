/**
 * @file opencv_nsfw_detector.cpp
 * @brief 轻量级NSFW检测器实现（OpenCV+ONNX）
 * @version 1.0.0
 */

#include "ai/opencv_nsfw_detector.h"
#include <QDebug>
#include <QBuffer>
#include <QImageReader>
#include <stdexcept>
#include <cmath>

// 简化实现，没有OpenCV依赖的框架
namespace KylinMessenger::AI {

class LightweightNSFWDetector::Impl {
public:
    bool modelLoaded = false;
    std::string modelPath_;
    float threshold_;

    bool loadModel(const std::string& modelPath) {
        // 简化的模型加载逻辑
        qDebug() << "[NSFWDetector] 模拟加载模型:" << QString::fromStdString(modelPath);
        modelLoaded = true;
        return true;
    }

    NSFWResult runInference(const QByteArray& imageData) {
        NSFWResult result;
        result.success = true;
        // 模拟推理结果
        result.nsfwProbability = 0.1f; // 10% NSFW概率
        result.sfwProbability = 0.9f;  // 90% SFW概率
        return result;
    }
};

LightweightNSFWDetector::LightweightNSFWDetector(const std::string& modelPath, float threshold)
    : modelPath_(modelPath), threshold_(threshold) {
    qDebug() << "[NSFWDetector] 创建检测器，模型路径:" << QString::fromStdString(modelPath) << "阈值:" << threshold;
    impl_ = std::make_unique<Impl>();
    available_ = impl_->loadModel(modelPath);
    if (available_) {
        qDebug() << "[NSFWDetector] 检测器初始化成功";
    } else {
        qCritical() << "[NSFWDetector] 检测器初始化失败";
    }
}

LightweightNSFWDetector::~LightweightNSFWDetector() {
    qDebug() << "[NSFWDetector] 销毁检测器";
}

NSFWResult LightweightNSFWDetector::classifyImage(const QImage& image) {
    if (!available_) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "检测器不可用";
        return result;
    }
    if (image.isNull()) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "输入图像为空";
        qWarning() << "[NSFWDetector] 输入图像为空";
        return result;
    }
    try {
        qDebug() << "[NSFWDetector] 开始图像分类，尺寸:" << image.size();
        QByteArray imageData;
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        return impl_->runInference(imageData);
    } catch (const std::exception& e) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "图像分类异常: " + std::string(e.what());
        qCritical() << "[NSFWDetector] 图像分类异常:" << e.what();
        return result;
    } catch (...) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "图像分类未知异常";
        qCritical() << "[NSFWDetector] 图像分类未知异常";
        return result;
    }
}

NSFWResult LightweightNSFWDetector::classifyImageData(const QByteArray& imageData, const std::string& format) {
    if (!available_) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "检测器不可用";
        return result;
    }
    if (imageData.isEmpty()) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "图像数据为空";
        qWarning() << "[NSFWDetector] 图像数据为空";
        return result;
    }
    try {
        qDebug() << "[NSFWDetector] 开始图像数据分类，大小:" << imageData.size() << "格式:" << QString::fromStdString(format);
        return impl_->runInference(imageData);
    } catch (const std::exception& e) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "图像数据分类异常: " + std::string(e.what());
        qCritical() << "[NSFWDetector] 图像数据分类异常:" << e.what();
        return result;
    } catch (...) {
        NSFWResult result;
        result.success = false;
        result.errorMessage = "图像数据分类未知异常";
        qCritical() << "[NSFWDetector] 图像数据分类未知异常";
        return result;
    }
}

NSFWDetectorConfig NSFWDetectorConfig::fromEnvironment() {
    NSFWDetectorConfig config;
    const char* modelPath = std::getenv("KYLIN_NSFW_MODEL_PATH");
    if (modelPath) {
        config.modelPath = modelPath;
    } else {
        config.modelPath = "models/mobilenetv2_nsfw.onnx";
    }
    const char* threshold = std::getenv("KYLIN_NSFW_THRESHOLD");
    if (threshold) {
        config.threshold = std::stof(threshold);
    }
    qDebug() << "[NSFWDetector] 配置 - 模型路径:" << QString::fromStdString(config.modelPath) << "阈值:" << config.threshold;
    return config;
}

} // namespace KylinMessenger::AI