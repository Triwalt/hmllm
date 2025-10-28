/**
 * @file network_protocol.h
 * @brief Network Protocol Definitions for Kylin Messenger
 * 
 * Defines the P2P protocol for serverless LAN communication including:
 * - UDP broadcast for user discovery
 * - TCP for reliable messaging
 * - Message packet structures
 * 
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>

#include <QtGlobal>
#include <QDataStream>

namespace KylinMessenger {

constexpr quint32 PROTOCOL_MAGIC = 0x4B594C4E;  ///< 'KYLN'
constexpr quint16 PROTOCOL_VERSION = 1;
constexpr quint32 MAX_PACKET_SIZE = 1024 * 1024;  ///< 1 MiB

constexpr quint16 UDP_PORT = 2425;
constexpr quint16 TCP_PORT = 2426;

constexpr int HEADER_SIZE = sizeof(quint32) + sizeof(quint16) + sizeof(quint16) +
                            sizeof(quint32) + sizeof(quint32);

enum class UserStatus : quint8 {
    Offline = 0,
    Online = 1,
    Away = 2,
    Busy = 3,
    Invisible = 4
};

enum class MessageContentType : quint8 {
    PlainText = 0,
    Image = 1,
    File = 2,
    Emoji = 3,
    System = 4
};

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

struct FileAttachment {
    QString filename;
    QString filepath;
    quint64 filesize = 0;
    QString file_hash;
};

struct UserInfo {
    QString user_id;
    QString username;
    QString hostname;
    QString ip_address;
    quint16 port = 0;
    UserStatus status = UserStatus::Offline;
    QString status_text;
    QString avatar_hash;
    QString group_name;

    UserInfo();

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
};

struct ChatMessage {
    QString message_id;
    QString sender_id;
    QString receiver_id;
    QString group_id;
    MessageContentType message_type = MessageContentType::PlainText;
    QString content;
    QDateTime timestamp;
    bool is_read = false;
    QList<FileAttachment> attachments;
    QMap<QString, QString> metadata;

    ChatMessage();

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
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

    const PacketHeader& getHeader() const { return header; }
    const QByteArray& getPayload() const { return payload; }

    void setPayload(const QByteArray& data);

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    bool isValid() const;

    static NetworkPacket createPresencePacket(const UserInfo& user_info);
    static NetworkPacket createChatMessagePacket(const ChatMessage& message);
    static NetworkPacket createGroupMessagePacket(const QString& group_id,
                                                  const ChatMessage& message);
    static NetworkPacket createFileOfferPacket(const QString& filename,
                                               quint64 filesize,
                                               const QString& file_hash);
    static NetworkPacket createTypingIndicatorPacket(const QString& user_id,
                                                     bool is_typing);
    static NetworkPacket createReadReceiptPacket(const QString& message_id);

    static quint32 calculateChecksum(const QByteArray& data);

private:
    PacketHeader header{};
    QByteArray payload;
};

} // namespace KylinMessenger

#endif // NETWORK_PROTOCOL_H
