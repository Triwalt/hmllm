/**
 * @file micro_kernel.cpp
 * @brief 微内核架构核心实现
 * @version 1.0.0
 */

#include "core/micro_kernel.h"
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <algorithm>
#include <chrono>

namespace KylinMessenger::Core {

class MicroKernel::Impl {
public:
    std::unordered_map<std::string, std::unique_ptr<LightweightService>> services;
    mutable QMutex servicesMutex;
    std::vector<std::string> serviceOrder;
    mutable QMutex eventMutex;
    std::atomic<bool> initialized{false};
    std::atomic<bool> shuttingDown{false};
    std::atomic<size_t> eventCount{0};
    std::chrono::steady_clock::time_point startTime;

    bool initializeAllServices();
    void shutdownAllServices();
    void processEvent(const Event& event);
};

MicroKernel::MicroKernel() : impl_(std::make_unique<Impl>()) {
    qDebug() << "[MicroKernel] 初始化微内核";
}

MicroKernel::~MicroKernel() {
    if (isRunning()) {
        shutdown();
    }
}

void MicroKernel::loadService(std::unique_ptr<LightweightService> service, const std::string& name) {
    QMutexLocker locker(&impl_->servicesMutex);
    if (impl_->services.find(name) != impl_->services.end()) {
        qWarning() << "[MicroKernel] 服务已存在:" << QString::fromStdString(name);
        return;
    }
    if (impl_->shuttingDown) {
        qWarning() << "[MicroKernel] 正在关闭中，无法加载新服务";
        return;
    }
    qDebug() << "[MicroKernel] 加载服务:" << QString::fromStdString(name);
    impl_->services[name] = std::move(service);
    impl_->serviceOrder.push_back(name);
}

bool MicroKernel::start() {
    if (running_.load()) {
        qWarning() << "[MicroKernel] 内核已在运行中";
        return false;
    }
    if (impl_->shuttingDown) {
        qWarning() << "[MicroKernel] 正在关闭中，无法启动";
        return false;
    }
    qDebug() << "[MicroKernel] 启动微内核";
    impl_->startTime = std::chrono::steady_clock::now();
    if (!impl_->initializeAllServices()) {
        qCritical() << "[MicroKernel] 服务初始化失败";
        return false;
    }
    running_ = true;
    Event startEvent(Event::ServiceStarted);
    startEvent.setSource("MicroKernel");
    publishEvent(startEvent);
    qDebug() << "[MicroKernel] 微内核启动成功，服务数量:" << impl_->services.size();
    return true;
}

void MicroKernel::shutdown() {
    if (!running_.load()) {
        return;
    }
    qDebug() << "[MicroKernel] 开始关闭微内核";
    impl_->shuttingDown = true;
    Event shutdownEvent(Event::ShutdownRequested);
    shutdownEvent.setSource("MicroKernel");
    publishEvent(shutdownEvent);
    impl_->shutdownAllServices();
    running_ = false;
    auto duration = std::chrono::steady_clock::now() - impl_->startTime;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    qDebug() << "[MicroKernel] 微内核关闭完成，运行时间:" << seconds << "秒，处理事件:" << impl_->eventCount.load();
}

void MicroKernel::publishEvent(const Event& event) {
    if (!running_.load()) {
        qWarning() << "[MicroKernel] 内核未运行，无法发布事件";
        return;
    }
    if (impl_->shuttingDown) {
        qWarning() << "[MicroKernel] 正在关闭中，忽略事件";
        return;
    }
    impl_->eventCount++;
    impl_->processEvent(event);
}

std::vector<std::string> MicroKernel::getLoadedServices() const {
    QMutexLocker locker(&impl_->servicesMutex);
    std::vector<std::string> result;
    result.reserve(impl_->services.size());
    for (const auto& [name, service] : impl_->services) {
        result.push_back(name);
    }
    return result;
}

bool MicroKernel::Impl::initializeAllServices() {
    qDebug() << "[MicroKernel] 开始初始化所有服务";
    for (const auto& name : serviceOrder) {
        auto it = services.find(name);
        if (it == services.end()) continue;
        auto& service = it->second;
        qDebug() << "[MicroKernel] 初始化服务:" << QString::fromStdString(name);
        if (!service->initialize()) {
            qCritical() << "[MicroKernel] 服务初始化失败:" << QString::fromStdString(name);
            return false;
        }
        qDebug() << "[MicroKernel] 服务初始化成功:" << QString::fromStdString(name);
    }
    initialized = true;
    qDebug() << "[MicroKernel] 所有服务初始化完成";
    return true;
}

void MicroKernel::Impl::shutdownAllServices() {
    qDebug() << "[MicroKernel] 开始关闭所有服务";
    for (auto it = serviceOrder.rbegin(); it != serviceOrder.rend(); ++it) {
        const auto& name = *it;
        auto serviceIt = services.find(name);
        if (serviceIt == services.end()) continue;
        qDebug() << "[MicroKernel] 关闭服务:" << QString::fromStdString(name);
        serviceIt->second->shutdown();
        qDebug() << "[MicroKernel] 服务关闭完成:" << QString::fromStdString(name);
    }
    qDebug() << "[MicroKernel] 所有服务关闭完成";
}

void MicroKernel::Impl::processEvent(const Event& event) {
    QMutexLocker locker(&eventMutex);
    size_t processedCount = 0;
    size_t availableCount = 0;
    for (const auto& [name, service] : services) {
        if (service->isAvailable()) {
            availableCount++;
            try {
                service->processEvent(event);
                processedCount++;
            } catch (const std::exception& e) {
                qWarning() << "[MicroKernel] 服务处理事件异常:" << QString::fromStdString(name) << "异常:" << e.what();
            } catch (...) {
                qWarning() << "[MicroKernel] 服务处理事件未知异常:" << QString::fromStdString(name);
            }
        }
    }
    if (processedCount == 0) {
        qWarning() << "[MicroKernel] 没有服务处理事件，类型:" << event.type() << "可用服务:" << availableCount;
    } else {
        qDebug() << "[MicroKernel] 事件处理完成，类型:" << event.type() << "处理服务数:" << processedCount << "总服务数:" << availableCount;
    }
}

} // namespace KylinMessenger::Core