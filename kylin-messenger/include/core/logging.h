#ifndef KYLIN_MESSENGER_CORE_LOGGING_H
#define KYLIN_MESSENGER_CORE_LOGGING_H

#include <QString>

namespace KylinMessenger::Core::Logging {

void initialize(const QString& applicationId = QStringLiteral("kylin-messenger"));
void shutdown();

} // namespace KylinMessenger::Core::Logging

#endif // KYLIN_MESSENGER_CORE_LOGGING_H

