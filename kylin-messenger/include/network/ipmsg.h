#ifndef KYLIN_MESSENGER_NETWORK_IPMSG_H
#define KYLIN_MESSENGER_NETWORK_IPMSG_H

#include <QByteArray>
#include <QHostAddress>
#include <QString>

namespace KylinMessenger::Network::IPMSG {

constexpr quint32 kVersion = 0x0001;

// 基本命令
enum Command : quint32 {
    BR_ENTRY = 0x00000001,
    BR_EXIT = 0x00000002,
    ANS_ENTRY = 0x00000003,
    BR_ABSENCE = 0x00000004,

    SENDMSG = 0x00000020,
    RECVMSG = 0x00000021,
    READMSG = 0x00000030,
    DELMSG = 0x00000031,

    GETINFO = 0x00000040,
    SENDINFO = 0x00000041,

        GETFILEDATA = 0x00000060,
        RELEASEFILES = 0x00000061,

    GET_ABSENCE_INFO = 0x00000050,
    SEND_ABSENCE_INFO = 0x00000051
};

// 选项位
enum Option : quint32 {
    SENDCHECK = 0x00000100,
    SECRETOPT = 0x00000200,
    BROADCASTOPT = 0x00000400,
    ABSENCEOPT = 0x00000800,
    SERVEROPT = 0x00001000,
    DIALUPOPT = 0x00002000,
    FILEATTACHOPT = 0x00200000,
    ENCRYPTOPT = 0x00400000
};

struct Packet {
    quint32 version = kVersion;
    quint32 packet_no = 0;
    QString sender_name;
    QString sender_host;
    quint32 command = 0;
    QString additional;

    bool hasOption(Option opt) const { return command & static_cast<quint32>(opt); }
    quint32 pureCommand() const { return command & 0x000000ff; }
};

QString buildDatagram(const Packet& packet);
bool parseDatagram(const QByteArray& payload, Packet& packet);

} // namespace KylinMessenger::Network::IPMSG

#endif // KYLIN_MESSENGER_NETWORK_IPMSG_H



