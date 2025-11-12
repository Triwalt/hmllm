#ifndef KYLIN_MESSENGER_NETWORK_GATEWAY_CLIENT_H
#define KYLIN_MESSENGER_NETWORK_GATEWAY_CLIENT_H

#include <QAbstractSocket>
#include <QObject>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QtWebSockets/QWebSocket>

#include <memory>

#include "core/models.h"
#include "network/protocol.h"

namespace KylinMessenger::Network {

class GatewayClient : public QObject
{
    Q_OBJECT

public:
    explicit GatewayClient(QObject* parent = nullptr);
    ~GatewayClient() override;

    bool initialize(const QUrl& url, const Core::UserInfo& localUser);
    void broadcastPresence(const Core::UserInfo& userInfo);
    void sendPacket(const QString& targetId, const NetworkPacket& packet);
    void shutdown();

    bool isConnected() const { return m_socket.state() == QAbstractSocket::ConnectedState; }
    QUrl endpoint() const { return m_gatewayUrl; }

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& message);
    void presenceReceived(const Core::UserInfo& userInfo, bool viaGateway);
    void packetReceived(const QString& senderId, const NetworkPacket& packet);

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleTextMessage(const QString& message);
    void handleError(QAbstractSocket::SocketError error);
    void handleRetryTimeout();

private:
    void resetRetryDelay();
    void scheduleReconnect();
    void flushPending();
    void enqueueMessage(const QJsonObject& message);
    void sendJson(const QJsonObject& message);

    QWebSocket m_socket;
    QUrl m_gatewayUrl;
    Core::UserInfo m_localUser;
    bool m_initialized = false;
    bool m_explicitClose = false;

    QTimer m_retryTimer;
    int m_retryAttempts = 0;
    QVector<QJsonObject> m_pending;
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_GATEWAY_CLIENT_H


