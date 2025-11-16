#include "rknn_nsfw_compliance_service.h"

#include <QLoggingCategory>
#include <QProcessEnvironment>

namespace KylinMessenger {

#ifdef HAVE_RKNN_RT

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include <algorithm>
#include <cstring>

#include <rknn_api.h>

static Q_LOGGING_CATEGORY(lcRknnCompliance, "kylin.compliance.rknn")

namespace {
constexpr double kDefaultBlockThreshold = 0.7;
constexpr double kDefaultReviewThreshold = 0.5;
constexpr int kDefaultInputWidth = 224;
constexpr int kDefaultInputHeight = 224;
constexpr int kDefaultChannels = 3;

QString defaultRknnModelPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString candidate = QDir(appDir).filePath(QStringLiteral("models/open_nsfw.rknn"));
    if (QFileInfo::exists(candidate)) {
        return candidate;
    }

    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataLocation.isEmpty()) {
        candidate = QDir(dataLocation).filePath(QStringLiteral("open_nsfw.rknn"));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    candidate = QDir::current().filePath(QStringLiteral("models/open_nsfw.rknn"));
    return candidate;
}

} // namespace

RknnComplianceConfig RknnComplianceConfig::fromEnvironment()
{
    RknnComplianceConfig cfg;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    const QString modelEnv = env.value(QStringLiteral("KYLIN_NSFW_RKNN_MODEL")).trimmed();
    if (!modelEnv.isEmpty()) {
        cfg.modelPath = modelEnv;
    } else {
        cfg.modelPath = defaultRknnModelPath();
    }

    bool ok = false;
    const QString blockEnv = env.value(QStringLiteral("KYLIN_NSFW_BLOCK_THRESHOLD"));
    if (!blockEnv.isEmpty()) {
        const double value = blockEnv.toDouble(&ok);
        if (ok && value >= 0.0 && value <= 1.0) {
            cfg.blockThreshold = value;
        }
    }

    const QString reviewEnv = env.value(QStringLiteral("KYLIN_NSFW_REVIEW_THRESHOLD"));
    if (!reviewEnv.isEmpty()) {
        const double value = reviewEnv.toDouble(&ok);
        if (ok && value >= 0.0 && value <= 1.0) {
            cfg.reviewThreshold = value;
        }
    }

    if (cfg.blockThreshold < cfg.reviewThreshold) {
        cfg.blockThreshold = kDefaultBlockThreshold;
        cfg.reviewThreshold = kDefaultReviewThreshold;
    }

    return cfg;
}

RknnNsfwComplianceService::RknnNsfwComplianceService(RknnComplianceConfig config)
    : m_config(std::move(config))
{
    if (m_config.blockThreshold <= 0.0 || m_config.blockThreshold > 1.0) {
        m_config.blockThreshold = kDefaultBlockThreshold;
    }
    if (m_config.reviewThreshold < 0.0 || m_config.reviewThreshold > 1.0) {
        m_config.reviewThreshold = kDefaultReviewThreshold;
    }
    if (m_config.reviewThreshold > m_config.blockThreshold) {
        m_config.reviewThreshold = std::min(m_config.blockThreshold * 0.8, m_config.blockThreshold);
    }

    m_runtimeReady = initializeRuntime();
}

RknnNsfwComplianceService::~RknnNsfwComplianceService()
{
    if (m_context) {
        rknn_destroy(m_context);
        m_context = 0;
    }
}

bool RknnNsfwComplianceService::initializeRuntime()
{
    if (m_config.modelPath.isEmpty()) {
        qCWarning(lcRknnCompliance) << "RKNN NSFW 模型路径未配置";
        return false;
    }
    QFileInfo info(m_config.modelPath);
    if (!info.exists()) {
        qCWarning(lcRknnCompliance) << "RKNN NSFW 模型不存在:" << m_config.modelPath;
        return false;
    }

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcRknnCompliance) << "无法打开 RKNN 模型文件" << info.absoluteFilePath();
        return false;
    }
    m_modelBlob = file.readAll();
    file.close();

    if (m_modelBlob.isEmpty()) {
        qCWarning(lcRknnCompliance) << "RKNN 模型内容为空";
        return false;
    }

    int ret = rknn_init(&m_context,
                        m_modelBlob.data(),
                        m_modelBlob.size(),
                        0,
                        nullptr);
    if (ret != RKNN_SUCC) {
        qCWarning(lcRknnCompliance) << "rknn_init 失败" << ret;
        m_context = 0;
        return false;
    }

    rknn_input_output_num io_num{};
    ret = rknn_query(m_context,
                     RKNN_QUERY_IN_OUT_NUM,
                     &io_num,
                     sizeof(io_num));
    if (ret != RKNN_SUCC || io_num.n_input < 1 || io_num.n_output < 1) {
        qCWarning(lcRknnCompliance) << "rknn_query 输入输出失败" << ret;
        return false;
    }

    rknn_tensor_attr input_attr{};
    input_attr.index = 0;
    ret = rknn_query(m_context,
                     RKNN_QUERY_INPUT_ATTR,
                     &input_attr,
                     sizeof(input_attr));
    if (ret != RKNN_SUCC) {
        qCWarning(lcRknnCompliance) << "查询输入属性失败" << ret;
        return false;
    }

    if (input_attr.n_dims >= 4) {
        if (input_attr.fmt == RKNN_TENSOR_NHWC) {
            m_inputHeight = input_attr.dims[1];
            m_inputWidth = input_attr.dims[2];
            m_inputChannels = input_attr.dims[3];
        } else {
            m_inputChannels = input_attr.dims[1];
            m_inputHeight = input_attr.dims[2];
            m_inputWidth = input_attr.dims[3];
        }
    }

    if (m_inputWidth <= 0) {
        m_inputWidth = kDefaultInputWidth;
    }
    if (m_inputHeight <= 0) {
        m_inputHeight = kDefaultInputHeight;
    }
    if (m_inputChannels <= 0) {
        m_inputChannels = kDefaultChannels;
    }

    qCInfo(lcRknnCompliance) << "RKNN NSFW 运行时已初始化，输入尺寸"
                             << m_inputWidth << 'x' << m_inputHeight
                             << "通道" << m_inputChannels;
    return true;
}

