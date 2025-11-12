#include "nsfw_compliance_service.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTemporaryFile>

#include "qt_compat.h"

#include <algorithm>

namespace KylinMessenger {

Q_LOGGING_CATEGORY(lcCompliance, "kylin.compliance")

namespace {
constexpr double kDefaultBlockThreshold = 0.7;
constexpr double kDefaultReviewThreshold = 0.5;
constexpr int kMaxImageDimension = 1024;
constexpr int kDefaultJpegQuality = 85;
constexpr int kDefaultPngQuality = 90;

QString detectPythonExecutable(const QProcessEnvironment& env)
{
    const QString existing = env.value(QStringLiteral("KYLIN_NSFW_PYTHON")).trimmed();
    if (!existing.isEmpty()) {
        return existing;
    }

    const QString pathEnv = env.value(QStringLiteral("PATH"));
    const QStringList pathEntries = pathEnv.split(QDir::listSeparator(), KYLIN_SPLIT_SKIP_EMPTY);
    QStringList candidates;

#if defined(Q_OS_WIN)
    candidates << QStringLiteral("python.exe")
               << QStringLiteral("python3.exe")
               << QStringLiteral("py.exe");
#else
    candidates << QStringLiteral("python3")
               << QStringLiteral("python");
#endif

    for (const QString& candidate : candidates) {
        const QString resolved = QStandardPaths::findExecutable(candidate, pathEntries);
        if (!resolved.isEmpty()) {
            return resolved;
        }
    }

#if defined(Q_OS_WIN)
    // 常见的微软存放路径
    const QString localAppData = env.value(QStringLiteral("LOCALAPPDATA"));
    if (!localAppData.isEmpty()) {
        for (int major = 3; major <= 3; ++major) {
            for (int minor = 7; minor <= 12; ++minor) {
                const QString candidate = QDir(localAppData)
                                             .filePath(QStringLiteral("Programs/Python/Python%1%2/python.exe")
                                                           .arg(major)
                                                           .arg(minor, 1, 10, QLatin1Char('0')));
                if (QFileInfo::exists(candidate)) {
                    return candidate;
                }
            }
        }
    }
#else
    const QString usrLocalBin = QStringLiteral("/usr/local/bin/python3");
    if (QFileInfo::exists(usrLocalBin)) {
        return usrLocalBin;
    }
#endif

    // 退回一个通用命令名，让后续 QProcess 尝试从 PATH 解析
#if defined(Q_OS_WIN)
    return QStringLiteral("python");
#else
    return QStringLiteral("python3");
#endif
}

struct PreparedImage {
    QByteArray bytes;
    QString suffix;
};

PreparedImage prepareImagePayload(const QByteArray& data)
{
    PreparedImage result{data, QStringLiteral("png")};

    QImage image;
    if (!image.loadFromData(data)) {
        return result;
    }

    QSize targetSize = image.size();
    if (targetSize.width() > kMaxImageDimension || targetSize.height() > kMaxImageDimension) {
        targetSize.scale(kMaxImageDimension, kMaxImageDimension, Qt::KeepAspectRatio);
        image = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QByteArray encoded;
    QBuffer buffer(&encoded);
    buffer.open(QIODevice::WriteOnly);

    if (image.hasAlphaChannel()) {
        result.suffix = QStringLiteral("png");
        if (!image.save(&buffer, "PNG", kDefaultPngQuality)) {
            return result;
        }
    } else {
        result.suffix = QStringLiteral("jpg");
        if (!image.save(&buffer, "JPEG", kDefaultJpegQuality)) {
            return result;
        }
    }

    result.bytes = encoded;
    return result;
}
}

NsfwComplianceConfig NsfwComplianceConfig::fromEnvironment()
{
    NsfwComplianceConfig cfg;
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    cfg.pythonPath = detectPythonExecutable(env);

    const QString modelEnv = env.value(QStringLiteral("KYLIN_NSFW_MODEL")).trimmed();
    if (!modelEnv.isEmpty()) {
        cfg.modelPath = modelEnv;
    } else {
        cfg.modelPath = NsfwComplianceService::defaultModelPath();
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

    const QString timeoutEnv = env.value(QStringLiteral("KYLIN_NSFW_TIMEOUT_MS"));
    if (!timeoutEnv.isEmpty()) {
        const int timeout = timeoutEnv.toInt(&ok);
        if (ok && timeout > 0) {
            cfg.processTimeoutMs = timeout;
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
        m_config.reviewThreshold = m_config.blockThreshold * 0.8;
    }
    if (m_config.processTimeoutMs <= 0) {
        m_config.processTimeoutMs = 15'000;
    }
}

NsfwComplianceService::~NsfwComplianceService()
{
    if (!m_scriptPath.isEmpty()) {
        QFile::remove(m_scriptPath);
    }
}

bool NsfwComplianceService::isAvailable() const
{
    if (m_config.pythonPath.isEmpty()) {
        qCWarning(lcCompliance) << "Python path is empty; NSFW compliance disabled";
        return false;
    }
    if (m_config.modelPath.isEmpty()) {
        qCWarning(lcCompliance) << "NSFW model path is not configured";
        return false;
    }
    if (!QFileInfo::exists(m_config.modelPath)) {
        qCWarning(lcCompliance) << "Configured NSFW model not found:" << m_config.modelPath;
        return false;
    }
    if (!const_cast<NsfwComplianceService*>(this)->ensureScriptReady()) {
        qCWarning(lcCompliance) << "Failed to prepare NSFW helper script";
        return false;
    }
    return true;
}

ComplianceResult NsfwComplianceService::evaluate(const CompliancePayload& payload)
{
    if (!isAvailable()) {
        return {ComplianceVerdict::Allowed, QString()};
    }

    switch (payload.target) {
    case ComplianceTarget::Image:
        if (!payload.binary.isEmpty()) {
            return evaluateImage(payload.binary, QStringLiteral("image"));
        }
        break;
    case ComplianceTarget::File:
        if (!payload.binary.isEmpty()) {
            QImage image;
            if (image.loadFromData(payload.binary)) {
                return evaluateImage(payload.binary, QStringLiteral("file"));
            }
        }
        break;
    default:
        break;
    }

    return {ComplianceVerdict::Allowed, QString()};
}

bool NsfwComplianceService::ensureScriptReady()
{
    if (m_scriptExtracted && QFileInfo::exists(m_scriptPath)) {
        return true;
    }

    const QString script = extractScript();
    if (script.isEmpty()) {
        return false;
    }
    m_scriptPath = script;
    m_scriptExtracted = true;
    return true;
}

ComplianceResult NsfwComplianceService::evaluateImage(const QByteArray& data,
                                                      const QString& sourceDescription) const
{
    if (!const_cast<NsfwComplianceService*>(this)->ensureScriptReady()) {
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("NSFW 检测脚本不可用，内容需要人工复核。")};
    }

    const PreparedImage prepared = prepareImagePayload(data);

    QTemporaryFile temp(QDir::tempPath()
                            + QStringLiteral("/kylin-nsfw-XXXXXX.%1").arg(prepared.suffix));
    temp.setAutoRemove(false);
    if (!temp.open()) {
        qCWarning(lcCompliance) << "Unable to create temporary file for NSFW evaluation";
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("无法创建临时文件，请人工复核内容。")};
    }
    if (temp.write(prepared.bytes) != prepared.bytes.size()) {
        qCWarning(lcCompliance) << "Failed to write payload for NSFW evaluation";
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("无法写入待检测图像，请人工复核内容。")};
    }
    temp.close();
    const QString imagePath = temp.fileName();

