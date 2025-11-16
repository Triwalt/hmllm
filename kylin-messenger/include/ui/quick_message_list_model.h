#ifndef KYLIN_MESSENGER_UI_QUICK_MESSAGE_LIST_MODEL_H
#define KYLIN_MESSENGER_UI_QUICK_MESSAGE_LIST_MODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QDateTime>

namespace KylinMessenger {

struct QuickMessageEntry {
    QString senderId;
    QString senderName;
    QString content;
    QDateTime timestamp;
    bool isOutgoing = false;
};

class QuickMessageListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        SenderIdRole = Qt::UserRole + 1,
        SenderNameRole,
        ContentRole,
        TimestampRole,
        IsOutgoingRole
    };

    explicit QuickMessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    void appendMessage(const QString& senderId,
                       const QString& senderName,
                       const QString& content,
                       const QDateTime& timestamp,
                       bool isOutgoing);

private:
    QVector<QuickMessageEntry> m_entries;
    int m_capacity = 500;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_MESSAGE_LIST_MODEL_H
