#include "ui/quick_app_host.h"

#include <QDateTime>
#include <QDebug>
#include <QSettings>
#include <QUuid>

#include "core/models.h"
#include "network_manager.h"
#include "ui/quick_user_list_model.h"

namespace KylinMessenger {

QuickAppHost::QuickAppHost(QObject* parent)
    : QObject(parent)
    , m_user_list_model(new QuickUserListModel(this))
    , m_contact_list_model(new QuickContactListModel(this))
    , m_group_list_model(new QuickGroupListModel(this))
    , m_conversation_list_model(new QuickConversationListModel(this))
    , m_message_list_model(new QuickMessageListModel(this))
{
    setContactRepository(std::make_shared<Core::Repositories::SettingsContactRepository>());
}

void QuickAppHost::setNetworkManager(NetworkManager* manager)
{
    if (m_network_manager == manager) {
        return;
    }

    if (m_network_manager) {
        disconnect(m_network_manager, nullptr, this, nullptr);
    }

    m_loopback_registered = false;
    m_loopback_user = Core::UserInfo();

    m_network_manager = manager;
    if (m_user_list_model) {
        m_user_list_model->setNetworkManager(manager);
    }

    if (m_network_manager) {
        connect(m_network_manager,
                &NetworkManager::messageReceived,
                this,
                [this](const Core::ChatMessage& message) {
                    const QString conversationId = message.sender_id;
                    const QString displayName = resolveDisplayName(conversationId);
                    recordMessage(message,
                                  conversationId,
                                  displayName,
                                  false,
                                  false);
                    emit incomingChat(conversationId, displayName, message.content);
                });

        connect(m_network_manager,
                &NetworkManager::groupMessageReceived,
                this,
                [this](const QString& groupId, const Core::ChatMessage& message) {
                    const QString groupName = resolveGroupName(groupId);
                    recordMessage(message,
                                  groupId,
                                  groupName,
                                  false,
                                  true);
                    emit incomingChat(groupId, groupName, message.content);
                });

        connect(m_network_manager,
                &NetworkManager::networkError,
                this,
                &QuickAppHost::toastRequested);

        connect(m_network_manager,
                &NetworkManager::userInfoUpdated,
                this,
                [this](const Core::UserInfo& user) {
                    if (!m_network_manager) {
                        return;
                    }
                    if (user.user_id == m_network_manager->getLocalUserId()) {
                        updateLocalUserSnapshot();
                    }
                });
    }

    emit hasNetworkChanged();
    updateLocalUserSnapshot();
}

void QuickAppHost::setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository)
{
    if (m_contact_repository == repository) {
        return;
    }

    m_contact_repository = repository;
    if (m_contact_list_model) {
        m_contact_list_model->setContactRepository(m_contact_repository);
    }
}

void QuickAppHost::setMessageRepository(std::shared_ptr<Core::Repositories::MessageRepository> repository)
{
    m_message_repository = repository;
}

void QuickAppHost::refreshOnlineUsers()
{
    if (m_user_list_model) {
        m_user_list_model->refresh();
    }
}

bool QuickAppHost::sendTextMessage(const QString& userId, const QString& text)
{
    return sendConversationTextMessage(userId, false, text);
}

bool QuickAppHost::broadcastTextMessage(const QString& text)
{
    if (!m_network_manager) {
        emit toastRequested(tr("网络尚未就绪"));
        return false;
    }

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        emit toastRequested(tr("请输入消息内容"));
        return false;
    }

    Core::ChatMessage message;
    message.message_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    message.sender_id = m_network_manager->getLocalUserId();
    message.content = trimmed;
    message.timestamp = QDateTime::currentDateTime();
    message.message_type = Core::MessageContentType::PlainText;

    if (m_network_manager->sendBroadcastMessage(message)) {
        emit toastRequested(tr("广播已发送"));
        recordMessage(message,
                      QStringLiteral("broadcast"),
                      tr("广播"),
                      true,
                      true);
        return true;
    }

    emit toastRequested(tr("广播发送失败"));
    return false;
}

