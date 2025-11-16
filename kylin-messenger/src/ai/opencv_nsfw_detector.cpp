/**
 * @file opencv_nsfw_detector.cpp
 * @brief 轻量级NSFW检测器实现（TensorFlow Lite）
 * @version 2.0.0
 */

#include "ai/opencv_nsfw_detector.h"

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLibrary>
#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QStandardPaths>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace KylinMessenger::AI {

namespace {

constexpr int kExpectedChannels = 3;

#if defined(Q_OS_WIN)
constexpr const char* kDefaultTfliteLib = "tensorflowlite_c.dll";
constexpr const char* kDefaultGpuDelegateLib = "tensorflowlite_gpu_delegate.dll";
#elif defined(Q_OS_MACOS)
constexpr const char* kDefaultTfliteLib = "libtensorflowlite_c.dylib";
constexpr const char* kDefaultGpuDelegateLib = "libtensorflowlite_gpu_delegate.dylib";
#else
constexpr const char* kDefaultTfliteLib = "libtensorflowlite_c.so";
constexpr const char* kDefaultGpuDelegateLib = "libtensorflowlite_gpu_delegate.so";
#endif

enum TfLiteStatus {
    kTfLiteOk = 0,
    kTfLiteError = 1,
    kTfLiteDelegateError = 2,
    kTfLiteApplicationError = 3
};

enum TfLiteType {
    kTfLiteNoType = 0,
    kTfLiteFloat32 = 1,
    kTfLiteInt32 = 2,
    kTfLiteUInt8 = 3,
    kTfLiteInt64 = 4,
    kTfLiteString = 5,
    kTfLiteBool = 6,
    kTfLiteInt16 = 7,
    kTfLiteComplex64 = 8,
    kTfLiteInt8 = 9,
    kTfLiteFloat16 = 10,
    kTfLiteFloat64 = 11,
    kTfLiteComplex128 = 12,
    kTfLiteUInt64 = 13,
    kTfLiteResource = 14,
    kTfLiteVariant = 15,
    kTfLiteUInt32 = 16,
    kTfLiteUInt16 = 17,
    kTfLiteInt4 = 18,
};

struct TfLiteModel;
struct TfLiteInterpreter;
struct TfLiteInterpreterOptions;
struct TfLiteTensor;
struct TfLiteDelegate;

using TfLiteModelCreateFromFileFn = TfLiteModel* (*)(const char* filename);
using TfLiteModelDeleteFn = void (*)(TfLiteModel* model);
using TfLiteInterpreterOptionsCreateFn = TfLiteInterpreterOptions* (*)();
using TfLiteInterpreterOptionsDeleteFn = void (*)(TfLiteInterpreterOptions* options);
using TfLiteInterpreterOptionsSetNumThreadsFn = void (*)(TfLiteInterpreterOptions* options, int32_t num_threads);
using TfLiteInterpreterCreateFn = TfLiteInterpreter* (*)(const TfLiteModel* model,
                                                        const TfLiteInterpreterOptions* optional_options);
using TfLiteInterpreterDeleteFn = void (*)(TfLiteInterpreter* interpreter);
using TfLiteInterpreterAllocateTensorsFn = TfLiteStatus (*)(TfLiteInterpreter* interpreter);
using TfLiteInterpreterInvokeFn = TfLiteStatus (*)(TfLiteInterpreter* interpreter);
using TfLiteInterpreterGetInputTensorFn = TfLiteTensor* (*)(TfLiteInterpreter* interpreter, int32_t input_index);
using TfLiteInterpreterGetOutputTensorFn = const TfLiteTensor* (*)(const TfLiteInterpreter* interpreter,
                                                                  int32_t output_index);
using TfLiteTensorTypeFn = TfLiteType (*)(const TfLiteTensor* tensor);
using TfLiteTensorNumDimsFn = int32_t (*)(const TfLiteTensor* tensor);
using TfLiteTensorDimFn = int32_t (*)(const TfLiteTensor* tensor, int32_t dim_index);
using TfLiteTensorByteSizeFn = size_t (*)(const TfLiteTensor* tensor);
using TfLiteTensorCopyFromBufferFn = TfLiteStatus (*)(TfLiteTensor* tensor, const void* input_data, size_t input_data_size);
using TfLiteTensorCopyToBufferFn = TfLiteStatus (*)(const TfLiteTensor* tensor, void* output_data, size_t output_data_size);

class TfLiteRuntime {
public:
    static TfLiteRuntime& instance()
    {
        static TfLiteRuntime runtime;
        return runtime;
    }

