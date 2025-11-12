#include "network/protocol.h"
#include "network/protocol_adapter.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QIODevice>

#include "core/models.h"
#include "qt_compat.h"

#include <zlib.h>

namespace KylinMessenger::Network {

using Core::ChatMessage;
using Core::UserInfo;

PacketHeader::PacketHeader()
    : magic_number(PROTOCOL_MAGIC)
    , version(PROTOCOL_VERSION)
    , message_type(MessageType::UserPresence)
    , payload_size(0)
    , checksum(0)
{
}

QByteArray PacketHeader::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << magic_number;
    stream << version;
    stream << static_cast<quint16>(message_type);
    stream << payload_size;
    stream << checksum;

    return data;
}

bool PacketHeader::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    quint16 type_value = 0;
    stream >> magic_number;
    stream >> version;
    stream >> type_value;
    stream >> payload_size;
    stream >> checksum;

    message_type = static_cast<MessageType>(type_value);
    return stream.status() == QDataStream::Ok;
}

bool PacketHeader::isValid() const
{
    return magic_number == PROTOCOL_MAGIC && version == PROTOCOL_VERSION &&
           payload_size <= MAX_PACKET_SIZE;
}

NetworkPacket::NetworkPacket() = default;

NetworkPacket::NetworkPacket(MessageType type)
    : header_{}
{
    header_.message_type = type;
}

void NetworkPacket::setPayload(const QByteArray& data)
{
    payload_ = data;
    header_.payload_size = data.size();
    header_.checksum = calculateChecksum(data);
}

QByteArray NetworkPacket::serialize() const
{
    QByteArray header_data = header_.serialize();
    return header_data + payload_;
}

bool NetworkPacket::deserialize(const QByteArray& data)
{
    if (data.size() < static_cast<int>(HEADER_SIZE)) {
        qWarning() << "数据包太小，无法包含完整头部";
        return false;
    }

    QByteArray header_data = data.left(HEADER_SIZE);
    if (!header_.deserialize(header_data)) {
        qWarning() << "头部反序列化失败";
        return false;
    }

    if (!header_.isValid()) {
        qWarning() << "无效的数据包头部";
        return false;
    }

    if (data.size() < static_cast<int>(HEADER_SIZE + header_.payload_size)) {
        qWarning() << "数据包不完整";
        return false;
    }

    payload_ = data.mid(HEADER_SIZE, header_.payload_size);

    const quint32 calculated_checksum = calculateChecksum(payload_);
    if (calculated_checksum != header_.checksum) {
        qWarning() << "校验和不匹配:" << "期望" << header_.checksum << "实际" << calculated_checksum;
        return false;
    }

    return true;
}

bool NetworkPacket::isValid() const
{
    return header_.isValid() &&
           header_.payload_size == static_cast<quint32>(payload_.size()) &&
           header_.checksum == calculateChecksum(payload_);
}

NetworkPacket NetworkPacket::createPresencePacket(const UserInfo& user_info)
{
    NetworkPacket packet(MessageType::UserPresence);
    QByteArray payload;
    bool encoded = false;
#ifdef HAVE_PROTOBUF
    encoded = ProtocolAdapter::encodePresence(user_info, payload);
#endif
    if (!encoded) {
        payload = user_info.serialize();
    }
    packet.setPayload(payload);
    return packet;
}

NetworkPacket NetworkPacket::createChatMessagePacket(const ChatMessage& message)
{
    NetworkPacket packet(MessageType::ChatMessage);
    QByteArray payload;
    bool encoded = false;
#ifdef HAVE_PROTOBUF
    encoded = ProtocolAdapter::encodeChatMessage(message, payload);
#endif
    if (!encoded) {
        payload = message.serialize();
    }
    packet.setPayload(payload);
    return packet;
}

NetworkPacket NetworkPacket::createGroupMessagePacket(const QString& group_id,
                                                      const ChatMessage& message)
{
    NetworkPacket packet(MessageType::GroupMessage);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << group_id;
    QByteArray message_data;
    bool encoded = false;
#ifdef HAVE_PROTOBUF
    encoded = ProtocolAdapter::encodeChatMessage(message, message_data);
#endif
    if (!encoded) {
        message_data = message.serialize();
    }
    stream << message_data;

    packet.setPayload(data);
    return packet;
}

NetworkPacket NetworkPacket::createFileOfferPacket(const QString& filename,
                                                   quint64 filesize,
                                                   const QString& file_hash)
{
    NetworkPacket packet(MessageType::FileOffer);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << filename;
    stream << filesize;
    stream << file_hash;

    packet.setPayload(data);
    return packet;
}

NetworkPacket NetworkPacket::createTypingIndicatorPacket(const QString& user_id,
                                                         bool is_typing)
{
    NetworkPacket packet(MessageType::TypingIndicator);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << user_id;
    stream << is_typing;

    packet.setPayload(data);
    return packet;
}

NetworkPacket NetworkPacket::createReadReceiptPacket(const QString& message_id)
{
    NetworkPacket packet(MessageType::ReadReceipt);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << message_id;

    packet.setPayload(data);
    return packet;
}

quint32 NetworkPacket::calculateChecksum(const QByteArray& data)
{
    return crc32(0L, reinterpret_cast<const Bytef*>(data.constData()), data.size());
}

} // namespace KylinMessenger::Network

