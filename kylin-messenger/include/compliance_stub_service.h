#ifndef KYLIN_MESSENGER_COMPLIANCE_STUB_SERVICE_H
#define KYLIN_MESSENGER_COMPLIANCE_STUB_SERVICE_H

#include "compliance_service.h"

namespace KylinMessenger {

class ComplianceStubService : public IComplianceService {
public:
    ComplianceStubService() = default;
    ~ComplianceStubService() override = default;

    bool isAvailable() const override { return true; }

    ComplianceResult evaluate(const CompliancePayload& payload) override;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_COMPLIANCE_STUB_SERVICE_H
