#include "ui/contact_list_page.h"
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
#include <QComboBox>
#include <QDebug>

namespace KylinMessenger {

ContactListPage::ContactListPage(QWidget* parent)
    : QWidget(parent)
    , m_network_manager(nullptr)
    , m_contact_list(nullptr)
    , m_search_edit(nullptr)
    , m_group_filter(nullptr)
    , m_add_button(nullptr)
    , m_edit_button(nullptr)
    , m_delete_button(nullptr)
    , m_contact_count_label(nullptr)
    , m_current_filter_group(QStringLiteral("全部"))
{
    qInfo() << "[ContactListPage] 构造函数开始";
    setupUI();
    qInfo() << "[ContactListPage] 构造函数完成";
}

void ContactListPage::setupUI()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // 工具栏
    QHBoxLayout* toolbar_layout = new QHBoxLayout();
    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText("搜索联系人...");
    m_search_edit->setClearButtonEnabled(true);
    toolbar_layout->addWidget(m_search_edit, 1);

    m_group_filter = new QComboBox(this);
    m_group_filter->addItem("全部");
    m_group_filter->setMinimumWidth(100);
    toolbar_layout->addWidget(m_group_filter);

    toolbar_layout->addSpacing(10);

    m_add_button = new QPushButton("添加", this);
    m_edit_button = new QPushButton("编辑", this);
    m_delete_button = new QPushButton("删除", this);
    m_edit_button->setEnabled(false);
    m_delete_button->setEnabled(false);

    toolbar_layout->addWidget(m_add_button);
    toolbar_layout->addWidget(m_edit_button);
    toolbar_layout->addWidget(m_delete_button);

    main_layout->addLayout(toolbar_layout);

    // 联系人列表
    m_contact_list = new QListWidget(this);
    m_contact_list->setContextMenuPolicy(Qt::CustomContextMenu);
    main_layout->addWidget(m_contact_list, 1);

    // 联系人数量标签
    m_contact_count_label = new QLabel("联系人: 0", this);
    m_contact_count_label->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(m_contact_count_label);

    // 连接信号
    connect(m_search_edit, &QLineEdit::textChanged, this, &ContactListPage::onSearchTextChanged);
    connect(m_group_filter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ContactListPage::onGroupFilterChanged);
    connect(m_add_button, &QPushButton::clicked, this, &ContactListPage::onAddContact);
    connect(m_edit_button, &QPushButton::clicked, this, &ContactListPage::onEditContact);
    connect(m_delete_button, &QPushButton::clicked, this, &ContactListPage::onDeleteContact);
    connect(m_contact_list, &QListWidget::itemDoubleClicked, this, &ContactListPage::onContactItemDoubleClicked);
    connect(m_contact_list, &QListWidget::itemSelectionChanged, this, [this]() {
        const bool has_selection = m_contact_list->currentItem() != nullptr;
        m_edit_button->setEnabled(has_selection);
        m_delete_button->setEnabled(has_selection);
    });
    connect(m_contact_list, &QListWidget::customContextMenuRequested, this, &ContactListPage::onContactItemContextMenu);
}

void ContactListPage::setNetworkManager(NetworkManager* network_manager)
{
    m_network_manager = network_manager;
}

void ContactListPage::setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository)
{
    qInfo() << "[ContactListPage::setContactRepository] 开始";
    m_contact_repository = repository;
    qInfo() << "[ContactListPage::setContactRepository] 调用 refreshContactList";
    refreshContactList();
    qInfo() << "[ContactListPage::setContactRepository] 完成";
}

void ContactListPage::onUserOnline(const Core::UserInfo& user_info)
{
    if (m_contact_repository && m_contact_repository->hasContact(user_info.user_id)) {
        Core::ContactInfo contact = m_contact_repository->getContact(user_info.user_id);
        contact.updateFromUserInfo(user_info);
        m_contact_repository->saveContact(contact);
        updateContactListItem(contact);
    }
}

void ContactListPage::onUserInfoUpdated(const Core::UserInfo& user_info)
{
    onUserOnline(user_info);
}

void ContactListPage::refreshContactList()
{
    qInfo() << "[ContactListPage::refreshContactList] 开始";
    loadContacts();
    qInfo() << "[ContactListPage::refreshContactList] 完成";
}

