#ifndef KYLIN_MESSENGER_NETWORK_PROTOCOL_H
#define KYLIN_MESSENGER_NETWORK_PROTOCOL_H

#include <QByteArray>
#include <QString>

#include "core/models.h"

namespace KylinMessenger::Network {

constexpr quint32 PROTOCOL_MAGIC = 0x4B594C4E;  // 'KYLN'
constexpr quint16 PROTOCOL_VERSION = 1;
constexpr quint32 MAX_PACKET_SIZE = 1024 * 1024;  // 1 MiB

constexpr int HEADER_SIZE = sizeof(quint32) + sizeof(quint16) + sizeof(quint16) +
                            sizeof(quint32) + sizeof(quint32);

enum class MessageType : quint16 {
    UserPresence = 0x0001,
    ChatMessage = 0x0010,
    GroupMessage = 0x0011,
    BroadcastMessage = 0x0012,
    FileOffer = 0x0020,
    FileTransferData = 0x0021,
    TypingIndicator = 0x0050,
    ReadReceipt = 0x0051,
    Ping = 0x00FE,
    Pong = 0x00FF
};

struct PacketHeader {
    quint32 magic_number;
    quint16 version;
    MessageType message_type;
    quint32 payload_size;
    quint32 checksum;

    PacketHeader();

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    bool isValid() const;
};

class NetworkPacket {
public:
    NetworkPacket();
    explicit NetworkPacket(MessageType type);

    const PacketHeader& header() const { return header_; }
    const QByteArray& payload() const { return payload_; }

    void setPayload(const QByteArray& data);

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    bool isValid() const;

    static NetworkPacket createPresencePacket(const Core::UserInfo& user_info);
    static NetworkPacket createChatMessagePacket(const Core::ChatMessage& message);
    static NetworkPacket createGroupMessagePacket(const QString& group_id,
                                                  const Core::ChatMessage& message);
    static NetworkPacket createFileOfferPacket(const QString& filename,
                                               quint64 filesize,
                                               const QString& file_hash);
    static NetworkPacket createTypingIndicatorPacket(const QString& user_id,
                                                     bool is_typing);
    static NetworkPacket createReadReceiptPacket(const QString& message_id);

    static quint32 calculateChecksum(const QByteArray& data);

private:
    PacketHeader header_{};
    QByteArray payload_;
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_PROTOCOL_H

