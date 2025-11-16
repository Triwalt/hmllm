// moved from src/ui/main_window_new.cpp -- archived as legacy
// 新增正确的函数，稍后替换损坏的部分
void MainWindow::setupConnections()
{
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

    // 网络事件
    connect(m_network_manager, &NetworkManager::userOnline,
            this, &MainWindow::onUserOnline);
    connect(m_network_manager, &NetworkManager::userOffline,
            this, &MainWindow::onUserOffline);
    connect(m_network_manager, &NetworkManager::userInfoUpdated,
            this, &MainWindow::onUserInfoUpdated);
    connect(m_network_manager, &NetworkManager::messageReceived,
            this, &MainWindow::onMessageReceived);
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    // 转发到页面进行搜索
    if (m_user_list_page) {
        m_user_list_page->onSearchTextChanged(text);
    }
}
