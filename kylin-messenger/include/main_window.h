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
#include <memory>

#include "network_manager.h"
#include "ai_service.h"

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
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
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
    
protected:
    void closeEvent(QCloseEvent* event) override;
    
private slots:
    // 网络事件处理
    void onUserOnline(const UserInfo& user_info);
    void onUserOffline(const QString& user_id);
    void onUserInfoUpdated(const UserInfo& user_info);
    void onMessageReceived(const ChatMessage& message);
    
    // UI事件处理
    void onUserItemDoubleClicked(QListWidgetItem* item);
    void onUserItemContextMenu(const QPoint& pos);
    void onSearchTextChanged(const QString& text);
    void onStatusChanged(int index);
    void onStatusTextChanged(const QString& text);
    
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
    void updateUserListItem(const UserInfo& user_info);
    void removeUserListItem(const QString& user_id);
    
    ChatWindow* openChatWindow(const UserInfo& user_info);
    ChatWindow* findChatWindow(const QString& user_id);
    
    void showNotification(const QString& title, const QString& message);
    
private:
    // 网络
    NetworkManager* m_network_manager;
    UserInfo m_local_user;
    
    // AI服务
    std::shared_ptr<IAIService> m_ai_service;
    
    // UI组件
    QListWidget* m_user_list;
    QLineEdit* m_search_edit;
    QComboBox* m_status_combo;
    QLineEdit* m_status_text_edit;
    QLabel* m_user_count_label;
    
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
    
    // 右键菜单
    QMenu* m_user_context_menu;
    QListWidgetItem* m_current_context_item;
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_MAIN_WINDOW_H
