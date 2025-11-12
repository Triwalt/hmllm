#ifndef KYLIN_MESSENGER_NETWORK_ASYNC_TRANSPORT_H
#define KYLIN_MESSENGER_NETWORK_ASYNC_TRANSPORT_H

#include <QAbstractSocket>
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QScopedPointer>
#include <QTcpSocket>
#include <QTimer>

#include "network/protocol.h"

namespace KylinMessenger::Network {

class AsyncTransport : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Connecting,
        Connected,
        Reconnecting
    };
    Q_ENUM(State)

    explicit AsyncTransport(const QString& peerId, QObject* parent = nullptr);
    ~AsyncTransport() override = default;

    void setEndpoint(const QString& host, quint16 port);
    void sendPacket(const NetworkPacket& packet);
    void close();

    QString peerId() const { return m_peerId; }
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    State state() const { return m_state; }

signals:
    void packetReceived(const NetworkPacket& packet);
    void stateChanged(State state);
    void errorOccurred(const QString& message);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout();
    void onReconnectTimeout();

private:
    void ensureConnected();
    void flushQueue();
    void sendHeartbeat();
    void scheduleReconnect();
    void resetReconnectDelay();

    QString m_peerId;
    QString m_host;
    quint16 m_port = 0;

    QScopedPointer<QTcpSocket> m_socket;
    QQueue<NetworkPacket> m_outgoing;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    QElapsedTimer m_lastActivity;
    QByteArray m_incomingBuffer;

    State m_state = State::Idle;
    int m_reconnectAttempts = 0;
    bool m_connecting = false;

    const int m_heartbeatIntervalMs = 10'000;
    const int m_idleTimeoutMs = 30'000;
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_ASYNC_TRANSPORT_H

