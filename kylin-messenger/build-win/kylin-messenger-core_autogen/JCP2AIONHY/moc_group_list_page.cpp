/****************************************************************************
** Meta object code from reading C++ file 'group_list_page.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/ui/group_list_page.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'group_list_page.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t {};
} // unnamed namespace

template <> constexpr inline auto KylinMessenger::GroupListPage::qt_create_metaobjectdata<qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KylinMessenger::GroupListPage",
        "groupDoubleClicked",
        "",
        "Core::GroupInfo",
        "group_info",
        "groupContextMenuRequested",
        "pos",
        "onGroupCreated",
        "onGroupUpdated",
        "onGroupDeleted",
        "group_id",
        "onUserJoinedGroup",
        "user_id",
        "onUserLeftGroup",
        "refreshGroupList",
        "onGroupItemDoubleClicked",
        "QListWidgetItem*",
        "item",
        "onGroupItemContextMenu",
        "onCreateGroup",
        "onJoinGroup",
        "onLeaveGroup",
        "onSearchTextChanged",
        "text"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'groupDoubleClicked'
        QtMocHelpers::SignalData<void(const Core::GroupInfo &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'groupContextMenuRequested'
        QtMocHelpers::SignalData<void(const QPoint &, const Core::GroupInfo &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QPoint, 6 }, { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onGroupCreated'
        QtMocHelpers::SlotData<void(const Core::GroupInfo &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onGroupUpdated'
        QtMocHelpers::SlotData<void(const Core::GroupInfo &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onGroupDeleted'
        QtMocHelpers::SlotData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 },
        }}),
        // Slot 'onUserJoinedGroup'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 }, { QMetaType::QString, 12 },
        }}),
        // Slot 'onUserLeftGroup'
        QtMocHelpers::SlotData<void(const QString &, const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 10 }, { QMetaType::QString, 12 },
        }}),
        // Slot 'refreshGroupList'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onGroupItemDoubleClicked'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 16, 17 },
        }}),
        // Slot 'onGroupItemContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QPoint, 6 },
        }}),
        // Slot 'onCreateGroup'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onJoinGroup'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLeaveGroup'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 23 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GroupListPage, qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KylinMessenger::GroupListPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>.metaTypes,
    nullptr
} };

void KylinMessenger::GroupListPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GroupListPage *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->groupDoubleClicked((*reinterpret_cast< std::add_pointer_t<Core::GroupInfo>>(_a[1]))); break;
        case 1: _t->groupContextMenuRequested((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Core::GroupInfo>>(_a[2]))); break;
        case 2: _t->onGroupCreated((*reinterpret_cast< std::add_pointer_t<Core::GroupInfo>>(_a[1]))); break;
        case 3: _t->onGroupUpdated((*reinterpret_cast< std::add_pointer_t<Core::GroupInfo>>(_a[1]))); break;
        case 4: _t->onGroupDeleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onUserJoinedGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->onUserLeftGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->refreshGroupList(); break;
        case 8: _t->onGroupItemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 9: _t->onGroupItemContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 10: _t->onCreateGroup(); break;
        case 11: _t->onJoinGroup(); break;
        case 12: _t->onLeaveGroup(); break;
        case 13: _t->onSearchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GroupListPage::*)(const Core::GroupInfo & )>(_a, &GroupListPage::groupDoubleClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GroupListPage::*)(const QPoint & , const Core::GroupInfo & )>(_a, &GroupListPage::groupContextMenuRequested, 1))
            return;
    }
}

const QMetaObject *KylinMessenger::GroupListPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KylinMessenger::GroupListPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger13GroupListPageE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int KylinMessenger::GroupListPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void KylinMessenger::GroupListPage::groupDoubleClicked(const Core::GroupInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void KylinMessenger::GroupListPage::groupContextMenuRequested(const QPoint & _t1, const Core::GroupInfo & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
