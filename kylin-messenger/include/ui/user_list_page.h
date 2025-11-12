#ifndef KYLIN_MESSENGER_USER_LIST_PAGE_H
#define KYLIN_MESSENGER_USER_LIST_PAGE_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include "core/models.h"

namespace KylinMessenger {

class NetworkManager;

/**
 * @brief 用户列表页面
 * 显示在线用户列表，支持搜索和过滤
 */
class UserListPage : public QWidget
{
    Q_OBJECT

public:
    explicit UserListPage(QWidget* parent = nullptr);
    virtual ~UserListPage() = default;

    void setNetworkManager(NetworkManager* network_manager);

public slots:
    void onUserOnline(const Core::UserInfo& user_info);
    void onUserOffline(const QString& user_id);
    void onUserInfoUpdated(const Core::UserInfo& user_info);
    void onSearchTextChanged(const QString& text);
    void updateUserList();

signals:
    void userDoubleClicked(const Core::UserInfo& user_info);
    void userContextMenuRequested(const QPoint& pos, const Core::UserInfo& user_info);

private slots:
    void onUserItemDoubleClicked(QListWidgetItem* item);
    void onUserItemContextMenu(const QPoint& pos);

private:
    void setupUI();
    void updateUserListItem(const Core::UserInfo& user_info);
    void removeUserListItem(const QString& user_id);
    QString userDisplayName(const Core::UserInfo& user_info) const;
    QIcon statusIcon(Core::UserStatus status) const;

    NetworkManager* m_network_manager;
    QListWidget* m_user_list;
    QLineEdit* m_search_edit;
    QLabel* m_user_count_label;
    QHash<QString, Core::UserInfo> m_cached_users;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_USER_LIST_PAGE_H

