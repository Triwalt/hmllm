// main_window.cpp - 主窗口实现（第1部分）
#include "main_window.h"
#include "chat_window.h"
#include "core/di/service_locator.h"
#include "core/repositories/contact_repository.h"
#include "version_info.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QStyle>
#include <QLocale>
#include <QFile>
#include <QPalette>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QDir>
#include <QStringList>
#include <QFormLayout>
#include <QGroupBox>
#include <QMap>
#include <QHash>
#include <QSet>
#include <QTabWidget>
#include <QStackedWidget>
#include <algorithm>

namespace KylinMessenger {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_network_manager(nullptr)
    , m_stacked_widget(nullptr)
    , m_user_list_page(nullptr)
    , m_contact_list_page(nullptr)
    , m_group_list_page(nullptr)
    , m_tab_widget(nullptr)
    , m_status_combo(nullptr)
    , m_status_text_edit(nullptr)
    , m_user_count_label(nullptr)
    , m_tray_icon(nullptr)
    , m_current_context_item(nullptr)
    , m_view_info_action(nullptr)
    , m_current_theme(QStringLiteral("light"))
{
    qInfo() << "[MainWindow] 构造函数开始";
    qInfo() << "[MainWindow] 开始 setupUI()";
    setupUI();
    qInfo() << "[MainWindow] setupUI() 完成";
    
    qInfo() << "[MainWindow] 开始 setupMenuBar()";
    setupMenuBar();
    qInfo() << "[MainWindow] setupMenuBar() 完成";
    
    qInfo() << "[MainWindow] 开始 setupToolBar()";
    setupToolBar();
    qInfo() << "[MainWindow] setupToolBar() 完成";
    
    qInfo() << "[MainWindow] 开始 setupStatusBar()";
    setupStatusBar();
    qInfo() << "[MainWindow] setupStatusBar() 完成";
    
    qInfo() << "[MainWindow] 开始 setupSystemTray()";
    setupSystemTray();
    qInfo() << "[MainWindow] setupSystemTray() 完成";
    
    qInfo() << "[MainWindow] 开始 setupConnections()";
    setupConnections();
    qInfo() << "[MainWindow] setupConnections() 完成";
    
    qInfo() << "[MainWindow] 开始解析 MessageRepository";
    m_message_repository = Core::DI::ServiceLocator::instance()
                               .resolve<Core::Repositories::MessageRepository>();
    qInfo() << "[MainWindow] MessageRepository 解析完成";
    
    // 在UI组件创建后再加载设置
    qInfo() << "[MainWindow] 开始 loadSettings()";
    loadSettings();
    qInfo() << "[MainWindow] loadSettings() 完成";
    
    qInfo() << "[MainWindow] 开始 ensureLoopbackEntry()";
    ensureLoopbackEntry();
    qInfo() << "[MainWindow] ensureLoopbackEntry() 完成";
    
    setWindowTitle("麒麟信使 - Kylin Messenger");
    resize(300, 600);
    qInfo() << "[MainWindow] 构造函数完成";
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
    qInfo() << "[MainWindow::setNetworkManager] 开始";
    m_network_manager = network_manager;
    qInfo() << "[MainWindow::setNetworkManager] NetworkManager 已设置";
    
    if (!m_message_repository) {
        qInfo() << "[MainWindow::setNetworkManager] 解析 MessageRepository";
        m_message_repository = Core::DI::ServiceLocator::instance()
                                     .resolve<Core::Repositories::MessageRepository>();
    }
    
    // 初始化联系人存储库
    if (!m_contact_repository) {
        qInfo() << "[MainWindow::setNetworkManager] 创建 ContactRepository";
        m_contact_repository = std::make_shared<Core::Repositories::SettingsContactRepository>();
    }
    
    // 初始化各个页面
    qInfo() << "[MainWindow::setNetworkManager] 设置 UserListPage";
    if (m_user_list_page) {
        m_user_list_page->setNetworkManager(network_manager);
    }
    
    qInfo() << "[MainWindow::setNetworkManager] 设置 ContactListPage";
    if (m_contact_list_page) {
        qInfo() << "[MainWindow::setNetworkManager] 调用 ContactListPage::setNetworkManager";
        m_contact_list_page->setNetworkManager(network_manager);
        qInfo() << "[MainWindow::setNetworkManager] 调用 ContactListPage::setContactRepository";
        m_contact_list_page->setContactRepository(m_contact_repository);
        qInfo() << "[MainWindow::setNetworkManager] ContactListPage 设置完成";
    }
    
    qInfo() << "[MainWindow::setNetworkManager] 设置 GroupListPage";
    if (m_group_list_page) {
        m_group_list_page->setNetworkManager(network_manager);
    }
    
    qInfo() << "[MainWindow::setNetworkManager] 开始连接网络事件信号";
    if (m_network_manager) {
        qInfo() << "[MainWindow::setNetworkManager] 连接 userOnline 信号";
        connect(m_network_manager, &NetworkManager::userOnline,
                this, &MainWindow::onUserOnline);
        
        qInfo() << "[MainWindow::setNetworkManager] 连接 userOffline 信号";
        connect(m_network_manager, &NetworkManager::userOffline,
                this, &MainWindow::onUserOffline);
        
        connect(m_network_manager, &NetworkManager::userInfoUpdated,
                this, &MainWindow::onUserInfoUpdated);
        
        connect(m_network_manager, &NetworkManager::messageReceived,
                this, &MainWindow::onMessageReceived);
        
        qInfo() << "[MainWindow::setNetworkManager] 连接 groupMessageReceived 信号";
        connect(m_network_manager, &NetworkManager::groupMessageReceived,
                this, &MainWindow::onGroupMessageReceived);

        connect(m_network_manager, &NetworkManager::groupMembersUpdated,
                this, [this](const QString& group_id, const QList<Core::UserInfo>& members) {
                    QString key = QStringLiteral("group:%1").arg(group_id);
                    ChatWindow* wnd = findChatWindow(key);
                    if (wnd) {
                        wnd->updateGroupMembers(members);
                    }
                });

        // 文件接收流程：收到文件提供后提示保存并调用 acceptFile
        connect(m_network_manager, &NetworkManager::fileOfferReceived,
                this, [this](const QString& senderId,
                             const QString& senderIp,
                             quint32 /*packetNo*/,
                             quint32 /*fileId*/,
                             const QString& filename,
                             quint64 filesize) {
            Q_UNUSED(senderIp);
            const QString size_text = filesize == 0
                                           ? QStringLiteral("未知大小")
                                           : QLocale().formattedDataSize(static_cast<qint64>(filesize));

            QString display_id = senderId;

            Core::UserInfo info;
            bool have_info = false;
            if (m_cached_users.contains(senderId)) {
                info = m_cached_users.value(senderId);
                have_info = true;
            } else if (m_network_manager) {
                const auto users = m_network_manager->getOnlineUsers();
                for (const auto& user : users) {
                    if (user.user_id == senderId) {
                        info = user;
                        have_info = true;
                        break;
                    }
                }
            }

            if (have_info) {
                display_id = userDisplayName(info);
                openChatWindow(info, false);
            }

            statusBar()->showMessage(
                QStringLiteral("收到来自 %1 的文件：%2 (%3)")
                    .arg(display_id, filename, size_text),
                4000);
            showNotification(QStringLiteral("收到文件"),
                             QStringLiteral("来自 %1 的文件：%2")
                                 .arg(display_id, filename));
        });
        connect(m_network_manager, &NetworkManager::fileTransferProgress,
                this, [this](const QString& peerId, quint32, quint32, quint64 done, quint64 total) {
            if (total == 0) return;
            const int percent = static_cast<int>((done * 100) / total);
            statusBar()->showMessage(
                QStringLiteral("正在接收来自 %1 的文件：%2%").arg(peerId).arg(percent), 2000);
        });
        connect(m_network_manager, &NetworkManager::fileTransferFinished,
                this, [this](const QString& peerId, quint32, quint32, const QString& path) {
            showNotification(QStringLiteral("接收完成"),
                             QStringLiteral("来自 %1 的文件已保存到：%2")
                                 .arg(peerId, QDir::toNativeSeparators(path)));
        });
        connect(m_network_manager, &NetworkManager::fileTransferFailed,
                this, [this](const QString& peerId, quint32, quint32, const QString& reason) {
            showNotification(QStringLiteral("接收失败"),
                             QStringLiteral("来自 %1 的文件接收失败：%2")
                                 .arg(peerId, reason));
        });
        
        qInfo() << "[MainWindow::setNetworkManager] 获取本地用户信息";
        m_local_user = m_network_manager->getLocalUser();
        qInfo() << "[MainWindow::setNetworkManager] 确保回环条目";
        ensureLoopbackEntry();
        qInfo() << "[MainWindow::setNetworkManager] 更新 FeiQ 详情";
        m_network_manager->updateLocalFeiqDetails(m_local_feiq_details);
    }
    qInfo() << "[MainWindow::setNetworkManager] 完成";
}

void MainWindow::setAIService(std::shared_ptr<IAIService> ai_service)
{
    m_ai_service = ai_service;
}

void MainWindow::setComplianceService(std::shared_ptr<IComplianceService> compliance_service)
{
    m_compliance_service = compliance_service;

    for (auto it = m_chat_windows.begin(); it != m_chat_windows.end(); ++it) {
        if (it.value()) {
            it.value()->setComplianceService(m_compliance_service);
        }
    }
}

// ============================================================================
// UI设置
// ============================================================================

void MainWindow::setupUI()
{
    qInfo() << "[MainWindow::setupUI] 开始";
    
    // 创建标签页组件来管理多个页面
    qInfo() << "[MainWindow::setupUI] 创建 QTabWidget";
    m_tab_widget = new QTabWidget(this);
    
    // 创建各个页面
    qInfo() << "[MainWindow::setupUI] 创建 UserListPage";
    m_user_list_page = new UserListPage(this);
    qInfo() << "[MainWindow::setupUI] UserListPage 创建完成";
    
    qInfo() << "[MainWindow::setupUI] 创建 ContactListPage";
    m_contact_list_page = new ContactListPage(this);
    qInfo() << "[MainWindow::setupUI] ContactListPage 创建完成";
    
    qInfo() << "[MainWindow::setupUI] 创建 GroupListPage";
    m_group_list_page = new GroupListPage(this);
    qInfo() << "[MainWindow::setupUI] GroupListPage 创建完成";
    
    // 添加到标签页
    qInfo() << "[MainWindow::setupUI] 添加标签页";
    m_tab_widget->addTab(m_user_list_page, "在线用户");
    m_tab_widget->addTab(m_contact_list_page, "联系人");
    m_tab_widget->addTab(m_group_list_page, "群组");
    qInfo() << "[MainWindow::setupUI] 标签页添加完成";
    
    // 状态栏（放在底部）
    qInfo() << "[MainWindow::setupUI] 创建状态栏组件";
    QWidget* status_widget = new QWidget(this);
    QHBoxLayout* status_layout = new QHBoxLayout(status_widget);
    status_layout->setContentsMargins(5, 5, 5, 5);
    
    qInfo() << "[MainWindow::setupUI] 创建状态组合框";
    m_status_combo = new QComboBox(this);
    m_status_combo->addItem(statusIcon(Core::UserStatus::Online), "在线", static_cast<int>(Core::UserStatus::Online));
    m_status_combo->addItem(statusIcon(Core::UserStatus::Away), "离开", static_cast<int>(Core::UserStatus::Away));
    m_status_combo->addItem(statusIcon(Core::UserStatus::Busy), "忙碌", static_cast<int>(Core::UserStatus::Busy));
    m_status_combo->addItem(statusIcon(Core::UserStatus::Invisible), "隐身", static_cast<int>(Core::UserStatus::Invisible));
    status_layout->addWidget(m_status_combo);
    
    m_status_text_edit = new QLineEdit(this);
    m_status_text_edit->setPlaceholderText("状态消息");
    status_layout->addWidget(m_status_text_edit, 1);
    
    // 主布局
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
    
    main_layout->addWidget(m_tab_widget, 1);
    main_layout->addWidget(status_widget);
    
    setCentralWidget(central_widget);
    
    // 连接页面切换信号
    qInfo() << "[MainWindow::setupUI] 连接页面切换信号";
    connect(m_tab_widget, &QTabWidget::currentChanged, this, &MainWindow::onPageChanged);
    
    // 连接用户列表页面信号
    qInfo() << "[MainWindow::setupUI] 连接用户列表页面信号";
    connect(m_user_list_page, &UserListPage::userDoubleClicked, this, [this](const Core::UserInfo& user_info) {
        openChatWindow(user_info, true);
    });
    connect(m_user_list_page, &UserListPage::userContextMenuRequested, this, [this](const QPoint& pos, const Core::UserInfo& user_info) {
        showUserContextMenu(pos, user_info);
    });
    
    // 连接联系人页面信号
    qInfo() << "[MainWindow::setupUI] 连接联系人页面信号";
    connect(m_contact_list_page, &ContactListPage::contactDoubleClicked, this, [this](const Core::ContactInfo& contact) {
        // 从联系人创建UserInfo并打开聊天窗口
        Core::UserInfo user_info;
        user_info.user_id = contact.contact_id;
        user_info.username = contact.username;
        user_info.hostname = contact.hostname;
        user_info.ip_address = contact.ip_address;
        openChatWindow(user_info, true);
    });
    
    // 连接群组页面信号
    qInfo() << "[MainWindow::setupUI] 连接群组页面信号";
    connect(m_group_list_page, &GroupListPage::groupDoubleClicked, this, [this](const Core::GroupInfo& group_info) {
        openGroupChatWindow(group_info, true);
    });
    
    // 创建右键菜单
    qInfo() << "[MainWindow::setupUI] 创建右键菜单";
    m_user_context_menu = new QMenu(this);
    m_user_context_menu->addAction("发送消息", this, &MainWindow::onSendMessage);
    m_user_context_menu->addAction("发送文件", this, &MainWindow::onSendFile);
    m_user_context_menu->addSeparator();
    m_view_info_action = m_user_context_menu->addAction("查看信息", this, &MainWindow::onViewUserInfo);
    m_user_context_menu->addAction("添加到联系人", this, &MainWindow::onAddContactFromContext);
    
    qInfo() << "[MainWindow::setupUI] 完成";
}

void MainWindow::setupMenuBar()
{
    // 文件菜单
    m_file_menu = menuBar()->addMenu("文件(&F)");
    m_file_menu->addAction("设置...", this, &MainWindow::onSettings);
    m_file_menu->addSeparator();
    {
        QAction* quitAction = new QAction("退出", this);
        quitAction->setShortcut(QKeySequence::Quit);
        connect(quitAction, &QAction::triggered, this, &MainWindow::onQuit);
        m_file_menu->addAction(quitAction);
    }
    
    // 工具菜单
    m_tools_menu = menuBar()->addMenu("工具(&T)");
    {
        QAction* screenshotAction = new QAction("截图工具", this);
        screenshotAction->setShortcut(QKeySequence("Ctrl+Alt+A"));
        connect(screenshotAction, &QAction::triggered, this, &MainWindow::onSendScreenshot);
        m_tools_menu->addAction(screenshotAction);
    }
    m_tools_menu->addAction("AI聊天助手", this, &MainWindow::onOpenAIChat);
    
    // 帮助菜单
    m_help_menu = menuBar()->addMenu("帮助(&H)");
    m_help_menu->addAction("关于", this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    m_toolbar = addToolBar("主工具栏");
    m_toolbar->setIconSize(QSize(20, 20));

    m_toolbar->addAction(actionIcon("message-square"), "消息", this, &MainWindow::onSendMessage);
    m_toolbar->addAction(actionIcon("file"), "文件", this, &MainWindow::onSendFile);
    m_toolbar->addAction(actionIcon("camera"), "截图", this, &MainWindow::onSendScreenshot);
    auto ai_action = m_toolbar->addAction(actionIcon("cpu"), "AI", this, &MainWindow::onOpenAIChat);
    ai_action->setToolTip("AI助手");
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
    m_tray_icon->setIcon(QIcon(":/icons/app_icon.svg"));
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
    // 状态变更
    if (m_status_combo) {
    connect(m_status_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStatusChanged);
    }
    if (m_status_text_edit) {
    connect(m_status_text_edit, &QLineEdit::textChanged,
            this, &MainWindow::onStatusTextChanged);
    }
    
    // 系统托盘
    if (m_tray_icon) {
    connect(m_tray_icon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
    connect(m_tray_icon, &QSystemTrayIcon::messageClicked,
            this, &MainWindow::onTrayIconMessageClicked);
    }
    
    // 注意：网络事件的连接在 setNetworkManager 中处理
    // 因为此时 m_network_manager 可能还未设置
}

// ============================================================================
// 网络事件处理
// ============================================================================

void MainWindow::onUserOnline(const Core::UserInfo& user_info)
{
    // 转发到用户列表页面
    if (m_user_list_page) {
        m_user_list_page->onUserOnline(user_info);
    }
    
    // 转发到联系人页面（更新联系人信息）
    if (m_contact_list_page) {
        m_contact_list_page->onUserOnline(user_info);
    }
    
    // 更新缓存
    m_cached_users.insert(user_info.user_id, user_info);
    
    const QString display_name = userDisplayName(user_info);
    const QString ip = user_info.ip_address.trimmed();

    QString message;
    if (ip.isEmpty() || ip == display_name) {
        message = QStringLiteral("%1 上线了").arg(display_name);
    } else {
        message = QStringLiteral("%1 (%2) 上线了")
                      .arg(display_name, ip);
    }
    
    statusBar()->showMessage(message, 3000);
    showNotification("用户上线", message);
    
    updateUserList();
}

void MainWindow::onUserOffline(const QString& user_id)
{
    // 转发到用户列表页面
    if (m_user_list_page) {
        m_user_list_page->onUserOffline(user_id);
    }
    
    m_cached_users.remove(user_id);
    updateUserList();
}

void MainWindow::onUserInfoUpdated(const Core::UserInfo& user_info)
{
    // 转发到用户列表页面
    if (m_user_list_page) {
        m_user_list_page->onUserInfoUpdated(user_info);
    }
    
    // 转发到联系人页面
    if (m_contact_list_page) {
        m_contact_list_page->onUserInfoUpdated(user_info);
    }
    
    // 更新缓存
    m_cached_users.insert(user_info.user_id, user_info);

    ChatWindow* chat = findChatWindow(user_info.user_id);
    if (chat) {
        chat->updatePeerInfo(user_info);
    }
}

void MainWindow::onMessageReceived(const Core::ChatMessage& message)
{
    QString sender_id = message.sender_id;
    Core::UserInfo sender_info;
    bool has_sender_info = false;
    
    // 查找或创建聊天窗口
    ChatWindow* chat_window = findChatWindow(sender_id);
    
    if (!chat_window) {
        // 从在线用户列表获取用户信息
        if (m_network_manager) {
            const auto users = m_network_manager->getOnlineUsers();
            for (const auto& user : users) {
                if (user.user_id == sender_id) {
                    sender_info = user;
                    has_sender_info = true;
                    break;
                }
            }
        }
        
        if (has_sender_info) {
            chat_window = openChatWindow(sender_info, false);
        }
    }
    else if (m_cached_users.contains(sender_id)) {
        sender_info = m_cached_users.value(sender_id);
        has_sender_info = true;
    }
    
    if (chat_window) {
        bool wasActive = chat_window->isActiveWindow();
        chat_window->addReceivedMessage(message);

        if (!wasActive) {
            incrementUnread(sender_id);
            QString display_name = sender_id;
            QString ip;

            if (has_sender_info) {
                display_name = userDisplayName(sender_info);
                ip = sender_info.ip_address;
            } else if (m_cached_users.contains(sender_id)) {
                const Core::UserInfo& cached = m_cached_users.value(sender_id);
                display_name = userDisplayName(cached);
                ip = cached.ip_address;
            }

            if (!ip.isEmpty() && ip != display_name) {
                display_name += QStringLiteral(" [%1]").arg(ip);
            }

            QString notification = QStringLiteral("%1: %2")
                .arg(display_name)
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

    if (m_message_repository) {
        m_message_repository->saveMessage(message, conversationKey(sender_id));
    }
}

// ============================================================================
// UI事件处理
// ============================================================================

void MainWindow::showUserContextMenu(const QPoint& pos, const Core::UserInfo& user_info)
{
    m_context_user_info = user_info;
    m_user_context_menu->exec(pos);
}

void MainWindow::onPageChanged(int index)
{
    // 页面切换时的处理
    Q_UNUSED(index);
    // 可以在这里更新状态栏或其他UI
}

void MainWindow::onViewUserInfo()
{
    if (m_context_user_info.user_id.isEmpty()) {
        QMessageBox::information(this, "提示", "请从用户列表选择用户再查看信息");
        return;
    }
    QMap<QString, QString> details;
    if (m_network_manager) {
        details = m_network_manager->getUserDetails(m_context_user_info.user_id);
    }
    showUserInfoDialog(m_context_user_info, details);
}

void MainWindow::onAddContactFromContext()
{
    if (!m_contact_repository) {
        QMessageBox::warning(this, "提示", "联系人仓库未初始化");
        return;
    }
    if (m_context_user_info.user_id.isEmpty()) {
        QMessageBox::information(this, "提示", "请从用户列表选择用户");
        return;
    }
    Core::ContactInfo c;
    c.contact_id = m_context_user_info.user_id;
    c.display_name = userDisplayName(m_context_user_info);
    c.username = m_context_user_info.username;
    c.hostname = m_context_user_info.hostname;
    c.ip_address = m_context_user_info.ip_address;
    c.group_name = m_context_user_info.group_name;
    c.notes = QString();
    c.created_at = QDateTime::currentDateTime();
    c.last_seen = QDateTime::currentDateTime();
    if (m_contact_repository->saveContact(c)) {
        QMessageBox::information(this, "成功", QStringLiteral("已将 %1 添加到联系人").arg(c.display_name));
        if (m_contact_list_page) {
            m_contact_list_page->refreshContactList();
        }
    } else {
        QMessageBox::warning(this, "失败", "保存联系人失败");
    }
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    // 搜索功能已迁移到 UserListPage
    // 这里保留空实现以保持兼容性
    Q_UNUSED(text);
    if (m_user_list_page) {
        m_user_list_page->onSearchTextChanged(text);
    }
}

void MainWindow::onStatusChanged(int index)
{
    if (m_network_manager) {
        Core::UserStatus status = static_cast<Core::UserStatus>(
            m_status_combo->itemData(index).toInt());
        
        m_network_manager->updateLocalUserStatus(status, 
            m_status_text_edit->text().toStdString());
    }
}

void MainWindow::onStatusTextChanged(const QString& text)
{
    if (m_network_manager) {
        Core::UserStatus status = static_cast<Core::UserStatus>(
            m_status_combo->currentData().toInt());
        
        m_network_manager->updateLocalUserStatus(status, text.toStdString());
    }
}

// ============================================================================
// 菜单动作
// ============================================================================

void MainWindow::onSendMessage()
{
    // 如果没有选中的用户，打开回环测试窗口
        openChatWindow(m_loopback_user, true);
}

void MainWindow::onSendFile()
{
    ChatWindow* target_window = nullptr;
    QString user_id;

    if (m_current_context_item) {
        user_id = m_current_context_item->data(Qt::UserRole).toString();
    }

    if (user_id.isEmpty()) {
        target_window = openChatWindow(m_loopback_user, true);
    } else if (user_id == QStringLiteral("loopback")) {
        target_window = openChatWindow(m_loopback_user, true);
    } else if (m_cached_users.contains(user_id)) {
        target_window = openChatWindow(m_cached_users.value(user_id), true);
    } else if (m_network_manager) {
        const auto users = m_network_manager->getOnlineUsers();
        for (const auto& user : users) {
            if (user.user_id == user_id) {
                target_window = openChatWindow(user, true);
                break;
            }
        }
    }

    if (!target_window) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请选择要发送文件的联系人。"));
        return;
    }

    target_window->onSendFile();
}

void MainWindow::onSendScreenshot()
{
    // TODO: 实现截图工具
    QMessageBox::information(this, "提示", "截图工具开发中...");
}

void MainWindow::onOpenAIChat()
{
    // 创建AI助手聊天窗口
    Core::UserInfo ai_user;
    ai_user.user_id = "ai_assistant";
    ai_user.username = "AI助手";
    ai_user.status = Core::UserStatus::Online;
    
    ChatWindow* chat_window = openChatWindow(ai_user, true);
    if (chat_window && m_ai_service) {
        chat_window->setAIService(m_ai_service);
    }
}

void MainWindow::onSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("设置");
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setMinimumWidth(320);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QGroupBox* appearance_group = new QGroupBox("界面主题", &dialog);
    QVBoxLayout* appearance_layout = new QVBoxLayout(appearance_group);

    QLabel* theme_label = new QLabel("选择主题", appearance_group);
    QFont label_font = theme_label->font();
    label_font.setBold(true);
    theme_label->setFont(label_font);
    appearance_layout->addWidget(theme_label);

    QComboBox* theme_combo = new QComboBox(appearance_group);
    theme_combo->addItem("现代浅色", QVariant::fromValue(QStringLiteral("light")));
    theme_combo->addItem("现代暗色", QVariant::fromValue(QStringLiteral("dark")));
    theme_combo->addItem("系统默认", QVariant::fromValue(QStringLiteral("system")));

    int current_index = theme_combo->findData(m_current_theme);
    if (current_index < 0) {
        current_index = theme_combo->findData(QStringLiteral("light"));
    }
    theme_combo->setCurrentIndex(current_index);
    appearance_layout->addWidget(theme_combo);

    QLabel* hint = new QLabel("主题切换将立即生效，并会在下次启动时自动应用。", appearance_group);
    hint->setWordWrap(true);
    appearance_layout->addWidget(hint);

    layout->addWidget(appearance_group);

    layout->addSpacing(12);

    QGroupBox* image_group = new QGroupBox("图片接收", &dialog);
    QVBoxLayout* image_layout = new QVBoxLayout(image_group);

    QCheckBox* auto_image_checkbox = new QCheckBox("自动下载并显示收到的图片", image_group);
    // 确保从设置中读取最新状态
    QSettings settings_check("KylinMessenger", "KylinMessenger");
    bool saved_enabled = settings_check.value("autoImageDownload/enabled", m_auto_download_images).toBool();
    auto_image_checkbox->setChecked(saved_enabled);
    image_layout->addWidget(auto_image_checkbox);

    QHBoxLayout* path_layout = new QHBoxLayout();
    QLineEdit* auto_image_path_edit = new QLineEdit(image_group);
    auto_image_path_edit->setPlaceholderText(defaultImageDownloadDir());
    auto_image_path_edit->setText(m_auto_image_download_dir.isEmpty() ? defaultImageDownloadDir()
                                                                     : m_auto_image_download_dir);
    QPushButton* browse_button = new QPushButton("浏览...", image_group);
    path_layout->addWidget(auto_image_path_edit, 1);
    path_layout->addWidget(browse_button);
    image_layout->addLayout(path_layout);

    QLabel* image_hint = new QLabel("启用后，符合条件的图片会自动保存到指定目录并直接显示。", image_group);
    image_hint->setWordWrap(true);
    image_layout->addWidget(image_hint);

    const auto setImageControlsEnabled = [auto_image_path_edit, browse_button](bool enabled) {
        auto_image_path_edit->setEnabled(enabled);
        browse_button->setEnabled(enabled);
    };

    setImageControlsEnabled(auto_image_checkbox->isChecked());
    QObject::connect(auto_image_checkbox, &QCheckBox::toggled,
                     &dialog, [auto_image_path_edit, browse_button](bool enabled) {
                         auto_image_path_edit->setEnabled(enabled);
                         browse_button->setEnabled(enabled);
                     });
    QObject::connect(browse_button, &QPushButton::clicked, &dialog, [auto_image_path_edit, this]() {
        const QString initial = auto_image_path_edit->text().trimmed().isEmpty()
            ? defaultImageDownloadDir()
            : auto_image_path_edit->text().trimmed();
        const QString selected = QFileDialog::getExistingDirectory(nullptr,
                                                                   QStringLiteral("选择图片保存目录"),
                                                                   initial);
        if (!selected.isEmpty()) {
            auto_image_path_edit->setText(QDir::toNativeSeparators(selected));
        }
    });

    layout->addWidget(image_group);

    layout->addSpacing(12);

    QGroupBox* feiq_group = new QGroupBox("FeiQ 兼容信息", &dialog);
    QFormLayout* feiq_layout = new QFormLayout(feiq_group);
    QHash<QString, QLineEdit*> feiq_editors;

    struct FieldDef {
        QString key;
        QString label;
        QString placeholder;
    };

    const QList<FieldDef> field_defs{
        {QStringLiteral("nickname"), QStringLiteral("昵称"), m_local_user.username},
        {QStringLiteral("group"), QStringLiteral("群组"), m_local_user.group_name},
        {QStringLiteral("mac_address"), QStringLiteral("MAC 地址"), QString()},
        {QStringLiteral("login_name"), QStringLiteral("登录名"), QString()},
        {QStringLiteral("department"), QStringLiteral("部门"), QString()},
        {QStringLiteral("position"), QStringLiteral("职位"), QString()},
        {QStringLiteral("phone"), QStringLiteral("电话"), QString()},
        {QStringLiteral("mobile"), QStringLiteral("手机"), QString()},
        {QStringLiteral("ip_address"), QStringLiteral("IP 地址"), m_local_user.ip_address},
        {QStringLiteral("user_level"), QStringLiteral("用户级别"), QStringLiteral("10000001")},
        {QStringLiteral("capabilities"), QStringLiteral("能力标识"), QString()},
        {QStringLiteral("signature"), QStringLiteral("签名信息"), m_local_user.status_text},
        {QStringLiteral("host_name"), QStringLiteral("主机信息"), m_local_user.hostname},
        {QStringLiteral("client_type"), QStringLiteral("客户端类型"), QStringLiteral("KD")},
        {QStringLiteral("client_version"), QStringLiteral("客户端版本"), QStringLiteral("1.0.0")}
    };

    for (const FieldDef& def : field_defs) {
        QLineEdit* edit = new QLineEdit(feiq_group);
        edit->setText(m_local_feiq_details.value(def.key));
        if (!def.placeholder.trimmed().isEmpty()) {
            edit->setPlaceholderText(def.placeholder.trimmed());
        }
        feiq_layout->addRow(def.label + QStringLiteral(":"), edit);
        feiq_editors.insert(def.key, edit);
    }

    QLabel* feiq_hint = new QLabel("留空表示使用程序默认信息；填写后将按 FeiQ 格式广播。", feiq_group);
    feiq_hint->setWordWrap(true);
    feiq_layout->addRow(feiq_hint);

    layout->addWidget(feiq_group);

    layout->addStretch(1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal,
                                                     &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QSettings settings("KylinMessenger", "KylinMessenger");

        const QString selected_theme = theme_combo->currentData().toString();
        if (selected_theme != m_current_theme) {
            applyTheme(selected_theme);
            settings.setValue("theme", m_current_theme);
        }

        QMap<QString, QString> new_feiq_details;
        for (auto it = feiq_editors.cbegin(); it != feiq_editors.cend(); ++it) {
            const QString value = it.value()->text().trimmed();
            if (!value.isEmpty()) {
                new_feiq_details.insert(it.key(), value);
            }
        }

        if (new_feiq_details != m_local_feiq_details) {
            m_local_feiq_details = new_feiq_details;
            if (m_network_manager) {
                m_network_manager->updateLocalFeiqDetails(m_local_feiq_details);
            }
        }

        QVariantMap feiq_variant;
        for (auto it = m_local_feiq_details.cbegin(); it != m_local_feiq_details.cend(); ++it) {
            feiq_variant.insert(it.key(), it.value());
        }
        settings.setValue("feiqProfile", feiq_variant);

        bool requested_auto = auto_image_checkbox->isChecked();
        QString requested_dir = auto_image_path_edit->text().trimmed();
        if (requested_dir.isEmpty()) {
            requested_dir = m_auto_image_download_dir.isEmpty()
                ? defaultImageDownloadDir()
                : m_auto_image_download_dir;
        }

        QString normalized_dir = requested_dir;
        bool path_valid = true;
        if (requested_auto) {
            QDir dir(normalized_dir);
            if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
                QMessageBox::warning(&dialog,
                                     QStringLiteral("提示"),
                                     QStringLiteral("无法创建图片保存目录：%1")
                                         .arg(QDir::toNativeSeparators(normalized_dir)));
                path_valid = false;
            } else {
                normalized_dir = QDir::toNativeSeparators(dir.absolutePath());
            }
        } else if (!normalized_dir.isEmpty()) {
            normalized_dir = QDir::toNativeSeparators(QDir(normalized_dir).absolutePath());
        }

        if (path_valid) {
            settings.setValue("autoImageDownload/enabled", requested_auto);
            settings.setValue("autoImageDownload/path", normalized_dir);

            const bool state_changed = (requested_auto != m_auto_download_images) ||
                                        (normalized_dir != m_auto_image_download_dir);
            m_auto_download_images = requested_auto;
            m_auto_image_download_dir = normalized_dir;
            if (state_changed) {
                for (ChatWindow* window : std::as_const(m_chat_windows)) {
                    applyAutoDownloadConfig(window);
                }
            }
        }

        settings.sync();
    }
}

void MainWindow::onAbout()
{
    const QString aboutHtml = QStringLiteral(
        "<h2>麒麟信使 v%1</h2>"
        "<p>Git标识: %2</p>"
        "<p>构建时间: %3</p>"
        "<hr/>"
        "<p>一个现代化的P2P局域网通讯应用</p>"
        "<p>集成NPU加速的AI功能</p>"
        "<p><b>特性:</b></p>"
        "<ul>"
        "<li>无服务器P2P通信</li>"
        "<li>AI聊天助手</li>"
        "<li>智能图像标注</li>"
        "<li>智能回复建议</li>"
        "</ul>"
        "<p>Copyright © 2025</p>")
                                   .arg(QString::fromUtf8(KylinMessenger::kAppVersion))
                                   .arg(QString::fromUtf8(KylinMessenger::kGitDescribe))
                                   .arg(QString::fromUtf8(KylinMessenger::kBuildTimestamp));

    QMessageBox::about(this, QStringLiteral("关于麒麟信使"), aboutHtml);
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
    if (m_tray_icon && m_tray_icon->isVisible()) {
        hide();
        event->ignore();
        
        m_tray_icon->showMessage("麒麟信使",
                                 "应用已最小化到系统托盘",
                                 QSystemTrayIcon::Information,
                                 2000);
    } else {
        event->accept();
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    const QString theme = settings.value("theme", QStringLiteral("light")).toString();
    applyTheme(theme, true);
    
    const QString status_text = settings.value("statusText", "").toString();
    if (m_status_text_edit) {
    m_status_text_edit->setText(status_text);
    }

    m_local_feiq_details.clear();
    const QVariantMap feiq_variant = settings.value("feiqProfile").toMap();
    for (auto it = feiq_variant.cbegin(); it != feiq_variant.cend(); ++it) {
        const QString value = it.value().toString().trimmed();
        if (!value.isEmpty()) {
            m_local_feiq_details.insert(it.key(), value);
        }
    }

    m_auto_download_images = settings.value("autoImageDownload/enabled", true).toBool();
    QString configured_dir = settings.value("autoImageDownload/path", defaultImageDownloadDir()).toString().trimmed();
    if (configured_dir.isEmpty()) {
        configured_dir = defaultImageDownloadDir();
    }
    QDir image_dir(configured_dir);
    if (m_auto_download_images && !image_dir.exists()) {
        image_dir.mkpath(QStringLiteral("."));
    }
    m_auto_image_download_dir = QDir::toNativeSeparators(image_dir.absolutePath());

    for (ChatWindow* window : std::as_const(m_chat_windows)) {
        applyAutoDownloadConfig(window);
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("KylinMessenger", "KylinMessenger");
    
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    if (m_status_text_edit) {
    settings.setValue("statusText", m_status_text_edit->text());
    }
    settings.setValue("theme", m_current_theme);

    QVariantMap feiq_variant;
    for (auto it = m_local_feiq_details.cbegin(); it != m_local_feiq_details.cend(); ++it) {
        feiq_variant.insert(it.key(), it.value());
    }
    settings.setValue("feiqProfile", feiq_variant);
    settings.setValue("autoImageDownload/enabled", m_auto_download_images);
    const QString stored_dir = m_auto_image_download_dir.isEmpty()
        ? defaultImageDownloadDir()
        : m_auto_image_download_dir;
    settings.setValue("autoImageDownload/path", stored_dir);
}

void MainWindow::updateUserList()
{
    if (m_user_list_page) {
        m_user_list_page->updateUserList();
    }
    
    if (!m_user_count_label) {
        return;
    }

    if (m_network_manager) {
        const QList<Core::UserInfo> users = m_network_manager->getOnlineUsers();
        const int total = users.size();
        const bool has_loopback = std::any_of(users.cbegin(), users.cend(), [this](const Core::UserInfo& info) {
            if (!m_loopback_user.user_id.isEmpty() && info.user_id == m_loopback_user.user_id) {
                return true;
            }
            if (!m_loopback_user.ip_address.isEmpty() && info.ip_address == m_loopback_user.ip_address) {
                return true;
            }
            return false;
        });

        QString label = QStringLiteral("在线用户: %1").arg(total);
        if (has_loopback && total > 0) {
            label.append(QStringLiteral("（含本机测试）"));
        }
        m_user_count_label->setText(label);
    } else {
        m_user_count_label->setText(QStringLiteral("在线用户: 0"));
    }
}

// updateUserListItem 和 removeUserListItem 已迁移到 UserListPage
// 保留这些方法以保持兼容性，但实际功能由页面组件处理
void MainWindow::updateUserListItem(const Core::UserInfo& user_info)
{
    // 功能已迁移到 UserListPage
    Q_UNUSED(user_info);
}

void MainWindow::removeUserListItem(const QString& user_id)
{
    // 功能已迁移到 UserListPage
    Q_UNUSED(user_id);
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

    if (!m_local_user.ip_address.isEmpty()) {
        m_loopback_user.ip_address = m_local_user.ip_address;
    } else {
        m_loopback_user.ip_address = QStringLiteral("127.0.0.1");
    }

    m_loopback_user.status = Core::UserStatus::Online;
    m_loopback_user.status_text = QStringLiteral("回环测试");

    updateUserListItem(m_loopback_user);

    if (m_network_manager) {
        m_network_manager->registerLoopbackPeer(m_loopback_user);
    }
}

ChatWindow* MainWindow::openChatWindow(const Core::UserInfo& user_info, bool activate)
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
    if (m_ai_service) {
        window->setAIService(m_ai_service);
    }
    if (m_compliance_service) {
        window->setComplianceService(m_compliance_service);
    }
    applyAutoDownloadConfig(window);
    m_chat_windows[user_id] = window;
    
    connect(window, &QWidget::destroyed, this, [this, user_id]() {
        m_chat_windows.remove(user_id);
    });

    connect(window, &ChatWindow::chatActivated, this,
            [this](const QString& id) { markConversationRead(id); });
    
    window->show();
    if (activate) {
        window->raise();
        window->activateWindow();
    }
    
    return window;
}

ChatWindow* MainWindow::openGroupChatWindow(const Core::GroupInfo& group_info, bool activate)
{
    QString group_key = QStringLiteral("group:%1").arg(group_info.group_id);

    ChatWindow* window = findChatWindow(group_key);
    if (window) {
        window->show();
        if (activate) {
            window->raise();
            window->activateWindow();
        }
        return window;
    }

    window = new ChatWindow(group_info, this);
    window->setNetworkManager(m_network_manager);
    if (m_ai_service) {
        window->setAIService(m_ai_service);
    }
    if (m_compliance_service) {
        window->setComplianceService(m_compliance_service);
    }
    applyAutoDownloadConfig(window);
    m_chat_windows[group_key] = window;

    connect(window, &QWidget::destroyed, this, [this, group_key]() {
        m_chat_windows.remove(group_key);
    });

    connect(window, &ChatWindow::chatActivated, this,
            [this](const QString& id) { markConversationRead(id); });

    connect(window, &ChatWindow::chatClosed, this,
            [this](const QString& id) { markConversationRead(id); });

    if (m_message_repository) {
        const QList<ChatMessage> history =
            m_message_repository->recentMessages(conversationKey(group_key));
        const QString local_id = m_network_manager ? m_network_manager->getLocalUserId() : QString();
        for (const auto& msg : history) {
            const bool is_outgoing = !local_id.isEmpty() && msg.sender_id == local_id;
            window->appendHistoryMessage(msg, is_outgoing);
        }
    }

    window->show();
    if (activate) {
        window->raise();
        window->activateWindow();
    }

    markConversationRead(group_key);
    return window;
}

void MainWindow::onGroupMessageReceived(const QString& group_id, const Core::ChatMessage& message)
{
    // 查找或创建群组聊天窗口
    QString group_key = QStringLiteral("group:%1").arg(group_id);
    ChatWindow* chat_window = findChatWindow(group_key);
    
    if (!chat_window) {
        // 从群组列表获取群组信息
        Core::GroupInfo group_info;
        if (m_group_list_page) {
            // 尝试从 GroupListPage 获取群组信息
            // 这里需要添加一个方法来获取群组信息
            // 暂时创建一个基本的群组信息
            group_info.group_id = group_id;
            group_info.group_name = group_id;  // 默认使用ID作为名称
        }
        
        if (!group_info.group_id.isEmpty()) {
            chat_window = openGroupChatWindow(group_info, false);
        } else {
            // 如果无法获取群组信息，创建一个临时群组
            group_info.group_id = group_id;
            group_info.group_name = QStringLiteral("群组 %1").arg(group_id);
            chat_window = openGroupChatWindow(group_info, false);
        }
    }
    
    if (chat_window) {
        bool wasActive = chat_window->isActiveWindow();
        chat_window->addReceivedMessage(message);

        if (!wasActive) {
            incrementUnread(group_key);
            QString display_name = QStringLiteral("群组: %1").arg(group_id);
            
            showNotification(QStringLiteral("群组消息"),
                             QStringLiteral("来自 %1: %2")
                                 .arg(message.sender_id, message.content.left(50)));
        }
    }
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

void MainWindow::showUserInfoDialog(const Core::UserInfo& user_info,
                                    const QMap<QString, QString>& details)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("%1 的信息")
                              .arg(userDisplayName(user_info)));
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QFormLayout* form = new QFormLayout();
    bool hasRows = false;

    const auto addRow = [&](const QString& labelText, const QString& valueText) {
        const QString trimmed = valueText.trimmed();
        if (trimmed.isEmpty()) {
            return;
        }
        QLabel* valueLabel = new QLabel(trimmed, &dialog);
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        valueLabel->setWordWrap(true);
        form->addRow(labelText + QStringLiteral(":"), valueLabel);
        hasRows = true;
    };

    addRow(QStringLiteral("显示名称"), userDisplayName(user_info));
    addRow(QStringLiteral("用户名"), user_info.username);
    addRow(QStringLiteral("群组"), user_info.group_name);
    addRow(QStringLiteral("IP 地址"), user_info.ip_address);
    addRow(QStringLiteral("主机名"), user_info.hostname);
    addRow(QStringLiteral("状态说明"), user_info.status_text);

    const auto labelForKey = [](const QString& key) -> QString {
        static const QHash<QString, QString> labels{
            {QStringLiteral("nickname"), QStringLiteral("FeiQ 昵称")},
            {QStringLiteral("group"), QStringLiteral("FeiQ 群组")},
            {QStringLiteral("mac_address"), QStringLiteral("MAC 地址")},
            {QStringLiteral("login_name"), QStringLiteral("登录名")},
            {QStringLiteral("department"), QStringLiteral("部门")},
            {QStringLiteral("position"), QStringLiteral("职位")},
            {QStringLiteral("phone"), QStringLiteral("电话")},
            {QStringLiteral("mobile"), QStringLiteral("手机")},
            {QStringLiteral("ip_address"), QStringLiteral("FeiQ IP")},
            {QStringLiteral("user_level"), QStringLiteral("用户级别")},
            {QStringLiteral("capabilities"), QStringLiteral("能力标识")},
            {QStringLiteral("signature"), QStringLiteral("签名信息")},
            {QStringLiteral("host_name"), QStringLiteral("主机信息")},
            {QStringLiteral("client_type"), QStringLiteral("客户端类型")},
            {QStringLiteral("client_version"), QStringLiteral("客户端版本")},
            {QStringLiteral("raw_fields"), QStringLiteral("原始字段")}
        };
        return labels.value(key, QStringLiteral("FeiQ 字段 %1").arg(key));
    };

    const QStringList orderedKeys{
        QStringLiteral("nickname"),
        QStringLiteral("group"),
        QStringLiteral("mac_address"),
        QStringLiteral("login_name"),
        QStringLiteral("department"),
        QStringLiteral("position"),
        QStringLiteral("phone"),
        QStringLiteral("mobile"),
        QStringLiteral("ip_address"),
        QStringLiteral("user_level"),
        QStringLiteral("capabilities"),
        QStringLiteral("signature"),
        QStringLiteral("host_name"),
        QStringLiteral("client_type"),
        QStringLiteral("client_version"),
        QStringLiteral("raw_fields")
    };

    QSet<QString> displayedKeys;
    const auto addDetail = [&](const QString& key) {
        if (!details.contains(key) || displayedKeys.contains(key)) {
            return;
        }
        const QString value = details.value(key);
        if (key == QStringLiteral("nickname") && value.trimmed() == user_info.username.trimmed()) {
            return;
        }
        if (key == QStringLiteral("group") && value.trimmed() == user_info.group_name.trimmed()) {
            return;
        }
        if (key == QStringLiteral("ip_address") && value.trimmed() == user_info.ip_address.trimmed()) {
            return;
        }
        addRow(labelForKey(key), value);
        displayedKeys.insert(key);
    };

    for (const QString& key : orderedKeys) {
        addDetail(key);
    }

    for (auto it = details.cbegin(); it != details.cend(); ++it) {
        addDetail(it.key());
    }

    if (!hasRows) {
        QLabel* empty = new QLabel(QStringLiteral("暂无可显示的信息。"), &dialog);
        empty->setAlignment(Qt::AlignCenter);
        layout->addWidget(empty);
    } else {
        layout->addLayout(form);
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttons);

    dialog.resize(420, dialog.sizeHint().height());
    dialog.exec();
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

QString MainWindow::conversationKey(const QString& user_id) const
{
    QStringList participants{m_local_user.user_id, user_id};
    participants.removeAll(QString());
    if (participants.isEmpty()) {
        return QString();
    }
    std::sort(participants.begin(), participants.end());
    return participants.join(QLatin1Char(':'));
}

QString MainWindow::userDisplayName(const Core::UserInfo& user_info) const
{
    const QString username = user_info.username.trimmed();
    if (!username.isEmpty()) {
        return username;
    }

    const QString ip = user_info.ip_address.trimmed();
    if (!ip.isEmpty()) {
        return ip;
    }

    const QString user_id = user_info.user_id.trimmed();
    if (!user_id.isEmpty()) {
        return user_id;
    }

    return QStringLiteral("未知用户");
}

QIcon MainWindow::statusIcon(Core::UserStatus status) const
{
    switch (status) {
    case Core::UserStatus::Online:
        return QIcon(":/icons/status_online.svg");
    case Core::UserStatus::Away:
        return QIcon(":/icons/status_away.svg");
    case Core::UserStatus::Busy:
        return QIcon(":/icons/status_busy.svg");
    case Core::UserStatus::Invisible:
        return QIcon(":/icons/status_invisible.svg");
    default:
        return QIcon(":/icons/status_offline.svg");
    }
}

QIcon MainWindow::actionIcon(const QString& name) const
{
    const QString path = QStringLiteral(":/icons/%1.svg").arg(name);
    QIcon icon(path);
    if (!icon.isNull()) {
        return icon;
    }
    return style()->standardIcon(QStyle::SP_FileIcon);
}

void MainWindow::applyTheme(const QString& theme, bool silent)
{
    QString normalized = theme;
    if (normalized != QStringLiteral("dark") &&
        normalized != QStringLiteral("system") &&
        normalized != QStringLiteral("light")) {
        normalized = QStringLiteral("light");
    }

    QString stylesheet;
    if (normalized != QStringLiteral("system")) {
        const QString path = resolveThemePath(normalized);
        if (!path.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                stylesheet = QString::fromUtf8(file.readAll());
            } else {
                qWarning() << "无法加载主题样式表:" << path;
            }
        }
    }

    if (normalized == QStringLiteral("system")) {
        qApp->setStyleSheet(QString());
        qApp->setPalette(qApp->style()->standardPalette());
    } else {
        qApp->setStyleSheet(stylesheet);
        if (normalized == QStringLiteral("dark")) {
            QPalette palette;
            palette.setColor(QPalette::Window, QColor("#1E1F25"));
            palette.setColor(QPalette::WindowText, QColor("#E9EDF5"));
            palette.setColor(QPalette::Base, QColor("#262936"));
            palette.setColor(QPalette::AlternateBase, QColor("#1F2027"));
            palette.setColor(QPalette::ToolTipBase, QColor("#262936"));
            palette.setColor(QPalette::ToolTipText, QColor("#E9EDF5"));
            palette.setColor(QPalette::Text, QColor("#E9EDF5"));
            palette.setColor(QPalette::Button, QColor("#262936"));
            palette.setColor(QPalette::ButtonText, QColor("#E9EDF5"));
            palette.setColor(QPalette::Highlight, QColor("#526DFF"));
            palette.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
            palette.setColor(QPalette::Link, QColor("#7AA2FF"));
            qApp->setPalette(palette);
        } else {
            qApp->setPalette(qApp->style()->standardPalette());
        }
    }

    m_current_theme = normalized;
    if (!silent && statusBar()) {
        QString human_readable;
        if (normalized == QStringLiteral("dark")) {
            human_readable = QStringLiteral("现代暗色");
        } else if (normalized == QStringLiteral("system")) {
            human_readable = QStringLiteral("系统默认");
        } else {
            human_readable = QStringLiteral("现代浅色");
        }
        statusBar()->showMessage(QStringLiteral("主题已切换为 %1").arg(human_readable), 2500);
    }
}

QString MainWindow::resolveThemePath(const QString& theme) const
{
    if (theme == QStringLiteral("dark")) {
        return QStringLiteral(":/themes/dark.qss");
    }
    if (theme == QStringLiteral("light")) {
        return QStringLiteral(":/themes/light.qss");
    }
    return QString();
}

QString MainWindow::defaultImageDownloadDir() const
{
    QDir base(QCoreApplication::applicationDirPath());
    const QString path = base.filePath(QStringLiteral("media-cache"));
    return QDir::toNativeSeparators(path);
}

void MainWindow::applyAutoDownloadConfig(ChatWindow* window) const
{
    if (!window) {
        return;
    }
    window->setAutoImageDownloadConfig(m_auto_download_images, m_auto_image_download_dir);
}

} // namespace KylinMessenger
