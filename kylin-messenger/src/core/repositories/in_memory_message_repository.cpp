#include "core/repositories/message_repository.h"

#include <QMutexLocker>

namespace KylinMessenger::Core::Repositories {

void InMemoryMessageRepository::saveMessage(const ChatMessage& message, const QString& conversationId)
{
    QMutexLocker locker(&mutex_);
    QList<ChatMessage>& messages = storage_[normalizeKey(conversationId)];
    for (ChatMessage& existing : messages) {
        if (existing.message_id == message.message_id && !existing.message_id.isEmpty()) {
            existing = message;
            return;
        }
    }
    messages.append(message);
}

QList<ChatMessage> InMemoryMessageRepository::recentMessages(const QString& conversationId, int limit) const
{
    QMutexLocker locker(&mutex_);
    const auto it = storage_.constFind(normalizeKey(conversationId));
    if (it == storage_.constEnd()) {
        return {};
    }

    const QList<ChatMessage>& messages = it.value();
    if (limit <= 0 || messages.size() <= limit) {
        return messages;
    }

    return messages.mid(messages.size() - limit, limit);
}

void InMemoryMessageRepository::clearConversation(const QString& conversationId)
{
    QMutexLocker locker(&mutex_);
    storage_.remove(normalizeKey(conversationId));
}

bool InMemoryMessageRepository::loadMessage(const QString& messageId,
                                            const QString& conversationId,
                                            ChatMessage& outMessage) const
{
    QMutexLocker locker(&mutex_);
    const QString key = normalizeKey(conversationId);
    if (!storage_.contains(key)) {
        return false;
    }

    const QList<ChatMessage>& list = storage_.value(key);
    for (const ChatMessage& msg : list) {
        if (msg.message_id == messageId) {
            outMessage = msg;
            return true;
        }
    }
    return false;
}

QString InMemoryMessageRepository::normalizeKey(const QString& conversationId) const
{
    return conversationId.trimmed().toLower();
}

} // namespace KylinMessenger::Core::Repositories

