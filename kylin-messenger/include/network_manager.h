/**
 * @file network_manager.h
 * @brief Network Manager for P2P Communication
 * 
 * Handles all network operations including:
 * - UDP broadcasting for user discovery
 * - TCP connections for reliable messaging
 * - File transfer management
 * 
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "network_protocol.h"
#include <QObject>
#include <QDateTime>
#include <QHostAddress>
#include <QImage>
#include <QList>
#include <QMap>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>
#include <string>

namespace KylinMessenger {

class NetworkManager : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;
    
    bool initialize(const UserInfo& user_info);
    void shutdown();
    
    UserInfo getLocalUser() const;
    QList<UserInfo> getOnlineUsers() const;
    
    bool sendMessage(const QString& receiver_id, const ChatMessage& message);
    bool sendGroupMessage(const QString& group_id, const ChatMessage& message);
    bool sendBroadcastMessage(const ChatMessage& message);
    bool sendFile(const QString& receiver_id, const QString& filepath);
    bool sendTypingIndicator(const QString& receiver_id, bool is_typing);
    bool sendReadReceipt(const QString& receiver_id, const QString& message_id);
    void updateLocalUserStatus(UserStatus status, const QString& status_text);
    void updateLocalUserStatus(UserStatus status, const std::string& status_text);
    
signals:
    void userOnline(const UserInfo& user);
    void userOffline(const QString& userId);
    void userInfoUpdated(const UserInfo& user);
    void messageReceived(const ChatMessage& message);
    void groupMessageReceived(const QString& group_id, const ChatMessage& message);
    void imageReceived(const QString& senderId, const QImage& image);
    void fileOfferReceived(const QString& senderIp,
                           const QString& filename,
                           quint64 filesize,
                           const QString& file_hash);
    void typingIndicator(const QString& userId, bool isTyping);
    void messageRead(const QString& messageId);
    void networkError(const QString& error);
    
private slots:
    void handleUdpData();
    void handleNewConnection();
    void broadcastPresence();
    void cleanupOfflineUsers();
    
private:
    void setupConnections();
    void handleUserPresence(const UserInfo& user_info, const QHostAddress& address);
    void handleTcpData(QTcpSocket* socket);
    void handleChatMessage(const NetworkPacket& packet);
    void handleGroupMessage(const NetworkPacket& packet);
    void handleFileOffer(const NetworkPacket& packet, QTcpSocket* socket);
    void handleTypingIndicator(const NetworkPacket& packet);
    void handleReadReceipt(const NetworkPacket& packet);
    bool sendTcpPacket(const QString& host, quint16 port, const NetworkPacket& packet);
    QString getLocalIPAddress() const;

private:
    UserInfo m_local_user;
    QUdpSocket* m_udp_socket;
    QTcpServer* m_tcp_server;
    QTimer* m_presence_timer;
    QTimer* m_cleanup_timer;
    quint16 m_local_port;
    QMap<QString, UserInfo> m_online_users;
    QMap<QString, QTcpSocket*> m_tcp_connections;
    QMap<QString, QDateTime> m_user_last_seen;

    static constexpr int PRESENCE_INTERVAL = 5000;
    static constexpr int OFFLINE_TIMEOUT = 15000;
    static constexpr int CLEANUP_INTERVAL = 3000;
};

} // namespace KylinMessenger

#endif // NETWORK_MANAGER_H