    bool ensureLoaded(const QStringList& candidates)
    {
        if (ready_) {
            return true;
        }

        QStringList searchList = candidates;
        if (searchList.isEmpty()) {
            const QString env = qEnvironmentVariable("KYLIN_TFLITE_RUNTIME");
            if (!env.isEmpty()) {
                searchList << env;
            }
            searchList << QString::fromUtf8(kDefaultTfliteLib);
        }

        for (const QString& candidate : searchList) {
            if (candidate.isEmpty()) {
                continue;
            }
            library_.setFileName(candidate);
            if (!library_.load()) {
                qWarning() << "[NSFWDetector] 无法加载 TensorFlow Lite 运行库" << candidate << ':' << library_.errorString();
                library_.setFileName(QString());
                continue;
            }
            if (!resolveAll(library_)) {
                library_.unload();
                library_.setFileName(QString());
                continue;
            }
            loadedPath_ = candidate;
            ready_ = true;
            qInfo() << "[NSFWDetector] TensorFlow Lite 运行库已加载:" << candidate;
            break;
        }

        if (!ready_) {
            qWarning() << "[NSFWDetector] 未能加载任何 TensorFlow Lite 运行库";
        }
        return ready_;
    }

    bool isReady() const { return ready_; }

    TfLiteModelCreateFromFileFn modelCreateFromFile = nullptr;
    TfLiteModelDeleteFn modelDelete = nullptr;
    TfLiteInterpreterOptionsCreateFn optionsCreate = nullptr;
    TfLiteInterpreterOptionsDeleteFn optionsDelete = nullptr;
    TfLiteInterpreterOptionsSetNumThreadsFn optionsSetNumThreads = nullptr;
    TfLiteInterpreterCreateFn interpreterCreate = nullptr;
    TfLiteInterpreterDeleteFn interpreterDelete = nullptr;
    TfLiteInterpreterAllocateTensorsFn allocateTensors = nullptr;
    TfLiteInterpreterInvokeFn invoke = nullptr;
    TfLiteInterpreterGetInputTensorFn getInputTensor = nullptr;
    TfLiteInterpreterGetOutputTensorFn getOutputTensor = nullptr;
    TfLiteTensorTypeFn tensorType = nullptr;
    TfLiteTensorNumDimsFn tensorNumDims = nullptr;
    TfLiteTensorDimFn tensorDim = nullptr;
    TfLiteTensorByteSizeFn tensorByteSize = nullptr;
    TfLiteTensorCopyFromBufferFn tensorCopyFromBuffer = nullptr;
    TfLiteTensorCopyToBufferFn tensorCopyToBuffer = nullptr;

private:
    TfLiteRuntime() = default;

