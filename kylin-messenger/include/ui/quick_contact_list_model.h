#ifndef KYLIN_MESSENGER_UI_QUICK_CONTACT_LIST_MODEL_H
#define KYLIN_MESSENGER_UI_QUICK_CONTACT_LIST_MODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <memory>

#include "core/models.h"
#include "core/repositories/contact_repository.h"

namespace KylinMessenger {

class QuickContactListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString groupFilter READ groupFilter WRITE setGroupFilter NOTIFY groupFilterChanged)

public:
    enum Roles {
        ContactIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        UsernameRole,
        HostnameRole,
        IpRole,
        GroupRole,
        NotesRole,
        FavoriteRole
    };

    explicit QuickContactListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const { return m_filter_text; }
    QString groupFilter() const { return m_group_filter; }

    void setFilterText(const QString& text);
    void setGroupFilter(const QString& group);

    void setContactRepository(std::shared_ptr<Core::Repositories::IContactRepository> repository);

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void filterTextChanged();
    void groupFilterChanged();

private:
    void reload();
    void applyFilter();
    QString contactDisplayName(const Core::ContactInfo& contact) const;

    std::shared_ptr<Core::Repositories::IContactRepository> m_repository;
    QVector<Core::ContactInfo> m_all_contacts;
    QVector<Core::ContactInfo> m_filtered_contacts;
    QString m_filter_text;
    QString m_group_filter;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_CONTACT_LIST_MODEL_H