bool QuickAppHost::sendConversationTextMessage(const QString& conversationId,
                                               bool isGroup,
                                               const QString& text)
{
    if (!m_network_manager) {
        emit toastRequested(tr("网络尚未就绪"));
        return false;
    }

    const QString trimmed = text.trimmed();
    if (conversationId.isEmpty() || trimmed.isEmpty()) {
        emit toastRequested(tr("请选择目标并输入内容"));
        return false;
    }

    Core::ChatMessage message;
    message.message_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    message.sender_id = m_network_manager->getLocalUserId();
    message.content = trimmed;
    message.timestamp = QDateTime::currentDateTime();
    message.message_type = Core::MessageContentType::PlainText;

    bool success = false;
    QString conversationTitle;
    if (isGroup) {
        message.group_id = conversationId;
        success = m_network_manager->sendGroupMessage(conversationId, message);
        conversationTitle = resolveGroupName(conversationId);
    } else {
        message.receiver_id = conversationId;
        success = m_network_manager->sendMessage(conversationId, message);
        conversationTitle = resolveDisplayName(conversationId);
    }

    if (success) {
        message.is_read = true;
        recordMessage(message,
                      conversationId,
                      conversationTitle,
                      true,
                      isGroup);
        emit toastRequested(tr("消息已发送"));
        return true;
    }

    emit toastRequested(tr("消息发送失败"));
    return false;
}

bool QuickAppHost::sendGroupTextMessage(const QString& groupId, const QString& text)
{
    return sendConversationTextMessage(groupId, true, text);
}

void QuickAppHost::openConversationWithUser(const QString& userId, const QString& displayName)
{
    if (userId.isEmpty()) {
        return;
    }

    QString title = displayName;
    if (title.isEmpty()) {
        title = resolveDisplayName(userId);
    }
    openConversation(userId, title, false);
}

void QuickAppHost::openConversationWithGroup(const QString& groupId, const QString& displayName)
{
    if (groupId.isEmpty()) {
        return;
    }

    QString title = displayName;
    if (title.isEmpty()) {
        title = resolveGroupName(groupId);
    }
    openConversation(groupId, title, true);
}

void QuickAppHost::markConversationRead(const QString& conversationId)
{
    if (conversationId.isEmpty()) {
        return;
    }

    if (m_conversation_list_model) {
        m_conversation_list_model->markConversationRead(conversationId);
    }
}

void QuickAppHost::openConversation(const QString& conversationId,
                                    const QString& title,
                                    bool isGroup)
{
    if (conversationId.isEmpty()) {
        return;
    }

    QString resolvedTitle = title;
    if (resolvedTitle.isEmpty()) {
        resolvedTitle = isGroup ? resolveGroupName(conversationId) : resolveDisplayName(conversationId);
    }

    const bool changed = (m_active_conversation_id != conversationId) ||
                         (m_active_conversation_is_group != isGroup) ||
                         (m_active_conversation_title != resolvedTitle);

    m_active_conversation_id = conversationId;
    m_active_conversation_is_group = isGroup;
    m_active_conversation_title = resolvedTitle;

    if (m_conversation_list_model) {
        m_conversation_list_model->markConversationRead(conversationId);
    }

    loadConversationHistory(conversationId);

    if (changed) {
        emit activeConversationChanged();
    }
}

void QuickAppHost::loadConversationHistory(const QString& conversationId)
{
    if (!m_message_list_model) {
        return;
    }

    m_message_list_model->clear();

    if (!m_message_repository || conversationId.isEmpty()) {
        return;
    }

    const auto history = m_message_repository->recentMessages(conversationId, 200);
    for (const auto& entry : history) {
        const bool outgoing = !m_local_user_id.isEmpty() && entry.sender_id == m_local_user_id;
        QString senderName = outgoing ? m_local_user_name : resolveDisplayName(entry.sender_id);
        if (senderName.isEmpty()) {
            senderName = entry.sender_id;
        }
        const QDateTime timestamp = entry.timestamp.isValid()
                                        ? entry.timestamp
                                        : QDateTime::currentDateTime();
        m_message_list_model->appendMessage(entry.sender_id,
                                            senderName,
                                            entry.content,
                                            timestamp,
                                            outgoing);
    }
}

void QuickAppHost::updateLocalUserSnapshot()
{
    QString name;
    QString status;
    QString id;

    if (m_network_manager) {
        const auto local = m_network_manager->getLocalUser();
        name = local.username;
        status = local.status_text;
        id = local.user_id;
    }

    if (name == m_local_user_name &&
        status == m_local_status_text &&
        id == m_local_user_id) {
        return;
    }

    m_local_user_id = id;
    m_local_user_name = name;
    m_local_status_text = status;
    emit localUserChanged();

    ensureLoopbackEntry();
}

