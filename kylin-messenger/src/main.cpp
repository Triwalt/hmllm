// main.cpp - 应用程序入口
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QUuid>
#include <iostream>

#include "main_window.h"
#include "network_manager.h"
#include "ai_service_factory.h"

using namespace KylinMessenger;

// 日志处理器
void messageHandler(QtMsgType type, const QMessageLogContext& context, 
                   const QString& msg)
{
    QString log_level;
    switch (type) {
        case QtDebugMsg:
            log_level = "DEBUG";
            break;
        case QtInfoMsg:
            log_level = "INFO";
            break;
        case QtWarningMsg:
            log_level = "WARN";
            break;
        case QtCriticalMsg:
            log_level = "ERROR";
            break;
        case QtFatalMsg:
            log_level = "FATAL";
            break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString log_message = QString("[%1] [%2] %3")
        .arg(timestamp)
        .arg(log_level)
        .arg(msg);
    
    // 输出到控制台
    std::cerr << log_message.toStdString() << std::endl;
    
    // 输出到日志文件
    QFile log_file("kylin-messenger.log");
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&log_file);
        stream << log_message << "\n";
        log_file.close();
    }
    
    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char* argv[])
{
    // 安装日志处理器
    qInstallMessageHandler(messageHandler);
    
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    QApplication::setApplicationName("Kylin Messenger");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("KylinMessenger");
    QApplication::setOrganizationDomain("kylinmessenger.org");
    
    qInfo() << "========================================";
    qInfo() << "麒麟信使启动中...";
    qInfo() << "版本: 1.0.0";
    qInfo() << "Qt版本:" << QT_VERSION_STR;
    qInfo() << "========================================";
    
    // 创建网络管理器
    NetworkManager* network_manager = new NetworkManager();
    
    // 初始化本地用户信息
    UserInfo local_user;
    local_user.user_id = QUuid::createUuid().toString();
    local_user.username = qgetenv("USER").isEmpty() ? 
        QStringLiteral("User") : QString::fromLocal8Bit(qgetenv("USER"));
    local_user.status = UserStatus::Online;
    local_user.status_text = "在线";
    
    qInfo() << "本地用户ID:" << local_user.user_id;
    qInfo() << "用户名:" << local_user.username;
    
    // 初始化网络
    if (!network_manager->initialize(local_user)) {
        QMessageBox::critical(nullptr, "错误", 
            "无法初始化网络管理器\n请检查网络连接和权限");
        return 1;
    }
    
    qInfo() << "网络管理器初始化成功";
    
    // 创建AI服务
    std::shared_ptr<IAIService> ai_service;
    
#ifdef ENABLE_AI_FEATURES
    qInfo() << "初始化AI服务...";
    
    // 列出可用的AI服务
    auto services = AIServiceFactory::getRegisteredServices();
    qInfo() << "可用的AI服务:";
    for (const auto& service : services) {
        qInfo() << "  -" << QString::fromStdString(service);
    }
    
    // 创建Echo AI服务（测试用）
    ai_service = AIServiceFactory::createService("echo");
    if (ai_service) {
        if (ai_service->initialize("")) {
            qInfo() << "AI服务初始化成功";
        } else {
            qWarning() << "AI服务初始化失败";
        }
    }
    
#ifdef HAVE_LLM_RKNN
    // TODO: 如果需要真实的LLM功能，可以在这里创建LLM服务
    // ai_service = AIServiceFactory::createService("llm_chat");
    // if (ai_service) {
    //     ai_service->initialize("/path/to/model.rknn");
    // }
#endif
    
#else
    qInfo() << "AI功能已禁用";
#endif
    
    // 创建主窗口
    MainWindow main_window;
    main_window.setNetworkManager(network_manager);
    
    if (ai_service) {
        main_window.setAIService(ai_service);
    }
    
    main_window.show();
    
    qInfo() << "主窗口已显示";
    qInfo() << "应用程序就绪";
    
    // 运行应用程序
    int result = app.exec();
    
    // 清理
    qInfo() << "应用程序退出，清理资源...";
    
    if (ai_service) {
        ai_service->shutdown();
    }
    
    network_manager->shutdown();
    delete network_manager;
    
    qInfo() << "再见！";
    
    return result;
}