    qCDebug(lcCompliance) << "Prepared image for NSFW" << sourceDescription
                          << "size" << prepared.bytes.size()
                          << "path" << imagePath;

    QProcess process;
    QStringList args;
    args << m_scriptPath
         << QStringLiteral("--model") << m_config.modelPath
         << QStringLiteral("--image") << imagePath;

    process.setProgram(m_config.pythonPath);
    process.setArguments(args);
    process.start();

    if (!process.waitForStarted(m_config.processTimeoutMs)) {
        qCWarning(lcCompliance) << "Failed to start Python process for NSFW evaluation";
        QFile::remove(imagePath);
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("NSFW 检测服务启动失败，内容需要人工复核。")};
    }

    if (!process.waitForFinished(m_config.processTimeoutMs)) {
        qCWarning(lcCompliance) << "NSFW evaluation timed out";
        process.kill();
        process.waitForFinished(2000);
        QFile::remove(imagePath);
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("NSFW 检测超时，内容需要人工复核。")};
    }

    QFile::remove(imagePath);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString stderrOutput = QString::fromUtf8(process.readAllStandardError());
        qCWarning(lcCompliance) << "NSFW classifier failed:" << stderrOutput;
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("NSFW 检测失败：%1").arg(stderrOutput.trimmed())};
    }

    const QByteArray stdoutData = process.readAllStandardOutput();
    const QJsonDocument doc = QJsonDocument::fromJson(stdoutData);
    if (!doc.isObject()) {
        qCWarning(lcCompliance) << "Unexpected NSFW output" << stdoutData;
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("NSFW 检测结果无法解析，需人工复核。")};
    }

    const QJsonObject obj = doc.object();
    const double pornScore = obj.value(QStringLiteral("porn")).toDouble(0.0);
    const double hentaiScore = obj.value(QStringLiteral("hentai")).toDouble(0.0);
    const double sexyScore = obj.value(QStringLiteral("sexy")).toDouble(0.0);

    const double highRiskScore = std::max(pornScore, hentaiScore);
    const double combinedScore = pornScore + hentaiScore;
    const double softScore = sexyScore + combinedScore;

    qCInfo(lcCompliance) << "NSFW scores" << sourceDescription
                         << "porn" << pornScore
                         << "hentai" << hentaiScore
                         << "sexy" << sexyScore;

    if (highRiskScore >= m_config.blockThreshold || combinedScore >= m_config.blockThreshold) {
        return {ComplianceVerdict::Blocked,
                QStringLiteral("检测到疑似涉黄内容，已阻止发送。")};
    }

    if (softScore >= m_config.reviewThreshold) {
        return {ComplianceVerdict::NeedsReview,
                QStringLiteral("内容可能涉黄，请人工复核。")};
    }

    return {ComplianceVerdict::Allowed, QString()};
}

QString NsfwComplianceService::defaultModelPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString candidate = QDir(appDir).filePath(QStringLiteral("models/nsfw_mobilenet2.224x224.h5"));
    if (QFileInfo::exists(candidate)) {
        return candidate;
    }

    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataLocation.isEmpty()) {
        candidate = QDir(dataLocation).filePath(QStringLiteral("nsfw_mobilenet2.224x224.h5"));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

QString NsfwComplianceService::extractScript()
{
    QFile scriptRes(QStringLiteral(":/scripts/nsfw_review.py"));
    if (!scriptRes.open(QIODevice::ReadOnly)) {
        qCWarning(lcCompliance) << "Failed to open embedded NSFW script resource";
        return QString();
    }

    QTemporaryFile tmp(QDir::tempPath() + QLatin1String("/kylin-nsfw-script-XXXXXX.py"));
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        qCWarning(lcCompliance) << "Unable to extract NSFW script";
        return QString();
    }

    tmp.write(scriptRes.readAll());
    tmp.close();
    return tmp.fileName();
}

} // namespace KylinMessenger

