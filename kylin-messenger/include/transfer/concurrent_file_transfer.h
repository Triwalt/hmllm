/**
 * @file concurrent_file_transfer.h
 * @brief 并发文件传输管理器
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_TRANSFER_CONCURRENT_FILE_TRANSFER_H
#define KYLIN_MESSENGER_TRANSFER_CONCURRENT_FILE_TRANSFER_H

#include "core/micro_kernel.h"
#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QTimer>
#include <QUuid>
#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace KylinMessenger::Transfer {

/**
 * @brief 传输任务状态
 */
enum class TransferStatus {
    Pending,      // 等待中
    Connecting,   // 连接中
    Transferring, // 传输中
    Paused,       // 已暂停
    Completed,    // 已完成
    Failed,       // 失败
    Cancelled     // 已取消
};

/**
 * @brief 传输任务信息
 */
struct TransferTask {
    QString taskId;
    QString filePath;
    QString fileName;
    qint64 totalSize = 0;
    qint64 transferred = 0;
    TransferStatus status = TransferStatus::Pending;
    QHostAddress targetAddress;
    quint16 targetPort = 0;
    QString peerId;
    bool isSender = false;
    std::shared_ptr<std::atomic<int>> activeBlocks;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    // 构造函数
    TransferTask() : activeBlocks(std::make_shared<std::atomic<int>>(0)) {}

    // 分块信息
    static constexpr qint64 BLOCK_SIZE = 256 * 1024; // 256KB
    static constexpr int MAX_CONCURRENT_BLOCKS = 4;  // 最大并发块数

    // 计算进度
    int progress() const {
        if (totalSize <= 0) return 0;
        return static_cast<int>((transferred * 100) / totalSize);
    }

    // 计算速度（字节/秒）
    qint64 speed() const {
        if (status != TransferStatus::Transferring) return 0;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        if (elapsed <= 0) return 0;
        return transferred / elapsed;
    }
};

/**
 * @brief 传输块信息
 */
struct TransferBlock {
    QString taskId;
    qint64 offset = 0;
    qint64 size = 0;
    qint64 transferred = 0;
    bool completed = false;
    int retryCount = 0;
    static constexpr int MAX_RETRIES = 3;
};

/**
 * @brief 并发文件传输管理器
 * 支持多线程分块传输，提高传输效率
 */
class ConcurrentFileTransfer : public QObject, public Core::LightweightService {
    Q_OBJECT

public:
    explicit ConcurrentFileTransfer(QObject* parent = nullptr);
    ~ConcurrentFileTransfer() override;

    // LightweightService接口实现
    bool initialize() override;
    void processEvent(const Core::Event& event) override;
    void shutdown() override;
    std::string getName() const override { return "ConcurrentFileTransfer"; }
    bool isAvailable() const override { return available_; }

    /**
     * @brief 发送文件
     * @param filePath 文件路径
     * @param targetAddress 目标地址
     * @param targetPort 目标端口
     * @param peerId 对端ID
     * @return 任务ID
     */
    QString sendFile(const QString& filePath,
                     const QHostAddress& targetAddress,
                     quint16 targetPort,
                     const QString& peerId);

    /**
     * @brief 接收文件
     * @param filePath 保存路径
     * @param expectedSize 期望文件大小
     * @param peerId 对端ID
     * @return 任务ID
     */
    QString receiveFile(const QString& filePath,
                        qint64 expectedSize,
                        const QString& peerId);

    /**
     * @brief 接受文件传输请求
     * @param taskId 任务ID
     * @return 是否成功
     */
    bool acceptTransfer(const QString& taskId);

    /**
     * @brief 拒绝文件传输
     * @param taskId 任务ID
     */
    void rejectTransfer(const QString& taskId);

    /**
     * @brief 取消传输
     * @param taskId 任务ID
     */
    void cancelTransfer(const QString& taskId);

    /**
     * @brief 暂停传输
     * @param taskId 任务ID
     */
    void pauseTransfer(const QString& taskId);

    /**
     * @brief 恢复传输
     * @param taskId 任务ID
     */
    void resumeTransfer(const QString& taskId);

    /**
     * @brief 获取任务信息
     */
    std::optional<TransferTask> getTask(const QString& taskId) const;

    /**
     * @brief 获取所有任务
     */
    std::vector<TransferTask> getAllTasks() const;

    /**
     * @brief 获取活跃任务
     */
    std::vector<TransferTask> getActiveTasks() const;

signals:
    void transferStarted(const QString& taskId);
    void transferProgress(const QString& taskId, qint64 transferred, qint64 total);
    void transferCompleted(const QString& taskId, const QString& filePath);
    void transferFailed(const QString& taskId, const QString& reason);
    void transferCancelled(const QString& taskId);
    void transferPaused(const QString& taskId);
    void transferResumed(const QString& taskId);
    void incomingTransferRequest(const QString& taskId, 
                                 const QString& fileName, 
                                 qint64 fileSize,
                                 const QString& peerId);

private slots:
    void handleNewConnection();
    void handleSocketReadyRead();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError error);
    void processTransferQueue();
    void cleanupCompletedTasks();

private:
    // 传输管理
    struct TransferState {
        TransferTask task;
        std::unique_ptr<QFile> file;
        std::vector<TransferBlock> blocks;
        std::vector<std::unique_ptr<QTcpSocket>> sockets;
        std::mutex mutex;
        std::condition_variable cv;
    };
    
    bool available_ = false;
    QTcpServer* tcpServer_ = nullptr;
    QTimer* queueTimer_ = nullptr;
    QTimer* cleanupTimer_ = nullptr;

    // 任务管理
    std::unordered_map<QString, std::unique_ptr<TransferState>> transfers_;
    std::queue<QString> pendingTasks_;
    mutable std::mutex transfersMutex_;
    
    // 线程池
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> stopWorkers_{false};
    
    // 配置
    static constexpr quint16 DEFAULT_PORT = 2425;
    static constexpr int MAX_WORKER_THREADS = 4;
    static constexpr int CLEANUP_INTERVAL_MS = 60000; // 1分钟清理一次
    
    // 辅助方法
    void setupConnections();
    bool startServer();
    void stopServer();
    void workerThreadMain();
    void processTask(TransferState* state);
    void sendBlock(TransferState* state, TransferBlock* block, QTcpSocket* socket);
    void receiveBlock(TransferState* state, TransferBlock* block, QTcpSocket* socket);
    void initializeBlocks(TransferState* state);
    TransferBlock* getNextBlock(TransferState* state);
    void updateTaskProgress(TransferState* state);
    void completeTask(TransferState* state);
    void failTask(TransferState* state, const QString& reason);
    QString generateTaskId() const;
    bool validateFile(const QString& filePath, qint64 expectedSize) const;
};

} // namespace KylinMessenger::Transfer

#endif // KYLIN_MESSENGER_TRANSFER_CONCURRENT_FILE_TRANSFER_H