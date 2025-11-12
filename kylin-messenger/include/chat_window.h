// chat_window.h - 聊天窗口
#ifndef KYLIN_MESSENGER_CHAT_WINDOW_H
#define KYLIN_MESSENGER_CHAT_WINDOW_H

#include <QWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QHash>
#include <QMap>
#include <memory>
#include <QString>
#include <QIcon>
#include <QByteArray>
#include <QUrl>

#include "compliance_service.h"

class QMenu;
class QToolButton;

#include "network_protocol.h"
#include "network_manager.h"
#include "ai_service.h"
#include "core/repositories/message_repository.h"
#include "core/models.h"

namespace KylinMessenger {

/**
 * @brief 聊天窗口类
 * 
 * 单个聊天会话的窗口，包含：
 * - 消息历史显示
 * - 消息输入框
 * - 工具栏（表情、文件、截图等）
 * - 智能回复建议
 * - AI功能集成
 */
class ChatWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit ChatWindow(const UserInfo& peer_info, QWidget* parent = nullptr);
    explicit ChatWindow(const Core::GroupInfo& group_info, QWidget* parent = nullptr);
    virtual ~ChatWindow();
    
    /**
     * @brief 设置网络管理器
     */
    void setNetworkManager(NetworkManager* network_manager);
    void setAutoImageDownloadConfig(bool enabled, const QString& directory);
    
    /**
     * @brief 设置AI服务（用于AI助手窗口）
     */
    void setAIService(std::shared_ptr<IAIService> ai_service);
    void setComplianceService(std::shared_ptr<IComplianceService> compliance_service);
    
    /**
     * @brief 添加接收到的消息
     */
    void addReceivedMessage(const ChatMessage& message);
    void appendHistoryMessage(const ChatMessage& message, bool is_outgoing);

    const UserInfo& peerInfo() const { return m_peer_info; }
    QString peerId() const { return m_peer_info.user_id; }
    void updatePeerInfo(const UserInfo& info);
    
    // 群组相关
    bool isGroupChat() const { return !m_group_id.isEmpty(); }
    QString groupId() const { return m_group_id; }
    const Core::GroupInfo& groupInfo() const { return m_group_info; }
    void updateGroupInfo(const Core::GroupInfo& info);
    void updateGroupMembers(const QList<Core::UserInfo>& members);
    
public slots:
    void onSendMessage();
    void onSendFile();
    void onSendScreenshot();
    void onSendEmoji();
    void onInputTextChanged();
    void onTypingTimeout();
    
signals:
    void chatActivated(const QString& user_id);
    void chatClosed(const QString& user_id);

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    
    void addMessageToDisplay(const ChatMessage& message, bool is_sent);
    void displaySmartReplies(const std::vector<std::string>& suggestions);
    void clearSmartReplies();
    void appendSystemMessage(const QString& text);
    void onMessageLinkActivated(const QUrl& url);
    void registerFileTransfer(const ChatMessage& message);
    void persistMessage(const ChatMessage& message);
    QString conversationKey() const;
    
    void sendTextMessage(const QString& text);
    void sendImageMessage(const QImage& image);
    void sendEmojiMessage(const QString& emoji);
    bool evaluateCompliance(const CompliancePayload& payload, ComplianceResult& result) const;

    void updateWindowTitle();
    void updatePresenceIcon();
    void updatePresenceTooltip();
    QString statusDisplayText(UserStatus status) const;
    QString peerDisplayName() const;
    QString renderEmojiHtml(const QString& text) const;
    QString emojiImageTag(const QString& emoji) const;
    QString imageMimeType(const QString& filename) const;
    QString formatFileSize(quint64 bytes) const;

    QIcon actionIcon(const QString& name) const;
    QIcon statusIcon() const;

