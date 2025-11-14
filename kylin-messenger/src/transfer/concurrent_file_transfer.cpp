/**
 * @file concurrent_file_transfer.cpp
 * @brief 并发文件传输管理器实现
 * @version 1.0.0
 */

#include "transfer/concurrent_file_transfer.h"
#include "core/micro_kernel.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkInterface>
#include <algorithm>
#include <random>

namespace KylinMessenger::Transfer {

ConcurrentFileTransfer::ConcurrentFileTransfer(QObject* parent)
    : QObject(parent) {
    qDebug() << "[ConcurrentFileTransfer] 创建并发文件传输服务";
}

ConcurrentFileTransfer::~ConcurrentFileTransfer() {
    qDebug() << "[ConcurrentFileTransfer] 销毁并发文件传输服务";
    shutdown();
}

bool ConcurrentFileTransfer::initialize() {
    qDebug() << "[ConcurrentFileTransfer] 初始化传输服务(简化版)";
    available_ = true;
    qDebug() << "[ConcurrentFileTransfer] 传输服务初始化成功";
    return true;
}

void ConcurrentFileTransfer::processEvent(const Core::Event& event) {
    switch (event.type()) {
        case Core::Event::ShutdownRequested:
            qDebug() << "[ConcurrentFileTransfer] 处理关闭请求";
            shutdown();
            break;
        case Core::Event::UserOffline:
            qDebug() << "[ConcurrentFileTransfer] 用户下线，清理相关传输";
            break;
        default:
            break;
    }
}

void ConcurrentFileTransfer::shutdown() {
    qDebug() << "[ConcurrentFileTransfer] 关闭传输服务";
    available_ = false;
    stopWorkers_ = true;
    {
        std::lock_guard<std::mutex> lock(transfersMutex_);
        transfers_.clear();
        while (!pendingTasks_.empty()) pendingTasks_.pop();
    }
    qDebug() << "[ConcurrentFileTransfer] 传输服务关闭完成";
}

QString ConcurrentFileTransfer::sendFile(const QString& filePath,
                                        const QHostAddress& targetAddress,
                                        quint16 targetPort,
                                        const QString& peerId) {
    if (!available_) {
        qWarning() << "[ConcurrentFileTransfer] 服务不可用，无法发送文件";
        return QString();
    }
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qWarning() << "[ConcurrentFileTransfer] 文件不存在:" << filePath;
        return QString();
    }
    QString taskId = generateTaskId();
    auto state = std::make_unique<TransferState>();
    state->task.taskId = taskId;
    state->task.filePath = filePath;
    state->task.fileName = fileInfo.fileName();
    state->task.totalSize = fileInfo.size();
    state->task.transferred = 0;
    state->task.status = TransferStatus::Pending;
    state->task.targetAddress = targetAddress;
    state->task.targetPort = targetPort;
    state->task.peerId = peerId;
    state->task.isSender = true;
    state->task.startTime = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(transfersMutex_);
        transfers_[taskId] = std::move(state);
        pendingTasks_.push(taskId);
    }
    qDebug() << "[ConcurrentFileTransfer] 创建发送任务:" << taskId
             << "文件:" << fileInfo.fileName()
             << "大小:" << fileInfo.size() << "字节"
             << "目标:" << targetAddress.toString() << ":" << targetPort;
    return taskId;
}

QString ConcurrentFileTransfer::receiveFile(const QString& filePath,
                                           qint64 expectedSize,
                                           const QString& peerId) {
    if (!available_) {
        qWarning() << "[ConcurrentFileTransfer] 服务不可用，无法接收文件";
        return QString();
    }
    QString taskId = generateTaskId();
    auto state = std::make_unique<TransferState>();
    state->task.taskId = taskId;
    state->task.filePath = filePath;
    state->task.fileName = QFileInfo(filePath).fileName();
    state->task.totalSize = expectedSize;
    state->task.transferred = 0;
    state->task.status = TransferStatus::Pending;
    state->task.peerId = peerId;
    state->task.isSender = false;
    state->task.startTime = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(transfersMutex_);
        transfers_[taskId] = std::move(state);
    }
    qDebug() << "[ConcurrentFileTransfer] 创建接收任务:" << taskId
             << "文件:" << filePath
             << "期望大小:" << expectedSize << "字节"
             << "对端:" << peerId;
    return taskId;
}

bool ConcurrentFileTransfer::acceptTransfer(const QString& taskId) {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it == transfers_.end()) {
        qWarning() << "[ConcurrentFileTransfer] 任务不存在:" << taskId;
        return false;
    }
    TransferState* state = it->second.get();
    if (state->task.status != TransferStatus::Pending) {
        qWarning() << "[ConcurrentFileTransfer] 任务状态无效:" << taskId;
        return false;
    }
    state->task.status = TransferStatus::Transferring;
    pendingTasks_.push(taskId);
    qDebug() << "[ConcurrentFileTransfer] 接受传输任务:" << taskId;
    return true;
}

void ConcurrentFileTransfer::rejectTransfer(const QString& taskId) {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it == transfers_.end()) return;
    TransferState* state = it->second.get();
    if (state->task.status == TransferStatus::Pending) {
        state->task.status = TransferStatus::Cancelled;
        emit transferCancelled(taskId);
        qDebug() << "[ConcurrentFileTransfer] 拒绝传输任务:" << taskId;
    }
}

void ConcurrentFileTransfer::cancelTransfer(const QString& taskId) {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it == transfers_.end()) return;
    TransferState* state = it->second.get();
    if (state->task.status == TransferStatus::Transferring || state->task.status == TransferStatus::Paused) {
        state->task.status = TransferStatus::Cancelled;
        state->cv.notify_all();
        emit transferCancelled(taskId);
        qDebug() << "[ConcurrentFileTransfer] 取消传输任务:" << taskId;
    }
}

