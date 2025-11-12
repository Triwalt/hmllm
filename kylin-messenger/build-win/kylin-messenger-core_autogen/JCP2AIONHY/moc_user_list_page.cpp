/****************************************************************************
** Meta object code from reading C++ file 'user_list_page.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/ui/user_list_page.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'user_list_page.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14KylinMessenger12UserListPageE_t {};
} // unnamed namespace

template <> constexpr inline auto KylinMessenger::UserListPage::qt_create_metaobjectdata<qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KylinMessenger::UserListPage",
        "userDoubleClicked",
        "",
        "Core::UserInfo",
        "user_info",
        "userContextMenuRequested",
        "pos",
        "onUserOnline",
        "onUserOffline",
        "user_id",
        "onUserInfoUpdated",
        "onSearchTextChanged",
        "text",
        "updateUserList",
        "onUserItemDoubleClicked",
        "QListWidgetItem*",
        "item",
        "onUserItemContextMenu"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'userDoubleClicked'
        QtMocHelpers::SignalData<void(const Core::UserInfo &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'userContextMenuRequested'
        QtMocHelpers::SignalData<void(const QPoint &, const Core::UserInfo &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QPoint, 6 }, { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onUserOnline'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onUserOffline'
        QtMocHelpers::SlotData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'onUserInfoUpdated'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 },
        }}),
        // Slot 'updateUserList'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onUserItemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 },
        }}),
        // Slot 'onUserItemContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(17, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QPoint, 6 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<UserListPage, qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KylinMessenger::UserListPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>.metaTypes,
    nullptr
} };

void KylinMessenger::UserListPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UserListPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->userDoubleClicked((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 1: _t->userContextMenuRequested((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[2]))); break;
        case 2: _t->onUserOnline((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 3: _t->onUserOffline((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->onUserInfoUpdated((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 5: _t->onSearchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->updateUserList(); break;
        case 7: _t->onUserItemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 8: _t->onUserItemContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (UserListPage::*)(const Core::UserInfo & )>(_a, &UserListPage::userDoubleClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (UserListPage::*)(const QPoint & , const Core::UserInfo & )>(_a, &UserListPage::userContextMenuRequested, 1))
            return;
    }
}

const QMetaObject *KylinMessenger::UserListPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KylinMessenger::UserListPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger12UserListPageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int KylinMessenger::UserListPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void KylinMessenger::UserListPage::userDoubleClicked(const Core::UserInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void KylinMessenger::UserListPage::userContextMenuRequested(const QPoint & _t1, const Core::UserInfo & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
