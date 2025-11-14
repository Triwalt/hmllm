/**
 * @file micro_kernel.h
 * @brief 微内核架构核心
 * @version 1.0.0
 */

#ifndef KYLIN_MESSENGER_CORE_MICRO_KERNEL_H
#define KYLIN_MESSENGER_CORE_MICRO_KERNEL_H

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <atomic>

namespace KylinMessenger::Core {

class Event;
class LightweightService;

/**
 * @brief 微内核核心类
 * 负责服务的加载、事件分发和生命周期管理
 */
class MicroKernel {
public:
    MicroKernel();
    ~MicroKernel();

    // 禁止拷贝
    MicroKernel(const MicroKernel&) = delete;
    MicroKernel& operator=(const MicroKernel&) = delete;

    /**
     * @brief 加载服务
     * @param service 服务实例
     * @param name 服务名称
     */
    void loadService(std::unique_ptr<LightweightService> service, const std::string& name);

    /**
     * @brief 启动所有服务
     * @return 是否成功启动
     */
    bool start();

    /**
     * @brief 优雅关闭所有服务
     */
    void shutdown();

    /**
     * @brief 发布事件
     * @param event 事件对象
     */
    void publishEvent(const Event& event);

    /**
     * @brief 获取服务状态
     * @return 运行状态
     */
    bool isRunning() const { return running_.load(); }

    /**
     * @brief 获取已加载的服务列表
     * @return 服务名称列表
     */
    std::vector<std::string> getLoadedServices() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::atomic<bool> running_{false};
};

/**
 * @brief 事件基类
 * 所有事件都继承自此基类
 */
class Event {
public:
    enum Type {
        // 系统事件
        ServiceStarted,
        ServiceStopped,
        ShutdownRequested,
        
        // 网络事件
        UserOnline,
        UserOffline,
        MessageReceived,
        FileTransferStarted,
        FileTransferProgress,
        FileTransferCompleted,
        
        // AI事件
        ContentAnalysisRequested,
        ContentAnalysisCompleted,
        
        // UI事件
        WindowShown,
        WindowHidden,
        UserInteraction
    };

    explicit Event(Type type) : type_(type) {}
    virtual ~Event() = default;

    Type type() const { return type_; }
    const std::string& source() const { return source_; }
    void setSource(const std::string& source) { source_ = source; }

private:
    Type type_;
    std::string source_;
};

/**
 * @brief 轻量级服务接口
 * 所有服务必须实现此接口
 */
class LightweightService {
public:
    virtual ~LightweightService() = default;

    /**
     * @brief 初始化服务
     * @return 是否成功
     */
    virtual bool initialize() = 0;

    /**
     * @brief 处理事件
     * @param event 事件对象
     */
    virtual void processEvent(const Event& event) = 0;

    /**
     * @brief 关闭服务
     */
    virtual void shutdown() = 0;

    /**
     * @brief 获取服务名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 检查服务是否可用
     */
    virtual bool isAvailable() const = 0;
};

} // namespace KylinMessenger::Core

#endif // KYLIN_MESSENGER_CORE_MICRO_KERNEL_H