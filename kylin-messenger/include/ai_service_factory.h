// ai_service_factory.h - AI服务工厂
#ifndef KYLIN_MESSENGER_AI_SERVICE_FACTORY_H
#define KYLIN_MESSENGER_AI_SERVICE_FACTORY_H

#include "ai_service.h"
#include <memory>
#include <map>
#include <functional>
#include <string>

namespace KylinMessenger {

/**
 * @brief AI服务工厂类
 * 
 * 使用工厂模式创建不同类型的AI服务实例。
 * 支持动态注册新的AI服务类型。
 */
class AIServiceFactory
{
public:
    using ServiceCreator = std::function<std::unique_ptr<IAIService>()>;
    
    /**
     * @brief 创建AI服务实例
     * @param service_name 服务名称（如 "echo", "llm_chat", "image_tagger"）
     * @return 服务实例，失败返回nullptr
     */
    static std::unique_ptr<IAIService> createService(
        const std::string& service_name);
    
    /**
     * @brief 注册新的AI服务类型
     * @param service_name 服务名称
     * @param creator 创建函数
     * @return 成功返回true
     */
    static bool registerService(
        const std::string& service_name,
        ServiceCreator creator);
    
    /**
     * @brief 获取所有已注册的服务名称
     * @return 服务名称列表
     */
    static std::vector<std::string> getRegisteredServices();
    
    /**
     * @brief 检查服务是否已注册
     * @param service_name 服务名称
     * @return 已注册返回true
     */
    static bool isServiceRegistered(const std::string& service_name);
    
private:
    static std::map<std::string, ServiceCreator>& getRegistry();
    static void registerBuiltinServices();
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_AI_SERVICE_FACTORY_H
