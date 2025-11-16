#ifndef KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H
#define KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H

#include "ai/opencv_nsfw_detector.h"
#include "compliance_service.h"

#include <QLoggingCategory>
#include <memory>

namespace KylinMessenger {

Q_DECLARE_LOGGING_CATEGORY(lcCompliance)

struct NsfwComplianceConfig {
    AI::NSFWDetectorConfig detectorConfig;
    QString modelPath;
    double blockThreshold = 0.7;      // Strong likelihood of porn/hentai
    double reviewThreshold = 0.5;     // Needs manual review when exceeding this score

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
    ComplianceResult evaluateImage(const QByteArray& data,
                                   const QString& sourceDescription) const;

    NsfwComplianceConfig m_config;
    std::unique_ptr<AI::LightweightNSFWDetector> m_detector;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_NSFW_COMPLIANCE_SERVICE_H

