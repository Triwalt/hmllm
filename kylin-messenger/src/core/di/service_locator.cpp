#include "core/di/service_locator.h"

namespace KylinMessenger::Core::DI {

ServiceLocator& ServiceLocator::instance()
{
    static ServiceLocator locator;
    return locator;
}

void ServiceLocator::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
}

} // namespace KylinMessenger::Core::DI

