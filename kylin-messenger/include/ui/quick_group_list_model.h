#ifndef KYLIN_MESSENGER_UI_QUICK_GROUP_LIST_MODEL_H
#define KYLIN_MESSENGER_UI_QUICK_GROUP_LIST_MODEL_H

#include <QAbstractListModel>
#include <QSettings>
#include <QString>
#include <QVector>

#include "core/models.h"

namespace KylinMessenger {

class QuickGroupListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    enum Roles {
        GroupIdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        MemberCountRole
    };

    explicit QuickGroupListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const { return m_filter_text; }
    void setFilterText(const QString& text);

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void filterTextChanged();

private:
    void reload();
    void applyFilter();

    QVector<Core::GroupInfo> m_all_groups;
    QVector<Core::GroupInfo> m_filtered_groups;
    QString m_filter_text;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_UI_QUICK_GROUP_LIST_MODEL_H
