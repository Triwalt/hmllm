#include "ui/quick_contact_list_model.h"

#include <QVariantMap>

#include <algorithm>

namespace {

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
}

bool contactLess(const KylinMessenger::Core::ContactInfo& lhs,
                 const KylinMessenger::Core::ContactInfo& rhs)
{
    const QString left = lhs.display_name.isEmpty() ? lhs.username : lhs.display_name;
    const QString right = rhs.display_name.isEmpty() ? rhs.username : rhs.display_name;
    const int cmp = QString::localeAwareCompare(left, right);
    if (cmp == 0) {
        return lhs.contact_id < rhs.contact_id;
    }
    return cmp < 0;
}

} // namespace

namespace KylinMessenger {

QuickContactListModel::QuickContactListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int QuickContactListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_filtered_contacts.size();
}

QVariant QuickContactListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filtered_contacts.size()) {
        return QVariant();
    }

    const auto& contact = m_filtered_contacts.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case DisplayNameRole:
        return contactDisplayName(contact);
    case ContactIdRole:
        return contact.contact_id;
    case UsernameRole:
        return contact.username;
    case HostnameRole:
        return contact.hostname;
    case IpRole:
        return contact.ip_address;
    case GroupRole:
        return contact.group_name;
    case NotesRole:
        return contact.notes;
    case FavoriteRole:
        return contact.is_favorite;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QuickContactListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ContactIdRole] = "contactId";
    roles[DisplayNameRole] = "displayName";
    roles[UsernameRole] = "username";
    roles[HostnameRole] = "hostname";
    roles[IpRole] = "ip";
    roles[GroupRole] = "group";
    roles[NotesRole] = "notes";
    roles[FavoriteRole] = "favorite";
    return roles;
}

void QuickContactListModel::setFilterText(const QString& text)
{
    if (m_filter_text == text) {
        return;
    }
    m_filter_text = text;
    emit filterTextChanged();
    applyFilter();
}

void QuickContactListModel::setGroupFilter(const QString& group)
{
    if (m_group_filter == group) {
        return;
    }
    m_group_filter = group;
    emit groupFilterChanged();
    applyFilter();
}

void QuickContactListModel::setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository)
{
    if (m_repository == repository) {
        return;
    }

    m_repository = std::move(repository);
    reload();
}

QVariantMap QuickContactListModel::get(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_filtered_contacts.size()) {
        return map;
    }

    const auto& contact = m_filtered_contacts.at(row);
    map.insert(QStringLiteral("contactId"), contact.contact_id);
    map.insert(QStringLiteral("displayName"), contactDisplayName(contact));
    map.insert(QStringLiteral("username"), contact.username);
    map.insert(QStringLiteral("hostname"), contact.hostname);
    map.insert(QStringLiteral("ip"), contact.ip_address);
    map.insert(QStringLiteral("group"), contact.group_name);
    map.insert(QStringLiteral("notes"), contact.notes);
    map.insert(QStringLiteral("favorite"), contact.is_favorite);
    return map;
}

void QuickContactListModel::refresh()
{
    reload();
}

void QuickContactListModel::reload()
{
    if (!m_repository) {
        beginResetModel();
        m_all_contacts.clear();
        m_filtered_contacts.clear();
        endResetModel();
        emit countChanged();
        return;
    }

    auto contacts = m_repository->getAllContacts();
    std::sort(contacts.begin(), contacts.end(), contactLess);

    beginResetModel();
    m_all_contacts = QVector<Core::ContactInfo>::fromList(contacts);
    endResetModel();

    applyFilter();
}

void QuickContactListModel::applyFilter()
{
    beginResetModel();

    m_filtered_contacts.clear();
    const QString filter = normalized(m_filter_text);
    const QString group = normalized(m_group_filter);

    for (const auto& contact : m_all_contacts) {
        if (!group.isEmpty() && normalized(contact.group_name) != group) {
            continue;
        }
        if (!filter.isEmpty()) {
            const QString display_name = contactDisplayName(contact).toLower();
            if (!display_name.contains(filter) &&
                !contact.username.toLower().contains(filter) &&
                !contact.ip_address.toLower().contains(filter)) {
                continue;
            }
        }
        m_filtered_contacts.push_back(contact);
    }

    endResetModel();
    emit countChanged();
}

QString QuickContactListModel::contactDisplayName(const Core::ContactInfo& contact) const
{
    if (!contact.display_name.isEmpty()) {
        if (contact.display_name.compare(contact.username, Qt::CaseInsensitive) == 0) {
            return contact.display_name;
        }
        return QStringLiteral("%1 (%2)").arg(contact.display_name, contact.username);
    }
    return contact.username.isEmpty() ? contact.contact_id : contact.username;
}

} // namespace KylinMessenger