    bool resolveAll(QLibrary& lib)
    {
#define TFLITE_RESOLVE(member, symbol)                                                                                    \
        member = reinterpret_cast<decltype(member)>(lib.resolve(#symbol));                                                \
        if (!member) {                                                                                                    \
            qWarning() << "[NSFWDetector] 缺少符号" << #symbol << "于" << lib.fileName();                                   \
            return false;                                                                                                \
        }

        TFLITE_RESOLVE(modelCreateFromFile, TfLiteModelCreateFromFile);
        TFLITE_RESOLVE(modelDelete, TfLiteModelDelete);
        TFLITE_RESOLVE(optionsCreate, TfLiteInterpreterOptionsCreate);
        TFLITE_RESOLVE(optionsDelete, TfLiteInterpreterOptionsDelete);
        TFLITE_RESOLVE(optionsSetNumThreads, TfLiteInterpreterOptionsSetNumThreads);
        TFLITE_RESOLVE(interpreterCreate, TfLiteInterpreterCreate);
        TFLITE_RESOLVE(interpreterDelete, TfLiteInterpreterDelete);
        TFLITE_RESOLVE(allocateTensors, TfLiteInterpreterAllocateTensors);
        TFLITE_RESOLVE(invoke, TfLiteInterpreterInvoke);
        TFLITE_RESOLVE(getInputTensor, TfLiteInterpreterGetInputTensor);
        TFLITE_RESOLVE(getOutputTensor, TfLiteInterpreterGetOutputTensor);
        TFLITE_RESOLVE(tensorType, TfLiteTensorType);
        TFLITE_RESOLVE(tensorNumDims, TfLiteTensorNumDims);
        TFLITE_RESOLVE(tensorDim, TfLiteTensorDim);
        TFLITE_RESOLVE(tensorByteSize, TfLiteTensorByteSize);
        TFLITE_RESOLVE(tensorCopyFromBuffer, TfLiteTensorCopyFromBuffer);
        TFLITE_RESOLVE(tensorCopyToBuffer, TfLiteTensorCopyToBuffer);

#undef TFLITE_RESOLVE
        return true;
    }

    bool ready_ = false;
    QLibrary library_;
    QString loadedPath_;
};

QString defaultModelPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString candidate = QDir(appDir).filePath(QStringLiteral("models/open_nsfw.tflite"));
    if (QFileInfo::exists(candidate)) {
        return candidate;
    }

    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataLocation.isEmpty()) {
        candidate = QDir(dataLocation).filePath(QStringLiteral("open_nsfw.tflite"));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    candidate = QDir::current().filePath(QStringLiteral("models/open_nsfw.tflite"));
    if (QFileInfo::exists(candidate)) {
        return candidate;
    }
    return QStringLiteral("models/open_nsfw.tflite");
}

} // namespace

class LightweightNSFWDetector::Impl {
public:
    explicit Impl(NSFWDetectorConfig config)
        : config_(std::move(config))
    {
    }

    ~Impl()
    {
        shutdown();
    }

    bool initialize()
    {
        QStringList runtimeCandidates;
        if (!config_.tfliteRuntimePath.empty()) {
            runtimeCandidates << QString::fromStdString(config_.tfliteRuntimePath);
        }
        if (!TfLiteRuntime::instance().ensureLoaded(runtimeCandidates)) {
            return false;
        }

        auto& api = TfLiteRuntime::instance();
        model_.reset(api.modelCreateFromFile(config_.modelPath.c_str()));
        if (!model_) {
            qWarning() << "[NSFWDetector] 无法加载模型文件" << QString::fromStdString(config_.modelPath);
            return false;
        }

        options_.reset(api.optionsCreate());
        if (options_) {
            api.optionsSetNumThreads(options_.get(), std::max(1, config_.numThreads));
        }

        interpreter_.reset(api.interpreterCreate(model_.get(), options_.get()));
        if (!interpreter_) {
            qWarning() << "[NSFWDetector] 创建解释器失败";
            return false;
        }

        if (api.allocateTensors(interpreter_.get()) != kTfLiteOk) {
            qWarning() << "[NSFWDetector] 分配张量失败";
            return false;
        }

        inputTensor_ = api.getInputTensor(interpreter_.get(), 0);
        outputTensor_ = api.getOutputTensor(interpreter_.get(), 0);
        if (!inputTensor_ || !outputTensor_) {
            qWarning() << "[NSFWDetector] 无法获取输入/输出张量";
            return false;
        }

        if (api.tensorType(inputTensor_) != kTfLiteFloat32) {
            qWarning() << "[NSFWDetector] 输入张量类型不是 Float32";
            return false;
        }

        if (api.tensorNumDims(inputTensor_) != 4) {
            qWarning() << "[NSFWDetector] 输入张量维度异常";
            return false;
        }

        inputHeight_ = api.tensorDim(inputTensor_, 1);
        inputWidth_ = api.tensorDim(inputTensor_, 2);
        inputChannels_ = api.tensorDim(inputTensor_, 3);

        if (inputChannels_ != kExpectedChannels) {
            qWarning() << "[NSFWDetector] 输入通道数异常:" << inputChannels_;
            return false;
        }

        ready_ = true;
        qInfo() << "[NSFWDetector] 已加载模型" << QString::fromStdString(config_.modelPath)
                << "输入尺寸:" << inputWidth_ << "x" << inputHeight_ << "线程:" << config_.numThreads;
        return true;
    }

