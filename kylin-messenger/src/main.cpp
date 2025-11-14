// main.cpp - 应用程序入口
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QResource>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <cstdio>
#include <QCoreApplication>
#include <memory>

#include "main_window.h"
#include "network_manager.h"
#include "ai_service_factory.h"
#include "compliance_stub_service.h"
#include "nsfw_compliance_service.h"
#include "rknn_nsfw_compliance_service.h"
#include "version_info.h"
#include "core/logging.h"
#include "core/di/service_locator.h"
#include "core/repositories/message_repository.h"
// 新增轻量级组件
#include "core/micro_kernel.h"
#include "ai/opencv_nsfw_detector.h"
#include "network/lightweight_discovery.h"
#include "transfer/concurrent_file_transfer.h"

using namespace KylinMessenger;

namespace {

QString ensureLogFilePath()
{
    static QString logfile;
    if (!logfile.isEmpty()) {
        return logfile;
    }

    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    if (base.isEmpty()) {
        base = QDir::tempPath();
    }

    QDir dir(base);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    logfile = dir.filePath(QStringLiteral("kylin-messenger/ipmsg.log"));
    QDir logDir = QFileInfo(logfile).dir();
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    return logfile;
}

void fileMessageHandler(QtMsgType type,
                        const QMessageLogContext& context,
                        const QString& msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    const QString level = [type]() {
        switch (type) {
        case QtDebugMsg:
            return QStringLiteral("DEBUG");
        case QtInfoMsg:
            return QStringLiteral("INFO");
        case QtWarningMsg:
            return QStringLiteral("WARN");
        case QtCriticalMsg:
            return QStringLiteral("CRIT");
        case QtFatalMsg:
            return QStringLiteral("FATAL");
        }
        return QStringLiteral("LOG");
    }();

    const QString category = context.category ? QString::fromUtf8(context.category) : QStringLiteral("default");
    const QString line = QStringLiteral("%1 [%2] (%3) %4")
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"),
                                  level,
                                  category,
                                  msg);

    QTextStream stderrStream(stderr);
    stderrStream << line
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                 << Qt::endl;
#else
                 << endl;
#endif

    QFile file(ensureLogFilePath());
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << line << '\n';
        out.flush();
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void setupLogging()
{
    if (!qEnvironmentVariableIsSet("QT_LOGGING_RULES") || qEnvironmentVariable("QT_LOGGING_RULES").trimmed().isEmpty()) {
        // 默认关闭默认分类的 info 日志，开启 IPMSG 分类
        QLoggingCategory::setFilterRules(QStringLiteral(
            "default.info=false\n"
            "kylin.ipmsg.info=true\n"
            "kylin.ipmsg.debug=true"));
    }

    qInstallMessageHandler(fileMessageHandler);

    QFile file(ensureLogFilePath());
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << "\n===== Session start "
            << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
            << " =====\n";
    }

    qInfo() << "日志输出路径:" << ensureLogFilePath();
}

} // namespace

