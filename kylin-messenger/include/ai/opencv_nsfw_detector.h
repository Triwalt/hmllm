/**
 * @file opencv_nsfw_detector.h
 * @brief 轻量级NSFW检测器（OpenCV+ONNX）
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_AI_OPENCV_NSFW_DETECTOR_H
#define KYLIN_MESSENGER_AI_OPENCV_NSFW_DETECTOR_H

#include <QImage>
#include <optional>
#include <memory>
#include <string>

// 前向声明，避免直接依赖OpenCV头文件
namespace cv {
    class Mat;
    namespace dnn {
        class Net;
    }
}

namespace KylinMessenger::AI {

/**
 * @brief NSFW检测结果
 */
struct NSFWResult {
    bool success = false;
    float nsfwProbability = 0.0f;
    float sfwProbability = 0.0f;
    std::string errorMessage;
    
    bool isNSFW() const { return nsfwProbability > 0.5f; }
};

/**
 * @brief 轻量级NSFW检测器
 * 使用MobileNetV2+ONNX模型，纯C++实现，无需Python依赖
 */
class LightweightNSFWDetector {
public:
    /**
     * @brief 构造函数
     * @param modelPath ONNX模型路径
     * @param threshold 检测阈值（默认0.75）
     */
    explicit LightweightNSFWDetector(const std::string& modelPath, float threshold = 0.75f);
    
    /**
     * @brief 析构函数
     */
    ~LightweightNSFWDetector();
    
    /**
     * @brief 检测图像是否包含NSFW内容
     * @param image QImage图像
     * @return 检测结果
     */
    NSFWResult classifyImage(const QImage& image);
    
    /**
     * @brief 检测图像数据
     * @param imageData 图像字节数据
     * @param format 图像格式（如"jpg", "png"）
     * @return 检测结果
     */
    NSFWResult classifyImageData(const QByteArray& imageData, const std::string& format);
    
    /**
     * @brief 检查检测器是否可用
     */
    bool isAvailable() const { return available_; }
    
    /**
     * @brief 获取模型路径
     */
    const std::string& getModelPath() const { return modelPath_; }
    
    /**
     * @brief 获取检测阈值
     */
    float getThreshold() const { return threshold_; }
    
    /**
     * @brief 设置检测阈值
     */
    void setThreshold(float threshold) { threshold_ = threshold; }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    std::string modelPath_;
    float threshold_;
    bool available_ = false;
    
    // 辅助方法
    cv::Mat convertQImageToMat(const QImage& image);
    cv::Mat preprocessImage(const cv::Mat& image);
    NSFWResult runInference(const cv::Mat& inputBlob);
};

/**
 * @brief 检测器配置
 */
struct NSFWDetectorConfig {
    std::string modelPath;
    float threshold = 0.75f;
    int inputWidth = 224;
    int inputHeight = 224;
    bool useGPU = false;  // 默认CPU推理
    
    static NSFWDetectorConfig fromEnvironment();
};

} // namespace KylinMessenger::AI

#endif // KYLIN_MESSENGER_AI_OPENCV_NSFW_DETECTOR_H