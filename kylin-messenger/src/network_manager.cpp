// network_manager.cpp - P2P网络管理器实现
#include "network_manager.h"
#include "network_protocol.h"
#include <QHostInfo>
#include <QNetworkInterface>
#include <QFile>
#include <QDebug>

namespace KylinMessenger {

// ============================================================================
// NetworkManager 构造与初始化
// ============================================================================

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_udp_socket(new QUdpSocket(this))
    , m_tcp_server(new QTcpServer(this))
    , m_presence_timer(new QTimer(this))
    , m_cleanup_timer(new QTimer(this))
    , m_local_port(TCP_PORT)
{
    setupConnections();
}

NetworkManager::~NetworkManager()
{
    shutdown();
}

bool NetworkManager::initialize(const UserInfo& local_user)
{
    m_local_user = local_user;
    
    // 获取本机IP地址
    m_local_user.ip_address = getLocalIPAddress();
    m_local_user.hostname = QHostInfo::localHostName();
    
    // 绑定UDP socket用于广播
    if (!m_udp_socket->bind(UDP_PORT, QUdpSocket::ShareAddress)) {
        qCritical() << "无法绑定UDP端口" << UDP_PORT;
        return false;
    }
    
    // 启动TCP服务器
    if (!m_tcp_server->listen(QHostAddress::Any, TCP_PORT)) {
        qCritical() << "无法启动TCP服务器:" << m_tcp_server->errorString();
        return false;
    }
    
    m_local_user.port = m_tcp_server->serverPort();
    m_local_port = m_local_user.port;
    
    qInfo() << "网络管理器已初始化:"
            << "IP =" << m_local_user.ip_address
            << "UDP =" << UDP_PORT
            << "TCP =" << m_local_port;
    
    // 启动定时器
    m_presence_timer->start(PRESENCE_INTERVAL);
    m_cleanup_timer->start(CLEANUP_INTERVAL);
    
    // 发送初始在线广播
    broadcastPresence();
    
    return true;
}

void NetworkManager::shutdown()
{
    // 发送离线通知
    m_local_user.status = UserStatus::Offline;
    broadcastPresence();
    
    // 停止定时器
    m_presence_timer->stop();
    m_cleanup_timer->stop();
    
    // 关闭所有TCP连接
    for (auto* socket : m_tcp_connections.values()) {
        socket->disconnectFromHost();
        socket->deleteLater();
    }
    m_tcp_connections.clear();
    
    // 关闭服务器
    m_tcp_server->close();
    m_udp_socket->close();
    
    qInfo() << "网络管理器已关闭";
}

// ============================================================================
// 信号槽连接设置
// ============================================================================

void NetworkManager::setupConnections()
{
    // UDP数据接收
    connect(m_udp_socket, &QUdpSocket::readyRead,
            this, &NetworkManager::handleUdpData);
    
    // TCP新连接
    connect(m_tcp_server, &QTcpServer::newConnection,
            this, &NetworkManager::handleNewConnection);
    
    // 定时器
    connect(m_presence_timer, &QTimer::timeout,
            this, &NetworkManager::broadcastPresence);
    
    connect(m_cleanup_timer, &QTimer::timeout,
            this, &NetworkManager::cleanupOfflineUsers);
}

// ============================================================================
// UDP广播处理
// ============================================================================

void NetworkManager::broadcastPresence()
{
    NetworkPacket packet = NetworkPacket::createPresencePacket(m_local_user);
    QByteArray data = packet.serialize();
    
    m_udp_socket->writeDatagram(data, QHostAddress::Broadcast, UDP_PORT);
    
    qDebug() << "广播用户在线状态:" << m_local_user.username;
}

void NetworkManager::handleUdpData()
{
    while (m_udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udp_socket->pendingDatagramSize());
        
        QHostAddress sender;
        quint16 sender_port;
        
        m_udp_socket->readDatagram(datagram.data(), datagram.size(),
                                    &sender, &sender_port);
        
        // 解析数据包
        NetworkPacket packet;
        if (!packet.deserialize(datagram)) {
            qWarning() << "无法解析UDP数据包";
            continue;
        }
        
        // 处理用户在线广播
        if (packet.getHeader().message_type == MessageType::UserPresence) {
            UserInfo user_info;
            if (user_info.deserialize(packet.getPayload())) {
                handleUserPresence(user_info, sender);
            }
        }
    }
}

