#include "rknn_nsfw_compliance_service.h"

#include <QLoggingCategory>
#include <QProcessEnvironment>

namespace KylinMessenger {

#ifdef HAVE_RKNN_RT

#include <QFileInfo>
#include <rknn_api.h>

static Q_LOGGING_CATEGORY(lcRknnCompliance, "kylin.compliance.rknn")

RknnComplianceConfig RknnComplianceConfig::fromEnvironment()
{
    RknnComplianceConfig cfg;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    const QString modelEnv = env.value(QStringLiteral("KYLIN_NSFW_RKNN_MODEL")).trimmed();
    if (!modelEnv.isEmpty()) {
        cfg.modelPath = modelEnv;
    }

    bool ok = false;
    const QString deviceEnv = env.value(QStringLiteral("KYLIN_NSFW_RKNN_DEVICE"));
    if (!deviceEnv.isEmpty()) {
        const int id = deviceEnv.toInt(&ok);
        if (ok && id >= 0) {
            cfg.deviceId = id;
        }
    }

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
        cfg.blockThreshold = 0.7;
        cfg.reviewThreshold = 0.5;
    }

    return cfg;
}

RknnNsfwComplianceService::RknnNsfwComplianceService(RknnComplianceConfig config)
    : m_config(std::move(config))
{
    m_runtimeReady = initializeRuntime();
}

RknnNsfwComplianceService::~RknnNsfwComplianceService() = default;

bool RknnNsfwComplianceService::initializeRuntime()
{
    if (m_config.modelPath.isEmpty()) {
        qCWarning(lcRknnCompliance) << "RKNN NSFW 模型路径未配置";
        return false;
    }
    if (!QFileInfo::exists(m_config.modelPath)) {
        qCWarning(lcRknnCompliance) << "RKNN NSFW 模型不存在:" << m_config.modelPath;
        return false;
    }

    // 目前仅做基本文件检查；实际模型加载将在后续实现中补充。
    qCInfo(lcRknnCompliance) << "RKNN NSFW 结构已预留，等待模型接入";
    return false;
}

bool RknnNsfwComplianceService::isAvailable() const
{
    return m_runtimeReady;
}

ComplianceResult RknnNsfwComplianceService::evaluate(const CompliancePayload&)
{
    return {ComplianceVerdict::NeedsReview,
            QStringLiteral("RKNN NSFW 审核尚未实现，已回退至人工复核流程。")};
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