void QuickAppHost::ensureLoopbackEntry()
{
    if (!m_network_manager) {
        return;
    }

    const auto local = m_network_manager->getLocalUser();
    if (local.user_id.isEmpty()) {
        return;
    }

    Core::UserInfo loopback;
    loopback.user_id = QStringLiteral("loopback");
    loopback.username = local.username.isEmpty()
        ? tr("本机测试")
        : tr("本机测试 (%1)").arg(local.username);
    loopback.hostname = local.hostname;
    loopback.ip_address = local.ip_address.isEmpty()
        ? QStringLiteral("127.0.0.1")
        : local.ip_address;
    loopback.status = Core::UserStatus::Online;
    loopback.status_text = tr("回环测试");
    loopback.group_name = local.group_name;

    const bool sameName = m_loopback_user.username == loopback.username;
    const bool sameIp = m_loopback_user.ip_address == loopback.ip_address;
    if (m_loopback_registered && sameName && sameIp) {
        return;
    }

    m_loopback_user = loopback;
    m_loopback_registered = true;
    m_network_manager->registerLoopbackPeer(m_loopback_user);
}

QString QuickAppHost::resolveDisplayName(const QString& userId) const
{
    if (m_contact_repository) {
        const auto contact = m_contact_repository->getContact(userId);
        if (!contact.contact_id.isEmpty()) {
            if (!contact.display_name.isEmpty()) {
                return contact.display_name;
            }
            if (!contact.username.isEmpty()) {
                return contact.username;
            }
        }
    }

    if (m_network_manager) {
        const auto users = m_network_manager->getOnlineUsers();
        for (const auto& user : users) {
            if (user.user_id == userId) {
                return user.username.isEmpty() ? userId : user.username;
            }
        }
    }

    return userId;
}

QString QuickAppHost::resolveGroupName(const QString& groupId) const
{
    if (groupId.isEmpty()) {
        return groupId;
    }

    QSettings settings(QStringLiteral("KylinMessenger"), QStringLiteral("KylinMessenger"));
    settings.beginGroup(QStringLiteral("groups"));
    const QByteArray data = settings.value(groupId).toByteArray();
    settings.endGroup();

    if (!data.isEmpty()) {
        Core::GroupInfo group;
        if (group.deserialize(data) && !group.group_name.isEmpty()) {
            return group.group_name;
        }
    }

    return groupId;
}

void QuickAppHost::recordMessage(const Core::ChatMessage& message,
                                 const QString& conversationId,
                                 const QString& conversationTitle,
                                 bool outgoing,
                                 bool isGroup)
{
    persistMessage(message, conversationId);

    const QString title = conversationTitle.isEmpty() ? conversationId : conversationTitle;
    const QString preview = message.content;
    const QDateTime timestamp = message.timestamp.isValid()
                                    ? message.timestamp
                                    : QDateTime::currentDateTime();
    const int unreadDelta = (!outgoing && conversationId != m_active_conversation_id) ? 1 : 0;

    if (m_conversation_list_model) {
        m_conversation_list_model->upsertConversation(conversationId,
                                                      title,
                                                      preview,
                                                      timestamp,
                                                      isGroup,
                                                      unreadDelta);
        if (conversationId == m_active_conversation_id) {
            m_conversation_list_model->markConversationRead(conversationId);
        }
    }

    if (!m_message_list_model || conversationId != m_active_conversation_id) {
        return;
    }

    QString senderName = outgoing ? m_local_user_name : resolveDisplayName(message.sender_id);
    if (senderName.isEmpty()) {
        senderName = outgoing ? m_local_user_id : message.sender_id;
    }

    m_message_list_model->appendMessage(message.sender_id,
                                        senderName,
                                        preview,
                                        timestamp,
                                        outgoing);
}

void QuickAppHost::persistMessage(const Core::ChatMessage& message,
                                  const QString& conversationId)
{
    if (!m_message_repository || conversationId.isEmpty()) {
        return;
    }

    m_message_repository->saveMessage(message, conversationId);
}

} // namespace KylinMessenger