void ContactListPage::loadContacts()
{
    qInfo() << "[ContactListPage::loadContacts] 开始";
    if (!m_contact_repository || !m_contact_list) {
        qInfo() << "[ContactListPage::loadContacts] 提前返回（缺少 repository 或 list）";
        return;
    }

    qInfo() << "[ContactListPage::loadContacts] 清空列表";
    m_contact_list->clear();
    m_cached_contacts.clear();

    qInfo() << "[ContactListPage::loadContacts] 获取联系人列表";
    QList<Core::ContactInfo> contacts;
    if (m_current_filter_group == QStringLiteral("全部")) {
        qInfo() << "[ContactListPage::loadContacts] 调用 getAllContacts";
        contacts = m_contact_repository->getAllContacts();
    } else {
        qInfo() << "[ContactListPage::loadContacts] 调用 getContactsByGroup";
        contacts = m_contact_repository->getContactsByGroup(m_current_filter_group);
    }
    qInfo() << "[ContactListPage::loadContacts] 获取到" << contacts.size() << "个联系人";

    // 更新分组过滤器
    qInfo() << "[ContactListPage::loadContacts] 调用 getContactGroups";
    QList<QString> groups = getContactGroups();
    qInfo() << "[ContactListPage::loadContacts] getContactGroups 返回" << groups.size() << "个分组";
    
    if (!m_group_filter) {
        qWarning() << "[ContactListPage::loadContacts] m_group_filter 为空！";
        return;
    }
    
    qInfo() << "[ContactListPage::loadContacts] 更新分组过滤器 UI";
    QString current_group = m_group_filter->currentText();
    
    // 临时断开信号连接，防止在更新时触发 onGroupFilterChanged 导致递归
    m_group_filter->blockSignals(true);
    m_group_filter->clear();
    m_group_filter->addItem("全部");
    for (const QString& group : groups) {
        m_group_filter->addItem(group);
    }
    int index = m_group_filter->findText(current_group);
    if (index >= 0) {
        m_group_filter->setCurrentIndex(index);
    } else {
        m_group_filter->setCurrentIndex(0);
        m_current_filter_group = QStringLiteral("全部");
    }
    m_group_filter->blockSignals(false);

    // 添加联系人到列表
    qInfo() << "[ContactListPage::loadContacts] 添加联系人到列表";
    if (!m_contact_list) {
        qWarning() << "[ContactListPage::loadContacts] m_contact_list 为空！";
        return;
    }
    
    for (const Core::ContactInfo& contact : contacts) {
        m_cached_contacts.insert(contact.contact_id, contact);
        updateContactListItem(contact);
    }

    if (m_contact_count_label) {
        m_contact_count_label->setText(QString("联系人: %1").arg(contacts.size()));
    }
    qInfo() << "[ContactListPage::loadContacts] 完成";
}

void ContactListPage::updateContactListItem(const Core::ContactInfo& contact)
{
    QListWidgetItem* item = nullptr;
    for (int i = 0; i < m_contact_list->count(); ++i) {
        QListWidgetItem* candidate = m_contact_list->item(i);
        if (candidate->data(Qt::UserRole).toString() == contact.contact_id) {
            item = candidate;
            break;
        }
    }

    if (!item) {
        item = new QListWidgetItem(m_contact_list);
        item->setData(Qt::UserRole, contact.contact_id);
    }

    const QString display_name = contactDisplayName(contact);
    item->setText(display_name);
    item->setToolTip(QString("%1\n%2\n%3\n%4")
                     .arg(display_name, contact.username, contact.hostname, contact.ip_address));
}

void ContactListPage::removeContactListItem(const QString& contact_id)
{
    for (int i = 0; i < m_contact_list->count(); ++i) {
        QListWidgetItem* item = m_contact_list->item(i);
        if (item->data(Qt::UserRole).toString() == contact_id) {
            delete m_contact_list->takeItem(i);
            break;
        }
    }
}

QString ContactListPage::contactDisplayName(const Core::ContactInfo& contact) const
{
    if (!contact.display_name.isEmpty() && contact.display_name != contact.username) {
        return QString("%1 (%2)").arg(contact.display_name, contact.username);
    }
    return contact.username;
}

void ContactListPage::onContactItemDoubleClicked(QListWidgetItem* item)
{
    const QString contact_id = item->data(Qt::UserRole).toString();
    if (m_cached_contacts.contains(contact_id)) {
        emit contactDoubleClicked(m_cached_contacts.value(contact_id));
    }
}

void ContactListPage::onContactItemContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_contact_list->itemAt(pos);
    if (!item) {
        return;
    }

    const QString contact_id = item->data(Qt::UserRole).toString();
    if (m_cached_contacts.contains(contact_id)) {
        emit contactContextMenuRequested(m_contact_list->mapToGlobal(pos), m_cached_contacts.value(contact_id));
    }
}

void ContactListPage::onAddContact()
{
    showContactEditDialog(Core::ContactInfo(), false);
}

void ContactListPage::onEditContact()
{
    QListWidgetItem* item = m_contact_list->currentItem();
    if (!item) {
        return;
    }

    const QString contact_id = item->data(Qt::UserRole).toString();
    if (m_cached_contacts.contains(contact_id)) {
        showContactEditDialog(m_cached_contacts.value(contact_id), true);
    }
}

