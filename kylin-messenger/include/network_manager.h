/**
 * @file network_manager.h
 * @brief Network Manager for IPMSG/FeiQ P2P Communication
 * 
 * Handles all network operations using IPMSG protocol:
 * - UDP broadcasting for user discovery
 * - UDP messaging with ACK retry
 * - File transfer management (planned)
 * 
 * @version 2.0.0
 * @date 2025-01-XX
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "network_protocol.h"
#include "network/constants.h"
#include "network/ipmsg.h"
#include <QObject>
#include <QDateTime>
#include <QHostAddress>
#include <QImage>
#include <QByteArray>
#include <QList>
#include <QHash>
#include <QMap>
#include <QString>
#include <QQueue>
#include <QVector>
#include <QSet>
#include <QTimer>
#include <QUdpSocket>
#include <QAbstractSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QPair>
#include <QSharedPointer>
#include <string>
#include <memory>

namespace KylinMessenger {

class NetworkManager : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager() override;
    
    bool initialize(const UserInfo& user_info);
    void shutdown();
    
    UserInfo getLocalUser() const;
    QString getLocalUserId() const { return m_local_user.user_id; }
    QList<UserInfo> getOnlineUsers() const;
    void updateLocalFeiqDetails(const QMap<QString, QString>& details);
    QMap<QString, QString> getUserDetails(const QString& userId) const;
    
    bool sendMessage(const QString& receiver_id, const ChatMessage& message);
    bool sendGroupMessage(const QString& group_id, const ChatMessage& message);
    bool sendBroadcastMessage(const ChatMessage& message);
    bool sendFile(const QString& receiver_id,
                  const QString& filepath,
                  quint32* out_packet_no = nullptr,
                  quint32* out_file_id = nullptr);
    void registerLoopbackPeer(const UserInfo& user_info);
    bool sendTypingIndicator(const QString& receiver_id, bool is_typing);
    bool sendReadReceipt(const QString& receiver_id, const QString& message_id);
    bool acceptFile(const QString& sender_id,
                    quint32 packet_no,
                    quint32 file_id,
                    const QString& save_path);
    void updateLocalUserStatus(UserStatus status, const QString& status_text);
    void updateLocalUserStatus(UserStatus status, const std::string& status_text);
    
signals:
    void userOnline(const UserInfo& user);
    void userOffline(const QString& userId);
    void userInfoUpdated(const UserInfo& user);
    void messageReceived(const ChatMessage& message);
    void groupMessageReceived(const QString& group_id, const ChatMessage& message);
    void imageReceived(const QString& senderId, const QImage& image);
    void fileOfferReceived(const QString& senderId,
                           const QString& senderIp,
                           quint32 packetNo,
                           quint32 fileId,
                           const QString& filename,
                           quint64 filesize);
    void fileTransferProgress(const QString& peerId,
                              quint32 packetNo,
                              quint32 fileId,
                              quint64 bytesTransferred,
                              quint64 totalBytes);
    void fileTransferFinished(const QString& peerId,
                              quint32 packetNo,
                              quint32 fileId,
                              const QString& savePath);
    void fileTransferFailed(const QString& peerId,
                            quint32 packetNo,
                            quint32 fileId,
                            const QString& reason);
    void typingIndicator(const QString& userId, bool isTyping);
    void messageRead(const QString& messageId);
    void networkError(const QString& error);
    
private slots:
    void handleUdpData();
    void broadcastPresence();
    void cleanupOfflineUsers();
    void retryPendingMessages();
    void handleFileServerConnection();
    void handleFileSocketBytesWritten(qint64 bytes);
    void handleFileSocketDisconnected();
    void handleFileSocketError(QAbstractSocket::SocketError error);
    void handleReceiveSocketReadyRead();
    void handleReceiveSocketDisconnected();
    void handleReceiveSocketError(QAbstractSocket::SocketError error);
    
private:
    struct PendingMessage;
    struct SharedFileEntry;
    struct IncomingFileOffer;
    struct PendingSendTransfer;
    struct ActiveSendTransfer;
    struct ActiveReceiveTransfer;

    void setupConnections();
    bool sendIpMsg(const QHostAddress& target,
                   quint16 port,
                   const Network::IPMSG::Packet& packet);
    void sendBroadcastCommand(Network::IPMSG::Command command,
                              const QString& additional = QString());
    QString buildLocalPresenceAdditional() const;
    void handleIpMsgPacket(const Network::IPMSG::Packet& packet,
                           const QHostAddress& sender,
                           quint16 sender_port);
    void handlePresencePacket(const Network::IPMSG::Packet& packet,
                              const QHostAddress& sender,
                              bool is_answer);
    void handleSendMessagePacket(const Network::IPMSG::Packet& packet,
                                 const QHostAddress& sender,
                                 quint16 sender_port);
    void handleRecvMessagePacket(const Network::IPMSG::Packet& packet);
    void handleExitPacket(const Network::IPMSG::Packet& packet,
                          const QHostAddress& sender);
    QString makeUserId(const QString& username, const QHostAddress& address) const;
    void upsertUser(const QString& user_id,
                    UserInfo user_info,
                    const QHostAddress& address);
    void removeUser(const QString& user_id);
    void refreshLocalAddresses();
    bool isLocalPacket(const Network::IPMSG::Packet& packet,
                      const QHostAddress& sender) const;
    QString getLocalIPAddress() const;
    QVector<QHostAddress> getBroadcastAddresses() const;
    quint32 nextPacketNo();
    quint32 nextFileId();
    quint64 makeFileKey(quint32 packetNo, quint32 fileId) const;
    void setupFileServer();
    void scheduleFileSend(const QString& receiverId,
                          const QHostAddress& receiverAddr,
                          quint32 packetNo,
                          quint32 fileId,
                          const QString& filepath,
                          quint64 filesize,
                          quint32 fileAttr,
                          quint32 fileMtime);
    void processPendingSend(QTcpSocket* socket);
    void pumpFileSocket(QTcpSocket* socket);
    void finalizeSendTransfer(QTcpSocket* socket, bool success, const QString& reason = QString());
    void finalizeReceiveTransfer(QTcpSocket* socket, bool success, const QString& reason = QString());
    QList<IncomingFileOffer> handleFileAttachments(const QString& senderId,
                                                  const QHostAddress& senderAddress,
                                                  quint32 packetNo,
                                                  const QString& attachmentsPart);
    bool sendGetFileData(const QHostAddress& target,
                         quint32 packetNo,
                         quint32 fileId,
                         quint64 offset = 0);
    void sendReleaseFiles(const QHostAddress& target,
                          quint32 packetNo,
                          quint32 fileId);
    void handleGetFileDataPacket(const Network::IPMSG::Packet& packet,
                                 const QHostAddress& sender);
    void handleReleaseFilePacket(const Network::IPMSG::Packet& packet,
                                 const QHostAddress& sender);

    UserInfo m_local_user;
    QUdpSocket* m_udp_socket = nullptr;
    QTimer* m_presence_timer = nullptr;
    QTimer* m_cleanup_timer = nullptr;
    QTimer* m_retry_timer = nullptr;
    QTcpServer* m_file_server = nullptr;
    QMap<QString, UserInfo> m_online_users;
    QMap<QString, QDateTime> m_user_last_seen;
    QHash<QString, QMap<QString, QString>> m_user_details;

    struct PendingMessage {
        ChatMessage message;
        Network::IPMSG::Packet packet;
        QHostAddress target_address;
        quint16 target_port = Network::kUdpDiscoveryPort;
        QDateTime timestamp;
        int retries = 0;
    };

    QHash<quint32, PendingMessage> m_pending_messages;
    QSet<QString> m_local_addresses;
    quint32 m_packet_seed = 0;
    QVector<QHostAddress> m_broadcast_addresses;
    QMap<QString, QString> m_local_feiq_details;

    struct SharedFileEntry {
        QString peer_id;
        QHostAddress peer_address;
        QString file_path;
        QString file_name;
        quint32 packet_no = 0;
        quint32 file_id = 0;
        quint64 file_size = 0;
        quint32 file_attr = 0;
        quint32 file_mtime = 0;
    };

    struct IncomingFileOffer {
        QString sender_id;
        QHostAddress sender_address;
        quint32 packet_no = 0;
        quint32 file_id = 0;
        QString file_name;
        quint64 file_size = 0;
        quint32 file_attr = 0;
        quint32 file_mtime = 0;
    };

    struct PendingSendTransfer {
        SharedFileEntry file;
        quint64 offset = 0;
    };

    struct ActiveSendTransfer {
        SharedFileEntry file;
        QSharedPointer<QFile> handle;
        quint64 bytes_sent = 0;
        quint64 offset = 0;
    };

    struct ActiveReceiveTransfer {
        IncomingFileOffer offer;
        QString save_path;
        QSharedPointer<QFile> handle;
        quint64 bytes_received = 0;
        quint64 expected_size = 0;
    };

    QHash<quint64, SharedFileEntry> m_shared_files;
    QHash<QString, QList<IncomingFileOffer>> m_incoming_file_offers;
    QHash<quint64, IncomingFileOffer> m_incoming_file_index;
    QHash<QString, QList<PendingSendTransfer>> m_pending_send_transfers;
    QHash<QTcpSocket*, ActiveSendTransfer> m_active_send_transfers;
    QHash<QTcpSocket*, ActiveReceiveTransfer> m_active_receive_transfers;
    quint32 m_file_id_seed = 0;

    QSet<QString> m_loopback_peer_ids;
    QSet<QString> m_loopback_addresses;
    QHash<QString, QString> m_loopback_address_map;

    static constexpr int PRESENCE_INTERVAL = Network::kPresenceIntervalMs;
    static constexpr int OFFLINE_TIMEOUT = Network::kOfflineTimeoutMs;
    static constexpr int CLEANUP_INTERVAL = Network::kCleanupIntervalMs;
    static constexpr int MESSAGE_ACK_TIMEOUT_MS = 5'000;
    static constexpr int MESSAGE_MAX_RETRY = 3;
};

} // namespace KylinMessenger

#endif // NETWORK_MANAGER_H
