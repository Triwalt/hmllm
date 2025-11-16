#include "ui/quick_message_list_model.h"

#include <algorithm>

namespace KylinMessenger {

QuickMessageListModel::QuickMessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int QuickMessageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant QuickMessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const auto& entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case ContentRole:
        return entry.content;
    case SenderIdRole:
        return entry.senderId;
    case SenderNameRole:
        return entry.senderName;
    case TimestampRole:
        return entry.timestamp.toString("HH:mm:ss");
    case IsOutgoingRole:
        return entry.isOutgoing;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QuickMessageListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SenderIdRole] = "senderId";
    roles[SenderNameRole] = "senderName";
    roles[ContentRole] = "content";
    roles[TimestampRole] = "timestamp";
    roles[IsOutgoingRole] = "outgoing";
    return roles;
}

void QuickMessageListModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

void QuickMessageListModel::appendMessage(const QString& senderId,
                                          const QString& senderName,
                                          const QString& content,
                                          const QDateTime& timestamp,
                                          bool isOutgoing)
{
    QuickMessageEntry entry;
    entry.senderId = senderId;
    entry.senderName = senderName;
    entry.content = content;
    entry.timestamp = timestamp;
    entry.isOutgoing = isOutgoing;

    const int insertRow = m_entries.size();
    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_entries.push_back(std::move(entry));
    endInsertRows();

    if (m_entries.size() > m_capacity) {
        const int removeCount = m_entries.size() - m_capacity;
        beginRemoveRows(QModelIndex(), 0, removeCount - 1);
        m_entries.erase(m_entries.begin(), m_entries.begin() + removeCount);
        endRemoveRows();
    }
}

} // namespace KylinMessenger
