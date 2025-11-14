/**
 * @file lightweight_discovery.h
 * @brief 智能网络发现服务
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_NETWORK_LIGHTWEIGHT_DISCOVERY_H
#define KYLIN_MESSENGER_NETWORK_LIGHTWEIGHT_DISCOVERY_H

#include "core/micro_kernel.h"
#include "core/models.h"
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <unordered_map>
#include <memory>

namespace KylinMessenger::Network {

/**
 * @brief 智能网络发现服务
 * 基于IPMSG协议，使用自适应心跳和智能广播
 */
class LightweightDiscovery : public QObject, public Core::LightweightService {
    Q_OBJECT

public:
    explicit LightweightDiscovery(QObject* parent = nullptr);
    ~LightweightDiscovery() override;

    // LightweightService接口实现
    bool initialize() override;
    void processEvent(const Core::Event& event) override;
    void shutdown() override;
    std::string getName() const override { return "LightweightDiscovery"; }
    bool isAvailable() const override { return available_; }

    /**
     * @brief 获取在线用户列表
     */
    QList<UserInfo> getOnlineUsers() const;

    /**
     * @brief 获取特定用户信息
     */
    std::optional<UserInfo> getUser(const QString& userId) const;

    /**
     * @brief 更新本地用户信息
     */
    void updateLocalUser(const UserInfo& user);

    /**
     * @brief 注册回环测试用户
     */
    void registerLoopbackPeer(const UserInfo& user);

signals:
    void userOnline(const UserInfo& user);
    void userOffline(const QString& userId);
    void userInfoUpdated(const UserInfo& user);
    void networkError(const QString& error);

private slots:
    void handleUdpData();
    void sendPresenceBroadcast();
    void cleanupOfflineUsers();
    void adjustHeartbeatInterval();

private:
    struct UserEntry {
        UserInfo info;
        QDateTime lastSeen;
        QHostAddress address;
        quint16 port;
        bool isLoopback = false;
    };

    bool available_ = false;
    UserInfo localUser_;
    QUdpSocket* udpSocket_ = nullptr;
    
    // 智能心跳相关
    QTimer* presenceTimer_ = nullptr;
    QTimer* cleanupTimer_ = nullptr;
    QTimer* adaptiveTimer_ = nullptr;
    int currentHeartbeatInterval_ = 30000; // 30秒
    int baseHeartbeatInterval_ = 30000;
    int minHeartbeatInterval_ = 5000;      // 最小5秒
    int maxHeartbeatInterval_ = 60000;     // 最大60秒
    
    // 用户管理
    std::unordered_map<QString, UserEntry> onlineUsers_;
    QSet<QString> loopbackPeers_;
    QSet<QString> localAddresses_;
    
    // 网络状态
    int networkActivity_ = 0;
    QDateTime lastActivityUpdate_;
    
    // 辅助方法
    void setupConnections();
    bool bindSocket();
    void refreshLocalAddresses();
    bool isLocalAddress(const QHostAddress& address) const;
    QString makeUserId(const QString& username, const QHostAddress& address) const;
    void handlePresencePacket(const QByteArray& data, const QHostAddress& sender, quint16 port);
    void handleExitPacket(const QHostAddress& sender);
    QByteArray buildPresenceDatagram() const;
    void broadcastDatagram(const QByteArray& data);
    QVector<QHostAddress> getBroadcastAddresses() const;
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_LIGHTWEIGHT_DISCOVERY_H