void NetworkManager::handleUserPresence(const UserInfo& user_info, 
                                        const QHostAddress& address)
{
    // 忽略自己的广播
    if (user_info.user_id == m_local_user.user_id) {
        return;
    }
    
    QString user_id = user_info.user_id;
    bool is_new_user = !m_online_users.contains(user_id);
    
    // 更新用户信息
    m_online_users[user_id] = user_info;
    m_user_last_seen[user_id] = QDateTime::currentDateTime();
    
    // 发出信号
    if (user_info.status == UserStatus::Offline) {
        m_online_users.remove(user_id);
        m_user_last_seen.remove(user_id);
        emit userOffline(user_id);
        qInfo() << "用户下线:" << user_info.username;
    } else if (is_new_user) {
        emit userOnline(user_info);
        qInfo() << "发现新用户:" << user_info.username 
                << "@" << user_info.ip_address;
    } else {
        emit userInfoUpdated(user_info);
    }
}

void NetworkManager::cleanupOfflineUsers()
{
    QDateTime now = QDateTime::currentDateTime();
    QStringList offline_users;
    
    for (auto it = m_user_last_seen.begin(); it != m_user_last_seen.end(); ++it) {
        qint64 seconds_since_seen = it.value().secsTo(now);
        if (seconds_since_seen > OFFLINE_TIMEOUT / 1000) {
            offline_users.append(it.key());
        }
    }
    
    for (const QString& user_id : offline_users) {
        qInfo() << "用户超时下线:" << m_online_users[user_id].username;
        m_online_users.remove(user_id);
        m_user_last_seen.remove(user_id);
        emit userOffline(user_id);
    }
}

// ============================================================================
// TCP消息发送
// ============================================================================

bool NetworkManager::sendMessage(const QString& receiver_id, 
                                 const ChatMessage& message)
{
    if (!m_online_users.contains(receiver_id)) {
        qWarning() << "用户不在线:" << receiver_id;
        return false;
    }
    
    const UserInfo& receiver = m_online_users[receiver_id];
    NetworkPacket packet = NetworkPacket::createChatMessagePacket(message);
    
    return sendTcpPacket(receiver.ip_address, receiver.port, packet);
}

bool NetworkManager::sendGroupMessage(const QString& group_id, 
                                      const ChatMessage& message)
{
    bool all_sent = true;
    
    // 向所有在线用户发送组消息
    for (const UserInfo& user : m_online_users.values()) {
        if (user.group_name == group_id) {
            NetworkPacket packet = NetworkPacket::createGroupMessagePacket(
                group_id, message);
            
            if (!sendTcpPacket(user.ip_address, user.port, packet)) {
                all_sent = false;
            }
        }
    }
    
    return all_sent;
}

bool NetworkManager::sendBroadcastMessage(const ChatMessage& message)
{
    bool all_sent = true;
    
    for (const UserInfo& user : m_online_users.values()) {
        if (!sendMessage(user.user_id, message)) {
            all_sent = false;
        }
    }
    
    return all_sent;
}

bool NetworkManager::sendTypingIndicator(const QString& receiver_id, 
                                         bool is_typing)
{
    if (!m_online_users.contains(receiver_id)) {
        return false;
    }
    
    const UserInfo& receiver = m_online_users[receiver_id];
    NetworkPacket packet = NetworkPacket::createTypingIndicatorPacket(
        m_local_user.user_id, is_typing);
    
    return sendTcpPacket(receiver.ip_address, receiver.port, packet);
}

bool NetworkManager::sendReadReceipt(const QString& receiver_id, 
                                     const QString& message_id)
{
    if (!m_online_users.contains(receiver_id)) {
        return false;
    }
    
    const UserInfo& receiver = m_online_users[receiver_id];
    NetworkPacket packet = NetworkPacket::createReadReceiptPacket(message_id);
    
    return sendTcpPacket(receiver.ip_address, receiver.port, packet);
}

// ============================================================================
// TCP连接管理
// ============================================================================

bool NetworkManager::sendTcpPacket(const QString& host, 
                                   quint16 port, 
                                   const NetworkPacket& packet)
{
    QTcpSocket* socket = new QTcpSocket(this);
    
    // 连接信号
    connect(socket, &QTcpSocket::connected, this, [=]() {
        QByteArray data = packet.serialize();
        socket->write(data);
        socket->flush();
    });
    
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, [=](QAbstractSocket::SocketError error) {
        qWarning() << "TCP发送错误:" << socket->errorString();
        socket->deleteLater();
    });
    
    // 连接到目标
    socket->connectToHost(host, port);
    
    return socket->waitForConnected(3000);
}

void NetworkManager::handleNewConnection()
{
    while (m_tcp_server->hasPendingConnections()) {
        QTcpSocket* socket = m_tcp_server->nextPendingConnection();
        
        QString key = QString("%1:%2")
            .arg(socket->peerAddress().toString())
            .arg(socket->peerPort());
        
        m_tcp_connections[key] = socket;
        
        connect(socket, &QTcpSocket::readyRead,
                this, [=]() { handleTcpData(socket); });
        
        connect(socket, &QTcpSocket::disconnected,
                this, [=]() {
            m_tcp_connections.remove(key);
            socket->deleteLater();
        });
        
        qDebug() << "新TCP连接:" << key;
    }
}

