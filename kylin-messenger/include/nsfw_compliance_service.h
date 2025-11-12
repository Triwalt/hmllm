#ifndef KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H
#define KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H

#include "compliance_service.h"

#include <QLoggingCategory>
#include <QProcess>
#include <QTemporaryFile>

namespace KylinMessenger {

Q_DECLARE_LOGGING_CATEGORY(lcCompliance)

struct NsfwComplianceConfig {
    QString pythonPath = QStringLiteral("python");
    QString modelPath;
    double blockThreshold = 0.7;      // Strong likelihood of porn/hentai
    double reviewThreshold = 0.5;     // Needs manual review when exceeding this score
    int processTimeoutMs = 15'000;

    static NsfwComplianceConfig fromEnvironment();
};

class NsfwComplianceService final : public IComplianceService {
public:
    explicit NsfwComplianceService(NsfwComplianceConfig config);
    ~NsfwComplianceService() override;

    bool isAvailable() const override;
    ComplianceResult evaluate(const CompliancePayload& payload) override;

    static QString defaultModelPath();

private:
    bool ensureScriptReady();
    ComplianceResult evaluateImage(const QByteArray& data,
                                   const QString& sourceDescription) const;
    static QString extractScript();

    NsfwComplianceConfig m_config;
    mutable QString m_scriptPath;
    mutable bool m_scriptExtracted = false;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H