    void shutdown()
    {
        auto& api = TfLiteRuntime::instance();
        if (interpreter_) {
            api.interpreterDelete(interpreter_.release());
        }
        if (options_) {
            api.optionsDelete(options_.release());
        }
        if (model_) {
            api.modelDelete(model_.release());
        }
        inputTensor_ = nullptr;
        outputTensor_ = nullptr;
        ready_ = false;
    }

    NSFWResult classify(const QImage& image)
    {
        NSFWResult result;
        if (!ready_) {
            result.success = false;
            result.errorMessage = "TensorFlow Lite 解释器尚未就绪";
            return result;
        }
        if (image.isNull()) {
            result.success = false;
            result.errorMessage = "输入图像为空";
            return result;
        }

        std::vector<float> inputData;
        if (!preprocess(image, inputData)) {
            result.success = false;
            result.errorMessage = "图像预处理失败";
            return result;
        }

        auto& api = TfLiteRuntime::instance();
        QMutexLocker locker(&mutex_);
        if (api.tensorCopyFromBuffer(inputTensor_, inputData.data(), inputData.size() * sizeof(float)) != kTfLiteOk) {
            result.success = false;
            result.errorMessage = "写入输入张量失败";
            return result;
        }

        if (api.invoke(interpreter_.get()) != kTfLiteOk) {
            result.success = false;
            result.errorMessage = "TensorFlow Lite 推理失败";
            return result;
        }

        float score = 0.0f;
        if (api.tensorCopyToBuffer(outputTensor_, &score, sizeof(float)) != kTfLiteOk) {
            result.success = false;
            result.errorMessage = "读取输出张量失败";
            return result;
        }

        result.success = true;
        result.nsfwProbability = std::clamp(score, 0.0f, 1.0f);
        result.sfwProbability = 1.0f - result.nsfwProbability;
        return result;
    }

