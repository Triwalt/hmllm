// main_window.cpp - 主窗口实现（第1部分）
#include "main_window.h"
#include "chat_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QDebug>

namespace KylinMessenger {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_network_manager(nullptr)
    , m_user_list(nullptr)
    , m_search_edit(nullptr)
    , m_status_combo(nullptr)
    , m_status_text_edit(nullptr)
    , m_user_count_label(nullptr)
    , m_tray_icon(nullptr)
    , m_current_context_item(nullptr)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupSystemTray();
    setupConnections();
    
    loadSettings();
    ensureLoopbackEntry();
    
    setWindowTitle("麒麟信使 - Kylin Messenger");
    resize(300, 600);
}

MainWindow::~MainWindow()
{
    saveSettings();
    
    // 关闭所有聊天窗口
    for (auto* window : m_chat_windows) {
        window->close();
        window->deleteLater();
    }
    m_chat_windows.clear();
}

void MainWindow::setNetworkManager(NetworkManager* network_manager)
{
    m_network_manager = network_manager;
    
    if (m_network_manager) {
        connect(m_network_manager, &NetworkManager::userOnline,
                this, &MainWindow::onUserOnline);
        
        connect(m_network_manager, &NetworkManager::userOffline,
                this, &MainWindow::onUserOffline);
        
        connect(m_network_manager, &NetworkManager::userInfoUpdated,
                this, &MainWindow::onUserInfoUpdated);
        
        connect(m_network_manager, &NetworkManager::messageReceived,
                this, &MainWindow::onMessageReceived);
        
        m_local_user = m_network_manager->getLocalUser();
    ensureLoopbackEntry();
    }
}

void MainWindow::setAIService(std::shared_ptr<IAIService> ai_service)
{
    m_ai_service = ai_service;
}

// ============================================================================
// UI设置
// ============================================================================

void MainWindow::setupUI()
{
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
    
    // 搜索框
    m_search_edit = new QLineEdit(this);
    m_search_edit->setPlaceholderText("搜索用户...");
    m_search_edit->setClearButtonEnabled(true);
    main_layout->addWidget(m_search_edit);
    
    // 用户列表
    m_user_list = new QListWidget(this);
    m_user_list->setContextMenuPolicy(Qt::CustomContextMenu);
    main_layout->addWidget(m_user_list);
    m_user_list->setAlternatingRowColors(true);
    m_user_list->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 状态栏
    QHBoxLayout* status_layout = new QHBoxLayout();
    
    m_status_combo = new QComboBox(this);
    m_status_combo->addItem("🟢 在线", static_cast<int>(UserStatus::Online));
    m_status_combo->addItem("🟡 离开", static_cast<int>(UserStatus::Away));
    m_status_combo->addItem("🔴 忙碌", static_cast<int>(UserStatus::Busy));
    m_status_combo->addItem("⚫ 隐身", static_cast<int>(UserStatus::Invisible));
    status_layout->addWidget(m_status_combo);
    
    m_status_text_edit = new QLineEdit(this);
    m_status_text_edit->setPlaceholderText("状态消息");
    status_layout->addWidget(m_status_text_edit);
    
    main_layout->addLayout(status_layout);
    
    setCentralWidget(central_widget);
    
    // 创建右键菜单
    m_user_context_menu = new QMenu(this);
    m_user_context_menu->addAction("发送消息", this, &MainWindow::onSendMessage);
    m_user_context_menu->addAction("发送文件", this, &MainWindow::onSendFile);
    m_user_context_menu->addSeparator();
    m_user_context_menu->addAction("查看信息", [this]() {
        // TODO: 显示用户详细信息
    });
}