    QString formatMessageTime(const QDateTime& time) const;
    QString formatMessageContent(const ChatMessage& message);
    QString renderInlineImageHtml(const ChatMessage& message, bool* ok = nullptr);
    QString renderImageFromFile(const QString& file_path,
                                const QString& cache_key,
                                bool* ok = nullptr);
    QString makeUniqueFilePath(const QString& directory, const QString& filename) const;
    struct DeferredFileOffer;
    void applyPendingTransferMetadata(const QString& message_id,
                                      const QString& key,
                                      const QString& value,
                                      bool removed);
    void maybeAutoAcceptImageTransfer(const QString& message_id);
    void applyFileOfferToTransfer(const QString& message_id, const DeferredFileOffer& offer);
    bool isOutgoingMessage(const ChatMessage& message) const;
    void rebuildMessageDisplay();
    void onNetworkFileOffer(const QString& senderId,
                            const QString& senderIp,
                            quint32 packetNo,
                            quint32 fileId,
                            const QString& filename,
                            quint64 filesize);
    void onNetworkFileTransferProgress(const QString& peerId,
                                       quint32 packetNo,
                                       quint32 fileId,
                                       quint64 bytesTransferred,
                                       quint64 totalBytes);
    void onNetworkFileTransferFinished(const QString& peerId,
                                       quint32 packetNo,
                                       quint32 fileId,
                                       const QString& savePath);
    void onNetworkFileTransferFailed(const QString& peerId,
                                     quint32 packetNo,
                                     quint32 fileId,
                                     const QString& reason);
    quint64 transferKey(quint32 packetNo, quint32 fileId) const;
    QString defaultDownloadPath(const QString& filename) const;
    void updateMessageMetadata(const QString& message_id,
                               const QMap<QString, QString>& changes);
    
private:
    UserInfo m_peer_info;
    NetworkManager* m_network_manager;
    std::shared_ptr<IAIService> m_ai_service;
    std::shared_ptr<IComplianceService> m_compliance_service;
    
    // 群组相关
    QString m_group_id;
    Core::GroupInfo m_group_info;
    QList<Core::UserInfo> m_group_members;
    QWidget* m_member_list_widget;  // 成员列表侧边栏
    std::shared_ptr<Core::Repositories::MessageRepository> m_message_repository;
    
    // UI组件
    QTextBrowser* m_message_display;
    QLineEdit* m_input_edit;
    QPushButton* m_send_button;
    QToolBar* m_toolbar;
    QLabel* m_peer_name_label;
    QLabel* m_presence_icon;
    QLabel* m_typing_indicator;
    QMenu* m_emoji_menu;
    QToolButton* m_emoji_button;
    QHash<QString, QString> m_emoji_resource_map;

    // 文件传输
    struct PendingTransfer {
        QString message_id;
        QString filename;
        quint64 filesize = 0;
        QString file_hash;
        quint32 packet_no = 0;
        quint32 file_id = 0;
        QString peer_id;
        QString peer_ip;
        bool is_outgoing = false;
        bool accepted = false;
        bool completed = false;
        QString local_path;
        QString saved_path;
        quint64 bytes_transferred = 0;
        QString content_type;
    };

    struct DeferredFileOffer {
        QString sender_id;
        QString sender_ip;
        quint32 packet_no = 0;
        quint32 file_id = 0;
        QString filename;
        quint64 filesize = 0;
    };

    QHash<QString, PendingTransfer> m_pending_transfers;
    QHash<quint64, QString> m_transfer_index;
    QHash<quint64, DeferredFileOffer> m_pending_file_offers;
    
    // 智能回复
    QWidget* m_smart_reply_widget;
    QHBoxLayout* m_smart_reply_layout;
    
    // 消息历史
    std::vector<ChatMessage> m_message_history;
    
    // 打字指示器定时器
    QTimer* m_typing_timer;
    bool m_is_typing;
    
    // 是否为AI助手窗口
    bool m_is_ai_chat;
    bool m_is_loopback;

    QString m_local_user_id;

    bool m_auto_image_download_enabled = false;
    QString m_auto_image_download_dir;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_CHAT_WINDOW_H
