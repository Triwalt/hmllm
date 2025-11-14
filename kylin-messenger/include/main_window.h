// main_window.h - 主窗口界面
#ifndef KYLIN_MESSENGER_MAIN_WINDOW_H
#define KYLIN_MESSENGER_MAIN_WINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <memory>
#include <QStackedWidget>
#include <QIcon>

#include "network_manager.h"
#include "ai_service.h"
#include "compliance_service.h"
#include "core/repositories/message_repository.h"
#include "core/repositories/contact_repository.h"
#include "ui/user_list_page.h"
#include "ui/contact_list_page.h"
#include "ui/group_list_page.h"
// 新增轻量级组件
#include "core/micro_kernel.h"
#include "network/lightweight_discovery.h"
#include "transfer/concurrent_file_transfer.h"
#include "ai/opencv_nsfw_detector.h"

namespace KylinMessenger {

class ChatWindow;

/**
 * @brief 主窗口类
 *
 * 应用程序的主窗口，包含：
 * - 在线用户列表
 * - 用户搜索和过滤
 * - 状态设置
 * - 系统托盘
 * - 菜单和工具栏
 *
 * 支持微内核架构，通过事件驱动与轻量级服务通信
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param micro_kernel 微内核实例（可选，如果提供则启用事件驱动模式）
     * @param parent 父窗口
     */
    explicit MainWindow(Core::MicroKernel* micro_kernel = nullptr, QWidget* parent = nullptr);
    virtual ~MainWindow();
    
    /**
     * @brief 初始化网络管理器
     * @param network_manager 网络管理器实例
     */
    void setNetworkManager(NetworkManager* network_manager);
    
    /**
     * @brief 设置AI服务
     * @param ai_service AI服务实例
     */
    void setAIService(std::shared_ptr<IAIService> ai_service);

    void setComplianceService(std::shared_ptr<IComplianceService> compliance_service);

    /**
     * @brief 设置轻量级网络发现服务
     * @param discovery 轻量级网络发现服务
     */
    void setLightweightDiscovery(std::shared_ptr<Network::LightweightDiscovery> discovery);

    /**
     * @brief 设置并发文件传输服务
     * @param transfer 并发文件传输服务
     */
    void setConcurrentFileTransfer(std::shared_ptr<Transfer::ConcurrentFileTransfer> transfer);

#ifdef ENABLE_AI_FEATURES
    /**
     * @brief 设置NSFW检测器
     * @param detector NSFW检测器
     */
    void setNSFWDetector(std::shared_ptr<AI::LightweightNSFWDetector> detector);
#endif
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    // 网络事件处理
    void onUserOnline(const Core::UserInfo& user_info);
    void onUserOffline(const QString& user_id);
    void onUserInfoUpdated(const Core::UserInfo& user_info);
    void onMessageReceived(const Core::ChatMessage& message);
    void onGroupMessageReceived(const QString& group_id, const Core::ChatMessage& message);

    // 事件驱动处理（微内核架构）
    void onMicroKernelEvent(const Core::Event& event);
    void onFileTransferProgress(const QString& task_id, qint64 transferred, qint64 total);
    void onFileTransferCompleted(const QString& task_id);
    void onFileTransferFailed(const QString& task_id, const QString& error);
    
    // UI事件处理
    void onViewUserInfo();
    void onAddContactFromContext();
    void onSearchTextChanged(const QString& text);
    void onStatusChanged(int index);
    void onStatusTextChanged(const QString& text);
    void showUserContextMenu(const QPoint& pos, const Core::UserInfo& user_info);
    
    // 菜单动作
    void onSendMessage();
    void onSendFile();
    void onSendScreenshot();
    void onOpenAIChat();
    void onSettings();
    void onAbout();
    void onQuit();
    
    // 系统托盘
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayIconMessageClicked();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupConnections();
    
    void loadSettings();
    void saveSettings();
    
    void updateUserList();
    void updateUserListItem(const Core::UserInfo& user_info);
    void removeUserListItem(const QString& user_id);
    void ensureLoopbackEntry();
    void incrementUnread(const QString& user_id);
    void markConversationRead(const QString& user_id);
    QString conversationKey(const QString& user_id) const;
    QString userDisplayName(const Core::UserInfo& user_info) const;
    void showUserInfoDialog(const Core::UserInfo& user_info,
                            const QMap<QString, QString>& details);

    ChatWindow* openChatWindow(const Core::UserInfo& user_info, bool activate = true);
    ChatWindow* openGroupChatWindow(const Core::GroupInfo& group_info, bool activate = true);
    ChatWindow* findChatWindow(const QString& user_id);
    Core::UserInfo getSelectedUser() const { return m_local_user; } // Temporary helper

    void showNotification(const QString& title, const QString& message);
    QIcon statusIcon(Core::UserStatus status) const;
    QIcon actionIcon(const QString& name) const;
    void applyTheme(const QString& theme, bool silent = false);
    QString resolveThemePath(const QString& theme) const;
    QString defaultImageDownloadDir() const;
    void applyAutoDownloadConfig(ChatWindow* window) const;

private:
    // 微内核架构支持
    Core::MicroKernel* m_micro_kernel;
    bool m_use_event_driven;

    // 网络
    NetworkManager* m_network_manager;
    Core::UserInfo m_local_user;
    Core::UserInfo m_loopback_user;

    // AI服务
    std::shared_ptr<IAIService> m_ai_service;
    std::shared_ptr<IComplianceService> m_compliance_service;
    std::shared_ptr<Core::Repositories::MessageRepository> m_message_repository;
    std::shared_ptr<Core::Repositories::IContactRepository> m_contact_repository;

    // 轻量级服务
    std::shared_ptr<Network::LightweightDiscovery> m_lightweight_discovery;
    std::shared_ptr<Transfer::ConcurrentFileTransfer> m_concurrent_transfer;
    std::shared_ptr<AI::LightweightNSFWDetector> m_opencv_nsfw_detector;
    
    // UI组件 - 页面管理
    QStackedWidget* m_stacked_widget;
    UserListPage* m_user_list_page;
    ContactListPage* m_contact_list_page;
    GroupListPage* m_group_list_page;
    
    // 状态栏组件
    QComboBox* m_status_combo;
    QLineEdit* m_status_text_edit;
    QLabel* m_user_count_label;
    
    // 页面切换按钮（可选：使用标签页或侧边栏）
    QTabWidget* m_tab_widget;  // 使用标签页管理页面
    
    // 菜单
    QMenu* m_file_menu;
    QMenu* m_tools_menu;
    QMenu* m_help_menu;
    
    // 工具栏
    QToolBar* m_toolbar;
    
    // 系统托盘
    QSystemTrayIcon* m_tray_icon;
    QMenu* m_tray_menu;
    
    // 聊天窗口管理
    QMap<QString, ChatWindow*> m_chat_windows;
    QHash<QString, int> m_unread_counts;
    QHash<QString, Core::UserInfo> m_cached_users;
    QMap<QString, QString> m_local_feiq_details;
 
    // 右键菜单
    QMenu* m_user_context_menu;
    QListWidgetItem* m_current_context_item;
    QAction* m_view_info_action;

    // 上下文信息
    Core::UserInfo m_context_user_info;  // 当前右键选中的用户信息
    
    // 页面切换槽函数
    void onPageChanged(int index);

    // 主题
    QString m_current_theme;

    bool m_auto_download_images = true;
    QString m_auto_image_download_dir;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_MAIN_WINDOW_H
