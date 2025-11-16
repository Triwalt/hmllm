#ifndef KYLIN_MESSENGER_UI_QUICK_APP_HOST_H
#define KYLIN_MESSENGER_UI_QUICK_APP_HOST_H

#include <QObject>
#include <QString>
#include <memory>

#include "ui/quick_user_list_model.h"
#include "ui/quick_message_list_model.h"
#include "ui/quick_contact_list_model.h"
#include "ui/quick_group_list_model.h"
#include "ui/quick_conversation_list_model.h"
#include "core/models.h"
#include "core/repositories/contact_repository.h"
#include "core/repositories/message_repository.h"

namespace KylinMessenger {

class NetworkManager;
class QuickAppHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString localUserId READ localUserId NOTIFY localUserChanged)
    Q_PROPERTY(QString localUserName READ localUserName NOTIFY localUserChanged)
    Q_PROPERTY(QString localStatusText READ localStatusText NOTIFY localUserChanged)
    Q_PROPERTY(QuickUserListModel* userListModel READ userListModel CONSTANT)
    Q_PROPERTY(QuickContactListModel* contactListModel READ contactListModel CONSTANT)
    Q_PROPERTY(QuickGroupListModel* groupListModel READ groupListModel CONSTANT)
    Q_PROPERTY(QuickConversationListModel* conversationListModel READ conversationListModel CONSTANT)
    Q_PROPERTY(QuickMessageListModel* messageListModel READ messageListModel CONSTANT)
    Q_PROPERTY(QString activeConversationId READ activeConversationId NOTIFY activeConversationChanged)
    Q_PROPERTY(QString activeConversationTitle READ activeConversationTitle NOTIFY activeConversationChanged)
    Q_PROPERTY(bool activeConversationIsGroup READ activeConversationIsGroup NOTIFY activeConversationChanged)
    Q_PROPERTY(bool hasNetwork READ hasNetwork NOTIFY hasNetworkChanged)

public:
    explicit QuickAppHost(QObject* parent = nullptr);

    QuickUserListModel* userListModel() const { return m_user_list_model; }
    QuickContactListModel* contactListModel() const { return m_contact_list_model; }
    QuickGroupListModel* groupListModel() const { return m_group_list_model; }
    QuickConversationListModel* conversationListModel() const { return m_conversation_list_model; }
    QuickMessageListModel* messageListModel() const { return m_message_list_model; }

    QString localUserId() const { return m_local_user_id; }
    QString localUserName() const { return m_local_user_name; }
    QString localStatusText() const { return m_local_status_text; }
    QString activeConversationId() const { return m_active_conversation_id; }
    QString activeConversationTitle() const { return m_active_conversation_title; }
    bool activeConversationIsGroup() const { return m_active_conversation_is_group; }
    bool hasNetwork() const { return m_network_manager != nullptr; }

    void setNetworkManager(NetworkManager* manager);
    NetworkManager* networkManager() const { return m_network_manager; }
    void setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository);
    void setMessageRepository(std::shared_ptr<Core::Repositories::MessageRepository> repository);

    Q_INVOKABLE void refreshOnlineUsers();
    Q_INVOKABLE bool sendTextMessage(const QString& userId, const QString& text);
    Q_INVOKABLE bool broadcastTextMessage(const QString& text);
    Q_INVOKABLE bool sendConversationTextMessage(const QString& conversationId,
                                                 bool isGroup,
                                                 const QString& text);
    Q_INVOKABLE bool sendGroupTextMessage(const QString& groupId, const QString& text);
    Q_INVOKABLE void openConversationWithUser(const QString& userId, const QString& displayName = QString());
    Q_INVOKABLE void openConversationWithGroup(const QString& groupId, const QString& displayName = QString());
    Q_INVOKABLE void markConversationRead(const QString& conversationId);

signals:
    void toastRequested(const QString& message);
    void incomingChat(const QString& senderId,
                      const QString& senderName,
                      const QString& content);
    void localUserChanged();
    void activeConversationChanged();
    void hasNetworkChanged();

private:
    void updateLocalUserSnapshot();
    void ensureLoopbackEntry();
    QString resolveDisplayName(const QString& userId) const;
    QString resolveGroupName(const QString& groupId) const;
    void openConversation(const QString& conversationId,
                          const QString& title,
                          bool isGroup);
    void loadConversationHistory(const QString& conversationId);
    void recordMessage(const Core::ChatMessage& message,
                       const QString& conversationId,
                       const QString& conversationTitle,
                       bool outgoing,
                       bool isGroup);
    void persistMessage(const Core::ChatMessage& message,
                        const QString& conversationId);

    NetworkManager* m_network_manager = nullptr;
    QuickUserListModel* m_user_list_model = nullptr;
    QuickContactListModel* m_contact_list_model = nullptr;
    QuickGroupListModel* m_group_list_model = nullptr;
    QuickConversationListModel* m_conversation_list_model = nullptr;
    QuickMessageListModel* m_message_list_model = nullptr;
    std::shared_ptr<Core::Repositories::IContactRepository> m_contact_repository;
    std::shared_ptr<Core::Repositories::MessageRepository> m_message_repository;
    QString m_active_conversation_id;
    QString m_active_conversation_title;
    bool m_active_conversation_is_group = false;
    QString m_local_user_id;
    QString m_local_user_name;
    QString m_local_status_text;
    Core::UserInfo m_loopback_user;
    bool m_loopback_registered = false;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_APP_HOST_H