    bool preprocess(const QImage& source, std::vector<float>& buffer)
    {
        QImage formatted = source.convertToFormat(QImage::Format_RGB888);
        if (formatted.width() != inputWidth_ || formatted.height() != inputHeight_) {
            formatted = formatted.scaled(inputWidth_, inputHeight_, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        buffer.resize(static_cast<size_t>(inputWidth_ * inputHeight_ * inputChannels_));
        size_t index = 0;
        for (int y = 0; y < inputHeight_; ++y) {
            const uchar* row = formatted.constScanLine(y);
            for (int x = 0; x < inputWidth_; ++x) {
                const uchar* pixel = row + x * kExpectedChannels;
                for (int c = 0; c < kExpectedChannels; ++c) {
                    const float value = static_cast<float>(pixel[c]) / 127.5f - 1.0f;
                    buffer[index++] = value;
                }
            }
        }
        return true;
    }

    bool isReady() const { return ready_; }

    const NSFWDetectorConfig& config() const { return config_; }

private:
    struct TfLiteDeleter {
        void operator()(TfLiteModel* model) const
        {
            if (model) {
                TfLiteRuntime::instance().modelDelete(model);
            }
        }
        void operator()(TfLiteInterpreterOptions* options) const
        {
            if (options) {
                TfLiteRuntime::instance().optionsDelete(options);
            }
        }
        void operator()(TfLiteInterpreter* interpreter) const
        {
            if (interpreter) {
                TfLiteRuntime::instance().interpreterDelete(interpreter);
            }
        }
    };

    NSFWDetectorConfig config_;
    std::unique_ptr<TfLiteModel, TfLiteDeleter> model_;
    std::unique_ptr<TfLiteInterpreterOptions, TfLiteDeleter> options_;
    std::unique_ptr<TfLiteInterpreter, TfLiteDeleter> interpreter_;
    TfLiteTensor* inputTensor_ = nullptr;
    const TfLiteTensor* outputTensor_ = nullptr;
    int inputWidth_ = 224;
    int inputHeight_ = 224;
    int inputChannels_ = kExpectedChannels;
    bool ready_ = false;
    mutable QMutex mutex_;
};

LightweightNSFWDetector::LightweightNSFWDetector(const std::string& modelPath, float threshold)
    : modelPath_(modelPath), threshold_(threshold)
{
    NSFWDetectorConfig cfg;
    cfg.modelPath = modelPath;
    cfg.threshold = threshold;
    impl_ = std::make_unique<Impl>(cfg);
    available_ = impl_->initialize();
}

LightweightNSFWDetector::LightweightNSFWDetector(NSFWDetectorConfig config)
    : modelPath_(config.modelPath), threshold_(config.threshold)
{
    impl_ = std::make_unique<Impl>(config);
    available_ = impl_->initialize();
}

LightweightNSFWDetector::~LightweightNSFWDetector() = default;

NSFWResult LightweightNSFWDetector::classifyImage(const QImage& image)
{
    if (!available_) {
        return {false, 0.0f, 0.0f, "检测器不可用"};
    }
    return impl_->classify(image);
}

NSFWResult LightweightNSFWDetector::classifyImageData(const QByteArray& imageData, const std::string& format)
{
    if (!available_) {
        return {false, 0.0f, 0.0f, "检测器不可用"};
    }
    Q_UNUSED(format);
    QBuffer buffer(const_cast<QByteArray*>(&imageData));
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setDecideFormatFromContent(true);
    QImage image = reader.read();
    if (image.isNull()) {
        return {false, 0.0f, 0.0f, "无法解析图像数据"};
    }
    return impl_->classify(image);
}

NSFWDetectorConfig NSFWDetectorConfig::fromEnvironment()
{
    NSFWDetectorConfig config;

    const QByteArray modelEnv = qgetenv("KYLIN_NSFW_MODEL");
    const QByteArray legacyEnv = qgetenv("KYLIN_NSFW_MODEL_PATH");
    if (!modelEnv.isEmpty()) {
        config.modelPath = modelEnv.toStdString();
    } else if (!legacyEnv.isEmpty()) {
        config.modelPath = legacyEnv.toStdString();
    } else {
        config.modelPath = defaultModelPath().toStdString();
    }

    const QByteArray thresholdEnv = qgetenv("KYLIN_NSFW_THRESHOLD");
    if (!thresholdEnv.isEmpty()) {
        config.threshold = thresholdEnv.toFloat();
    }

    const QByteArray threadsEnv = qgetenv("KYLIN_NSFW_THREADS");
    if (!threadsEnv.isEmpty()) {
        bool ok = false;
        const int value = threadsEnv.toInt(&ok);
        if (ok && value > 0) {
            config.numThreads = value;
        }
    }

    const QByteArray runtimeEnv = qgetenv("KYLIN_TFLITE_RUNTIME");
    if (!runtimeEnv.isEmpty()) {
        config.tfliteRuntimePath = runtimeEnv.toStdString();
    }

    const QByteArray backendEnv = qgetenv("KYLIN_NSFW_BACKEND").trimmed().toLower();
    if (backendEnv == "gpu") {
        config.useGPU = true;
    }

    qInfo() << "[NSFWDetector] 配置 - 模型路径:" << QString::fromStdString(config.modelPath)
            << "阈值:" << config.threshold << "线程:" << config.numThreads
            << "GPU:" << config.useGPU;
    return config;
}

} // namespace KylinMessenger::AI