bool RknnNsfwComplianceService::isAvailable() const
{
    return m_runtimeReady;
}

ComplianceResult RknnNsfwComplianceService::evaluate(const CompliancePayload& payload)
{
    if (!m_runtimeReady || !m_context) {
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("RKNN NSFW 审核不可用，已回退至人工复核。")};
    }

    if (payload.binary.isEmpty()) {
        return {ComplianceVerdict::Allowed, QString()};
    }

    switch (payload.target) {
    case ComplianceTarget::Image:
    case ComplianceTarget::File:
        return evaluateImage(payload.binary, payload.message_id);
    default:
        return {ComplianceVerdict::Allowed, QString()};
    }
}

ComplianceResult RknnNsfwComplianceService::evaluateImage(const QByteArray& data,
                                                          const QString& sourceDescription) const
{
    QByteArray buffer;
    if (!preprocessImage(data, buffer)) {
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("RKNN 预处理失败，需人工复核。")};
    }

    rknn_input input{};
    input.index = 0;
    input.type = RKNN_TENSOR_UINT8;
    input.size = buffer.size();
    input.fmt = RKNN_TENSOR_NHWC;
    input.buf = buffer.data();

    const rknn_context ctx = m_context;
    int ret = rknn_inputs_set(ctx, 1, &input);
    if (ret != RKNN_SUCC) {
        qCWarning(lcRknnCompliance) << "rknn_inputs_set 失败" << ret;
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("RKNN 输入设置失败，需人工复核。")};
    }

    ret = rknn_run(ctx, nullptr);
    if (ret != RKNN_SUCC) {
        qCWarning(lcRknnCompliance) << "rknn_run 失败" << ret;
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("RKNN 推理失败，需人工复核。")};
    }

    rknn_output output{};
    output.want_float = 1;
    ret = rknn_outputs_get(ctx, 1, &output, nullptr);
    if (ret != RKNN_SUCC) {
        qCWarning(lcRknnCompliance) << "rknn_outputs_get 失败" << ret;
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("无法获取 RKNN 结果，需人工复核。")};
    }

    float score = 0.0f;
    if (output.size >= sizeof(float) && output.buf) {
        std::memcpy(&score, output.buf, sizeof(float));
    } else if (output.size >= sizeof(int16_t) && output.buf) {
        score = *reinterpret_cast<int16_t*>(output.buf) / 32768.0f;
    }

    rknn_outputs_release(ctx, 1, &output);

    const double nsfwScore = std::clamp(static_cast<double>(score), 0.0, 1.0);
    qCInfo(lcRknnCompliance) << "RKNN NSFW 评分" << sourceDescription << nsfwScore;

    if (nsfwScore >= m_config.blockThreshold) {
        return {ComplianceVerdict::Blocked,
                QStringLiteral("检测到疑似涉黄内容，已阻止发送。")};
    }

    if (nsfwScore >= m_config.reviewThreshold) {
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("内容可能涉黄，请人工复核。")};
    }

    return {ComplianceVerdict::Allowed, QString()};
}

bool RknnNsfwComplianceService::preprocessImage(const QByteArray& data, QByteArray& buffer) const
{
    QImage image;
    if (!image.loadFromData(data)) {
        qCWarning(lcRknnCompliance) << "无法解析图像数据";
        return false;
    }

    QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    if (rgb.width() != m_inputWidth || rgb.height() != m_inputHeight) {
        rgb = rgb.scaled(m_inputWidth, m_inputHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const int stride = m_inputWidth * m_inputChannels;
    buffer.resize(stride * m_inputHeight);
    uchar* dst = reinterpret_cast<uchar*>(buffer.data());
    for (int y = 0; y < m_inputHeight; ++y) {
        const uchar* srcRow = rgb.constScanLine(y);
        std::memcpy(dst + y * stride, srcRow, stride);
    }
    return true;
}

#else // HAVE_RKNN_RT

RknnComplianceConfig RknnComplianceConfig::fromEnvironment()
{
    return {};
}

RknnNsfwComplianceService::RknnNsfwComplianceService(RknnComplianceConfig config)
    : m_config(std::move(config))
{
}

RknnNsfwComplianceService::~RknnNsfwComplianceService() = default;

bool RknnNsfwComplianceService::isAvailable() const
{
    return false;
}

ComplianceResult RknnNsfwComplianceService::evaluate(const CompliancePayload&)
{
    return {ComplianceVerdict::NeedsReview,
            QStringLiteral("构建未启用 RKNN 运行时，NSFW 审核将回退。")};
}

#endif // HAVE_RKNN_RT

} // namespace KylinMessenger