void MainWindow::setupMenuBar()
{
    // 文件菜单
    m_file_menu = menuBar()->addMenu("文件(&F)");
    m_file_menu->addAction("设置...", this, &MainWindow::onSettings);
    m_file_menu->addSeparator();
    m_file_menu->addAction("退出", this, &MainWindow::onQuit, 
                          QKeySequence::Quit);
    
    // 工具菜单
    m_tools_menu = menuBar()->addMenu("工具(&T)");
    m_tools_menu->addAction("截图工具", this, &MainWindow::onSendScreenshot,
                           QKeySequence("Ctrl+Alt+A"));
    m_tools_menu->addAction("AI聊天助手", this, &MainWindow::onOpenAIChat);
    
    // 帮助菜单
    m_help_menu = menuBar()->addMenu("帮助(&H)");
    m_help_menu->addAction("关于", this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    m_toolbar = addToolBar("主工具栏");
    
    m_toolbar->addAction("💬 消息", this, &MainWindow::onSendMessage);
    m_toolbar->addAction("📁 文件", this, &MainWindow::onSendFile);
    m_toolbar->addAction("📷 截图", this, &MainWindow::onSendScreenshot);
    m_toolbar->addSeparator();
    m_toolbar->addAction("🤖 AI", this, &MainWindow::onOpenAIChat);
}

void MainWindow::setupStatusBar()
{
    m_user_count_label = new QLabel("在线用户: 0", this);
    statusBar()->addPermanentWidget(m_user_count_label);
    
    statusBar()->showMessage("就绪");
}

void MainWindow::setupSystemTray()
{
    m_tray_icon = new QSystemTrayIcon(this);
    m_tray_icon->setIcon(QIcon(":/icons/app_icon.png"));
    m_tray_icon->setToolTip("麒麟信使");
    
    m_tray_menu = new QMenu(this);
    m_tray_menu->addAction("显示主窗口", this, &QWidget::show);
    m_tray_menu->addAction("AI助手", this, &MainWindow::onOpenAIChat);
    m_tray_menu->addSeparator();
    m_tray_menu->addAction("退出", this, &MainWindow::onQuit);
    
    m_tray_icon->setContextMenu(m_tray_menu);
    m_tray_icon->show();
}

void MainWindow::setupConnections()
{
    // 用户列表事件
    connect(m_user_list, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onUserItemDoubleClicked);
    
    connect(m_user_list, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onUserItemContextMenu);
    
    // 搜索
    connect(m_search_edit, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);
    
    // 状态变更
    connect(m_status_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStatusChanged);
    
    connect(m_status_text_edit, &QLineEdit::textChanged,
            this, &MainWindow::onStatusTextChanged);
    
    // 系统托盘
    connect(m_tray_icon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
    
    connect(m_tray_icon, &QSystemTrayIcon::messageClicked,
            this, &MainWindow::onTrayIconMessageClicked);
}

// ============================================================================
// 网络事件处理
// ============================================================================

void MainWindow::onUserOnline(const UserInfo& user_info)
{
    updateUserListItem(user_info);
    
    QString message = QStringLiteral("%1 上线了").arg(user_info.username);
    
    statusBar()->showMessage(message, 3000);
    showNotification("用户上线", message);
    
    updateUserList();
}

void MainWindow::onUserOffline(const QString& user_id)
{
    removeUserListItem(user_id);
    updateUserList();
}

void MainWindow::onUserInfoUpdated(const UserInfo& user_info)
{
    updateUserListItem(user_info);
}

void MainWindow::onMessageReceived(const ChatMessage& message)
{
    QString sender_id = message.sender_id;
    
    // 查找或创建聊天窗口
    ChatWindow* chat_window = findChatWindow(sender_id);
    
    if (!chat_window) {
        // 从在线用户列表获取用户信息
        UserInfo sender_info;
        bool found = false;
        
        if (m_network_manager) {
            auto users = m_network_manager->getOnlineUsers();
            for (const auto& user : users) {
                if (user.user_id == message.sender_id) {
                    sender_info = user;
                    found = true;
                    break;
                }
            }
        }
        
        if (found) {
            chat_window = openChatWindow(sender_info, false);
        }
    }
    
    if (chat_window) {
        bool wasActive = chat_window->isActiveWindow();
        chat_window->addReceivedMessage(message);

        if (!wasActive) {
            incrementUnread(sender_id);
            QString notification = QStringLiteral("%1: %2")
                .arg(message.sender_id)
                .arg(message.content.left(50));
            
            showNotification("新消息", notification);
            
            // 托盘图标闪烁
            m_tray_icon->showMessage("新消息", notification,
                                     QSystemTrayIcon::Information, 3000);
        }
        else {
            markConversationRead(sender_id);
        }
    }
    else {
        incrementUnread(sender_id);
    }
}

// ============================================================================
// UI事件处理
// ============================================================================

void MainWindow::onUserItemDoubleClicked(QListWidgetItem* item)
{
    QString user_id = item->data(Qt::UserRole).toString();
    
    if (user_id == QStringLiteral("loopback")) {
        openChatWindow(m_loopback_user, true);
        return;
    }

    if (m_network_manager) {
        auto users = m_network_manager->getOnlineUsers();
        for (const auto& user : users) {
            if (user.user_id == user_id) {
                openChatWindow(user, true);
                break;
            }
        }
    }
}

void MainWindow::onUserItemContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_user_list->itemAt(pos);
    if (item) {
        m_current_context_item = item;
        m_user_context_menu->exec(m_user_list->mapToGlobal(pos));
    }
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* item = m_user_list->item(i);
        QString username = item->text();
        
        bool matches = username.contains(text, Qt::CaseInsensitive);
        item->setHidden(!matches);
    }
}

