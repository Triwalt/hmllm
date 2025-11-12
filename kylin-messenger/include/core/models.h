#ifndef KYLIN_MESSENGER_CORE_MODELS_H
#define KYLIN_MESSENGER_CORE_MODELS_H

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>

namespace KylinMessenger::Core {

constexpr quint16 DEFAULT_TCP_PORT = 2426;
constexpr quint16 DEFAULT_UDP_PORT = 2425;

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
    quint16 port = DEFAULT_TCP_PORT;
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
    QString group_id;  // 群组ID，空字符串表示私聊
    MessageContentType message_type = MessageContentType::PlainText;
    QString content;
    QDateTime timestamp;
    bool is_read = false;
    QList<FileAttachment> attachments;
    QMap<QString, QString> metadata;

    ChatMessage();

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    
    // 判断是否为群组消息
    bool isGroupMessage() const { return !group_id.isEmpty(); }
};

// 联系人信息
struct ContactInfo {
    QString contact_id;  // 唯一标识（通常是user_id）
    QString display_name;  // 显示名称（可自定义）
    QString username;  // 用户名
    QString hostname;  // 主机名
    QString ip_address;  // IP地址
    QString group_name;  // 所属分组
    QString notes;  // 备注
    QString avatar_hash;  // 头像哈希
    QDateTime created_at;  // 创建时间
    QDateTime last_seen;  // 最后在线时间
    bool is_favorite = false;  // 是否收藏
    QMap<QString, QString> custom_fields;  // 自定义字段（如电话、邮箱等）

    ContactInfo();
    ContactInfo(const UserInfo& user_info);

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    
    // 从UserInfo更新信息
    void updateFromUserInfo(const UserInfo& user_info);
};

// 群组信息
struct GroupInfo {
    QString group_id;  // 群组唯一标识
    QString group_name;  // 群组名称
    QString description;  // 群组描述
    QString creator_id;  // 创建者ID
    QDateTime created_at;  // 创建时间
    QList<QString> member_ids;  // 成员ID列表
    QMap<QString, QString> member_roles;  // 成员角色（admin, member等）
    bool is_public = true;  // 是否公开群组
    QString password;  // 群组密码（如果有）
    QMap<QString, QString> metadata;  // 扩展元数据

    GroupInfo();

    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    
    // 判断用户是否为成员
    bool isMember(const QString& user_id) const;
    
    // 添加成员
    void addMember(const QString& user_id, const QString& role = QStringLiteral("member"));
    
    // 移除成员
    void removeMember(const QString& user_id);
    
    // 获取成员数量
    int memberCount() const { return member_ids.size(); }
};

// 群组成员信息
struct GroupMember {
    QString user_id;
    QString role;  // admin, member等
    QDateTime joined_at;  // 加入时间
    QString nickname;  // 群内昵称
    
    GroupMember();
    GroupMember(const QString& uid, const QString& r = QStringLiteral("member"));
};

} // namespace KylinMessenger::Core

#endif // KYLIN_MESSENGER_CORE_MODELS_H