int main(int argc, char* argv[])
{
    setupLogging();
    QApplication app(argc, argv);
    Core::Logging::initialize();
    auto& locator = Core::DI::ServiceLocator::instance();

    // 创建微内核实例
    auto micro_kernel = std::make_shared<Core::MicroKernel>();
    locator.registerService<Core::MicroKernel>(micro_kernel);

    Q_INIT_RESOURCE(icons);
    Q_INIT_RESOURCE(emojis);
    Q_INIT_RESOURCE(themes);
#ifdef ENABLE_NSFW
    Q_INIT_RESOURCE(scripts);
#endif
    
    // 设置应用程序信息
    QApplication::setApplicationName("Kylin Messenger");
    QApplication::setApplicationVersion(KylinMessenger::kAppVersion);
    QApplication::setOrganizationName("KylinMessenger");
    QApplication::setOrganizationDomain("kylinmessenger.org");
    
    qInfo() << "========================================";
    qInfo() << "麒麟信使启动中...";
    qInfo() << "版本:" << KylinMessenger::kAppVersion;
    qInfo() << "Git标识:" << KylinMessenger::kGitDescribe;
    qInfo() << "构建时间:" << KylinMessenger::kBuildTimestamp;
    qInfo() << "Qt版本:" << QT_VERSION_STR;
    qInfo() << "========================================";
    
    // 创建网络管理器（兼容模式）
    auto network_manager = std::make_shared<NetworkManager>();
    locator.registerService<NetworkManager>(network_manager);

    // 初始化轻量级服务
    qInfo() << "初始化轻量级服务...";

    // 暂时禁用微内核服务加载，使用独立服务
    auto lightweight_discovery = std::make_shared<Network::LightweightDiscovery>();
    auto concurrent_transfer = std::make_shared<Transfer::ConcurrentFileTransfer>();

#ifdef ENABLE_AI_FEATURES
    // 创建OpenCV NSFW检测器（如果可用）
    try {
        auto nsfw_config = AI::NSFWDetectorConfig::fromEnvironment();
        auto opencv_nsfw = std::make_shared<AI::LightweightNSFWDetector>(
            nsfw_config.modelPath, nsfw_config.threshold);
        if (opencv_nsfw->isAvailable()) {
            main_window->setNSFWDetector(opencv_nsfw);
            qInfo() << "OpenCV NSFW检测器初始化成功";
        } else {
            qWarning() << "OpenCV NSFW检测器不可用";
        }
    } catch (const std::exception& e) {
        qWarning() << "OpenCV NSFW检测器初始化失败:" << e.what();
    }
#endif

    // 启动微内核
    if (!micro_kernel->start()) {
        QMessageBox::critical(nullptr, "错误",
            "无法启动微内核服务\n请检查服务配置");
        return 1;
    }
    qInfo() << "微内核服务启动成功";

    // 初始化本地用户信息
    UserInfo local_user;
    local_user.user_id = QUuid::createUuid().toString();
    local_user.username = qgetenv("USER").isEmpty() ?
        QStringLiteral("User") : QString::fromLocal8Bit(qgetenv("USER"));
    local_user.status = UserStatus::Online;
    local_user.status_text = "在线";

    qInfo() << "本地用户ID:" << local_user.user_id;
    qInfo() << "用户名:" << local_user.username;

    // 设置轻量级网络发现的本地用户信息
    lightweight_discovery->updateLocalUser(local_user);

    // 初始化网络（兼容模式）
    if (!network_manager->initialize(local_user)) {
        QMessageBox::critical(nullptr, "错误",
            "无法初始化网络管理器\n请检查网络连接和权限");
        return 1;
    }

    qInfo() << "网络管理器初始化成功";

    auto message_repository = std::make_shared<Core::Repositories::InMemoryMessageRepository>();
    locator.registerService<Core::Repositories::MessageRepository>(message_repository);

    // 创建AI服务
    std::shared_ptr<IAIService> ai_service;
    std::shared_ptr<IComplianceService> compliance_service;
    
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
            locator.registerService<IAIService>(ai_service);
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

    qInfo() << "初始化合规审查服务...";
    qInfo() << "[main] 准备创建主窗口";
#ifdef ENABLE_NSFW
    {
        const QString backend = QString::fromLocal8Bit(qgetenv("KYLIN_NSFW_BACKEND")).trimmed().toLower();
#ifdef HAVE_RKNN_RT
        if (backend == QStringLiteral("rknn") || backend == QStringLiteral("npu")) {
            RknnComplianceConfig rknnConfig = RknnComplianceConfig::fromEnvironment();
            auto rknnService = std::make_shared<RknnNsfwComplianceService>(rknnConfig);
            if (rknnService->isAvailable()) {
                qInfo() << "已启用 RKNN NSFW 审核，模型路径:" << rknnConfig.modelPath;
                compliance_service = rknnService;
            } else {
                qWarning() << "RKNN NSFW 审核尚未就绪，尝试回退到 Python 后端";
            }
        }
#else
        if (backend == QStringLiteral("rknn") || backend == QStringLiteral("npu")) {
            qWarning() << "当前构建未启用 RKNN 运行时，无法加载 RKNN 后端，转用 Python 审核";
        }
#endif
        if (!compliance_service) {
            NsfwComplianceConfig config = NsfwComplianceConfig::fromEnvironment();
            auto nsfw_service = std::make_shared<NsfwComplianceService>(config);
            if (nsfw_service->isAvailable()) {
                qInfo() << "已启用 NSFW 审核，模型路径:" << config.modelPath;
                compliance_service = nsfw_service;
            } else {
                qWarning() << "NSFW 审核不可用，降级为占位实现";
                compliance_service = std::make_shared<ComplianceStubService>();
            }
        }
    }
#else
    compliance_service = std::make_shared<ComplianceStubService>();
#endif

    if (compliance_service && compliance_service->isAvailable()) {
        locator.registerService<IComplianceService>(compliance_service);
    } else {
        qWarning() << "合规审查服务当前不可用，所有消息将直接放行";
    }
    
    // 创建主窗口（支持微内核架构）
    qInfo() << "[main] 开始创建 MainWindow 对象";
    MainWindow main_window(micro_kernel.get());  // 传递微内核实例
    qInfo() << "[main] MainWindow 对象创建完成";

    qInfo() << "[main] 设置 NetworkManager";
    main_window.setNetworkManager(network_manager.get());
    qInfo() << "[main] NetworkManager 设置完成";

    // 设置轻量级服务
    qInfo() << "[main] 设置轻量级服务";
    main_window.setLightweightDiscovery(lightweight_discovery);
    main_window.setConcurrentFileTransfer(concurrent_transfer);

    if (ai_service) {
        qInfo() << "[main] 设置 AIService";
        main_window.setAIService(ai_service);
    }
    if (compliance_service) {
        qInfo() << "[main] 设置 ComplianceService";
        main_window.setComplianceService(compliance_service);
    }
    
    qInfo() << "[main] 显示主窗口";
    main_window.show();
    
    qInfo() << "主窗口已显示";
    qInfo() << "应用程序就绪";
    
    // 运行应用程序
    int result = app.exec();
    
    // 清理
    qInfo() << "应用程序退出，清理资源...";
    
    Q_CLEANUP_RESOURCE(themes);
    Q_CLEANUP_RESOURCE(emojis);
    Q_CLEANUP_RESOURCE(icons);
#ifdef ENABLE_NSFW
    Q_CLEANUP_RESOURCE(scripts);
#endif

    if (ai_service) {
        ai_service->shutdown();
    }
    
    network_manager->shutdown();
    locator.clear();
    
    qInfo() << "再见！";
    Core::Logging::shutdown();
    
    return result;
}
