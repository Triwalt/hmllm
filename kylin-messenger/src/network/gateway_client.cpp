#include "network/gateway_client.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketProtocol>

#include "network/constants.h"
#include "network/protocol_adapter.h"

namespace {
Q_LOGGING_CATEGORY(kGatewayLog, "kylin.gateway");

constexpr int kMaxBackoffMs = 30'000;
} // namespace

namespace KylinMessenger::Network {

GatewayClient::GatewayClient(QObject* parent)
    : QObject(parent)
{
    m_socket.setObjectName(QStringLiteral("GatewayClient"));
    connect(&m_socket, &QWebSocket::connected,
            this, &GatewayClient::handleConnected);
    connect(&m_socket, &QWebSocket::disconnected,
            this, &GatewayClient::handleDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &GatewayClient::handleTextMessage);
    connect(&m_socket, &QWebSocket::errorOccurred,
            this, &GatewayClient::handleError);

    m_retryTimer.setSingleShot(true);
    connect(&m_retryTimer, &QTimer::timeout,
            this, &GatewayClient::handleRetryTimeout);
}

GatewayClient::~GatewayClient()
{
    shutdown();
}

bool GatewayClient::initialize(const QUrl& url, const Core::UserInfo& localUser)
{
    if (!url.isValid()) {
        emit errorOccurred(tr("无效的网关地址: %1").arg(url.toString()));
        return false;
    }

    m_gatewayUrl = url;
    m_localUser = localUser;
    m_initialized = true;
    m_explicitClose = false;

    qCInfo(kGatewayLog) << "连接跨网关服务:" << url;
    m_socket.open(m_gatewayUrl);
    return true;
}

void GatewayClient::broadcastPresence(const Core::UserInfo& userInfo)
{
    NetworkPacket packet = NetworkPacket::createPresencePacket(userInfo);

    QJsonObject message{
        {QStringLiteral("type"), QStringLiteral("presence")},
        {QStringLiteral("payload"), QString::fromLatin1(packet.serialize().toBase64())}
    };

    enqueueMessage(message);
}

void GatewayClient::sendPacket(const QString& targetId, const NetworkPacket& packet)
{
    QJsonObject message{
        {QStringLiteral("type"), QStringLiteral("packet")},
        {QStringLiteral("target"), targetId},
        {QStringLiteral("payload"), QString::fromLatin1(packet.serialize().toBase64())}
    };

    enqueueMessage(message);
}

void GatewayClient::shutdown()
{
    m_explicitClose = true;
    m_retryTimer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close(QWebSocketProtocol::CloseCodeNormal, QStringLiteral("client shutdown"));
    }
    m_pending.clear();
}

void GatewayClient::handleConnected()
{
    qCInfo(kGatewayLog) << "跨网关连接成功";
    resetRetryDelay();
    flushPending();
    broadcastPresence(m_localUser);
    emit connected();
}

void GatewayClient::handleDisconnected()
{
    qCWarning(kGatewayLog) << "跨网关连接断开";
    emit disconnected();
    if (!m_explicitClose && m_initialized) {
        scheduleReconnect();
    }
}

void GatewayClient::handleTextMessage(const QString& message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        emit errorOccurred(tr("网关返回了无法解析的数据"));
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("ping")) {
        sendJson(QJsonObject{{QStringLiteral("type"), QStringLiteral("pong")}});
        return;
    }

    const QByteArray payload = QByteArray::fromBase64(obj.value(QStringLiteral("payload")).toString().toLatin1());
    if (payload.isEmpty()) {
        qCWarning(kGatewayLog) << "网关消息缺少有效载荷";
        return;
    }

    if (type == QStringLiteral("presence")) {
        Core::UserInfo info;
        bool decoded = info.deserialize(payload);
#ifdef HAVE_PROTOBUF
        if (!decoded) {
            decoded = Network::ProtocolAdapter::decodePresence(payload, info);
        }
#endif
        if (!decoded) {
            qCWarning(kGatewayLog) << "无法解码网关提供的用户信息";
            return;
        }

        const bool viaGateway = obj.value(QStringLiteral("viaGateway")).toBool(true);
        emit presenceReceived(info, viaGateway);
        return;
    }

    if (type == QStringLiteral("packet")) {
        NetworkPacket packet;
        if (!packet.deserialize(payload)) {
            qCWarning(kGatewayLog) << "无法解码网关提供的数据包";
            return;
        }

        if (packet.header().message_type == MessageType::UserPresence) {
            Core::UserInfo info;
            bool decoded = info.deserialize(packet.payload());
#ifdef HAVE_PROTOBUF
            if (!decoded) {
                decoded = Network::ProtocolAdapter::decodePresence(packet.payload(), info);
            }
#endif
            if (decoded) {
                emit presenceReceived(info, true);
            }
            return;
        }

        const QString senderId = obj.value(QStringLiteral("sender")).toString();
        emit packetReceived(senderId, packet);
    }
}

void GatewayClient::handleError(QAbstractSocket::SocketError)
{
    emit errorOccurred(m_socket.errorString());
    if (!m_explicitClose) {
        scheduleReconnect();
    }
}

void GatewayClient::handleRetryTimeout()
{
    qCInfo(kGatewayLog) << "重连跨网关服务" << m_gatewayUrl;
    m_socket.open(m_gatewayUrl);
}

void GatewayClient::resetRetryDelay()
{
    m_retryAttempts = 0;
    m_retryTimer.stop();
}

void GatewayClient::scheduleReconnect()
{
    if (!m_initialized) {
        return;
    }

    const int delay = qMin(kRetryBackoffBaseMs * (1 << m_retryAttempts), kMaxBackoffMs);
    ++m_retryAttempts;
    qCInfo(kGatewayLog) << "将在" << delay << "ms 后尝试重新连接";
    m_retryTimer.start(delay);
}

void GatewayClient::flushPending()
{
    if (!isConnected()) {
        return;
    }

    for (const auto& message : std::as_const(m_pending)) {
        sendJson(message);
    }
    m_pending.clear();
}

void GatewayClient::enqueueMessage(const QJsonObject& message)
{
    if (isConnected()) {
        sendJson(message);
    } else {
        m_pending.append(message);
        if (m_socket.state() == QAbstractSocket::UnconnectedState && m_initialized && !m_explicitClose) {
            m_socket.open(m_gatewayUrl);
        }
    }
}

void GatewayClient::sendJson(const QJsonObject& message)
{
    const QJsonDocument doc(message);
    const QString serialized = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    m_socket.sendTextMessage(serialized);
}

} // namespace KylinMessenger::Network


