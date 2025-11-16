#include "ui/quick_group_list_model.h"

#include <QVariantMap>

#include <algorithm>

namespace {

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
}

bool groupLess(const KylinMessenger::Core::GroupInfo& lhs,
               const KylinMessenger::Core::GroupInfo& rhs)
{
    const int cmp = QString::localeAwareCompare(lhs.group_name, rhs.group_name);
    if (cmp == 0) {
        return lhs.group_id < rhs.group_id;
    }
    return cmp < 0;
}

} // namespace

namespace KylinMessenger {

QuickGroupListModel::QuickGroupListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    reload();
}

int QuickGroupListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_filtered_groups.size();
}

QVariant QuickGroupListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filtered_groups.size()) {
        return QVariant();
    }

    const auto& group = m_filtered_groups.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return group.group_name;
    case GroupIdRole:
        return group.group_id;
    case DescriptionRole:
        return group.description;
    case MemberCountRole:
        return group.memberCount();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> QuickGroupListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[GroupIdRole] = "groupId";
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[MemberCountRole] = "memberCount";
    return roles;
}

void QuickGroupListModel::setFilterText(const QString& text)
{
    if (m_filter_text == text) {
        return;
    }
    m_filter_text = text;
    emit filterTextChanged();
    applyFilter();
}

QVariantMap QuickGroupListModel::get(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_filtered_groups.size()) {
        return map;
    }

    const auto& group = m_filtered_groups.at(row);
    map.insert(QStringLiteral("groupId"), group.group_id);
    map.insert(QStringLiteral("name"), group.group_name);
    map.insert(QStringLiteral("description"), group.description);
    map.insert(QStringLiteral("memberCount"), group.memberCount());
    return map;
}

void QuickGroupListModel::refresh()
{
    reload();
}

void QuickGroupListModel::reload()
{
    QSettings settings(QStringLiteral("KylinMessenger"), QStringLiteral("KylinMessenger"));
    settings.beginGroup(QStringLiteral("groups"));

    QList<Core::GroupInfo> groups;
    const auto keys = settings.childKeys();
    groups.reserve(keys.size());
    for (const QString& key : keys) {
        Core::GroupInfo group;
        const QByteArray data = settings.value(key).toByteArray();
        if (group.deserialize(data)) {
            groups.append(group);
        }
    }

    settings.endGroup();
    std::sort(groups.begin(), groups.end(), groupLess);

    beginResetModel();
    m_all_groups = QVector<Core::GroupInfo>::fromList(groups);
    endResetModel();

    applyFilter();
}

void QuickGroupListModel::applyFilter()
{
    beginResetModel();

    m_filtered_groups.clear();
    const QString filter = normalized(m_filter_text);
    for (const auto& group : m_all_groups) {
        if (!filter.isEmpty()) {
            const bool matches = group.group_name.toLower().contains(filter) ||
                                  group.description.toLower().contains(filter);
            if (!matches) {
                continue;
            }
        }
        m_filtered_groups.push_back(group);
    }

    endResetModel();
    emit countChanged();
}

} // namespace KylinMessenger
