// chat_window.cpp - 聊天窗口实现
#include "chat_window.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QUuid>
#include <QDebug>

namespace KylinMessenger {

ChatWindow::ChatWindow(const UserInfo& peer_info, QWidget* parent)
    : QWidget(parent)
    , m_peer_info(peer_info)
    , m_network_manager(nullptr)
    , m_message_display(nullptr)
    , m_input_edit(nullptr)
    , m_send_button(nullptr)
    , m_toolbar(nullptr)
    , m_peer_name_label(nullptr)
    , m_typing_indicator(nullptr)
    , m_smart_reply_widget(nullptr)
    , m_smart_reply_layout(nullptr)
    , m_typing_timer(new QTimer(this))
    , m_is_typing(false)
    , m_is_ai_chat(peer_info.user_id == "ai_assistant")
{
    setupUI();
    setupConnections();
    
    setWindowTitle(QString("聊天 - %1").arg(
        QString::fromStdString(peer_info.username)));
    
    resize(600, 500);
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::setNetworkManager(NetworkManager* network_manager)
{
    m_network_manager = network_manager;
}

void ChatWindow::setAIService(std::shared_ptr<IAIService> ai_service)
{
    m_ai_service = ai_service;
}

// ============================================================================
// UI设置
// ============================================================================

void ChatWindow::setupUI()
{
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    
    // 顶部工具栏
    m_toolbar = new QToolBar(this);
    
    // 对方信息
    m_peer_name_label = new QLabel(
        QString::fromStdString(m_peer_info.username), this);
    m_peer_name_label->setStyleSheet(
        "QLabel { font-weight: bold; font-size: 14px; padding: 5px; }");
    m_toolbar->addWidget(m_peer_name_label);
    
    m_toolbar->addSeparator();
    
    // 工具按钮
    m_toolbar->addAction("📁 文件", this, &ChatWindow::onSendFile);
    m_toolbar->addAction("📷 截图", this, &ChatWindow::onSendScreenshot);
    m_toolbar->addAction("😊 表情", this, &ChatWindow::onSendEmoji);
    
    main_layout->addWidget(m_toolbar);
    
    // 打字指示器
    m_typing_indicator = new QLabel("", this);
    m_typing_indicator->setStyleSheet(
        "QLabel { color: gray; font-style: italic; padding: 2px 10px; }");
    m_typing_indicator->hide();
    main_layout->addWidget(m_typing_indicator);
    
    // 消息显示区域
    m_message_display = new QTextEdit(this);
    m_message_display->setReadOnly(true);
    m_message_display->setStyleSheet(
        "QTextEdit {"
        "  border: 1px solid #ddd;"
        "  background-color: #f5f5f5;"
        "  padding: 10px;"
        "}");
    main_layout->addWidget(m_message_display);
    
    // 智能回复区域
    m_smart_reply_widget = new QWidget(this);
    m_smart_reply_layout = new QHBoxLayout(m_smart_reply_widget);
    m_smart_reply_layout->setContentsMargins(5, 5, 5, 5);
    
    QLabel* reply_label = new QLabel("💡 智能回复:", this);
    m_smart_reply_layout->addWidget(reply_label);
    m_smart_reply_layout->addStretch();
    
    m_smart_reply_widget->hide();
    main_layout->addWidget(m_smart_reply_widget);
    
    // 输入区域
    QHBoxLayout* input_layout = new QHBoxLayout();
    input_layout->setContentsMargins(5, 5, 5, 5);
    
    m_input_edit = new QLineEdit(this);
    m_input_edit->setPlaceholderText("输入消息...");
    m_input_edit->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #ddd;"
        "  border-radius: 3px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "}");
    input_layout->addWidget(m_input_edit);
    
    m_send_button = new QPushButton("发送", this);
    m_send_button->setFixedWidth(80);
    m_send_button->setStyleSheet(
        "QPushButton {"
        "  background-color: #0078d4;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 3px;"
        "  padding: 8px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #005a9e;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #004578;"
        "}");
    input_layout->addWidget(m_send_button);
    
    main_layout->addLayout(input_layout);
    
    setLayout(main_layout);
}

void ChatWindow::setupConnections()
{
    // 发送按钮
    connect(m_send_button, &QPushButton::clicked,
            this, &ChatWindow::onSendMessage);
    
    // 回车发送
    connect(m_input_edit, &QLineEdit::returnPressed,
            this, &ChatWindow::onSendMessage);
    
    // 输入变化
    connect(m_input_edit, &QLineEdit::textChanged,
            this, &ChatWindow::onInputTextChanged);
    
    // 打字定时器
    m_typing_timer->setSingleShot(true);
    connect(m_typing_timer, &QTimer::timeout,
            this, &ChatWindow::onTypingTimeout);
}

// ============================================================================
// 消息处理
// ============================================================================

void ChatWindow::onSendMessage()
{
    QString text = m_input_edit->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    
    m_input_edit->clear();
    
    // 如果是AI聊天
    if (m_is_ai_chat && m_ai_service) {
        // 显示用户消息
        ChatMessage user_msg;
        user_msg.message_id = QUuid::createUuid().toString().toStdString();
        user_msg.sender_id = "user";
        user_msg.content = text.toStdString();
        user_msg.timestamp = QDateTime::currentDateTime();
        
        addMessageToDisplay(user_msg, true);
        m_message_history.push_back(user_msg);
        
        // 显示"AI正在思考..."
        m_message_display->append(
            "<div style='color: gray; font-style: italic;'>"
            "AI正在思考..."
            "</div>");
        
        // 异步处理AI响应
        std::string input = text.toStdString();
        m_ai_service->processTextAsync(input, 
            [this](const AIResult& result) {
                // 在主线程中更新UI
                QMetaObject::invokeMethod(this, [this, result]() {
                    // 移除"正在思考"提示
                    QString html = m_message_display->toHtml();
                    int pos = html.lastIndexOf("AI正在思考...");
                    if (pos != -1) {
                        m_message_display->clear();
                        html.remove(pos - 50, 100); // 粗略移除
                        m_message_display->setHtml(html);
                    }
                    
                    // 显示AI响应
                    ChatMessage ai_msg;
                    ai_msg.message_id = QUuid::createUuid().toString().toStdString();
                    ai_msg.sender_id = "ai_assistant";
                    ai_msg.content = result.text_output;
                    ai_msg.timestamp = QDateTime::currentDateTime();
                    
                    addMessageToDisplay(ai_msg, false);
                    m_message_history.push_back(ai_msg);
                    
                    // 生成智能回复建议
                    if (m_ai_service->getCapabilities() & AICapability::SmartReply) {
                        std::vector<std::string> history_text;
                        for (const auto& msg : m_message_history) {
                            history_text.push_back(msg.content);
                        }
                        
                        auto reply_result = m_ai_service->generateSmartReplies(
                            history_text, 3);
                        
                        if (reply_result.success && 
                            !reply_result.suggestions.empty()) {
                            displaySmartReplies(reply_result.suggestions);
                        }
                    }
                }, Qt::QueuedConnection);
            });
    }
    else {
        // 普通P2P消息
        sendTextMessage(text);
    }
    
    // 清除智能回复
    clearSmartReplies();
}

void ChatWindow::sendTextMessage(const QString& text)
{
    if (!m_network_manager) {
        QMessageBox::warning(this, "错误", "网络管理器未初始化");
        return;
    }
    
    ChatMessage message;
    message.message_id = QUuid::createUuid().toString().toStdString();
    message.sender_id = "local"; // 将被网络管理器替换
    message.receiver_id = m_peer_info.user_id;
    message.message_type = MessageContentType::PlainText;
    message.content = text.toStdString();
    message.timestamp = QDateTime::currentDateTime();
    
    if (m_network_manager->sendMessage(
            QString::fromStdString(m_peer_info.user_id), message)) {
        addMessageToDisplay(message, true);
        m_message_history.push_back(message);
    } else {
        QMessageBox::warning(this, "错误", "消息发送失败");
    }
}

void ChatWindow::sendImageMessage(const QImage& image)
{
    if (!m_network_manager) {
        return;
    }
    
    // 将图像转换为Base64
    QByteArray image_data;
    QBuffer buffer(&image_data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    
    QString base64_image = QString::fromLatin1(image_data.toBase64());
    
    ChatMessage message;
    message.message_id = QUuid::createUuid().toString().toStdString();
    message.sender_id = "local";
    message.receiver_id = m_peer_info.user_id;
    message.message_type = MessageContentType::Image;
    message.content = base64_image.toStdString();
    message.timestamp = QDateTime::currentDateTime();
    
    m_network_manager->sendMessage(
        QString::fromStdString(m_peer_info.user_id), message);
    
    addMessageToDisplay(message, true);
    m_message_history.push_back(message);
}

void ChatWindow::addReceivedMessage(const ChatMessage& message)
{
    addMessageToDisplay(message, false);
    m_message_history.push_back(message);
    
    // 生成智能回复
    if (m_ai_service && 
        (m_ai_service->getCapabilities() & AICapability::SmartReply)) {
        std::vector<std::string> history_text;
        for (const auto& msg : m_message_history) {
            history_text.push_back(msg.content);
        }
        
        auto result = m_ai_service->generateSmartReplies(history_text, 3);
        if (result.success && !result.suggestions.empty()) {
            displaySmartReplies(result.suggestions);
        }
    }
}

void ChatWindow::addMessageToDisplay(const ChatMessage& message, bool is_sent)
{
    QString time_str = formatMessageTime(message.timestamp);
    QString content = formatMessageContent(message);
    
    QString alignment = is_sent ? "right" : "left";
    QString bg_color = is_sent ? "#dcf8c6" : "#ffffff";
    QString name = is_sent ? "我" : 
        QString::fromStdString(m_peer_info.username);
    
    QString html = QString(
        "<div style='text-align: %1; margin: 10px 0;'>"
        "  <div style='"
        "    display: inline-block;"
        "    background-color: %2;"
        "    border-radius: 10px;"
        "    padding: 10px 15px;"
        "    max-width: 70%%;"
        "    box-shadow: 0 1px 2px rgba(0,0,0,0.1);"
        "  '>"
        "    <div style='font-weight: bold; color: #333; margin-bottom: 5px;'>"
        "      %3"
        "    </div>"
        "    <div style='color: #000; word-wrap: break-word;'>"
        "      %4"
        "    </div>"
        "    <div style='color: #888; font-size: 11px; margin-top: 5px;'>"
        "      %5"
        "    </div>"
        "  </div>"
        "</div>")
        .arg(alignment)
        .arg(bg_color)
        .arg(name)
        .arg(content)
        .arg(time_str);
    
    m_message_display->append(html);
    
    // 滚动到底部
    QScrollBar* scrollbar = m_message_display->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

// ============================================================================
// 工具栏动作
// ============================================================================

void ChatWindow::onSendFile()
{
    QString filename = QFileDialog::getOpenFileName(
        this, "选择文件", "", "所有文件 (*)");
    
    if (!filename.isEmpty()) {
        if (m_network_manager) {
            m_network_manager->sendFile(
                QString::fromStdString(m_peer_info.user_id), filename);
        }
    }
}

void ChatWindow::onSendScreenshot()
{
    // TODO: 实现截图功能
    QMessageBox::information(this, "提示", "截图功能开发中...");
}

void ChatWindow::onSendEmoji()
{
    // TODO: 实现表情选择器
    QMessageBox::information(this, "提示", "表情功能开发中...");
}

// ============================================================================
// 输入处理
// ============================================================================

void ChatWindow::onInputTextChanged()
{
    if (!m_is_typing && m_network_manager) {
        m_is_typing = true;
        m_network_manager->sendTypingIndicator(
            QString::fromStdString(m_peer_info.user_id), true);
    }
    
    // 重置定时器
    m_typing_timer->start(2000);
}

void ChatWindow::onTypingTimeout()
{
    if (m_is_typing && m_network_manager) {
        m_is_typing = false;
        m_network_manager->sendTypingIndicator(
            QString::fromStdString(m_peer_info.user_id), false);
    }
}

// ============================================================================
// 智能回复
// ============================================================================

void ChatWindow::displaySmartReplies(
    const std::vector<std::string>& suggestions)
{
    clearSmartReplies();
    
    for (const auto& suggestion : suggestions) {
        QPushButton* reply_button = new QPushButton(
            QString::fromStdString(suggestion), this);
        
        reply_button->setStyleSheet(
            "QPushButton {"
            "  background-color: #e0e0e0;"
            "  border: 1px solid #bbb;"
            "  border-radius: 15px;"
            "  padding: 5px 15px;"
            "  margin: 2px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #d0d0d0;"
            "}");
        
        connect(reply_button, &QPushButton::clicked, this, [=]() {
            m_input_edit->setText(QString::fromStdString(suggestion));
            onSendMessage();
        });
        
        m_smart_reply_layout->addWidget(reply_button);
    }
    
    m_smart_reply_widget->show();
}

void ChatWindow::clearSmartReplies()
{
    // 移除所有回复按钮
    QLayoutItem* item;
    while ((item = m_smart_reply_layout->takeAt(1)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    m_smart_reply_widget->hide();
}

// ============================================================================
// 辅助函数
// ============================================================================

QString ChatWindow::formatMessageTime(const QDateTime& time) const
{
    QDateTime now = QDateTime::currentDateTime();
    
    if (time.date() == now.date()) {
        return time.toString("hh:mm");
    } else if (time.date() == now.date().addDays(-1)) {
        return "昨天 " + time.toString("hh:mm");
    } else {
        return time.toString("MM-dd hh:mm");
    }
}

QString ChatWindow::formatMessageContent(const ChatMessage& message) const
{
    QString content = QString::fromStdString(message.content);
    
    switch (message.message_type) {
        case MessageContentType::PlainText:
            // HTML转义
            content = content.toHtmlEscaped();
            // 转换URL为链接
            content.replace(
                QRegularExpression("(https?://[^\\s]+)"),
                "<a href='\\1'>\\1</a>");
            break;
            
        case MessageContentType::Image:
            // 显示图像
            content = QString("<img src='data:image/png;base64,%1' "
                            "style='max-width: 300px; max-height: 300px;' />")
                .arg(content);
            break;
            
        case MessageContentType::File:
            content = QString("📁 文件: %1").arg(content.toHtmlEscaped());
            break;
            
        default:
            break;
    }
    
    return content;
}

} // namespace KylinMessenger