void MainWindow::onStatusChanged(int index)
{
    if (m_network_manager) {
        UserStatus status = static_cast<UserStatus>(
            m_status_combo->itemData(index).toInt());
        
        m_network_manager->updateLocalUserStatus(status, 
            m_status_text_edit->text().toStdString());
    }
}

void MainWindow::onStatusTextChanged(const QString& text)
{
    if (m_network_manager) {
        UserStatus status = static_cast<UserStatus>(
            m_status_combo->currentData().toInt());
        
        m_network_manager->updateLocalUserStatus(status, text.toStdString());
    }
}

// ============================================================================
// 菜单动作
// ============================================================================

void MainWindow::onSendMessage()
{
    if (m_current_context_item) {
        onUserItemDoubleClicked(m_current_context_item);
    } else {
        openChatWindow(m_loopback_user, true);
    }
}

void MainWindow::onSendFile()
{
    // TODO: 实现文件发送对话框
    QMessageBox::information(this, "提示", "文件发送功能开发中...");
}

void MainWindow::onSendScreenshot()
{
    // TODO: 实现截图工具
    QMessageBox::information(this, "提示", "截图工具开发中...");
}

void MainWindow::onOpenAIChat()
{
    // 创建AI助手聊天窗口
    UserInfo ai_user;
    ai_user.user_id = "ai_assistant";
    ai_user.username = "AI助手";
    ai_user.status = UserStatus::Online;
    
    ChatWindow* chat_window = openChatWindow(ai_user, true);
    if (chat_window && m_ai_service) {
        chat_window->setAIService(m_ai_service);
    }
}

void MainWindow::onSettings()
{
    // TODO: 实现设置对话框
    QMessageBox::information(this, "提示", "设置功能开发中...");
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于麒麟信使",
        "<h2>麒麟信使 v1.0.0</h2>"
        "<p>一个现代化的P2P局域网通讯应用</p>"
        "<p>集成NPU加速的AI功能</p>"
        "<p><b>特性:</b></p>"
        "<ul>"
        "<li>无服务器P2P通信</li>"
        "<li>AI聊天助手</li>"
        "<li>智能图像标注</li>"
        "<li>智能回复建议</li>"
        "</ul>"
        "<p>Copyright © 2025</p>");
}

void MainWindow::onQuit()
{
    QApplication::quit();
}

// ============================================================================
// 系统托盘
// ============================================================================

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) {
            hide();
        } else {
            show();
            activateWindow();
        }
    }
}

void MainWindow::onTrayIconMessageClicked()
{
    show();
    activateWindow();
}

// ============================================================================
// 辅助函数 (待续)
// ============================================================================

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_tray_icon->isVisible()) {
        hide();
        event->ignore();
        
        m_tray_icon->showMessage("麒麟信使",
                                 "应用已最小化到系统托盘",
                                 QSystemTrayIcon::Information,
                                 2000);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    QString username = settings.value("username", "User").toString();
    QString status_text = settings.value("statusText", "").toString();
    
    m_status_text_edit->setText(status_text);
}

void MainWindow::saveSettings()
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("statusText", m_status_text_edit->text());
}

void MainWindow::updateUserList()
{
    if (m_network_manager) {
        int count = m_network_manager->getOnlineUsers().size();
        int total = count + 1; // 包含本机回环测试
        m_user_count_label->setText(QString("在线用户: %1 (含本机测试)").arg(total));
    } else {
        m_user_count_label->setText(QStringLiteral("在线用户: 1 (含本机测试)"));
    }
}

