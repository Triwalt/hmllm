/**
 * @file group_hub.h
 * @brief 轻量级群组通信中心
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_NETWORK_GROUP_HUB_H
#define KYLIN_MESSENGER_NETWORK_GROUP_HUB_H

#include "core/micro_kernel.h"
#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QUdpSocket>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <chrono>

namespace KylinMessenger::Network {

/**
 * @brief 群组信息
 */
struct GroupInfo {
    QString groupId;
    QString groupName;
    std::unordered_set<QString> members;  // 成员ID集合
    std::chrono::steady_clock::time_point lastActive;
    QString creatorId;
    QDateTime createdTime;
    
    bool isEmpty() const { return members.empty(); }
    int memberCount() const { return static_cast<int>(members.size()); }
};

/**
 * @brief 轻量级群组通信中心
 * 基于Gossip协议的群组消息分发
 */
class LightweightGroupHub : public QObject, public Core::LightweightService {
    Q_OBJECT

public:
    explicit LightweightGroupHub(QObject* parent = nullptr);
    ~LightweightGroupHub() override;

    // LightweightService接口实现
    bool initialize() override;
    void processEvent(const Core::Event& event) override;
    void shutdown() override;
    std::string getName() const override { return "LightweightGroupHub"; }
    bool isAvailable() const override { return available_; }

    /**
     * @brief 创建群组
     * @param groupName 群组名称
     * @param creatorId 创建者ID
     * @return 群组ID
     */
    QString createGroup(const QString& groupName, const QString& creatorId);

    /**
     * @brief 加入群组
     * @param groupId 群组ID
     * @param userId 用户ID
     * @return 是否成功
     */
    bool joinGroup(const QString& groupId, const QString& userId);

    /**
     * @brief 离开群组
     * @param groupId 群组ID
     * @param userId 用户ID
     */
    void leaveGroup(const QString& groupId, const QString& userId);

    /**
     * @brief 广播群组消息
     * @param groupId 群组ID
     * @param message 消息内容
     * @param senderId 发送者ID
     */
    void broadcastGroupMessage(const QString& groupId, 
                               const QByteArray& message,
                               const QString& senderId);

    /**
     * @brief 获取群组信息
     */
    std::optional<GroupInfo> getGroup(const QString& groupId) const;

    /**
     * @brief 获取用户所在的所有群组
     */
    std::vector<QString> getUserGroups(const QString& userId) const;

    /**
     * @brief 获取活跃群组列表
     */
    std::vector<GroupInfo> getActiveGroups() const;

    /**
     * @brief 清理不活跃的群组
     * @param timeoutMinutes 超时时间（分钟）
     */
    void cleanupInactiveGroups(int timeoutMinutes = 60);

signals:
    void groupMessageReceived(const QString& groupId, 
                             const QString& senderId, 
                             const QByteArray& message);
    void groupMemberJoined(const QString& groupId, const QString& userId);
    void groupMemberLeft(const QString& groupId, const QString& userId);
    void groupCreated(const QString& groupId, const QString& groupName);
    void groupDisbanded(const QString& groupId);

private:
    // Gossip协议参数
    static constexpr int GOSSIP_FANOUT = 3;           // 每个周期传播的节点数
    static constexpr int GOSSIP_INTERVAL_MS = 1000;   // Gossip间隔
    static constexpr int GOSSIP_MAX_AGE = 5;          // 消息最大传播跳数
    
    bool available_ = false;
    QUdpSocket* udpSocket_ = nullptr;
    QTimer* gossipTimer_ = nullptr;
    
    // 群组管理
    std::unordered_map<QString, GroupInfo> groups_;
    std::unordered_map<QString, std::vector<QString>> userGroups_;  // userId -> groupIds
    
    // Gossip状态
    std::mt19937 randomGenerator_;
    std::unordered_map<QString, std::unordered_set<QString>> messageCache_;  // 防止重复传播
    
    // 辅助方法
    void setupConnections();
    bool bindSocket();
    void handleUdpData();
    void gossipIteration();
    std::vector<QString> selectGossipTargets(const std::vector<QString>& members, 
                                             int k, 
                                             const QString& excludeUserId);
    void sendGossipMessage(const QHostAddress& target, 
                          quint16 port, 
                          const QByteArray& message);
    QByteArray serializeGroupMessage(const QString& groupId,
                                    const QString& senderId,
                                    const QByteArray& content,
                                    int hopCount);
    void deserializeAndProcessMessage(const QByteArray& data, 
                                     const QHostAddress& sender);
    QVector<QHostAddress> getMemberAddresses(const std::vector<QString>& memberIds) const;
    void updateGroupActivity(const QString& groupId);
    bool isMessageDuplicate(const QString& messageId);
    void cleanupMessageCache();
};

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_GROUP_HUB_H