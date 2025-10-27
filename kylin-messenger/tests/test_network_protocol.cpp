// test_network_protocol.cpp - 网络协议单元测试
#include <gtest/gtest.h>
#include "../include/network_protocol.h"
#include <QDebug>

using namespace KylinMessenger;

class NetworkProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试数据
    }
};

TEST_F(NetworkProtocolTest, UserInfoSerialization) {
    UserInfo user;
    user.user_id = "test-user-123";
    user.username = "Test User";
    user.hostname = "test-host";
    user.ip_address = "192.168.1.100";
    user.port = 2426;
    user.status = UserStatus::Online;
    user.status_text = "Hello World";
    
    // 序列化
    QByteArray data = user.serialize();
    ASSERT_FALSE(data.isEmpty());
    
    // 反序列化
    UserInfo user2;
    ASSERT_TRUE(user2.deserialize(data));
    
    // 验证
    EXPECT_EQ(user.user_id, user2.user_id);
    EXPECT_EQ(user.username, user2.username);
    EXPECT_EQ(user.hostname, user2.hostname);
    EXPECT_EQ(user.ip_address, user2.ip_address);
    EXPECT_EQ(user.port, user2.port);
    EXPECT_EQ(user.status, user2.status);
    EXPECT_EQ(user.status_text, user2.status_text);
}

TEST_F(NetworkProtocolTest, ChatMessageSerialization) {
    ChatMessage msg;
    msg.message_id = "msg-123";
    msg.sender_id = "user-1";
    msg.receiver_id = "user-2";
    msg.message_type = MessageContentType::PlainText;
    msg.content = "Hello, this is a test message!";
    msg.timestamp = QDateTime::currentDateTime();
    msg.is_read = false;
    
    // 序列化
    QByteArray data = msg.serialize();
    ASSERT_FALSE(data.isEmpty());
    
    // 反序列化
    ChatMessage msg2;
    ASSERT_TRUE(msg2.deserialize(data));
    
    // 验证
    EXPECT_EQ(msg.message_id, msg2.message_id);
    EXPECT_EQ(msg.sender_id, msg2.sender_id);
    EXPECT_EQ(msg.receiver_id, msg2.receiver_id);
    EXPECT_EQ(msg.message_type, msg2.message_type);
    EXPECT_EQ(msg.content, msg2.content);
    EXPECT_EQ(msg.is_read, msg2.is_read);
}

TEST_F(NetworkProtocolTest, PacketHeaderValidation) {
    PacketHeader header;
    header.magic_number = PROTOCOL_MAGIC;
    header.version = PROTOCOL_VERSION;
    header.message_type = MessageType::ChatMessage;
    header.payload_size = 1024;
    header.checksum = 0x12345678;
    
    EXPECT_TRUE(header.isValid());
    
    // 测试无效魔数
    PacketHeader invalid_header = header;
    invalid_header.magic_number = 0xDEADBEEF;
    EXPECT_FALSE(invalid_header.isValid());
    
    // 测试无效版本
    invalid_header = header;
    invalid_header.version = 99;
    EXPECT_FALSE(invalid_header.isValid());
    
    // 测试超大负载
    invalid_header = header;
    invalid_header.payload_size = MAX_PACKET_SIZE + 1;
    EXPECT_FALSE(invalid_header.isValid());
}

TEST_F(NetworkProtocolTest, NetworkPacketRoundTrip) {
    // 创建消息
    ChatMessage original_msg;
    original_msg.message_id = "test-123";
    original_msg.sender_id = "sender";
    original_msg.receiver_id = "receiver";
    original_msg.content = "Test content";
    original_msg.message_type = MessageContentType::PlainText;
    
    // 创建数据包
    NetworkPacket packet = NetworkPacket::createChatMessagePacket(original_msg);
    
    EXPECT_TRUE(packet.isValid());
    EXPECT_EQ(packet.getHeader().message_type, MessageType::ChatMessage);
    
    // 序列化
    QByteArray data = packet.serialize();
    ASSERT_FALSE(data.isEmpty());
    
    // 反序列化
    NetworkPacket packet2;
    ASSERT_TRUE(packet2.deserialize(data));
    EXPECT_TRUE(packet2.isValid());
    
    // 验证消息内容
    ChatMessage received_msg;
    ASSERT_TRUE(received_msg.deserialize(packet2.getPayload()));
    EXPECT_EQ(original_msg.message_id, received_msg.message_id);
    EXPECT_EQ(original_msg.content, received_msg.content);
}

TEST_F(NetworkProtocolTest, ChecksumValidation) {
    QByteArray data1("Hello World");
    QByteArray data2("Hello World");
    QByteArray data3("Different Data");
    
    quint32 checksum1 = NetworkPacket::calculateChecksum(data1);
    quint32 checksum2 = NetworkPacket::calculateChecksum(data2);
    quint32 checksum3 = NetworkPacket::calculateChecksum(data3);
    
    EXPECT_EQ(checksum1, checksum2);
    EXPECT_NE(checksum1, checksum3);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
