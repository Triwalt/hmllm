#ifndef KYLIN_MESSENGER_CORE_DI_SERVICE_LOCATOR_H
#define KYLIN_MESSENGER_CORE_DI_SERVICE_LOCATOR_H

#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace KylinMessenger::Core::DI {

class ServiceLocator {
public:
    static ServiceLocator& instance();

    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    template <typename Interface>
    void registerService(std::shared_ptr<Interface> service)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[std::type_index(typeid(Interface))] = std::move(service);
    }

    template <typename Interface>
    std::shared_ptr<Interface> resolve() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(std::type_index(typeid(Interface)));
        if (it == services_.end()) {
            return nullptr;
        }
        return std::static_pointer_cast<Interface>(it->second);
    }

    void clear();

private:
    ServiceLocator() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_;
};

} // namespace KylinMessenger::Core::DI

#endif // KYLIN_MESSENGER_CORE_DI_SERVICE_LOCATOR_H

