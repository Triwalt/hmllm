#include "core/models.h"

#include <QDataStream>
#include <QIODevice>

#include "qt_compat.h"

namespace KylinMessenger::Core {

UserInfo::UserInfo() = default;

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

    quint8 status_byte = 0;
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

ChatMessage::ChatMessage()
    : timestamp(QDateTime::currentDateTime())
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
    stream << group_id;
    stream << static_cast<quint8>(message_type);
    stream << content;
    stream << timestamp;
    stream << is_read;

    stream << static_cast<quint32>(attachments.size());
    for (const auto& attachment : attachments) {
        stream << attachment.filename;
        stream << attachment.filepath;
        stream << attachment.filesize;
        stream << attachment.file_hash;
    }

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

    quint8 type_byte = 0;
    stream >> message_id;
    stream >> sender_id;
    stream >> receiver_id;
    stream >> group_id;
    stream >> type_byte;
    stream >> content;
    stream >> timestamp;
    stream >> is_read;

    message_type = static_cast<MessageContentType>(type_byte);

    quint32 attachment_count = 0;
    stream >> attachment_count;
    attachments.clear();
    attachments.reserve(static_cast<int>(attachment_count));
    for (quint32 i = 0; i < attachment_count; ++i) {
        FileAttachment attachment;
        stream >> attachment.filename;
        stream >> attachment.filepath;
        stream >> attachment.filesize;
        stream >> attachment.file_hash;
        attachments.append(attachment);
    }

    quint32 metadata_count = 0;
    stream >> metadata_count;
    metadata.clear();
    for (quint32 i = 0; i < metadata_count; ++i) {
        QString key;
        QString value;
        stream >> key;
        stream >> value;
        metadata.insert(key, value);
    }

    return stream.status() == QDataStream::Ok;
}

// ============================================================================
// ContactInfo
// ============================================================================

ContactInfo::ContactInfo()
    : created_at(QDateTime::currentDateTime())
{
}

ContactInfo::ContactInfo(const UserInfo& user_info)
    : contact_id(user_info.user_id)
    , display_name(user_info.username)
    , username(user_info.username)
    , hostname(user_info.hostname)
    , ip_address(user_info.ip_address)
    , group_name(user_info.group_name)
    , created_at(QDateTime::currentDateTime())
    , last_seen(QDateTime::currentDateTime())
{
}

QByteArray ContactInfo::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << contact_id;
    stream << display_name;
    stream << username;
    stream << hostname;
    stream << ip_address;
    stream << group_name;
    stream << notes;
    stream << avatar_hash;
    stream << created_at;
    stream << last_seen;
    stream << is_favorite;

    stream << static_cast<quint32>(custom_fields.size());
    for (auto it = custom_fields.constBegin(); it != custom_fields.constEnd(); ++it) {
        stream << it.key();
        stream << it.value();
    }

    return data;
}

bool ContactInfo::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream >> contact_id;
    stream >> display_name;
    stream >> username;
    stream >> hostname;
    stream >> ip_address;
    stream >> group_name;
    stream >> notes;
    stream >> avatar_hash;
    stream >> created_at;
    stream >> last_seen;
    stream >> is_favorite;

    quint32 field_count = 0;
    stream >> field_count;
    custom_fields.clear();
    for (quint32 i = 0; i < field_count; ++i) {
        QString key;
        QString value;
        stream >> key;
        stream >> value;
        custom_fields.insert(key, value);
    }

    return stream.status() == QDataStream::Ok;
}

void ContactInfo::updateFromUserInfo(const UserInfo& user_info)
{
    if (contact_id.isEmpty() || contact_id == user_info.user_id) {
        contact_id = user_info.user_id;
        username = user_info.username;
        hostname = user_info.hostname;
        ip_address = user_info.ip_address;
        group_name = user_info.group_name;
        avatar_hash = user_info.avatar_hash;
        last_seen = QDateTime::currentDateTime();
    }
}

// ============================================================================
// GroupInfo
// ============================================================================

GroupInfo::GroupInfo()
    : created_at(QDateTime::currentDateTime())
{
}

QByteArray GroupInfo::serialize() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream << group_id;
    stream << group_name;
    stream << description;
    stream << creator_id;
    stream << created_at;
    stream << is_public;
    stream << password;

    stream << static_cast<quint32>(member_ids.size());
    for (const QString& id : member_ids) {
        stream << id;
    }

    stream << static_cast<quint32>(member_roles.size());
    for (auto it = member_roles.constBegin(); it != member_roles.constEnd(); ++it) {
        stream << it.key();
        stream << it.value();
    }

    stream << static_cast<quint32>(metadata.size());
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        stream << it.key();
        stream << it.value();
    }

    return data;
}

bool GroupInfo::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setVersion(KYLIN_QDATASTREAM_VERSION);

    stream >> group_id;
    stream >> group_name;
    stream >> description;
    stream >> creator_id;
    stream >> created_at;
    stream >> is_public;
    stream >> password;

    quint32 member_count = 0;
    stream >> member_count;
    member_ids.clear();
    member_ids.reserve(static_cast<int>(member_count));
    for (quint32 i = 0; i < member_count; ++i) {
        QString id;
        stream >> id;
        member_ids.append(id);
    }

    quint32 role_count = 0;
    stream >> role_count;
    member_roles.clear();
    for (quint32 i = 0; i < role_count; ++i) {
        QString id;
        QString role;
        stream >> id;
        stream >> role;
        member_roles.insert(id, role);
    }

    quint32 metadata_count = 0;
    stream >> metadata_count;
    metadata.clear();
    for (quint32 i = 0; i < metadata_count; ++i) {
        QString key;
        QString value;
        stream >> key;
        stream >> value;
        metadata.insert(key, value);
    }

    return stream.status() == QDataStream::Ok;
}

bool GroupInfo::isMember(const QString& user_id) const
{
    return member_ids.contains(user_id);
}

void GroupInfo::addMember(const QString& user_id, const QString& role)
{
    if (!member_ids.contains(user_id)) {
        member_ids.append(user_id);
        member_roles.insert(user_id, role);
    }
}

void GroupInfo::removeMember(const QString& user_id)
{
    member_ids.removeAll(user_id);
    member_roles.remove(user_id);
}

// ============================================================================
// GroupMember
// ============================================================================

GroupMember::GroupMember()
    : joined_at(QDateTime::currentDateTime())
{
}

GroupMember::GroupMember(const QString& uid, const QString& r)
    : user_id(uid)
    , role(r)
    , joined_at(QDateTime::currentDateTime())
{
}

} // namespace KylinMessenger::Core

