#ifndef KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H
#define KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H

#include <QString>

namespace KylinMessenger::Network::PayloadTags {

inline constexpr const char kImagePrefix[] = "[[KM_IMG]]";
inline constexpr int kImagePrefixSize = static_cast<int>(sizeof(kImagePrefix) - 1);

inline QString applyImagePrefix(const QString& base64Payload)
{
    QString result;
    result.reserve(kImagePrefixSize + base64Payload.size());
    result.append(QString::fromLatin1(kImagePrefix, kImagePrefixSize));
    result.append(base64Payload);
    return result;
}

inline bool hasImagePrefix(const QString& payload)
{
    return payload.startsWith(QString::fromLatin1(kImagePrefix, kImagePrefixSize));
}

inline QString stripImagePrefix(const QString& payload)
{
    return hasImagePrefix(payload)
        ? payload.mid(kImagePrefixSize)
        : payload;
}

} // namespace KylinMessenger::Network::PayloadTags

#endif // KYLIN_MESSENGER_NETWORK_PAYLOAD_TAGS_H
