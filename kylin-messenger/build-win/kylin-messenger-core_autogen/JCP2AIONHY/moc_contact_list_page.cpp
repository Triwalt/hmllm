/****************************************************************************
** Meta object code from reading C++ file 'contact_list_page.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/ui/contact_list_page.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'contact_list_page.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t {};
} // unnamed namespace

template <> constexpr inline auto KylinMessenger::ContactListPage::qt_create_metaobjectdata<qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KylinMessenger::ContactListPage",
        "contactDoubleClicked",
        "",
        "Core::ContactInfo",
        "contact",
        "contactContextMenuRequested",
        "pos",
        "onUserOnline",
        "Core::UserInfo",
        "user_info",
        "onUserInfoUpdated",
        "refreshContactList",
        "onContactItemDoubleClicked",
        "QListWidgetItem*",
        "item",
        "onContactItemContextMenu",
        "onAddContact",
        "onEditContact",
        "onDeleteContact",
        "onSearchTextChanged",
        "text",
        "onGroupFilterChanged",
        "index"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'contactDoubleClicked'
        QtMocHelpers::SignalData<void(const Core::ContactInfo &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'contactContextMenuRequested'
        QtMocHelpers::SignalData<void(const QPoint &, const Core::ContactInfo &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QPoint, 6 }, { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onUserOnline'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Slot 'onUserInfoUpdated'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Slot 'refreshContactList'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onContactItemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'onContactItemContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QPoint, 6 },
        }}),
        // Slot 'onAddContact'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onEditContact'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteContact'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Slot 'onGroupFilterChanged'
        QtMocHelpers::SlotData<void(int)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 22 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ContactListPage, qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KylinMessenger::ContactListPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>.metaTypes,
    nullptr
} };

void KylinMessenger::ContactListPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ContactListPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->contactDoubleClicked((*reinterpret_cast< std::add_pointer_t<Core::ContactInfo>>(_a[1]))); break;
        case 1: _t->contactContextMenuRequested((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Core::ContactInfo>>(_a[2]))); break;
        case 2: _t->onUserOnline((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 3: _t->onUserInfoUpdated((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 4: _t->refreshContactList(); break;
        case 5: _t->onContactItemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 6: _t->onContactItemContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 7: _t->onAddContact(); break;
        case 8: _t->onEditContact(); break;
        case 9: _t->onDeleteContact(); break;
        case 10: _t->onSearchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->onGroupFilterChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ContactListPage::*)(const Core::ContactInfo & )>(_a, &ContactListPage::contactDoubleClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ContactListPage::*)(const QPoint & , const Core::ContactInfo & )>(_a, &ContactListPage::contactContextMenuRequested, 1))
            return;
    }
}

const QMetaObject *KylinMessenger::ContactListPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KylinMessenger::ContactListPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger15ContactListPageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int KylinMessenger::ContactListPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void KylinMessenger::ContactListPage::contactDoubleClicked(const Core::ContactInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void KylinMessenger::ContactListPage::contactContextMenuRequested(const QPoint & _t1, const Core::ContactInfo & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
