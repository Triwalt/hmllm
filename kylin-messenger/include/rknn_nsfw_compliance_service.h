#ifndef KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H
#define KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H

#include "compliance_service.h"
#include "nsfw_compliance_service.h"

namespace KylinMessenger {

struct RknnComplianceConfig {
    QString modelPath;
    int deviceId = 0;
    double blockThreshold = 0.7;
    double reviewThreshold = 0.5;

    static RknnComplianceConfig fromEnvironment();
};

class RknnNsfwComplianceService final : public IComplianceService {
public:
    explicit RknnNsfwComplianceService(RknnComplianceConfig config);
    ~RknnNsfwComplianceService() override;

    bool isAvailable() const override;
    ComplianceResult evaluate(const CompliancePayload& payload) override;

private:
#ifdef HAVE_RKNN_RT
    bool initializeRuntime();
#endif
    RknnComplianceConfig m_config;
    bool m_runtimeReady = false;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H

