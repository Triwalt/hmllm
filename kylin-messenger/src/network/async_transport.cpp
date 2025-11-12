#include "network/async_transport.h"

#include <QHostAddress>
#include <QMetaObject>
#include <QtGlobal>

#include "qt_compat.h"

#include "network/constants.h"

namespace KylinMessenger::Network {

AsyncTransport::AsyncTransport(const QString& peerId, QObject* parent)
    : QObject(parent)
    , m_peerId(peerId)
    , m_socket(new QTcpSocket(this))
{
    m_socket->setObjectName(QStringLiteral("AsyncTransport/%1").arg(peerId));

    connect(m_socket.data(), &QTcpSocket::connected,
            this, &AsyncTransport::onSocketConnected);
    connect(m_socket.data(), &QTcpSocket::disconnected,
            this, &AsyncTransport::onSocketDisconnected);
    connect(m_socket.data(), &QTcpSocket::readyRead,
            this, &AsyncTransport::onSocketReadyRead);
    connect(m_socket.data(), KYLIN_QTCP_ERROR_SIGNAL,
            this, &AsyncTransport::onSocketError);

    m_heartbeatTimer.setSingleShot(false);
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &AsyncTransport::onHeartbeatTimeout);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &AsyncTransport::onReconnectTimeout);
}

void AsyncTransport::setEndpoint(const QString& host, quint16 port)
{
    if (m_host == host && m_port == port) {
        return;
    }

    m_host = host;
    m_port = port;

    if (m_socket->isOpen()) {
        m_socket->disconnectFromHost();
    }

    m_incomingBuffer.clear();

    ensureConnected();
}

void AsyncTransport::sendPacket(const NetworkPacket& packet)
{
    m_outgoing.enqueue(packet);
    ensureConnected();
    flushQueue();
}

void AsyncTransport::close()
{
    m_outgoing.clear();
    m_heartbeatTimer.stop();
    m_reconnectTimer.stop();
    m_state = State::Idle;
    m_socket->abort();
    m_incomingBuffer.clear();
}

void AsyncTransport::ensureConnected()
{
    if (m_host.isEmpty() || m_port == 0) {
        return;
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return;
    }

    if (m_connecting) {
        return;
    }

    m_connecting = true;
    m_state = State::Connecting;
    emit stateChanged(m_state);

    m_socket->connectToHost(m_host, m_port);
}

void AsyncTransport::flushQueue()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    while (!m_outgoing.isEmpty()) {
        const NetworkPacket packet = m_outgoing.dequeue();
        const QByteArray data = packet.serialize();
        if (m_socket->write(data) == -1) {
            emit errorOccurred(m_socket->errorString());
            m_outgoing.prepend(packet);
            scheduleReconnect();
            return;
        }
    }

    m_socket->flush();
}

void AsyncTransport::sendHeartbeat()
{
    NetworkPacket ping(MessageType::Ping);
    ping.setPayload(QByteArray());
    m_socket->write(ping.serialize());
    m_socket->flush();
}

void AsyncTransport::scheduleReconnect()
{
    if (m_state == State::Reconnecting) {
        return;
    }

    m_state = State::Reconnecting;
    emit stateChanged(m_state);

    const int delay = qMin(Network::kRetryBackoffBaseMs * (1 << m_reconnectAttempts), 30'000);
    m_reconnectTimer.start(delay);
    ++m_reconnectAttempts;
}

void AsyncTransport::resetReconnectDelay()
{
    m_reconnectAttempts = 0;
}

void AsyncTransport::onSocketConnected()
{
    m_connecting = false;
    resetReconnectDelay();
    m_state = State::Connected;
    emit stateChanged(m_state);

    m_lastActivity.restart();
    m_heartbeatTimer.start(m_heartbeatIntervalMs);
    flushQueue();
}

void AsyncTransport::onSocketDisconnected()
{
    m_connecting = false;
    m_heartbeatTimer.stop();
    if (!m_host.isEmpty()) {
        scheduleReconnect();
    } else {
        m_state = State::Idle;
        emit stateChanged(m_state);
    }
}

void AsyncTransport::onSocketReadyRead()
{
    m_lastActivity.restart();

    const QByteArray data = m_socket->readAll();
    if (data.isEmpty()) {
        return;
    }

    m_incomingBuffer.append(data);

    while (m_incomingBuffer.size() >= Network::HEADER_SIZE) {
        QByteArray headerBytes = m_incomingBuffer.left(Network::HEADER_SIZE);
        PacketHeader header;
        if (!header.deserialize(headerBytes) || !header.isValid()) {
            emit errorOccurred(QStringLiteral("无法解析网络数据"));
            m_incomingBuffer.clear();
            return;
        }

        if (header.payload_size > Network::MAX_PACKET_SIZE) {
            emit errorOccurred(QStringLiteral("收到过大的网络数据包"));
            m_incomingBuffer.clear();
            return;
        }

        const qint64 totalSize = Network::HEADER_SIZE + static_cast<qint64>(header.payload_size);
        if (m_incomingBuffer.size() < totalSize) {
            break;
        }

        QByteArray packetBytes = m_incomingBuffer.left(totalSize);
        m_incomingBuffer.remove(0, totalSize);

        NetworkPacket packet;
        if (!packet.deserialize(packetBytes)) {
            emit errorOccurred(QStringLiteral("无法解析网络数据"));
            continue;
        }

        const MessageType type = packet.header().message_type;
        if (type == MessageType::Pong) {
            continue;
        }

        if (type == MessageType::Ping) {
            NetworkPacket pong(MessageType::Pong);
            m_socket->write(pong.serialize());
            m_socket->flush();
            continue;
        }

        emit packetReceived(packet);
    }
}

void AsyncTransport::onSocketError(QAbstractSocket::SocketError)
{
    emit errorOccurred(m_socket->errorString());
    m_connecting = false;
    scheduleReconnect();
}

void AsyncTransport::onHeartbeatTimeout()
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    if (m_lastActivity.isValid() && m_lastActivity.elapsed() > m_idleTimeoutMs) {
        m_socket->disconnectFromHost();
        return;
    }

    sendHeartbeat();
}

void AsyncTransport::onReconnectTimeout()
{
    ensureConnected();
}

} // namespace KylinMessenger::Network

