#include "network/protocol_adapter.h"

#ifdef HAVE_PROTOBUF

#include <QByteArray>

namespace KylinMessenger::Network {

namespace {

inline kylin::messenger::v1::UserPresence::Status mapStatus(Core::UserStatus status)
{
    using ProtoStatus = kylin::messenger::v1::UserPresence_Status;
    switch (status) {
    case Core::UserStatus::Online:
        return ProtoStatus::UserPresence_Status_STATUS_ONLINE;
    case Core::UserStatus::Away:
        return ProtoStatus::UserPresence_Status_STATUS_AWAY;
    case Core::UserStatus::Busy:
        return ProtoStatus::UserPresence_Status_STATUS_BUSY;
    case Core::UserStatus::Invisible:
        return ProtoStatus::UserPresence_Status_STATUS_INVISIBLE;
    case Core::UserStatus::Offline:
        return ProtoStatus::UserPresence_Status_STATUS_OFFLINE;
    default:
        return ProtoStatus::UserPresence_Status_STATUS_UNKNOWN;
    }
}

inline Core::UserStatus mapStatus(kylin::messenger::v1::UserPresence::Status status)
{
    switch (status) {
    case kylin::messenger::v1::UserPresence_Status_STATUS_ONLINE:
        return Core::UserStatus::Online;
    case kylin::messenger::v1::UserPresence_Status_STATUS_AWAY:
        return Core::UserStatus::Away;
    case kylin::messenger::v1::UserPresence_Status_STATUS_BUSY:
        return Core::UserStatus::Busy;
    case kylin::messenger::v1::UserPresence_Status_STATUS_INVISIBLE:
        return Core::UserStatus::Invisible;
    case kylin::messenger::v1::UserPresence_Status_STATUS_OFFLINE:
        return Core::UserStatus::Offline;
    default:
        return Core::UserStatus::Offline;
    }
}

inline kylin::messenger::v1::ChatMessage::ContentType mapContentType(Core::MessageContentType type)
{
    using ProtoType = kylin::messenger::v1::ChatMessage_ContentType;
    switch (type) {
    case Core::MessageContentType::PlainText:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_TEXT;
    case Core::MessageContentType::Image:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_IMAGE;
    case Core::MessageContentType::File:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_FILE;
    case Core::MessageContentType::Emoji:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_EMOJI;
    case Core::MessageContentType::System:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_SYSTEM;
    default:
        return ProtoType::ChatMessage_ContentType_CONTENT_TYPE_UNKNOWN;
    }
}

inline Core::MessageContentType mapContentType(kylin::messenger::v1::ChatMessage::ContentType type)
{
    switch (type) {
    case kylin::messenger::v1::ChatMessage_ContentType_CONTENT_TYPE_TEXT:
        return Core::MessageContentType::PlainText;
    case kylin::messenger::v1::ChatMessage_ContentType_CONTENT_TYPE_IMAGE:
        return Core::MessageContentType::Image;
    case kylin::messenger::v1::ChatMessage_ContentType_CONTENT_TYPE_FILE:
        return Core::MessageContentType::File;
    case kylin::messenger::v1::ChatMessage_ContentType_CONTENT_TYPE_EMOJI:
        return Core::MessageContentType::Emoji;
    case kylin::messenger::v1::ChatMessage_ContentType_CONTENT_TYPE_SYSTEM:
        return Core::MessageContentType::System;
    default:
        return Core::MessageContentType::PlainText;
    }
}

} // namespace

bool ProtocolAdapter::encodePresence(const Core::UserInfo& info, QByteArray& out)
{
    kylin::messenger::v1::UserPresence proto;
    proto.set_user_id(info.user_id.toStdString());
    proto.set_username(info.username.toStdString());
    proto.set_hostname(info.hostname.toStdString());
    proto.set_ip_address(info.ip_address.toStdString());
    proto.set_port(info.port);
    proto.set_status(mapStatus(info.status));
    proto.set_status_text(info.status_text.toStdString());
    proto.set_avatar_hash(info.avatar_hash.toStdString());
    proto.set_group_name(info.group_name.toStdString());

    const std::string serialized = proto.SerializeAsString();
    if (serialized.empty()) {
        return false;
    }

    out = QByteArray::fromStdString(serialized);
    return true;
}

bool ProtocolAdapter::decodePresence(const QByteArray& data, Core::UserInfo& out)
{
    kylin::messenger::v1::UserPresence proto;
    if (!proto.ParseFromArray(data.constData(), data.size())) {
        return false;
    }

    out.user_id = QString::fromStdString(proto.user_id());
    out.username = QString::fromStdString(proto.username());
    out.hostname = QString::fromStdString(proto.hostname());
    out.ip_address = QString::fromStdString(proto.ip_address());
    out.port = static_cast<quint16>(proto.port());
    out.status = mapStatus(proto.status());
    out.status_text = QString::fromStdString(proto.status_text());
    out.avatar_hash = QString::fromStdString(proto.avatar_hash());
    out.group_name = QString::fromStdString(proto.group_name());

    return true;
}

bool ProtocolAdapter::encodeChatMessage(const Core::ChatMessage& message, QByteArray& out)
{
    kylin::messenger::v1::ChatMessage proto;
    proto.set_message_id(message.message_id.toStdString());
    proto.set_sender_id(message.sender_id.toStdString());
    proto.set_receiver_id(message.receiver_id.toStdString());
    proto.set_group_id(message.group_id.toStdString());
    proto.set_content_type(mapContentType(message.message_type));
    proto.set_content(message.content.toStdString());
    proto.set_timestamp_ms(message.timestamp.toMSecsSinceEpoch());
    proto.set_is_read(message.is_read);

    for (const auto& attachment : message.attachments) {
        auto* protoAttachment = proto.add_attachments();
        protoAttachment->set_filename(attachment.filename.toStdString());
        protoAttachment->set_filesize(attachment.filesize);
        protoAttachment->set_file_hash(attachment.file_hash.toStdString());
    }

    for (auto it = message.metadata.constBegin(); it != message.metadata.constEnd(); ++it) {
        (*proto.mutable_metadata())[it.key().toStdString()] = it.value().toStdString();
    }

    const std::string serialized = proto.SerializeAsString();
    if (serialized.empty()) {
        return false;
    }

    out = QByteArray::fromStdString(serialized);
    return true;
}

bool ProtocolAdapter::decodeChatMessage(const QByteArray& data, Core::ChatMessage& out)
{
    kylin::messenger::v1::ChatMessage proto;
    if (!proto.ParseFromArray(data.constData(), data.size())) {
        return false;
    }

    out.message_id = QString::fromStdString(proto.message_id());
    out.sender_id = QString::fromStdString(proto.sender_id());
    out.receiver_id = QString::fromStdString(proto.receiver_id());
    out.group_id = QString::fromStdString(proto.group_id());
    out.message_type = mapContentType(proto.content_type());
    out.content = QString::fromStdString(proto.content());
    out.timestamp = QDateTime::fromMSecsSinceEpoch(proto.timestamp_ms(), Qt::UTC);
    out.is_read = proto.is_read();

    out.attachments.clear();
    out.attachments.reserve(proto.attachments_size());
    for (const auto& attachment : proto.attachments()) {
        Core::FileAttachment file;
        file.filename = QString::fromStdString(attachment.filename());
        file.filesize = attachment.filesize();
        file.file_hash = QString::fromStdString(attachment.file_hash());
        out.attachments.append(file);
    }

    out.metadata.clear();
    for (const auto& entry : proto.metadata()) {
        out.metadata.insert(QString::fromStdString(entry.first),
                            QString::fromStdString(entry.second));
    }

    return true;
}

} // namespace KylinMessenger::Network

#endif // HAVE_PROTOBUF

