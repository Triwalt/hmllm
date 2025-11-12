#ifndef KYLIN_MESSENGER_CONTACT_REPOSITORY_H
#define KYLIN_MESSENGER_CONTACT_REPOSITORY_H

#include "core/models.h"
#include <QList>
#include <QString>

namespace KylinMessenger::Core::Repositories {

/**
 * @brief 联系人存储接口
 */
class IContactRepository {
public:
    virtual ~IContactRepository() = default;

    /**
     * @brief 保存联系人
     */
    virtual bool saveContact(const ContactInfo& contact) = 0;

    /**
     * @brief 根据ID获取联系人
     */
    virtual ContactInfo getContact(const QString& contact_id) const = 0;

    /**
     * @brief 获取所有联系人
     */
    virtual QList<ContactInfo> getAllContacts() const = 0;

    /**
     * @brief 根据分组获取联系人
     */
    virtual QList<ContactInfo> getContactsByGroup(const QString& group_name) const = 0;

    /**
     * @brief 搜索联系人
     */
    virtual QList<ContactInfo> searchContacts(const QString& keyword) const = 0;

    /**
     * @brief 删除联系人
     */
    virtual bool deleteContact(const QString& contact_id) = 0;

    /**
     * @brief 检查联系人是否存在
     */
    virtual bool hasContact(const QString& contact_id) const = 0;
};

/**
 * @brief 基于QSettings的联系人存储实现
 */
class SettingsContactRepository : public IContactRepository {
public:
    explicit SettingsContactRepository(const QString& organization = QStringLiteral("KylinMessenger"),
                                       const QString& application = QStringLiteral("KylinMessenger"));
    virtual ~SettingsContactRepository() = default;

    bool saveContact(const ContactInfo& contact) override;
    ContactInfo getContact(const QString& contact_id) const override;
    QList<ContactInfo> getAllContacts() const override;
    QList<ContactInfo> getContactsByGroup(const QString& group_name) const override;
    QList<ContactInfo> searchContacts(const QString& keyword) const override;
    bool deleteContact(const QString& contact_id) override;
    bool hasContact(const QString& contact_id) const override;

private:
    QString m_organization;
    QString m_application;
    QString contactKey(const QString& contact_id) const;
};

} // namespace KylinMessenger::Core::Repositories

#endif // KYLIN_MESSENGER_CONTACT_REPOSITORY_H

