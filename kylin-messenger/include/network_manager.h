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
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>
#include <QHostAddress>

namespace KylinMessenger {

/**
 * @brief Network manager class handling all network operations
 */
class NetworkManager : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;
    
    /**
     * @brief Initialize network services
     * @param userInfo Current user information
     * @return true if successful
     */
    bool initialize(const UserInfo& userInfo);
    
    /**
     * @brief Shutdown network services
     */
    void shutdown();
    
    /**
     * @brief Get current user info
     */
    UserInfo getCurrentUser() const { return currentUser_; }
    
    /**
     * @brief Update current user info
     */
    void updateCurrentUser(const UserInfo& userInfo);
    
    /**
     * @brief Get list of online users
     */
    QList<UserInfo> getOnlineUsers() const;
    
    /**
     * @brief Send text message to user
     */
    bool sendMessage(const QString& userId, const QString& message);
    
    /**
     * @brief Send message to group
     */
    bool sendGroupMessage(const QString& groupId, const QString& message);
    
    /**
     * @brief Broadcast message to all users
     */
    bool broadcastMessage(const QString& message);
    
    /**
     * @brief Send image to user
     */
    bool sendImage(const QString& userId, const QImage& image);
    
    /**
     * @brief Send file to user
     */
    bool sendFile(const QString& userId, const QString& filePath);
    
    /**
     * @brief Send typing indicator
     */
    void sendTypingIndicator(const QString& userId, bool isTyping);
    
signals:
    /**
     * @brief Emitted when a new user comes online
     */
    void userOnline(const UserInfo& user);
    
    /**
     * @brief Emitted when a user goes offline
     */
    void userOffline(const QString& userId);
    
    /**
     * @brief Emitted when user info is updated
     */
    void userUpdated(const UserInfo& user);
    
    /**
     * @brief Emitted when a message is received
     */
    void messageReceived(const ChatMessage& message);
    
    /**
     * @brief Emitted when an image is received
     */
    void imageReceived(const QString& senderId, const QImage& image);
    
    /**
     * @brief Emitted when file transfer is requested
     */
    void fileTransferRequested(const QString& senderId, 
                              const QString& filename, 
                              qint64 filesize);
    
    /**
     * @brief Emitted when typing indicator is received
     */
    void typingIndicator(const QString& userId, bool isTyping);
    
    /**
     * @brief Emitted on network error
     */
    void networkError(const QString& error);
    
private slots:
    /**
     * @brief Handle incoming UDP datagram
     */
    void handleUdpDatagram();
    
    /**
     * @brief Handle new TCP connection
     */
    void handleNewConnection();
    
    /**
     * @brief Handle TCP data ready
     */
    void handleTcpDataReady();
    
    /**
     * @brief Handle TCP disconnection
     */
    void handleTcpDisconnected();
    
    /**
     * @brief Broadcast presence announcement
     */
    void broadcastPresence();
    
    /**
     * @brief Check for offline users
     */
    void checkOfflineUsers();
    
private:
    /**
     * @brief Send UDP packet
     */
    bool sendUdpPacket(const NetworkPacket& packet, 
                      const QHostAddress& address = QHostAddress::Broadcast);
    
    /**
     * @brief Send TCP packet to user
     */
    bool sendTcpPacket(const QString& userId, const NetworkPacket& packet);
    
    /**
     * @brief Get or create TCP connection to user
     */
    QTcpSocket* getConnection(const QString& userId);
    
    /**
     * @brief Process received packet
     */
    void processPacket(const NetworkPacket& packet, const QHostAddress& sender);
    
    /**
     * @brief Update user in online list
     */
    void updateUser(const UserInfo& user);
    
    /**
     * @brief Remove user from online list
     */
    void removeUser(const QString& userId);
    
private:
    // Current user
    UserInfo currentUser_;
    
    // UDP for discovery
    QUdpSocket* udpSocket_;
    QTimer* presenceTimer_;
    QTimer* cleanupTimer_;
    
    // TCP for messaging
    QTcpServer* tcpServer_;
    QMap<QString, QTcpSocket*> connections_;  // userId -> socket
    QMap<QTcpSocket*, QByteArray> receiveBuffers_;
    
    // Online users
    QMap<QString, UserInfo> onlineUsers_;
    
    // Constants
    static constexpr int PRESENCE_INTERVAL = 5000;  // 5 seconds
    static constexpr int OFFLINE_TIMEOUT = 15000;   // 15 seconds
};

} // namespace KylinMessenger

#endif // NETWORK_MANAGER_H
