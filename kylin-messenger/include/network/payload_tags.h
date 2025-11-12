#ifndef KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H
#define KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H

#include <QLatin1StringView>
#include <QString>

namespace KylinMessenger::Network::PayloadTags {

inline constexpr QLatin1StringView kImagePrefix("[[KM_IMG]]");

inline QString applyImagePrefix(const QString& base64Payload)
{
    QString result;
    result.reserve(static_cast<int>(kImagePrefix.size()) + base64Payload.size());
    result.append(QString::fromLatin1(kImagePrefix.data(), kImagePrefix.size()));
    result.append(base64Payload);
    return result;
}

inline bool hasImagePrefix(const QString& payload)
{
    return payload.startsWith(kImagePrefix);
}

inline QString stripImagePrefix(const QString& payload)
{
    return hasImagePrefix(payload)
        ? payload.mid(static_cast<int>(kImagePrefix.size()))
        : payload;
}

} // namespace KylinMessenger::Network::PayloadTags

#endif // KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H
