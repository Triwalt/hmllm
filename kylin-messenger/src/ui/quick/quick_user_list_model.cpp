#include "ui/quick_user_list_model.h"

#include <algorithm>

#include <QCoreApplication>

#include "network_manager.h"

namespace {

bool userLess(const KylinMessenger::Core::UserInfo& lhs,
              const KylinMessenger::Core::UserInfo& rhs)
{
    if (lhs.username.compare(rhs.username, Qt::CaseInsensitive) == 0) {
        return lhs.user_id < rhs.user_id;
    }
    return QString::localeAwareCompare(lhs.username, rhs.username) < 0;
}

} // namespace

namespace KylinMessenger {

QuickUserListModel::QuickUserListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int QuickUserListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_users.size();
}

QVariant QuickUserListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_users.size()) {
        return QVariant();
    }

    const auto& user = m_users.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case UsernameRole:
        return user.username;
    case UserIdRole:
        return user.user_id;
    case HostnameRole:
        return user.hostname;
    case IpRole:
        return user.ip_address;
    case StatusRole:
        return QVariant::fromValue(static_cast<int>(user.status));
    case StatusTextRole:
        return user.status_text;
    case GroupRole:
        return user.group_name;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QuickUserListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UserIdRole] = "userId";
    roles[UsernameRole] = "username";
    roles[HostnameRole] = "hostname";
    roles[IpRole] = "ip";
    roles[StatusRole] = "status";
    roles[StatusTextRole] = "statusText";
    roles[GroupRole] = "group";
    return roles;
}

void QuickUserListModel::setNetworkManager(NetworkManager* manager)
{
    if (m_network_manager == manager) {
        return;
    }

    disconnectFromNetworkManager();
    m_network_manager = manager;
    connectToNetworkManager();
    reloadUsers();
    emit networkManagerChanged();
}

QVariantMap QuickUserListModel::get(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_users.size()) {
        return map;
    }

    const auto& user = m_users.at(row);
    map.insert(QStringLiteral("userId"), user.user_id);
    map.insert(QStringLiteral("username"), user.username);
    map.insert(QStringLiteral("hostname"), user.hostname);
    map.insert(QStringLiteral("ip"), user.ip_address);
    map.insert(QStringLiteral("status"), static_cast<int>(user.status));
    map.insert(QStringLiteral("statusText"), user.status_text);
    map.insert(QStringLiteral("group"), user.group_name);
    return map;
}

void QuickUserListModel::refresh()
{
    reloadUsers();
}

int QuickUserListModel::indexForUserId(const QString& userId) const
{
    for (int i = 0; i < m_users.size(); ++i) {
        if (m_users.at(i).user_id == userId) {
            return i;
        }
    }
    return -1;
}

void QuickUserListModel::handleUserOnline(const Core::UserInfo& user)
{
    const int idx = indexForUserId(user.user_id);
    if (idx >= 0) {
        if (m_users[idx].username != user.username ||
            m_users[idx].status != user.status ||
            m_users[idx].status_text != user.status_text ||
            m_users[idx].ip_address != user.ip_address ||
            m_users[idx].group_name != user.group_name) {
            m_users[idx] = user;
            emit dataChanged(index(idx, 0), index(idx, 0));
        }
        return;
    }

    const auto insertIt = std::lower_bound(m_users.begin(), m_users.end(), user, userLess);
    const int insertRow = static_cast<int>(std::distance(m_users.begin(), insertIt));
    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_users.insert(insertIt, user);
    endInsertRows();
}

void QuickUserListModel::handleUserOffline(const QString& userId)
{
    const int idx = indexForUserId(userId);
    if (idx < 0) {
        return;
    }
    beginRemoveRows(QModelIndex(), idx, idx);
    m_users.removeAt(idx);
    endRemoveRows();
}

void QuickUserListModel::handleUserInfoUpdated(const Core::UserInfo& user)
{
    const int idx = indexForUserId(user.user_id);
    if (idx < 0) {
        handleUserOnline(user);
        return;
    }
    m_users[idx] = user;
    emit dataChanged(index(idx, 0), index(idx, 0));
}

void QuickUserListModel::connectToNetworkManager()
{
    if (!m_network_manager) {
        return;
    }

    connect(m_network_manager,
            &NetworkManager::userOnline,
            this,
            &QuickUserListModel::handleUserOnline);
    connect(m_network_manager,
            &NetworkManager::userOffline,
            this,
            &QuickUserListModel::handleUserOffline);
    connect(m_network_manager,
            &NetworkManager::userInfoUpdated,
            this,
            &QuickUserListModel::handleUserInfoUpdated);
}

void QuickUserListModel::disconnectFromNetworkManager()
{
    if (!m_network_manager) {
        return;
    }
    disconnect(m_network_manager, nullptr, this, nullptr);
}

void QuickUserListModel::reloadUsers()
{
    if (!m_network_manager) {
        beginResetModel();
        m_users.clear();
        endResetModel();
        return;
    }

    auto users = m_network_manager->getOnlineUsers();
    QVector<Core::UserInfo> newUsers = QVector<Core::UserInfo>::fromList(users);
    std::sort(newUsers.begin(), newUsers.end(), userLess);

    beginResetModel();
    m_users = std::move(newUsers);
    endResetModel();
}

} // namespace KylinMessenger