void MainWindow::updateUserListItem(const UserInfo& user_info)
{
    QString user_id = user_info.user_id;
    m_cached_users.insert(user_id, user_info);
    
    // 查找是否已存在
    QListWidgetItem* item = nullptr;
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* existing = m_user_list->item(i);
        if (existing->data(Qt::UserRole).toString() == user_id) {
            item = existing;
            break;
        }
    }
    
    // 创建或更新
    if (!item) {
        item = new QListWidgetItem(m_user_list);
        item->setData(Qt::UserRole, user_id);
    }
    
    // 设置显示文本和图标
    QString display_text = user_info.username;
    if (!user_info.status_text.isEmpty()) {
        display_text += QStringLiteral(" - %1").arg(user_info.status_text);
    }
    
    int unread = m_unread_counts.value(user_id, 0);
    if (unread > 0) {
        display_text += QStringLiteral(" (%1)").arg(unread);
    }

    item->setText(display_text);

    // 状态图标
    QString icon;
    switch (user_info.status) {
        case UserStatus::Online:
            icon = "🟢";
            break;
        case UserStatus::Away:
            icon = "🟡";
            break;
        case UserStatus::Busy:
            icon = "🔴";
            break;
        default:
            icon = "⚫";
            break;
    }
    
    item->setText(icon + " " + display_text);

    QFont font = item->font();
    font.setBold(unread > 0);
    item->setFont(font);
    item->setForeground(unread > 0 ? QBrush(QColor("#d9534f"))
                                   : QBrush(Qt::black));
}

void MainWindow::removeUserListItem(const QString& user_id)
{
    for (int i = 0; i < m_user_list->count(); ++i) {
        QListWidgetItem* item = m_user_list->item(i);
        if (item->data(Qt::UserRole).toString() == user_id) {
            delete m_user_list->takeItem(i);
            break;
        }
    }

    m_unread_counts.remove(user_id);
    m_cached_users.remove(user_id);
}

void MainWindow::ensureLoopbackEntry()
{
    m_loopback_user.user_id = QStringLiteral("loopback");

    if (!m_local_user.username.isEmpty()) {
        m_loopback_user.username = QStringLiteral("本机测试 (%1)")
            .arg(m_local_user.username);
    } else {
        m_loopback_user.username = QStringLiteral("本机测试");
    }

    m_loopback_user.status = UserStatus::Online;
    m_loopback_user.status_text = QStringLiteral("回环测试");

    updateUserListItem(m_loopback_user);
}

ChatWindow* MainWindow::openChatWindow(const UserInfo& user_info, bool activate)
{
    QString user_id = user_info.user_id;
    
    ChatWindow* window = findChatWindow(user_id);
    if (window) {
        window->show();
        if (activate) {
            window->raise();
            window->activateWindow();
        }
        return window;
    }
    
    window = new ChatWindow(user_info, this);
    window->setNetworkManager(m_network_manager);
    m_chat_windows[user_id] = window;
    
    connect(window, &QWidget::destroyed, this, [this, user_id]() {
        m_chat_windows.remove(user_id);
    });

    connect(window, &ChatWindow::chatActivated, this,
            [this](const QString& id) { markConversationRead(id); });

    connect(window, &ChatWindow::chatClosed, this,
            [this](const QString& id) { markConversationRead(id); });
    
    window->show();
    if (activate) {
        window->raise();
        window->activateWindow();
    }
    markConversationRead(user_id);
    return window;
}

ChatWindow* MainWindow::findChatWindow(const QString& user_id)
{
    return m_chat_windows.value(user_id, nullptr);
}

void MainWindow::showNotification(const QString& title, const QString& message)
{
    if (m_tray_icon) {
        m_tray_icon->showMessage(title, message,
                                 QSystemTrayIcon::Information,
                                 3000);
    }
}

void MainWindow::incrementUnread(const QString& user_id)
{
    if (user_id.isEmpty()) {
        return;
    }

    m_unread_counts[user_id] = m_unread_counts.value(user_id, 0) + 1;

    auto it = m_cached_users.find(user_id);
    if (it != m_cached_users.end()) {
        updateUserListItem(it.value());
    }
}

void MainWindow::markConversationRead(const QString& user_id)
{
    if (user_id.isEmpty()) {
        return;
    }

    if (!m_unread_counts.contains(user_id)) {
        return;
    }

    if (m_unread_counts.value(user_id) == 0) {
        return;
    }

    m_unread_counts[user_id] = 0;

    auto it = m_cached_users.find(user_id);
    if (it != m_cached_users.end()) {
        updateUserListItem(it.value());
    }
}

} // namespace KylinMessenger