void NetworkManager::handleTcpData(QTcpSocket* socket)
{
    QByteArray data = socket->readAll();
    
    NetworkPacket packet;
    if (!packet.deserialize(data)) {
        qWarning() << "无法解析TCP数据包";
        return;
    }
    
    // 根据消息类型分发
    switch (packet.getHeader().message_type) {
        case MessageType::ChatMessage:
            handleChatMessage(packet);
            break;
            
        case MessageType::GroupMessage:
            handleGroupMessage(packet);
            break;
            
        case MessageType::FileOffer:
            handleFileOffer(packet, socket);
            break;
            
        case MessageType::TypingIndicator:
            handleTypingIndicator(packet);
            break;
            
        case MessageType::ReadReceipt:
            handleReadReceipt(packet);
            break;
            
        default:
            qWarning() << "未知的消息类型:" 
                       << static_cast<int>(packet.getHeader().message_type);
            break;
    }
}

// ============================================================================
// 消息处理
// ============================================================================

void NetworkManager::handleChatMessage(const NetworkPacket& packet)
{
    ChatMessage message;
    if (!message.deserialize(packet.getPayload())) {
        qWarning() << "无法解析聊天消息";
        return;
    }
    
    emit messageReceived(message);
    
    // 如果是图片消息，提取图片
    if (message.message_type == MessageContentType::Image) {
        QByteArray image_data = QByteArray::fromBase64(message.content.toUtf8());
        QImage image;
        if (image.loadFromData(image_data)) {
            emit imageReceived(message.sender_id, image);
        }
    }
}

void NetworkManager::handleGroupMessage(const NetworkPacket& packet)
{
    QDataStream stream(packet.getPayload());
    stream.setVersion(QDataStream::Qt_6_0);
    
    QString group_id;
    QByteArray message_data;
    
    stream >> group_id;
    stream >> message_data;
    
    ChatMessage message;
    if (message.deserialize(message_data)) {
        emit groupMessageReceived(group_id, message);
    }
}

void NetworkManager::handleFileOffer(const NetworkPacket& packet, 
                                     QTcpSocket* socket)
{
    QDataStream stream(packet.getPayload());
    stream.setVersion(QDataStream::Qt_6_0);
    
    QString filename;
    quint64 filesize;
    QString file_hash;
    
    stream >> filename;
    stream >> filesize;
    stream >> file_hash;
    
    emit fileOfferReceived(
        socket->peerAddress().toString(),
        filename,
        filesize,
        file_hash
    );
}

void NetworkManager::handleTypingIndicator(const NetworkPacket& packet)
{
    QDataStream stream(packet.getPayload());
    stream.setVersion(QDataStream::Qt_6_0);
    
    QString user_id;
    bool is_typing;
    
    stream >> user_id;
    stream >> is_typing;
    
    emit typingIndicator(user_id, is_typing);
}

void NetworkManager::handleReadReceipt(const NetworkPacket& packet)
{
    QDataStream stream(packet.getPayload());
    stream.setVersion(QDataStream::Qt_6_0);
    
    QString message_id;
    stream >> message_id;
    
    emit messageRead(message_id);
}

// ============================================================================
// 文件传输
// ============================================================================

bool NetworkManager::sendFile(const QString& receiver_id, 
                              const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filepath;
        return false;
    }
    
    QFileInfo file_info(filepath);
    QString filename = file_info.fileName();
    quint64 filesize = file_info.size();
    
    // 计算文件哈希
    QByteArray file_data = file.readAll();
    QString file_hash = QString::number(
        qHash(file_data), 16);
    
    // 发送文件提供通知
    if (!m_online_users.contains(receiver_id)) {
        return false;
    }
    
    const UserInfo& receiver = m_online_users[receiver_id];
    NetworkPacket offer_packet = NetworkPacket::createFileOfferPacket(
        filename, filesize, file_hash);
    
    if (!sendTcpPacket(receiver.ip_address, receiver.port, offer_packet)) {
        return false;
    }
    
    // TODO: 实现实际的文件传输逻辑
    // 这里需要建立文件传输会话，分块发送
    
    return true;
}

// ============================================================================
// 工具函数
// ============================================================================

QString NetworkManager::getLocalIPAddress() const
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    
    for (const QHostAddress& address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            !address.isLoopback()) {
            return address.toString();
        }
    }
    
    return "127.0.0.1";
}

QList<UserInfo> NetworkManager::getOnlineUsers() const
{
    return m_online_users.values();
}

UserInfo NetworkManager::getLocalUser() const
{
    return m_local_user;
}

void NetworkManager::updateLocalUserStatus(UserStatus status, 
                                           const QString& status_text)
{
    m_local_user.status = status;
    m_local_user.status_text = status_text;
    broadcastPresence();
}

} // namespace KylinMessenger
