/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/main_window.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14KylinMessenger10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto KylinMessenger::MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KylinMessenger::MainWindow",
        "onUserOnline",
        "",
        "Core::UserInfo",
        "user_info",
        "onUserOffline",
        "user_id",
        "onUserInfoUpdated",
        "onMessageReceived",
        "Core::ChatMessage",
        "message",
        "onGroupMessageReceived",
        "group_id",
        "onViewUserInfo",
        "onSearchTextChanged",
        "text",
        "onStatusChanged",
        "index",
        "onStatusTextChanged",
        "showUserContextMenu",
        "pos",
        "onSendMessage",
        "onSendFile",
        "onSendScreenshot",
        "onOpenAIChat",
        "onSettings",
        "onAbout",
        "onQuit",
        "onTrayIconActivated",
        "QSystemTrayIcon::ActivationReason",
        "reason",
        "onTrayIconMessageClicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onUserOnline'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onUserOffline'
        QtMocHelpers::SlotData<void(const QString &)>(5, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Slot 'onUserInfoUpdated'
        QtMocHelpers::SlotData<void(const Core::UserInfo &)>(7, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onMessageReceived'
        QtMocHelpers::SlotData<void(const Core::ChatMessage &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Slot 'onGroupMessageReceived'
        QtMocHelpers::SlotData<void(const QString &, const Core::ChatMessage &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 12 }, { 0x80000000 | 9, 10 },
        }}),
        // Slot 'onViewUserInfo'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSearchTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'onStatusChanged'
        QtMocHelpers::SlotData<void(int)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 17 },
        }}),
        // Slot 'onStatusTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'showUserContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &, const Core::UserInfo &)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QPoint, 20 }, { 0x80000000 | 3, 4 },
        }}),
        // Slot 'onSendMessage'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSendFile'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSendScreenshot'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onOpenAIChat'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSettings'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAbout'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onQuit'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTrayIconActivated'
        QtMocHelpers::SlotData<void(QSystemTrayIcon::ActivationReason)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Slot 'onTrayIconMessageClicked'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KylinMessenger::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>.metaTypes,
    nullptr
} };

void KylinMessenger::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onUserOnline((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 1: _t->onUserOffline((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->onUserInfoUpdated((*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[1]))); break;
        case 3: _t->onMessageReceived((*reinterpret_cast< std::add_pointer_t<Core::ChatMessage>>(_a[1]))); break;
        case 4: _t->onGroupMessageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Core::ChatMessage>>(_a[2]))); break;
        case 5: _t->onViewUserInfo(); break;
        case 6: _t->onSearchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->onStatusChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->onStatusTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->showUserContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<Core::UserInfo>>(_a[2]))); break;
        case 10: _t->onSendMessage(); break;
        case 11: _t->onSendFile(); break;
        case 12: _t->onSendScreenshot(); break;
        case 13: _t->onOpenAIChat(); break;
        case 14: _t->onSettings(); break;
        case 15: _t->onAbout(); break;
        case 16: _t->onQuit(); break;
        case 17: _t->onTrayIconActivated((*reinterpret_cast< std::add_pointer_t<QSystemTrayIcon::ActivationReason>>(_a[1]))); break;
        case 18: _t->onTrayIconMessageClicked(); break;
        default: ;
        }
    }
}

const QMetaObject *KylinMessenger::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KylinMessenger::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int KylinMessenger::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}
QT_WARNING_POP
