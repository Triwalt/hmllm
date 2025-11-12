/**
 * @file network_protocol.h
 * @brief Network Protocol Type Aliases for Kylin Messenger
 * 
 * Provides type aliases for core message and user models.
 * 
 * @version 2.0.0
 * @date 2025-01-XX
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include "core/models.h"
#include "network/constants.h"

namespace KylinMessenger {
using Core::ChatMessage;
using Core::FileAttachment;
using Core::MessageContentType;
using Core::UserInfo;
using Core::UserStatus;

inline constexpr quint16 UDP_PORT = Network::kUdpDiscoveryPort;
inline constexpr quint16 TCP_PORT = Network::kTcpListenPort;
} // namespace KylinMessenger

#endif // NETWORK_PROTOCOL_H
