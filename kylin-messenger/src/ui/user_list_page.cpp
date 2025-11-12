#include "ui/user_list_page.h"
#include "network_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QMenu>
#include <QIcon>
#include <QColor>
#include <QDebug>

namespace KylinMessenger {

UserListPage::UserListPage(QWidget* parent)
    : QWidget(parent)
    , m_network_manager(nullptr)
    , m_user_list(nullptr)
    , m_search_edit(nullptr)
    , m_user_count_label(nullptr)
{
    qInfo() << "[UserListPage] 构造函数开始";
    setupUI();
    qInfo() << "[UserListPage] 构造函数完成";
}

void UserListPage::setupUI()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // 搜索框
    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText("搜索用户...");
    m_search_edit->setClearButtonEnabled(true);
    main_layout->addWidget(m_search_edit);

    // 用户列表
    m_user_list = new QListWidget(this);
    m_user_list->setContextMenuPolicy(Qt::CustomContextMenu);
    main_layout->addWidget(m_user_list, 1);

    // 用户数量标签
    m_user_count_label = new QLabel("在线用户: 0", this);
    m_user_count_label->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(m_user_count_label);

    // 连接信号
    connect(m_search_edit, &QLineEdit::textChanged, this, &UserListPage::onSearchTextChanged);
    connect(m_user_list, &QListWidget::itemDoubleClicked, this, &UserListPage::onUserItemDoubleClicked);
    connect(m_user_list, &QListWidget::customContextMenuRequested, this, &UserListPage::onUserItemContextMenu);
}

void UserListPage::setNetworkManager(NetworkManager* network_manager)
{
    m_network_manager = network_manager;
    updateUserList();
}

void UserListPage::onUserOnline(const Core::UserInfo& user_info)
{
    m_cached_users.insert(user_info.user_id, user_info);
    updateUserListItem(user_info);
    updateUserList();
}

void UserListPage::onUserOffline(const QString& user_id)
{
    m_cached_users.remove(user_id);
    removeUserListItem(user_id);
    updateUserList();
}

void UserListPage::onUserInfoUpdated(const Core::UserInfo& user_info)
{
    m_cached_users.insert(user_info.user_id, user_info);
    updateUserListItem(user_info);
}

void UserListPage::onSearchTextChanged(const QString& text)
{
    const QString lower_text = text.toLower();
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* item = m_user_list->item(i);
        const QString user_id = item->data(Qt::UserRole).toString();
        if (m_cached_users.contains(user_id)) {
            const Core::UserInfo& info = m_cached_users.value(user_id);
            const QString display_name = userDisplayName(info);
            bool visible = text.isEmpty() ||
                          display_name.toLower().contains(lower_text) ||
                          info.hostname.toLower().contains(lower_text) ||
                          info.ip_address.contains(text);
            item->setHidden(!visible);
        }
    }
}

void UserListPage::updateUserList()
{
    if (!m_network_manager || !m_user_list) {
        return;
    }

    const QList<Core::UserInfo> users = m_network_manager->getOnlineUsers();
    m_user_count_label->setText(QString("在线用户: %1").arg(users.size()));

    // 更新缓存
    QSet<QString> current_user_ids;
    for (const Core::UserInfo& user : users) {
        current_user_ids.insert(user.user_id);
        m_cached_users.insert(user.user_id, user);
    }

    // 移除不在线的用户
    QHash<QString, Core::UserInfo>::iterator it = m_cached_users.begin();
    while (it != m_cached_users.end()) {
        if (!current_user_ids.contains(it.key())) {
            it = m_cached_users.erase(it);
        } else {
            ++it;
        }
    }

    // 更新列表项
    m_user_list->clear();
    for (const Core::UserInfo& user : users) {
        updateUserListItem(user);
    }

    // 重新应用搜索过滤
    onSearchTextChanged(m_search_edit->text());
}

void UserListPage::updateUserListItem(const Core::UserInfo& user_info)
{
    QListWidgetItem* item = nullptr;
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* candidate = m_user_list->item(i);
        if (candidate->data(Qt::UserRole).toString() == user_info.user_id) {
            item = candidate;
            break;
        }
    }

    if (!item) {
        item = new QListWidgetItem(m_user_list);
        item->setData(Qt::UserRole, user_info.user_id);
    }

    const QString display_name = userDisplayName(user_info);
    item->setText(display_name);
    item->setIcon(statusIcon(user_info.status));
    item->setToolTip(QString("%1\n%2\n%3")
                     .arg(display_name, user_info.hostname, user_info.ip_address));
}

void UserListPage::removeUserListItem(const QString& user_id)
{
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* item = m_user_list->item(i);
        if (item->data(Qt::UserRole).toString() == user_id) {
            delete m_user_list->takeItem(i);
            break;
        }
    }
}

QString UserListPage::userDisplayName(const Core::UserInfo& user_info) const
{
    if (!user_info.group_name.isEmpty()) {
        return QString("%1 [%2]").arg(user_info.username, user_info.group_name);
    }
    return user_info.username;
}

QIcon UserListPage::statusIcon(Core::UserStatus status) const
{
    // 简单的状态图标（可以使用实际图标资源）
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QColor color;
    switch (status) {
        case Core::UserStatus::Online:
            color = Qt::green;
            break;
        case Core::UserStatus::Away:
            color = Qt::yellow;
            break;
        case Core::UserStatus::Busy:
            color = Qt::red;
            break;
        case Core::UserStatus::Invisible:
            color = Qt::gray;
            break;
        default:
            color = Qt::lightGray;
            break;
    }
    pixmap.fill(color);
    return QIcon(pixmap);
}

void UserListPage::onUserItemDoubleClicked(QListWidgetItem* item)
{
    const QString user_id = item->data(Qt::UserRole).toString();
    if (m_cached_users.contains(user_id)) {
        emit userDoubleClicked(m_cached_users.value(user_id));
    }
}

void UserListPage::onUserItemContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_user_list->itemAt(pos);
    if (!item) {
        return;
    }

    const QString user_id = item->data(Qt::UserRole).toString();
    if (m_cached_users.contains(user_id)) {
        emit userContextMenuRequested(m_user_list->mapToGlobal(pos), m_cached_users.value(user_id));
    }
}

} // namespace KylinMessenger

