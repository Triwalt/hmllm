#ifndef KYLIN_MESSENGER_UI_QUICK_CONVERSATION_LIST_MODEL_H
#define KYLIN_MESSENGER_UI_QUICK_CONVERSATION_LIST_MODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

namespace KylinMessenger {

class QuickConversationListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        ConversationIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        LastMessageRole,
        TimestampRole,
        UnreadCountRole,
        IsGroupRole
    };

    explicit QuickConversationListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void clear();

    void upsertConversation(const QString& conversationId,
                            const QString& displayName,
                            const QString& lastMessagePreview,
                            const QDateTime& timestamp,
                            bool isGroup,
                            int unreadDelta);
    void markConversationRead(const QString& conversationId);

signals:
    void countChanged();

private:
    struct Entry {
        QString conversationId;
        QString displayName;
        QString lastMessage;
        QDateTime timestamp;
        int unreadCount = 0;
        bool isGroup = false;
    };

    int indexForConversation(const QString& conversationId) const;
    void moveToTop(int index);

    QVector<Entry> m_entries;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_CONVERSATION_LIST_MODEL_H
