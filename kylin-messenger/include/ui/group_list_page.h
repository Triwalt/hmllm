#ifndef KYLIN_MESSENGER_GROUP_LIST_PAGE_H
#define KYLIN_MESSENGER_GROUP_LIST_PAGE_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "core/models.h"

namespace KylinMessenger {

class NetworkManager;

/**
 * @brief 群组列表页面
 * 管理群组，支持创建、加入、退出群组等功能
 */
class GroupListPage : public QWidget
{
    Q_OBJECT

public:
    explicit GroupListPage(QWidget* parent = nullptr);
    virtual ~GroupListPage() = default;

    void setNetworkManager(NetworkManager* network_manager);

public slots:
    void onGroupCreated(const Core::GroupInfo& group_info);
    void onGroupUpdated(const Core::GroupInfo& group_info);
    void onGroupDeleted(const QString& group_id);
    void onUserJoinedGroup(const QString& group_id, const QString& user_id);
    void onUserLeftGroup(const QString& group_id, const QString& user_id);
    void refreshGroupList();

signals:
    void groupDoubleClicked(const Core::GroupInfo& group_info);
    void groupContextMenuRequested(const QPoint& pos, const Core::GroupInfo& group_info);

private slots:
    void onGroupItemDoubleClicked(QListWidgetItem* item);
    void onGroupItemContextMenu(const QPoint& pos);
    void onCreateGroup();
    void onJoinGroup();
    void onLeaveGroup();
    void onSearchTextChanged(const QString& text);

private:
    void setupUI();
    void loadGroups();
    void updateGroupListItem(const Core::GroupInfo& group_info);
    void removeGroupListItem(const QString& group_id);
    QString groupDisplayName(const Core::GroupInfo& group_info) const;
    void showGroupCreateDialog();
    void showGroupJoinDialog();
    void saveGroup(const Core::GroupInfo& group_info);
    void deleteGroup(const QString& group_id);
    QList<Core::GroupInfo> loadGroupsFromSettings() const;

    NetworkManager* m_network_manager;
    QListWidget* m_group_list;
    QLineEdit* m_search_edit;
    QPushButton* m_create_button;
    QPushButton* m_join_button;
    QPushButton* m_leave_button;
    QLabel* m_group_count_label;
    QHash<QString, Core::GroupInfo> m_cached_groups;
    QString m_local_user_id;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_GROUP_LIST_PAGE_H

