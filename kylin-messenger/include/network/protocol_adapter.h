#ifndef KYLIN_MESSENGER_NETWORK_PROTOCOL_ADAPTER_H
#define KYLIN_MESSENGER_NETWORK_PROTOCOL_ADAPTER_H

#include "core/models.h"
#include "network/protocol.h"

#ifdef HAVE_PROTOBUF
#    include "proto/messenger.pb.h"
#endif

namespace KylinMessenger::Network {

class ProtocolAdapter
{
public:
#ifdef HAVE_PROTOBUF
    static bool encodePresence(const Core::UserInfo& info, QByteArray& out);
    static bool decodePresence(const QByteArray& data, Core::UserInfo& out);

    static bool encodeChatMessage(const Core::ChatMessage& message, QByteArray& out);
    static bool decodeChatMessage(const QByteArray& data, Core::ChatMessage& out);
#else
    static bool encodePresence(const Core::UserInfo&, QByteArray&) { return false; }
    static bool decodePresence(const QByteArray&, Core::UserInfo&) { return false; }
    static bool encodeChatMessage(const Core::ChatMessage&, QByteArray&) { return false; }
    static bool decodeChatMessage(const QByteArray&, Core::ChatMessage&) { return false; }
#endif
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_PROTOCOL_ADAPTER_H

