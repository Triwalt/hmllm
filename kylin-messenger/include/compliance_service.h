#ifndef KYLIN_MESSENGER_COMPLIANCE_SERVICE_H
#define KYLIN_MESSENGER_COMPLIANCE_SERVICE_H

#include <QString>
#include <QByteArray>
#include <functional>

namespace KylinMessenger {

enum class ComplianceTarget {
    Text,
    Image,
    File
};

enum class ComplianceVerdict {
    Allowed,
    Blocked,
    NeedsReview
};

struct ComplianceResult {
    ComplianceVerdict verdict = ComplianceVerdict::Allowed;
    QString reason;
};

struct CompliancePayload {
    ComplianceTarget target = ComplianceTarget::Text;
    QString conversation_id;
    QString local_user_id;
    QString peer_user_id;
    QString message_id;
    QString text;
    QByteArray binary;
    QString content_type;
    bool is_outgoing = true;
};

class IComplianceService {
public:
    virtual ~IComplianceService() = default;

    virtual bool isAvailable() const = 0;

    virtual ComplianceResult evaluate(const CompliancePayload& payload) = 0;

    virtual void evaluateAsync(
        const CompliancePayload& payload,
        std::function<void(const ComplianceResult&)> callback)
    {
        if (callback) {
            callback(evaluate(payload));
        }
    }
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_COMPLIANCE_SERVICE_H
