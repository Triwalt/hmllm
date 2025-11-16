#include "nsfw_compliance_service.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include "qt_compat.h"

#include <algorithm>

namespace KylinMessenger {

Q_LOGGING_CATEGORY(lcCompliance, "kylin.compliance")

namespace {
constexpr double kDefaultBlockThreshold = 0.7;
constexpr double kDefaultReviewThreshold = 0.5;

QString resolveDefaultModel()
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

	return candidate;
}

} // namespace

NsfwComplianceConfig NsfwComplianceConfig::fromEnvironment()
{
	NsfwComplianceConfig cfg;
	cfg.detectorConfig = AI::NSFWDetectorConfig::fromEnvironment();

	if (cfg.detectorConfig.modelPath.empty()) {
		cfg.detectorConfig.modelPath = resolveDefaultModel().toStdString();
	}
	cfg.modelPath = QString::fromStdString(cfg.detectorConfig.modelPath);

	const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

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

NsfwComplianceService::NsfwComplianceService(NsfwComplianceConfig config)
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

	try {
		m_detector = std::make_unique<AI::LightweightNSFWDetector>(m_config.detectorConfig);
		if (!m_detector->isAvailable()) {
			qCWarning(lcCompliance) << "NSFW 检测器未就绪" << m_config.modelPath;
			m_detector.reset();
		} else {
			qCInfo(lcCompliance) << "NSFW 检测器已启用，模型:" << m_config.modelPath;
		}
	} catch (const std::exception& ex) {
		qCWarning(lcCompliance) << "初始化 NSFW 检测器失败:" << ex.what();
		m_detector.reset();
	}
}

NsfwComplianceService::~NsfwComplianceService() = default;

bool NsfwComplianceService::isAvailable() const
{
	return m_detector && m_detector->isAvailable();
}

ComplianceResult NsfwComplianceService::evaluate(const CompliancePayload& payload)
{
	if (!isAvailable()) {
		return {ComplianceVerdict::Allowed, QString()};
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

ComplianceResult NsfwComplianceService::evaluateImage(const QByteArray& data,
													  const QString& sourceDescription) const
{
	if (!m_detector) {
		return {ComplianceVerdict::NeedsReview,
				QStringLiteral("NSFW 检测器不可用，需人工复核。")};
	}

	const auto result = m_detector->classifyImageData(data, sourceDescription.toStdString());
	if (!result.success) {
		qCWarning(lcCompliance) << "NSFW 检测失败" << QString::fromStdString(result.errorMessage);
		return {ComplianceVerdict::NeedsReview,
				QStringLiteral("NSFW 检测失败：%1").arg(QString::fromStdString(result.errorMessage))};
	}

	const double nsfwScore = result.nsfwProbability;
	qCInfo(lcCompliance) << "NSFW 评分" << sourceDescription << "nsfw" << nsfwScore;

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

QString NsfwComplianceService::defaultModelPath()
{
	return resolveDefaultModel();
}

} // namespace KylinMessenger


} // namespace KylinMessenger

