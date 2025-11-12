/****************************************************************************
** Meta object code from reading C++ file 'network_manager.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/network_manager.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'network_manager.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t {};
} // unnamed namespace

template <> constexpr inline auto KylinMessenger::NetworkManager::qt_create_metaobjectdata<qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "KylinMessenger::NetworkManager",
        "userOnline",
        "",
        "UserInfo",
        "user",
        "userOffline",
        "userId",
        "userInfoUpdated",
        "messageReceived",
        "ChatMessage",
        "message",
        "groupMessageReceived",
        "group_id",
        "imageReceived",
        "senderId",
        "image",
        "fileOfferReceived",
        "senderIp",
        "packetNo",
        "fileId",
        "filename",
        "filesize",
        "fileTransferProgress",
        "peerId",
        "bytesTransferred",
        "totalBytes",
        "fileTransferFinished",
        "savePath",
        "fileTransferFailed",
        "reason",
        "typingIndicator",
        "isTyping",
        "messageRead",
        "messageId",
        "networkError",
        "error",
        "groupMembersUpdated",
        "QList<UserInfo>",
        "members",
        "handleUdpData",
        "broadcastPresence",
        "cleanupOfflineUsers",
        "retryPendingMessages",
        "handleFileServerConnection",
        "handleFileSocketBytesWritten",
        "bytes",
        "handleFileSocketDisconnected",
        "handleFileSocketError",
        "QAbstractSocket::SocketError",
        "handleReceiveSocketReadyRead",
        "handleReceiveSocketDisconnected",
        "handleReceiveSocketError"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'userOnline'
        QtMocHelpers::SignalData<void(const UserInfo &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'userOffline'
        QtMocHelpers::SignalData<void(const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Signal 'userInfoUpdated'
        QtMocHelpers::SignalData<void(const UserInfo &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'messageReceived'
        QtMocHelpers::SignalData<void(const ChatMessage &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Signal 'groupMessageReceived'
        QtMocHelpers::SignalData<void(const QString &, const ChatMessage &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'imageReceived'
        QtMocHelpers::SignalData<void(const QString &, const QImage &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 }, { QMetaType::QImage, 15 },
        }}),
        // Signal 'fileOfferReceived'
        QtMocHelpers::SignalData<void(const QString &, const QString &, quint32, quint32, const QString &, quint64)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 }, { QMetaType::QString, 17 }, { QMetaType::UInt, 18 }, { QMetaType::UInt, 19 },
            { QMetaType::QString, 20 }, { QMetaType::ULongLong, 21 },
        }}),
        // Signal 'fileTransferProgress'
        QtMocHelpers::SignalData<void(const QString &, quint32, quint32, quint64, quint64)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 23 }, { QMetaType::UInt, 18 }, { QMetaType::UInt, 19 }, { QMetaType::ULongLong, 24 },
            { QMetaType::ULongLong, 25 },
        }}),
        // Signal 'fileTransferFinished'
        QtMocHelpers::SignalData<void(const QString &, quint32, quint32, const QString &)>(26, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 23 }, { QMetaType::UInt, 18 }, { QMetaType::UInt, 19 }, { QMetaType::QString, 27 },
        }}),
        // Signal 'fileTransferFailed'
        QtMocHelpers::SignalData<void(const QString &, quint32, quint32, const QString &)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 23 }, { QMetaType::UInt, 18 }, { QMetaType::UInt, 19 }, { QMetaType::QString, 29 },
        }}),
        // Signal 'typingIndicator'
        QtMocHelpers::SignalData<void(const QString &, bool)>(30, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 }, { QMetaType::Bool, 31 },
        }}),
        // Signal 'messageRead'
        QtMocHelpers::SignalData<void(const QString &)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 33 },
        }}),
        // Signal 'networkError'
        QtMocHelpers::SignalData<void(const QString &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 35 },
        }}),
        // Signal 'groupMembersUpdated'
        QtMocHelpers::SignalData<void(const QString &, const QList<UserInfo> &)>(36, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 12 }, { 0x80000000 | 37, 38 },
        }}),
        // Slot 'handleUdpData'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'broadcastPresence'
        QtMocHelpers::SlotData<void()>(40, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'cleanupOfflineUsers'
        QtMocHelpers::SlotData<void()>(41, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'retryPendingMessages'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFileServerConnection'
        QtMocHelpers::SlotData<void()>(43, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFileSocketBytesWritten'
        QtMocHelpers::SlotData<void(qint64)>(44, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::LongLong, 45 },
        }}),
        // Slot 'handleFileSocketDisconnected'
        QtMocHelpers::SlotData<void()>(46, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleFileSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(47, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 48, 35 },
        }}),
        // Slot 'handleReceiveSocketReadyRead'
        QtMocHelpers::SlotData<void()>(49, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleReceiveSocketDisconnected'
        QtMocHelpers::SlotData<void()>(50, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleReceiveSocketError'
        QtMocHelpers::SlotData<void(QAbstractSocket::SocketError)>(51, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 48, 35 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<NetworkManager, qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject KylinMessenger::NetworkManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>.metaTypes,
    nullptr
} };

void KylinMessenger::NetworkManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NetworkManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->userOnline((*reinterpret_cast< std::add_pointer_t<UserInfo>>(_a[1]))); break;
        case 1: _t->userOffline((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->userInfoUpdated((*reinterpret_cast< std::add_pointer_t<UserInfo>>(_a[1]))); break;
        case 3: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<ChatMessage>>(_a[1]))); break;
        case 4: _t->groupMessageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<ChatMessage>>(_a[2]))); break;
        case 5: _t->imageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QImage>>(_a[2]))); break;
        case 6: _t->fileOfferReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[6]))); break;
        case 7: _t->fileTransferProgress((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[5]))); break;
        case 8: _t->fileTransferFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 9: _t->fileTransferFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<quint32>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 10: _t->typingIndicator((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 11: _t->messageRead((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->networkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: _t->groupMembersUpdated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<UserInfo>>>(_a[2]))); break;
        case 14: _t->handleUdpData(); break;
        case 15: _t->broadcastPresence(); break;
        case 16: _t->cleanupOfflineUsers(); break;
        case 17: _t->retryPendingMessages(); break;
        case 18: _t->handleFileServerConnection(); break;
        case 19: _t->handleFileSocketBytesWritten((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 20: _t->handleFileSocketDisconnected(); break;
        case 21: _t->handleFileSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 22: _t->handleReceiveSocketReadyRead(); break;
        case 23: _t->handleReceiveSocketDisconnected(); break;
        case 24: _t->handleReceiveSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 21:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 24:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const UserInfo & )>(_a, &NetworkManager::userOnline, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & )>(_a, &NetworkManager::userOffline, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const UserInfo & )>(_a, &NetworkManager::userInfoUpdated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const ChatMessage & )>(_a, &NetworkManager::messageReceived, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , const ChatMessage & )>(_a, &NetworkManager::groupMessageReceived, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , const QImage & )>(_a, &NetworkManager::imageReceived, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , const QString & , quint32 , quint32 , const QString & , quint64 )>(_a, &NetworkManager::fileOfferReceived, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , quint32 , quint32 , quint64 , quint64 )>(_a, &NetworkManager::fileTransferProgress, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , quint32 , quint32 , const QString & )>(_a, &NetworkManager::fileTransferFinished, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , quint32 , quint32 , const QString & )>(_a, &NetworkManager::fileTransferFailed, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , bool )>(_a, &NetworkManager::typingIndicator, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & )>(_a, &NetworkManager::messageRead, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & )>(_a, &NetworkManager::networkError, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (NetworkManager::*)(const QString & , const QList<UserInfo> & )>(_a, &NetworkManager::groupMembersUpdated, 13))
            return;
    }
}

const QMetaObject *KylinMessenger::NetworkManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KylinMessenger::NetworkManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14KylinMessenger14NetworkManagerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int KylinMessenger::NetworkManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}

// SIGNAL 0
void KylinMessenger::NetworkManager::userOnline(const UserInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void KylinMessenger::NetworkManager::userOffline(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void KylinMessenger::NetworkManager::userInfoUpdated(const UserInfo & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void KylinMessenger::NetworkManager::messageReceived(const ChatMessage & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void KylinMessenger::NetworkManager::groupMessageReceived(const QString & _t1, const ChatMessage & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void KylinMessenger::NetworkManager::imageReceived(const QString & _t1, const QImage & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1, _t2);
}

// SIGNAL 6
void KylinMessenger::NetworkManager::fileOfferReceived(const QString & _t1, const QString & _t2, quint32 _t3, quint32 _t4, const QString & _t5, quint64 _t6)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1, _t2, _t3, _t4, _t5, _t6);
}

// SIGNAL 7
void KylinMessenger::NetworkManager::fileTransferProgress(const QString & _t1, quint32 _t2, quint32 _t3, quint64 _t4, quint64 _t5)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2, _t3, _t4, _t5);
}

// SIGNAL 8
void KylinMessenger::NetworkManager::fileTransferFinished(const QString & _t1, quint32 _t2, quint32 _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 9
void KylinMessenger::NetworkManager::fileTransferFailed(const QString & _t1, quint32 _t2, quint32 _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 10
void KylinMessenger::NetworkManager::typingIndicator(const QString & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1, _t2);
}

// SIGNAL 11
void KylinMessenger::NetworkManager::messageRead(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void KylinMessenger::NetworkManager::networkError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 12, nullptr, _t1);
}

// SIGNAL 13
void KylinMessenger::NetworkManager::groupMembersUpdated(const QString & _t1, const QList<UserInfo> & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1, _t2);
}
QT_WARNING_POP
