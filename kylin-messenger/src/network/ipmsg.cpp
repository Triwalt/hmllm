#include "network/ipmsg.h"
#include "qt_compat.h"

#include <QStringList>

namespace KylinMessenger::Network::IPMSG {

QString buildDatagram(const Packet& packet)
{
    QStringList parts;
    parts << QString::number(packet.version)
          << QString::number(packet.packet_no)
          << packet.sender_name
          << packet.sender_host
          << QString::number(packet.command)
          << packet.additional;
    return parts.join(QLatin1Char(':'));
}

bool parseDatagram(const QByteArray& payload, Packet& packet)
{
    const QString text = QString::fromUtf8(payload);
    QStringList parts = text.split(QLatin1Char(':'), KYLIN_SPLIT_KEEP_EMPTY);
    if (parts.size() < 6) {
        return false;
    }

    bool ok = false;
    packet.version = parts.at(0).toUInt(&ok);
    if (!ok) {
        return false;
    }

    packet.packet_no = parts.at(1).toUInt(&ok);
    if (!ok) {
        return false;
    }

    packet.sender_name = parts.at(2);
    packet.sender_host = parts.at(3);

    packet.command = parts.at(4).toUInt(&ok);
    if (!ok) {
        return false;
    }

    packet.additional = parts.mid(5).join(QLatin1Char(':'));
    return true;
}

} // namespace KylinMessenger::Network::IPMSG



