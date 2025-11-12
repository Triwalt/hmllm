#include "compliance_stub_service.h"

namespace KylinMessenger {

ComplianceResult ComplianceStubService::evaluate(const CompliancePayload& payload)
{
    Q_UNUSED(payload);
    return {ComplianceVerdict::Allowed, QString()};
}

} // namespace KylinMessenger
