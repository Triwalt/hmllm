#include "ui/group_list_page.h"
#include "network_manager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QSettings>
#include <QUuid>
#include <QDebug>

namespace KylinMessenger {

GroupListPage::GroupListPage(QWidget* parent)
    : QWidget(parent)
    , m_network_manager(nullptr)
    , m_group_list(nullptr)
    , m_search_edit(nullptr)
    , m_create_button(nullptr)
    , m_join_button(nullptr)
    , m_leave_button(nullptr)
    , m_group_count_label(nullptr)
{
    qInfo() << "[GroupListPage] 构造函数开始";
    setupUI();
    qInfo() << "[GroupListPage] 构造函数完成";
}

void GroupListPage::setupUI()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // 工具栏
    QHBoxLayout* toolbar_layout = new QHBoxLayout();
    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText("搜索群组...");
    m_search_edit->setClearButtonEnabled(true);
    toolbar_layout->addWidget(m_search_edit, 1);

    toolbar_layout->addSpacing(10);

    m_create_button = new QPushButton("创建群组", this);
    m_join_button = new QPushButton("加入群组", this);
    m_leave_button = new QPushButton("退出群组", this);
    m_leave_button->setEnabled(false);

    toolbar_layout->addWidget(m_create_button);
    toolbar_layout->addWidget(m_join_button);
    toolbar_layout->addWidget(m_leave_button);

    main_layout->addLayout(toolbar_layout);

    // 群组列表
    m_group_list = new QListWidget(this);
    m_group_list->setContextMenuPolicy(Qt::CustomContextMenu);
    main_layout->addWidget(m_group_list, 1);

    // 群组数量标签
    m_group_count_label = new QLabel("群组: 0", this);
    m_group_count_label->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(m_group_count_label);

    // 连接信号
    connect(m_search_edit, &QLineEdit::textChanged, this, &GroupListPage::onSearchTextChanged);
    connect(m_create_button, &QPushButton::clicked, this, &GroupListPage::onCreateGroup);
    connect(m_join_button, &QPushButton::clicked, this, &GroupListPage::onJoinGroup);
    connect(m_leave_button, &QPushButton::clicked, this, &GroupListPage::onLeaveGroup);
    connect(m_group_list, &QListWidget::itemDoubleClicked, this, &GroupListPage::onGroupItemDoubleClicked);
    connect(m_group_list, &QListWidget::itemSelectionChanged, this, [this]() {
        QListWidgetItem* item = m_group_list->currentItem();
        if (item) {
            const QString group_id = item->data(Qt::UserRole).toString();
            if (m_cached_groups.contains(group_id)) {
                const Core::GroupInfo& group = m_cached_groups.value(group_id);
                m_leave_button->setEnabled(group.isMember(m_local_user_id));
            }
        } else {
            m_leave_button->setEnabled(false);
        }
    });
    connect(m_group_list, &QListWidget::customContextMenuRequested, this, &GroupListPage::onGroupItemContextMenu);
}

void GroupListPage::setNetworkManager(NetworkManager* network_manager)
{
    m_network_manager = network_manager;
    if (m_network_manager) {
        m_local_user_id = m_network_manager->getLocalUserId();
    }
    refreshGroupList();
}

void GroupListPage::onGroupCreated(const Core::GroupInfo& group_info)
{
    m_cached_groups.insert(group_info.group_id, group_info);
    saveGroup(group_info);
    updateGroupListItem(group_info);
    refreshGroupList();
}

void GroupListPage::onGroupUpdated(const Core::GroupInfo& group_info)
{
    m_cached_groups.insert(group_info.group_id, group_info);
    saveGroup(group_info);
    updateGroupListItem(group_info);
}

void GroupListPage::onGroupDeleted(const QString& group_id)
{
    m_cached_groups.remove(group_id);
    deleteGroup(group_id);
    removeGroupListItem(group_id);
    refreshGroupList();
}

void GroupListPage::onUserJoinedGroup(const QString& group_id, const QString& user_id)
{
    if (m_cached_groups.contains(group_id)) {
        Core::GroupInfo group = m_cached_groups.value(group_id);
        group.addMember(user_id);
        m_cached_groups.insert(group_id, group);
        saveGroup(group);
        updateGroupListItem(group);
    }
}

void GroupListPage::onUserLeftGroup(const QString& group_id, const QString& user_id)
{
    if (m_cached_groups.contains(group_id)) {
        Core::GroupInfo group = m_cached_groups.value(group_id);
        group.removeMember(user_id);
        m_cached_groups.insert(group_id, group);
        saveGroup(group);
        updateGroupListItem(group);
    }
}

void GroupListPage::refreshGroupList()
{
    loadGroups();
}

void GroupListPage::loadGroups()
{
    if (!m_group_list) {
        return;
    }
    
    m_group_list->clear();
    m_cached_groups.clear();

    QList<Core::GroupInfo> groups = loadGroupsFromSettings();
    for (const Core::GroupInfo& group : groups) {
        m_cached_groups.insert(group.group_id, group);
        updateGroupListItem(group);
    }

    m_group_count_label->setText(QString("群组: %1").arg(groups.size()));
}

void GroupListPage::updateGroupListItem(const Core::GroupInfo& group_info)
{
    QListWidgetItem* item = nullptr;
    for (int i = 0; i < m_group_list->count(); ++i) {
        QListWidgetItem* candidate = m_group_list->item(i);
        if (candidate->data(Qt::UserRole).toString() == group_info.group_id) {
            item = candidate;
            break;
        }
    }

    if (!item) {
        item = new QListWidgetItem(m_group_list);
        item->setData(Qt::UserRole, group_info.group_id);
    }

    const QString display_name = groupDisplayName(group_info);
    item->setText(display_name);
    item->setToolTip(QString("%1\n成员: %2\n%3")
                     .arg(display_name)
                     .arg(group_info.memberCount())
                     .arg(group_info.description));
}

