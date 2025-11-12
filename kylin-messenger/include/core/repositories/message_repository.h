#ifndef KYLIN_MESSENGER_CORE_REPOSITORIES_MESSAGE_REPOSITORY_H
#define KYLIN_MESSENGER_CORE_REPOSITORIES_MESSAGE_REPOSITORY_H

#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>
#include <memory>

#include "core/models.h"

namespace KylinMessenger::Core::Repositories {

class MessageRepository {
public:
    virtual ~MessageRepository() = default;

    virtual void saveMessage(const ChatMessage& message, const QString& conversationId) = 0;
    virtual QList<ChatMessage> recentMessages(const QString& conversationId, int limit = 50) const = 0;
    virtual void clearConversation(const QString& conversationId) = 0;
    virtual bool loadMessage(const QString& messageId,
                             const QString& conversationId,
                             ChatMessage& outMessage) const = 0;
};

class InMemoryMessageRepository : public MessageRepository {
public:
    void saveMessage(const ChatMessage& message, const QString& conversationId) override;
    QList<ChatMessage> recentMessages(const QString& conversationId, int limit) const override;
    void clearConversation(const QString& conversationId) override;
    bool loadMessage(const QString& messageId,
                     const QString& conversationId,
                     ChatMessage& outMessage) const override;

private:
    QString normalizeKey(const QString& conversationId) const;

    mutable QMutex mutex_;
    QHash<QString, QList<ChatMessage>> storage_;
};

} // namespace KylinMessenger::Core::Repositories

#endif // KYLIN_MESSENGER_CORE_REPOSITORIES_MESSAGE_REPOSITORY_H

