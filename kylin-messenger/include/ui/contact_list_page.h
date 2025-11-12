#ifndef KYLIN_MESSENGER_CONTACT_LIST_PAGE_H
#define KYLIN_MESSENGER_CONTACT_LIST_PAGE_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <memory>
#include "core/models.h"
#include "core/repositories/contact_repository.h"

namespace KylinMessenger {

class NetworkManager;

/**
 * @brief 联系人列表页面
 * 管理联系人信息，支持添加、编辑、删除、搜索等功能
 */
class ContactListPage : public QWidget
{
    Q_OBJECT

public:
    explicit ContactListPage(QWidget* parent = nullptr);
    virtual ~ContactListPage() = default;

    void setNetworkManager(NetworkManager* network_manager);
    void setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository);

public slots:
    void onUserOnline(const Core::UserInfo& user_info);
    void onUserInfoUpdated(const Core::UserInfo& user_info);
    void refreshContactList();

signals:
    void contactDoubleClicked(const Core::ContactInfo& contact);
    void contactContextMenuRequested(const QPoint& pos, const Core::ContactInfo& contact);

private slots:
    void onContactItemDoubleClicked(QListWidgetItem* item);
    void onContactItemContextMenu(const QPoint& pos);
    void onAddContact();
    void onEditContact();
    void onDeleteContact();
    void onSearchTextChanged(const QString& text);
    void onGroupFilterChanged(int index);

private:
    void setupUI();
    void loadContacts();
    void updateContactListItem(const Core::ContactInfo& contact);
    void removeContactListItem(const QString& contact_id);
    QString contactDisplayName(const Core::ContactInfo& contact) const;
    void showContactEditDialog(const Core::ContactInfo& contact = Core::ContactInfo(), bool is_edit = false);
    QList<QString> getContactGroups() const;

    NetworkManager* m_network_manager;
    std::shared_ptr<Core::Repositories::IContactRepository> m_contact_repository;
    QListWidget* m_contact_list;
    QLineEdit* m_search_edit;
    QComboBox* m_group_filter;
    QPushButton* m_add_button;
    QPushButton* m_edit_button;
    QPushButton* m_delete_button;
    QLabel* m_contact_count_label;
    QHash<QString, Core::ContactInfo> m_cached_contacts;
    QString m_current_filter_group;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_CONTACT_LIST_PAGE_H

