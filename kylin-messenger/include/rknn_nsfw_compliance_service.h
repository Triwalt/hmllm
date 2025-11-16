#ifndef KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H
#define KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H

#include "compliance_service.h"

#include <QByteArray>
#include <QImage>
#include <memory>

#ifdef HAVE_RKNN_RT
#include <rknn_api.h>
#endif

namespace KylinMessenger {

struct RknnComplianceConfig {
    QString modelPath;
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
    ComplianceResult evaluateImage(const QByteArray& data,
                                   const QString& sourceDescription) const;
    bool preprocessImage(const QByteArray& data, QByteArray& buffer) const;

    QByteArray m_modelBlob;
    mutable QByteArray m_inputBuffer;
    rknn_context m_context = 0;
    int m_inputWidth = 224;
    int m_inputHeight = 224;
    int m_inputChannels = 3;
#endif // HAVE_RKNN_RT
    RknnComplianceConfig m_config;
    bool m_runtimeReady = false;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_RKNN_NSFW_COMPLIANCE_SERVICE_H