void ConcurrentFileTransfer::pauseTransfer(const QString& taskId) {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it == transfers_.end()) return;
    TransferState* state = it->second.get();
    if (state->task.status == TransferStatus::Transferring) {
        state->task.status = TransferStatus::Paused;
        emit transferPaused(taskId);
        qDebug() << "[ConcurrentFileTransfer] 暂停传输任务:" << taskId;
    }
}

void ConcurrentFileTransfer::resumeTransfer(const QString& taskId) {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it == transfers_.end()) return;
    TransferState* state = it->second.get();
    if (state->task.status == TransferStatus::Paused) {
        state->task.status = TransferStatus::Transferring;
        pendingTasks_.push(taskId);
        state->cv.notify_all();
        emit transferResumed(taskId);
        qDebug() << "[ConcurrentFileTransfer] 恢复传输任务:" << taskId;
    }
}

std::optional<TransferTask> ConcurrentFileTransfer::getTask(const QString& taskId) const {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.find(taskId);
    if (it != transfers_.end()) {
        return it->second->task;
    }
    return std::nullopt;
}

std::vector<TransferTask> ConcurrentFileTransfer::getAllTasks() const {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    std::vector<TransferTask> tasks;
    tasks.reserve(transfers_.size());
    for (const auto& [taskId, state] : transfers_) {
        tasks.push_back(state->task);
    }
    return tasks;
}

std::vector<TransferTask> ConcurrentFileTransfer::getActiveTasks() const {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    std::vector<TransferTask> tasks;
    for (const auto& [taskId, state] : transfers_) {
        if (state->task.status == TransferStatus::Transferring || state->task.status == TransferStatus::Paused) {
            tasks.push_back(state->task);
        }
    }
    return tasks;
}

void ConcurrentFileTransfer::processTransferQueue() {
    std::vector<QString> currentTasks;
    {
        std::lock_guard<std::mutex> lock(transfersMutex_);
        while (!pendingTasks_.empty() && currentTasks.size() < MAX_WORKER_THREADS) {
            QString taskId = pendingTasks_.front();
            pendingTasks_.pop();
            auto it = transfers_.find(taskId);
            if (it != transfers_.end() && it->second->task.status == TransferStatus::Transferring) {
                currentTasks.push_back(taskId);
            }
        }
    }
    for (const QString& taskId : currentTasks) {
        auto it = transfers_.find(taskId);
        if (it != transfers_.end()) {
            it->second->cv.notify_all();
        }
    }
}

void ConcurrentFileTransfer::handleNewConnection() {
    // 处理新的套接字连接 - 简化实现
    qDebug() << "[ConcurrentFileTransfer] 新连接请求";
}

void ConcurrentFileTransfer::handleSocketReadyRead() {
    // 处理套接字读取就绪事件 - 简化实现
    qDebug() << "[ConcurrentFileTransfer] 套接字读就绪";
}

void ConcurrentFileTransfer::handleSocketDisconnected() {
    // 处理套接字断开连接事件 - 简化实现
    qDebug() << "[ConcurrentFileTransfer] 套接字断开";
}

void ConcurrentFileTransfer::handleSocketError(QAbstractSocket::SocketError error) {
    // 处理套接字错误 - 简化实现
    QString errorName;
    switch (error) {
        case QAbstractSocket::SocketError::ConnectionRefusedError:
            errorName = "连接被拒绝";
            break;
        case QAbstractSocket::SocketError::RemoteHostClosedError:
            errorName = "远程主机关闭连接";
            break;
        case QAbstractSocket::SocketError::HostNotFoundError:
            errorName = "主机未找到";
            break;
        case QAbstractSocket::SocketError::SocketAccessError:
            errorName = "套接字访问错误";
            break;
        case QAbstractSocket::SocketError::SocketResourceError:
            errorName = "套接字资源错误";
            break;
        case QAbstractSocket::SocketError::NetworkError:
            errorName = "网络错误";
            break;
        case QAbstractSocket::SocketError::UnknownSocketError:
        default:
            errorName = "未知套接字错误";
            break;
    }
    qDebug() << "[ConcurrentFileTransfer] 套接字错误:" << errorName << static_cast<int>(error);
}

void ConcurrentFileTransfer::cleanupCompletedTasks() {
    std::lock_guard<std::mutex> lock(transfersMutex_);
    auto it = transfers_.begin();
    while (it != transfers_.end()) {
        const TransferTask& task = it->second->task;
        if (task.status == TransferStatus::Completed || task.status == TransferStatus::Failed || task.status == TransferStatus::Cancelled) {
            auto endTime = task.endTime;
            if (endTime == std::chrono::steady_clock::time_point{}) {
                endTime = task.startTime;
            }
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(now - endTime);
            if (duration.count() > 60) {
                qDebug() << "[ConcurrentFileTransfer] 清理旧任务:" << task.taskId;
                it = transfers_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

QString ConcurrentFileTransfer::generateTaskId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace KylinMessenger::Transfer

// 协议常量定义
namespace KylinMessenger::Transfer::Protocol {
    const int CLEANUP_INTERVAL_MS = 60000;
    const int MAX_WORKER_THREADS = 4;
    const int BLOCK_SIZE = 256 * 1024;
    const int MAX_CONCURRENT_BLOCKS = 4;
    const quint16 DEFAULT_PORT = 2425;
    const char* MAGIC = "KYLINFT";
    const quint16 VERSION = 1;
    const int MAX_RETRIES = 3;
    const int TRANSFER_TIMEOUT_MS = 10000;
    const int CONNECT_TIMEOUT_MS = 5000;
    const int SOCKET_BUFFER_SIZE = 65536;
}