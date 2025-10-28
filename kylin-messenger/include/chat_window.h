// chat_window.h - 聊天窗口
#ifndef KYLIN_MESSENGER_CHAT_WINDOW_H
#define KYLIN_MESSENGER_CHAT_WINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <memory>
#include <QString>

#include "network_protocol.h"
#include "network_manager.h"
#include "ai_service.h"

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
    virtual ~ChatWindow();
    
    /**
     * @brief 设置网络管理器
     */
    void setNetworkManager(NetworkManager* network_manager);
    
    /**
     * @brief 设置AI服务（用于AI助手窗口）
     */
    void setAIService(std::shared_ptr<IAIService> ai_service);
    
    /**
     * @brief 添加接收到的消息
     */
    void addReceivedMessage(const ChatMessage& message);

    const UserInfo& peerInfo() const { return m_peer_info; }
    QString peerId() const { return m_peer_info.user_id; }
    
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
    
    void sendTextMessage(const QString& text);
    void sendImageMessage(const QImage& image);
    void sendLoopbackMessage(const QString& text);
    
    QString formatMessageTime(const QDateTime& time) const;
    QString formatMessageContent(const ChatMessage& message) const;
    
private:
    UserInfo m_peer_info;
    NetworkManager* m_network_manager;
    std::shared_ptr<IAIService> m_ai_service;
    
    // UI组件
    QTextEdit* m_message_display;
    QLineEdit* m_input_edit;
    QPushButton* m_send_button;
    QToolBar* m_toolbar;
    QLabel* m_peer_name_label;
    QLabel* m_typing_indicator;
    
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
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_CHAT_WINDOW_H
