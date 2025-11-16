#include "ui/quick_conversation_list_model.h"

#include <QVariantMap>
#include <algorithm>

namespace KylinMessenger {

QuickConversationListModel::QuickConversationListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int QuickConversationListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant QuickConversationListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const auto& entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return entry.displayName;
    case ConversationIdRole:
        return entry.conversationId;
    case LastMessageRole:
        return entry.lastMessage;
    case TimestampRole:
        return entry.timestamp.toString("yyyy-MM-dd HH:mm");
    case UnreadCountRole:
        return entry.unreadCount;
    case IsGroupRole:
        return entry.isGroup;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QuickConversationListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ConversationIdRole] = "conversationId";
    roles[DisplayNameRole] = "displayName";
    roles[LastMessageRole] = "lastMessage";
    roles[TimestampRole] = "timestamp";
    roles[UnreadCountRole] = "unread";
    roles[IsGroupRole] = "isGroup";
    return roles;
}

QVariantMap QuickConversationListModel::get(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_entries.size()) {
        return map;
    }

    const auto& entry = m_entries.at(row);
    map.insert(QStringLiteral("conversationId"), entry.conversationId);
    map.insert(QStringLiteral("displayName"), entry.displayName);
    map.insert(QStringLiteral("lastMessage"), entry.lastMessage);
    map.insert(QStringLiteral("timestamp"), entry.timestamp);
    map.insert(QStringLiteral("unread"), entry.unreadCount);
    map.insert(QStringLiteral("isGroup"), entry.isGroup);
    return map;
}

void QuickConversationListModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

void QuickConversationListModel::upsertConversation(const QString& conversationId,
                                                    const QString& displayName,
                                                    const QString& lastMessagePreview,
                                                    const QDateTime& timestamp,
                                                    bool isGroup,
                                                    int unreadDelta)
{
    int idx = indexForConversation(conversationId);
    if (idx < 0) {
        Entry entry;
        entry.conversationId = conversationId;
        entry.displayName = displayName;
        entry.lastMessage = lastMessagePreview;
        entry.timestamp = timestamp;
        entry.unreadCount = std::max(0, unreadDelta);
        entry.isGroup = isGroup;

        beginInsertRows(QModelIndex(), 0, 0);
        m_entries.prepend(entry);
        endInsertRows();
        emit countChanged();
        return;
    }

    auto& entry = m_entries[idx];
    entry.displayName = displayName;
    entry.lastMessage = lastMessagePreview;
    entry.timestamp = timestamp;
    entry.isGroup = isGroup;
    entry.unreadCount = std::max(0, entry.unreadCount + unreadDelta);

    if (idx != 0) {
        moveToTop(idx);
    } else {
        emit dataChanged(index(0, 0), index(0, 0));
    }
}

void QuickConversationListModel::markConversationRead(const QString& conversationId)
{
    const int idx = indexForConversation(conversationId);
    if (idx < 0) {
        return;
    }

    auto& entry = m_entries[idx];
    if (entry.unreadCount == 0) {
        return;
    }

    entry.unreadCount = 0;
    emit dataChanged(index(idx, 0), index(idx, 0), { UnreadCountRole });
}

int QuickConversationListModel::indexForConversation(const QString& conversationId) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).conversationId == conversationId) {
            return i;
        }
    }
    return -1;
}

void QuickConversationListModel::moveToTop(int index)
{
    if (index <= 0 || index >= m_entries.size()) {
        return;
    }

    beginMoveRows(QModelIndex(), index, index, QModelIndex(), 0);
    m_entries.move(index, 0);
    endMoveRows();
}

} // namespace KylinMessenger
