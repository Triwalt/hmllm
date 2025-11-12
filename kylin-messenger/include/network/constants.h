#ifndef KYLIN_MESSENGER_NETWORK_CONSTANTS_H
#define KYLIN_MESSENGER_NETWORK_CONSTANTS_H

#include <QtGlobal>

namespace KylinMessenger::Network {

constexpr quint16 kUdpDiscoveryPort = 2425;
constexpr quint16 kTcpListenPort = 2426;
constexpr quint16 kGrpcGatewayPort = 9740;
constexpr quint16 kWebsocketGatewayPort = 44330;

constexpr int kPresenceIntervalMs = 5000;
constexpr int kOfflineTimeoutMs = 15000;
constexpr int kCleanupIntervalMs = 3000;
constexpr int kTcpConnectTimeoutMs = 3000;
constexpr int kRetryBackoffBaseMs = 2000;

} // namespace KylinMessenger::Network

#endif // KYLIN_MESSENGER_NETWORK_CONSTANTS_H