void GroupListPage::removeGroupListItem(const QString& group_id)
{
    for (int i = 0; i < m_group_list->count(); ++i) {
        QListWidgetItem* item = m_group_list->item(i);
        if (item->data(Qt::UserRole).toString() == group_id) {
            delete m_group_list->takeItem(i);
            break;
        }
    }
}

QString GroupListPage::groupDisplayName(const Core::GroupInfo& group_info) const
{
    return QString("%1 (%2人)").arg(group_info.group_name).arg(group_info.memberCount());
}

void GroupListPage::onGroupItemDoubleClicked(QListWidgetItem* item)
{
    const QString group_id = item->data(Qt::UserRole).toString();
    if (m_cached_groups.contains(group_id)) {
        emit groupDoubleClicked(m_cached_groups.value(group_id));
    }
}

void GroupListPage::onGroupItemContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_group_list->itemAt(pos);
    if (!item) {
        return;
    }

    const QString group_id = item->data(Qt::UserRole).toString();
    if (m_cached_groups.contains(group_id)) {
        emit groupContextMenuRequested(m_group_list->mapToGlobal(pos), m_cached_groups.value(group_id));
    }
}

void GroupListPage::onCreateGroup()
{
    showGroupCreateDialog();
}

void GroupListPage::onJoinGroup()
{
    showGroupJoinDialog();
}

void GroupListPage::onLeaveGroup()
{
    QListWidgetItem* item = m_group_list->currentItem();
    if (!item) {
        return;
    }

    const QString group_id = item->data(Qt::UserRole).toString();
    if (!m_cached_groups.contains(group_id)) {
        return;
    }

    const Core::GroupInfo& group = m_cached_groups.value(group_id);
    if (!group.isMember(m_local_user_id)) {
        return;
    }

    int ret = QMessageBox::question(this, "确认退出", 
                                     QString("确定要退出群组 \"%1\" 吗？").arg(group.group_name),
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        onUserLeftGroup(group_id, m_local_user_id);
        refreshGroupList();
    }
}

void GroupListPage::onSearchTextChanged(const QString& text)
{
    const QString lower_text = text.toLower();
    for (int i = 0; i < m_group_list->count(); ++i) {
        QListWidgetItem* item = m_group_list->item(i);
        const QString group_id = item->data(Qt::UserRole).toString();
        if (m_cached_groups.contains(group_id)) {
            const Core::GroupInfo& group = m_cached_groups.value(group_id);
            const QString display_name = groupDisplayName(group);
            bool visible = text.isEmpty() ||
                          display_name.toLower().contains(lower_text) ||
                          group.description.toLower().contains(lower_text);
            item->setHidden(!visible);
        }
    }
}

void GroupListPage::showGroupCreateDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("创建群组");
    dialog.setMinimumWidth(400);

    QFormLayout* form_layout = new QFormLayout(&dialog);

    QLineEdit* name_edit = new QLineEdit(&dialog);
    form_layout->addRow("群组名称:", name_edit);

    QTextEdit* desc_edit = new QTextEdit(&dialog);
    desc_edit->setMaximumHeight(100);
    form_layout->addRow("群组描述:", desc_edit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form_layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        const QString name = name_edit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组名称不能为空");
            return;
        }

        Core::GroupInfo group;
        group.group_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        group.group_name = name;
        group.description = desc_edit->toPlainText().trimmed();
        group.creator_id = m_local_user_id;
        group.created_at = QDateTime::currentDateTime();
        group.is_public = true;
        group.addMember(m_local_user_id, QStringLiteral("admin"));

        onGroupCreated(group);
    }
}

void GroupListPage::showGroupJoinDialog()
{
    // 简化实现：通过输入群组ID加入
    // 实际应该通过广播发现群组或输入群组信息
    bool ok;
    QString group_id = QInputDialog::getText(this, "加入群组", "请输入群组ID:", 
                                              QLineEdit::Normal, QString(), &ok);
    if (ok && !group_id.isEmpty()) {
        // 这里应该通过网络协议加入群组
        // 暂时只是添加到本地列表
        QMessageBox::information(this, "提示", "加入群组功能需要网络协议支持，将在后续实现");
    }
}

void GroupListPage::saveGroup(const Core::GroupInfo& group_info)
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    const QString key = QStringLiteral("groups/%1").arg(group_info.group_id);
    const QByteArray data = group_info.serialize();
    settings.setValue(key, data);
    settings.sync();
}

void GroupListPage::deleteGroup(const QString& group_id)
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    const QString key = QStringLiteral("groups/%1").arg(group_id);
    settings.remove(key);
    settings.sync();
}

QList<Core::GroupInfo> GroupListPage::loadGroupsFromSettings() const
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    settings.beginGroup(QStringLiteral("groups"));

    QList<Core::GroupInfo> groups;
    const QStringList keys = settings.allKeys();
    groups.reserve(keys.size());

    for (const QString& key : keys) {
        const QByteArray data = settings.value(key).toByteArray();
        Core::GroupInfo group;
        if (group.deserialize(data)) {
            groups.append(group);
        }
    }

    settings.endGroup();
    return groups;
}

} // namespace KylinMessenger

