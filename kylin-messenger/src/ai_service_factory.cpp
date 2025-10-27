// ai_service_factory.cpp - AI服务工厂实现
#include "ai_service_factory.h"
#include "ai_echo_service.h"
#include <QDebug>

namespace KylinMessenger {

std::map<std::string, AIServiceFactory::ServiceCreator>& 
AIServiceFactory::getRegistry()
{
    static std::map<std::string, ServiceCreator> registry;
    static bool initialized = false;
    
    if (!initialized) {
        registerBuiltinServices();
        initialized = true;
    }
    
    return registry;
}

void AIServiceFactory::registerBuiltinServices()
{
    // 注册Echo服务
    registerService("echo", []() {
        return std::make_unique<EchoAIService>();
    });
    
    qInfo() << "已注册内置AI服务: echo";
    
    // TODO: 在这里注册其他内置服务
    // registerService("llm_chat", []() {
    //     return std::make_unique<LLMChatService>();
    // });
    //
    // registerService("image_tagger", []() {
    //     return std::make_unique<ImageTaggerService>();
    // });
}

std::unique_ptr<IAIService> AIServiceFactory::createService(
    const std::string& service_name)
{
    auto& registry = getRegistry();
    
    auto it = registry.find(service_name);
    if (it == registry.end()) {
        qWarning() << "未知的AI服务:" << QString::fromStdString(service_name);
        return nullptr;
    }
    
    try {
        auto service = it->second();
        qInfo() << "创建AI服务:" << QString::fromStdString(service_name);
        return service;
    }
    catch (const std::exception& e) {
        qCritical() << "创建AI服务失败:" << e.what();
        return nullptr;
    }
}

bool AIServiceFactory::registerService(
    const std::string& service_name,
    ServiceCreator creator)
{
    auto& registry = getRegistry();
    
    if (registry.find(service_name) != registry.end()) {
        qWarning() << "AI服务已存在，将被覆盖:" 
                   << QString::fromStdString(service_name);
    }
    
    registry[service_name] = creator;
    return true;
}

std::vector<std::string> AIServiceFactory::getRegisteredServices()
{
    auto& registry = getRegistry();
    std::vector<std::string> services;
    
    for (const auto& pair : registry) {
        services.push_back(pair.first);
    }
    
    return services;
}

bool AIServiceFactory::isServiceRegistered(const std::string& service_name)
{
    auto& registry = getRegistry();
    return registry.find(service_name) != registry.end();
}

} // namespace KylinMessenger
