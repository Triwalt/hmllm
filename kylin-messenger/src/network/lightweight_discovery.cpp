/**
 * @file lightweight_discovery.cpp
 * @brief 智能网络发现服务实现
 * @version 1.0.0
 */

#include "network/lightweight_discovery.h"
#include "core/micro_kernel.h"
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QDebug>
#include <QHostInfo>
#include <QRandomGenerator>
#include <algorithm>

namespace KylinMessenger::Network {

LightweightDiscovery::LightweightDiscovery(QObject* parent)
    : QObject(parent) {
    qDebug() << "[LightweightDiscovery] 创建网络发现服务";
}

LightweightDiscovery::~LightweightDiscovery() {
    qDebug() << "[LightweightDiscovery] 销毁网络发现服务";
    shutdown();
}

bool LightweightDiscovery::initialize() {
    qDebug() << "[LightweightDiscovery] 初始化网络发现服务";
    available_ = true;
    qDebug() << "[LightweightDiscovery] 网络发现服务初始化成功(简化版)";
    return true;
}

void LightweightDiscovery::processEvent(const Core::Event& event) {
    switch (event.type()) {
        case Core::Event::ShutdownRequested:
            qDebug() << "[LightweightDiscovery] 处理关闭请求";
            shutdown();
            break;
        case Core::Event::UserOffline:
            qDebug() << "[LightweightDiscovery] 用户下线，清理相关状态";
            break;
        default:
            break;
    }
}

void LightweightDiscovery::shutdown() {
    qDebug() << "[LightweightDiscovery] 关闭网络发现服务";
    available_ = false;
    onlineUsers_.clear();
    loopbackPeers_.clear();
    qDebug() << "[LightweightDiscovery] 网络发现服务关闭完成";
}

QList<UserInfo> LightweightDiscovery::getOnlineUsers() const {
    QList<UserInfo> users;
    for (const auto& [userId, entry] : onlineUsers_) {
        if (QDateTime::currentDateTime().secsTo(entry.lastSeen) > -30) {
            users.append(entry.info);
        }
    }
    std::sort(users.begin(), users.end(), [](const UserInfo& a, const UserInfo& b) {
        return a.username < b.username;
    });
    return users;
}

std::optional<UserInfo> LightweightDiscovery::getUser(const QString& userId) const {
    auto it = onlineUsers_.find(userId);
    if (it != onlineUsers_.end() && QDateTime::currentDateTime().secsTo(it->second.lastSeen) > -30) {
        return it->second.info;
    }
    return std::nullopt;
}

void LightweightDiscovery::updateLocalUser(const UserInfo& user) {
    qDebug() << "[LightweightDiscovery] 更新本地用户信息:" << user.username;
    localUser_ = user;
    if (!localUser_.user_id.isEmpty()) {
        sendPresenceBroadcast();
    }
}

void LightweightDiscovery::registerLoopbackPeer(const UserInfo& user) {
    qDebug() << "[LightweightDiscovery] 注册回环测试用户:" << user.username;
    UserEntry entry;
    entry.info = user;
    entry.lastSeen = QDateTime::currentDateTime();
    entry.address = QHostAddress::LocalHost;
    entry.port = 2425;
    entry.isLoopback = true;
    onlineUsers_[user.user_id] = entry;
    loopbackPeers_.insert(user.user_id);
    emit userOnline(user);
}

void LightweightDiscovery::sendPresenceBroadcast() {
    if (!available_ || localUser_.user_id.isEmpty()) {
        return;
    }
    qDebug() << "[LightweightDiscovery] 发送上线广播(简化版)";
}

void LightweightDiscovery::cleanupOfflineUsers() {
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> offlineUsers;
    for (auto it = onlineUsers_.begin(); it != onlineUsers_.end(); ++it) {
        const QString& userId = it->first;
        const UserEntry& entry = it->second;
        if (entry.isLoopback) continue;
        if (entry.lastSeen.secsTo(now) < -45) {
            offlineUsers.append(userId);
        }
    }
    for (const QString& userId : offlineUsers) {
        onlineUsers_.remove(userId);
        emit userOffline(userId);
        qDebug() << "[LightweightDiscovery] 用户离线:" << userId;
    }
    if (!offlineUsers.isEmpty()) {
        qDebug() << "[LightweightDiscovery] 清理了" << offlineUsers.size() << "个离线用户";
    }
}

void LightweightDiscovery::adjustHeartbeatInterval() {
    int newInterval = baseHeartbeatInterval_;
    if (networkActivity_ > 20) {
        newInterval = std::max(minHeartbeatInterval_, baseHeartbeatInterval_ / 2);
    } else if (networkActivity_ < 5) {
        newInterval = std::min(maxHeartbeatInterval_, baseHeartbeatInterval_ * 2);
    }
    if (newInterval != currentHeartbeatInterval_) {
        currentHeartbeatInterval_ = newInterval;
        if (presenceTimer_) presenceTimer_->setInterval(currentHeartbeatInterval_);
        qDebug() << "[LightweightDiscovery] 调整心跳间隔为:" << currentHeartbeatInterval_ << "ms";
    }
    networkActivity_ = 0;
}

QString LightweightDiscovery::makeUserId(const QString& username, const QHostAddress& address) const {
    return QString("%1@%2").arg(username, address.toString());
}

} // namespace KylinMessenger::Network