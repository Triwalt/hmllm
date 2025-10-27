/**
 * @file network_protocol.h
 * @brief Network Protocol Definitions for Kylin Messenger
 * 
 * Defines the P2P protocol for serverless LAN communication including:
 * - UDP broadcast for user discovery
 * - TCP for reliable messaging
 * - Message packet structures
 * 
 * @version 1.0.0
 * @date 2025-10-09
 */

#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <QString>
#include <QHostAddress>
#include <QDateTime>
#include <QByteArray>
#include <QDataStream>

namespace KylinMessenger {

/**
 * @brief Protocol version
 */
constexpr quint16 PROTOCOL_VERSION = 1;

/**
 * @brief Network ports
 */
constexpr quint16 UDP_BROADCAST_PORT = 2425;  // Discovery port
constexpr quint16 TCP_MESSAGE_PORT = 2426;    // Messaging port

/**
 * @brief Message types for the protocol
 */
enum class MessageType : quint8 {
    // Discovery messages (UDP)
    PRESENCE_ANNOUNCE = 0x01,    ///< Announce user presence
    PRESENCE_QUERY = 0x02,       ///< Query for online users
    PRESENCE_RESPONSE = 0x03,    ///< Response to presence query
    USER_OFFLINE = 0x04,         ///< User going offline
    
    // Chat messages (TCP)
    TEXT_MESSAGE = 0x10,         ///< Regular text message
    GROUP_MESSAGE = 0x11,        ///< Group chat message
    BROADCAST_MESSAGE = 0x12,    ///< Broadcast to all users
    
    // File transfer (TCP)
    FILE_TRANSFER_REQUEST = 0x20, ///< Request to send file
    FILE_TRANSFER_ACCEPT = 0x21,  ///< Accept file transfer
    FILE_TRANSFER_REJECT = 0x22,  ///< Reject file transfer
    FILE_TRANSFER_DATA = 0x23,    ///< File data chunk
    FILE_TRANSFER_COMPLETE = 0x24, ///< File transfer complete
    
    // Special messages
    IMAGE_MESSAGE = 0x30,         ///< Image/screenshot message
    EMOJI_MESSAGE = 0x31,         ///< Emoji message
    
    // AI-related messages
    AI_QUERY = 0x40,             ///< Query to AI assistant
    AI_RESPONSE = 0x41,          ///< Response from AI assistant
    
    // System messages
    TYPING_INDICATOR = 0x50,     ///< User is typing
    READ_RECEIPT = 0x51,         ///< Message read receipt
    PING = 0xFE,                 ///< Keep-alive ping
    PONG = 0xFF                  ///< Keep-alive pong
};

/**
 * @brief User information structure
 */
struct UserInfo {
    QString username;            ///< Display name
    QString hostname;            ///< Computer hostname
    QHostAddress ipAddress;      ///< IP address
    QString group;               ///< User group
    QString status;              ///< Status message
    QDateTime lastSeen;          ///< Last activity time
    bool isOnline;               ///< Online status
    
    UserInfo() : isOnline(false) {}
    
    // Serialization
    QByteArray serialize() const;
    static UserInfo deserialize(const QByteArray& data);
};

/**
 * @brief Chat message structure
 */
struct ChatMessage {
    QString messageId;           ///< Unique message ID
    QString senderId;            ///< Sender's user ID
    QString senderName;          ///< Sender's display name
    QString receiverId;          ///< Receiver's user ID (empty for broadcast)
    QString groupId;             ///< Group ID (if group message)
    MessageType type;            ///< Message type
    QString content;             ///< Message content
    QByteArray binaryData;       ///< Binary data (images, files)
    QDateTime timestamp;         ///< Send time
    QVariantMap metadata;        ///< Additional metadata
    
    ChatMessage() : type(MessageType::TEXT_MESSAGE) {
        timestamp = QDateTime::currentDateTime();
    }
    
    // Serialization
    QByteArray serialize() const;
    static ChatMessage deserialize(const QByteArray& data);
};

/**
 * @brief Protocol packet header
 */
struct PacketHeader {
    quint32 magic;               ///< Magic number: 0x4B594C4E ('KYLN')
    quint16 version;             ///< Protocol version
    MessageType type;            ///< Message type
    quint32 payloadSize;         ///< Payload size in bytes
    quint32 checksum;            ///< CRC32 checksum
    
    PacketHeader() 
        : magic(0x4B594C4E)
        , version(PROTOCOL_VERSION)
        , type(MessageType::TEXT_MESSAGE)
        , payloadSize(0)
        , checksum(0) {}
    
    // Serialization
    QByteArray serialize() const;
    static PacketHeader deserialize(const QByteArray& data);
    bool isValid() const;
};

/**
 * @brief Complete network packet
 */
class NetworkPacket {
public:
    NetworkPacket();
    NetworkPacket(MessageType type);
    
    // Header access
    PacketHeader& header() { return header_; }
    const PacketHeader& header() const { return header_; }
    
    // Payload access
    void setPayload(const QByteArray& data);
    QByteArray payload() const { return payload_; }
    
    // Convenience methods
    void setUserInfo(const UserInfo& user);
    UserInfo getUserInfo() const;
    
    void setChatMessage(const ChatMessage& message);
    ChatMessage getChatMessage() const;
    
    // Serialization
    QByteArray serialize() const;
    bool deserialize(const QByteArray& data);
    
    // Validation
    bool isValid() const;
    void updateChecksum();
    
private:
    PacketHeader header_;
    QByteArray payload_;
    
    static quint32 calculateChecksum(const QByteArray& data);
};

/**
 * @brief Helper functions for protocol
 */
class ProtocolHelper {
public:
    /**
     * @brief Generate unique message ID
     */
    static QString generateMessageId();
    
    /**
     * @brief Get user ID from IP address
     */
    static QString getUserId(const QHostAddress& addr);
    
    /**
     * @brief Check if message type requires TCP
     */
    static bool requiresTCP(MessageType type);
    
    /**
     * @brief Check if message type uses UDP
     */
    static bool usesUDP(MessageType type);
    
    /**
     * @brief Get message type name for debugging
     */
    static QString getMessageTypeName(MessageType type);
};

} // namespace KylinMessenger

#endif // NETWORK_PROTOCOL_H
