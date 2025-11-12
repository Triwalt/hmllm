#include "core/repositories/contact_repository.h"

#include <QSettings>
#include <QLoggingCategory>

namespace KylinMessenger::Core::Repositories {

namespace {
Q_LOGGING_CATEGORY(lcContactRepo, "kylin.contact.repository")
}

SettingsContactRepository::SettingsContactRepository(const QString& organization, const QString& application)
    : m_organization(organization)
    , m_application(application)
{
}

QString SettingsContactRepository::contactKey(const QString& contact_id) const
{
    return QStringLiteral("contacts/%1").arg(contact_id);
}

bool SettingsContactRepository::saveContact(const ContactInfo& contact)
{
    if (contact.contact_id.isEmpty()) {
        qCWarning(lcContactRepo) << "Cannot save contact with empty contact_id";
        return false;
    }

    QSettings settings(m_organization, m_application);
    const QString key = contactKey(contact.contact_id);
    const QByteArray data = contact.serialize();
    settings.setValue(key, data);
    settings.sync();

    qCDebug(lcContactRepo) << "Saved contact" << contact.contact_id << contact.display_name;
    return true;
}

ContactInfo SettingsContactRepository::getContact(const QString& contact_id) const
{
    if (contact_id.isEmpty()) {
        return ContactInfo();
    }

    QSettings settings(m_organization, m_application);
    const QString key = contactKey(contact_id);
    if (!settings.contains(key)) {
        return ContactInfo();
    }

    const QByteArray data = settings.value(key).toByteArray();
    ContactInfo contact;
    if (contact.deserialize(data)) {
        return contact;
    }

    return ContactInfo();
}

QList<ContactInfo> SettingsContactRepository::getAllContacts() const
{
    qInfo() << "[SettingsContactRepository::getAllContacts] 开始读取联系人";
    QSettings settings(m_organization, m_application);
    settings.beginGroup(QStringLiteral("contacts"));

    QList<ContactInfo> contacts;
    const QStringList keys = settings.allKeys();
    qInfo() << "[SettingsContactRepository::getAllContacts] 找到" << keys.size() << "个联系人键";
    contacts.reserve(keys.size());

    for (const QString& key : keys) {
        const QByteArray data = settings.value(key).toByteArray();
        ContactInfo contact;
        if (contact.deserialize(data)) {
            contacts.append(contact);
        }
    }

    settings.endGroup();
    qInfo() << "[SettingsContactRepository::getAllContacts] 返回" << contacts.size() << "个联系人";
    return contacts;
}

QList<ContactInfo> SettingsContactRepository::getContactsByGroup(const QString& group_name) const
{
    const QList<ContactInfo> all = getAllContacts();
    QList<ContactInfo> filtered;
    filtered.reserve(all.size());

    for (const ContactInfo& contact : all) {
        if (contact.group_name == group_name) {
            filtered.append(contact);
        }
    }

    return filtered;
}

QList<ContactInfo> SettingsContactRepository::searchContacts(const QString& keyword) const
{
    const QList<ContactInfo> all = getAllContacts();
    QList<ContactInfo> results;
    results.reserve(all.size());

    const QString lower_keyword = keyword.toLower();

    for (const ContactInfo& contact : all) {
        if (contact.display_name.toLower().contains(lower_keyword) ||
            contact.username.toLower().contains(lower_keyword) ||
            contact.hostname.toLower().contains(lower_keyword) ||
            contact.ip_address.contains(keyword) ||
            contact.notes.toLower().contains(lower_keyword)) {
            results.append(contact);
        }
    }

    return results;
}

bool SettingsContactRepository::deleteContact(const QString& contact_id)
{
    if (contact_id.isEmpty()) {
        return false;
    }

    QSettings settings(m_organization, m_application);
    const QString key = contactKey(contact_id);
    if (!settings.contains(key)) {
        return false;
    }

    settings.remove(key);
    settings.sync();

    qCDebug(lcContactRepo) << "Deleted contact" << contact_id;
    return true;
}

bool SettingsContactRepository::hasContact(const QString& contact_id) const
{
    if (contact_id.isEmpty()) {
        return false;
    }

    QSettings settings(m_organization, m_application);
    const QString key = contactKey(contact_id);
    return settings.contains(key);
}

} // namespace KylinMessenger::Core::Repositories

