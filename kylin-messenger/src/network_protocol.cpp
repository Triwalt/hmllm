// network_protocol.cpp - P2P网络协议实现
#include "network_protocol.h"
#include <QDataStream>
#include "qt_compat.h"
#include <QBuffer>
#include <QDebug>
#include <zlib.h>

namespace KylinMessenger {

// ============================================================================
// UserInfo 实现
// ============================================================================

UserInfo::UserInfo()
    : port(0)
    , status(UserStatus::Online)
{
}

QByteArray UserInfo::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);
    
    stream << user_id;
    stream << username;
    stream << hostname;
    stream << ip_address;
    stream << port;
    stream << static_cast<quint8>(status);
    stream << status_text;
    stream << avatar_hash;
    stream << group_name;
    
    return data;
}

bool UserInfo::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);
    
    quint8 status_byte;
    stream >> user_id;
    stream >> username;
    stream >> hostname;
    stream >> ip_address;
    stream >> port;
    stream >> status_byte;
    stream >> status_text;
    stream >> avatar_hash;
    stream >> group_name;
    
    status = static_cast<UserStatus>(status_byte);
    
    return stream.status() == QDataStream::Ok;
}

// ============================================================================
// ChatMessage 实现
// ============================================================================

ChatMessage::ChatMessage()
    : message_type(MessageContentType::PlainText)
    , timestamp(QDateTime::currentDateTime())
    , is_read(false)
{
}

QByteArray ChatMessage::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);
    
    stream << message_id;
    stream << sender_id;
    stream << receiver_id;
    stream << static_cast<quint8>(message_type);
    stream << content;
    stream << timestamp;
    stream << is_read;
    
    // 序列化附件
    stream << static_cast<quint32>(attachments.size());
    for (const auto& attachment : attachments) {
        stream << attachment.filename;
        stream << attachment.filepath;
        stream << attachment.filesize;
        stream << attachment.file_hash;
    }
    
    // 序列化元数据
    stream << static_cast<quint32>(metadata.size());
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        stream << it.key();
        stream << it.value();
    }
    
    return data;
}

bool ChatMessage::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);
    
    quint8 type_byte;
    stream >> message_id;
    stream >> sender_id;
    stream >> receiver_id;
    stream >> type_byte;
    stream >> content;
    stream >> timestamp;
    stream >> is_read;
    
    message_type = static_cast<MessageContentType>(type_byte);
    
    // 反序列化附件
    quint32 attachment_count;
    stream >> attachment_count;
    attachments.clear();
    for (quint32 i = 0; i < attachment_count; ++i) {
        FileAttachment attachment;
        stream >> attachment.filename;
        stream >> attachment.filepath;
        stream >> attachment.filesize;
        stream >> attachment.file_hash;
        attachments.append(attachment);
    }
    
    // 反序列化元数据
    quint32 metadata_count;
    stream >> metadata_count;
    metadata.clear();
    for (quint32 i = 0; i < metadata_count; ++i) {
        QString key, value;
        stream >> key;
        stream >> value;
        metadata[key] = value;
    }
    
    return stream.status() == QDataStream::Ok;
}

// ============================================================================
// PacketHeader 实现
// ============================================================================

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
    
    quint16 type_value;
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
    return magic_number == PROTOCOL_MAGIC && 
           version == PROTOCOL_VERSION &&
           payload_size <= MAX_PACKET_SIZE;
}

// ============================================================================
// NetworkPacket 实现
// ============================================================================

NetworkPacket::NetworkPacket()
{
}

NetworkPacket::NetworkPacket(MessageType type)
{
    header.message_type = type;
}

void NetworkPacket::setPayload(const QByteArray& data)
{
    payload = data;
    header.payload_size = data.size();
    header.checksum = calculateChecksum(data);
}

QByteArray NetworkPacket::serialize() const
{
    QByteArray header_data = header.serialize();
    return header_data + payload;
}

bool NetworkPacket::deserialize(const QByteArray& data)
{
    if (data.size() < static_cast<int>(HEADER_SIZE)) {
        qWarning() << "数据包太小，无法包含完整头部";
        return false;
    }
    
    // 解析头部
    QByteArray header_data = data.left(HEADER_SIZE);
    if (!header.deserialize(header_data)) {
        qWarning() << "头部反序列化失败";
        return false;
    }
    
    // 验证头部
    if (!header.isValid()) {
        qWarning() << "无效的数据包头部";
        return false;
    }
    
    // 提取负载
    if (data.size() < static_cast<int>(HEADER_SIZE + header.payload_size)) {
        qWarning() << "数据包不完整";
        return false;
    }
    
    payload = data.mid(HEADER_SIZE, header.payload_size);
    
    // 验证校验和
    quint32 calculated_checksum = calculateChecksum(payload);
    if (calculated_checksum != header.checksum) {
        qWarning() << "校验和不匹配:" 
                   << "期望" << header.checksum 
                   << "实际" << calculated_checksum;
        return false;
    }
    
    return true;
}

bool NetworkPacket::isValid() const
{
    return header.isValid() && 
           header.payload_size == static_cast<quint32>(payload.size()) &&
           header.checksum == calculateChecksum(payload);
}

quint32 NetworkPacket::calculateChecksum(const QByteArray& data)
{
    return crc32(0L, 
                 reinterpret_cast<const Bytef*>(data.constData()), 
                 data.size());
}

// ============================================================================
// 便捷的数据包创建函数
// ============================================================================

NetworkPacket NetworkPacket::createPresencePacket(const UserInfo& user_info)
{
    NetworkPacket packet(MessageType::UserPresence);
    packet.setPayload(user_info.serialize());
    return packet;
}

NetworkPacket NetworkPacket::createChatMessagePacket(const ChatMessage& message)
{
    NetworkPacket packet(MessageType::ChatMessage);
    packet.setPayload(message.serialize());
    return packet;
}

NetworkPacket NetworkPacket::createFileOfferPacket(
    const QString& filename,
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

NetworkPacket NetworkPacket::createTypingIndicatorPacket(
    const QString& user_id,
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

NetworkPacket NetworkPacket::createGroupMessagePacket(
    const QString& group_id,
    const ChatMessage& message)
{
    NetworkPacket packet(MessageType::GroupMessage);
    
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);
    
    stream << group_id;
    QByteArray message_data = message.serialize();
    stream << message_data;
    
    packet.setPayload(data);
    return packet;
}

} // namespace KylinMessenger
