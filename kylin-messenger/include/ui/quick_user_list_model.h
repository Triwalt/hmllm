#ifndef KYLIN_MESSENGER_UI_QUICK_USER_LIST_MODEL_H
#define KYLIN_MESSENGER_UI_QUICK_USER_LIST_MODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QVariantMap>

#include "core/models.h"

namespace KylinMessenger {

class NetworkManager;

class QuickUserListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        UserIdRole = Qt::UserRole + 1,
        UsernameRole,
        HostnameRole,
        IpRole,
        StatusRole,
        StatusTextRole,
        GroupRole
    };

    explicit QuickUserListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setNetworkManager(NetworkManager* manager);
    NetworkManager* networkManager() const { return m_network_manager; }

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void refresh();

signals:
    void networkManagerChanged();

private:
    int indexForUserId(const QString& userId) const;
    void handleUserOnline(const Core::UserInfo& user);
    void handleUserOffline(const QString& userId);
    void handleUserInfoUpdated(const Core::UserInfo& user);
    void connectToNetworkManager();
    void disconnectFromNetworkManager();
    void reloadUsers();

    NetworkManager* m_network_manager = nullptr;
    QVector<Core::UserInfo> m_users;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_USER_LIST_MODEL_H