void ContactListPage::onDeleteContact()
{
    QListWidgetItem* item = m_contact_list->currentItem();
    if (!item) {
        return;
    }

    const QString contact_id = item->data(Qt::UserRole).toString();
    if (!m_cached_contacts.contains(contact_id)) {
        return;
    }

    const Core::ContactInfo& contact = m_cached_contacts.value(contact_id);
    const QString display_name = contactDisplayName(contact);

    int ret = QMessageBox::question(this, "确认删除", 
                                     QString("确定要删除联系人 \"%1\" 吗？").arg(display_name),
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (m_contact_repository && m_contact_repository->deleteContact(contact_id)) {
            m_cached_contacts.remove(contact_id);
            removeContactListItem(contact_id);
            m_contact_count_label->setText(QString("联系人: %1").arg(m_cached_contacts.size()));
        }
    }
}

void ContactListPage::onSearchTextChanged(const QString& text)
{
    if (!m_contact_repository) {
        return;
    }

    if (text.isEmpty()) {
        loadContacts();
        return;
    }

    QList<Core::ContactInfo> results = m_contact_repository->searchContacts(text);
    m_contact_list->clear();
    m_cached_contacts.clear();

    for (const Core::ContactInfo& contact : results) {
        m_cached_contacts.insert(contact.contact_id, contact);
        updateContactListItem(contact);
    }

    m_contact_count_label->setText(QString("找到: %1").arg(results.size()));
}

void ContactListPage::onGroupFilterChanged(int index)
{
    if (index < 0) {
        return;
    }

    m_current_filter_group = m_group_filter->itemText(index);
    loadContacts();
}

void ContactListPage::showContactEditDialog(const Core::ContactInfo& contact, bool is_edit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(is_edit ? "编辑联系人" : "添加联系人");
    dialog.setMinimumWidth(400);

    QFormLayout* form_layout = new QFormLayout(&dialog);

    QLineEdit* display_name_edit = new QLineEdit(&dialog);
    display_name_edit->setText(contact.display_name);
    form_layout->addRow("显示名称:", display_name_edit);

    QLineEdit* username_edit = new QLineEdit(&dialog);
    username_edit->setText(contact.username);
    username_edit->setReadOnly(is_edit);
    form_layout->addRow("用户名:", username_edit);

    QLineEdit* hostname_edit = new QLineEdit(&dialog);
    hostname_edit->setText(contact.hostname);
    form_layout->addRow("主机名:", hostname_edit);

    QLineEdit* ip_edit = new QLineEdit(&dialog);
    ip_edit->setText(contact.ip_address);
    form_layout->addRow("IP地址:", ip_edit);

    QLineEdit* group_edit = new QLineEdit(&dialog);
    group_edit->setText(contact.group_name);
    form_layout->addRow("分组:", group_edit);

    QTextEdit* notes_edit = new QTextEdit(&dialog);
    notes_edit->setPlainText(contact.notes);
    notes_edit->setMaximumHeight(100);
    form_layout->addRow("备注:", notes_edit);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form_layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Core::ContactInfo new_contact = contact;
        new_contact.display_name = display_name_edit->text().trimmed();
        new_contact.username = username_edit->text().trimmed();
        new_contact.hostname = hostname_edit->text().trimmed();
        new_contact.ip_address = ip_edit->text().trimmed();
        new_contact.group_name = group_edit->text().trimmed();
        new_contact.notes = notes_edit->toPlainText().trimmed();

        if (new_contact.contact_id.isEmpty()) {
            // 新联系人，需要生成ID或从用户名生成
            if (new_contact.username.isEmpty()) {
                QMessageBox::warning(this, "错误", "用户名不能为空");
                return;
            }
            // 使用用户名作为contact_id（实际应该使用更可靠的ID生成方式）
            new_contact.contact_id = new_contact.username;
        }

        if (!is_edit) {
            new_contact.created_at = QDateTime::currentDateTime();
        }

        if (m_contact_repository && m_contact_repository->saveContact(new_contact)) {
            m_cached_contacts.insert(new_contact.contact_id, new_contact);
            updateContactListItem(new_contact);
            loadContacts();
        }
    }
}

QList<QString> ContactListPage::getContactGroups() const
{
    qInfo() << "[ContactListPage::getContactGroups] 开始";
    if (!m_contact_repository) {
        qInfo() << "[ContactListPage::getContactGroups] repository 为空，返回空列表";
        return {};
    }

    qInfo() << "[ContactListPage::getContactGroups] 调用 getAllContacts";
    QList<Core::ContactInfo> all_contacts = m_contact_repository->getAllContacts();
    qInfo() << "[ContactListPage::getContactGroups] getAllContacts 返回" << all_contacts.size() << "个联系人";
    QSet<QString> groups;
    for (const Core::ContactInfo& contact : all_contacts) {
        if (!contact.group_name.isEmpty()) {
            groups.insert(contact.group_name);
        }
    }

    QList<QString> group_list = groups.values();
    std::sort(group_list.begin(), group_list.end());
    qInfo() << "[ContactListPage::getContactGroups] 返回" << group_list.size() << "个分组";
    return group_list;
}

} // namespace KylinMessenger

