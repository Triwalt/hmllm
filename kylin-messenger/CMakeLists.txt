cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _project_version_raw)
string(STRIP "${_project_version_raw}" _project_version_stripped)
set(_project_version_regex "^[0-9]+\\.[0-9]+\\.[0-9]+$")
if(NOT _project_version_stripped MATCHES ${_project_version_regex})
    message(WARNING "VERSION 文件格式非法，使用 0.0.0 作为默认版本号")
    set(_project_version_stripped "0.0.0")
endif()

project(kylin-messenger VERSION ${_project_version_stripped} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 构建选项
option(ENABLE_AI_FEATURES "Enable AI features" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build API documentation" OFF)
option(ENABLE_NSFW "Enable NSFW compliance service" OFF)
# 仅使用 IPMSG 协议，不启用自定义二进制协议与 ProtoBuf

# 查找Qt：优先 Qt6，若不可用则回退 Qt5（便于 Ubuntu 20.04 开发调试）
set(QT_REQUIRED_COMPONENTS Core Widgets Network Gui Qml Quick QuickControls2)
set(HAVE_QT_WEBSOCKETS FALSE)

find_package(Qt6 COMPONENTS ${QT_REQUIRED_COMPONENTS})
if(Qt6_FOUND)
    message(STATUS "Using Qt6: ${Qt6_VERSION}")
    set(QT_PACKAGE_PREFIX Qt6)
    set(PROJECT_QT_VERSION ${Qt6_VERSION})
    find_package(Qt6 COMPONENTS WebSockets QUIET)
    if(Qt6WebSockets_FOUND)
        set(HAVE_QT_WEBSOCKETS TRUE)
        message(STATUS "Qt6 WebSockets module available")
    else()
        message(WARNING "Qt6 WebSockets 模块不可用，跨网关功能将被禁用")
    endif()
else()
    find_package(Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
    message(WARNING "Qt6 not found. Falling back to Qt5: ${Qt5Core_VERSION}")
    set(QT_PACKAGE_PREFIX Qt5)
    set(PROJECT_QT_VERSION ${Qt5Core_VERSION})
    find_package(Qt5 COMPONENTS WebSockets QUIET)
    if(Qt5WebSockets_FOUND)
        set(HAVE_QT_WEBSOCKETS TRUE)
        message(STATUS "Qt5 WebSockets module available")
    else()
        message(WARNING "Qt WebSockets 模块不可用，跨网关功能将被禁用")
    endif()
endif()

# 查找ZLIB (用于CRC32校验)
find_package(ZLIB REQUIRED)

# 查找OpenCV (用于轻量级NSFW检测)
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "找到OpenCV: ${OpenCV_VERSION}")
else()
    message(WARNING "未找到OpenCV，轻量级NSFW检测器将不可用")
endif()

# 查找线程库 (用于轻量级组件)
find_package(Threads REQUIRED)


find_package(Git QUIET)
set(PROJECT_VERSION_GIT ${PROJECT_VERSION})
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --always
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _git_describe
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_git_describe)
        set(PROJECT_VERSION_GIT ${_git_describe})
    endif()
endif()

string(TIMESTAMP PROJECT_VERSION_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
set(VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${VERSION_HEADER_DIR})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/version_info.h.in
    ${VERSION_HEADER_DIR}/version_info.h
    @ONLY
)

# 公共源文件
set(CORE_SOURCES
    src/core/models.cpp
    src/core/logging.cpp
    src/core/di/service_locator.cpp
    src/core/repositories/in_memory_message_repository.cpp
    src/core/repositories/contact_repository.cpp
    # 新增轻量级核心组件
    src/core/micro_kernel.cpp
    src/ai/opencv_nsfw_detector.cpp
    src/network/lightweight_discovery.cpp
    src/transfer/concurrent_file_transfer.cpp
    src/network/network_manager.cpp
    src/network/ipmsg.cpp
    src/services/compliance_stub_service.cpp
    src/ai_echo_service.cpp
    src/ai_service_factory.cpp
    src/ui/chat_window.cpp
    src/ui/main_window.cpp
    src/ui/user_list_page.cpp
    src/ui/contact_list_page.cpp
    src/ui/group_list_page.cpp
    src/ui/file_transfer_dialog.cpp
    src/ui/modern_style.cpp
    src/ui/quick/quick_app_host.cpp
    src/ui/quick/quick_user_list_model.cpp
    src/ui/quick/quick_message_list_model.cpp
    src/ui/quick/quick_contact_list_model.cpp
    src/ui/quick/quick_group_list_model.cpp
    src/ui/quick/quick_conversation_list_model.cpp
    resources/icons/icons.qrc
    resources/emojis/emojis.qrc
    resources/qss/themes.qrc
    resources/qml/qml.qrc
)

# 头文件
set(HEADERS
    include/core/models.h
    include/core/logging.h
    include/core/di/service_locator.h
    include/main_window.h
    include/chat_window.h
    include/network_protocol.h
    include/network_manager.h
    include/ai_service.h
    include/ai_echo_service.h
    include/ai_service_factory.h
    include/compliance_service.h
    include/compliance_stub_service.h
    include/nsfw_compliance_service.h
    include/rknn_nsfw_compliance_service.h
    include/network/constants.h
    include/core/repositories/message_repository.h
    include/core/repositories/contact_repository.h
    # 新增轻量级核心组件头文件
    include/core/micro_kernel.h
    include/ai/opencv_nsfw_detector.h
    include/network/lightweight_discovery.h
    include/transfer/concurrent_file_transfer.h
    include/ui/user_list_page.h
    include/ui/contact_list_page.h
    include/ui/group_list_page.h
    include/ui/file_transfer_dialog.h
    include/ui/modern_style.h
    include/ui/quick_app_host.h
    include/ui/quick_user_list_model.h
    include/ui/quick_message_list_model.h
    include/ui/quick_contact_list_model.h
    include/ui/quick_group_list_model.h
    include/ui/quick_conversation_list_model.h
)

# 网关客户端功能依赖自定义协议/封装，现阶段不编译

# 共享核心库（供可执行文件与单元测试复用）
add_library(${PROJECT_NAME}-core
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME}-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${VERSION_HEADER_DIR}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}-core
    PUBLIC
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Widgets
    ${QT_PACKAGE_PREFIX}::Network
    ${QT_PACKAGE_PREFIX}::Gui
    ${QT_PACKAGE_PREFIX}::Qml
    ${QT_PACKAGE_PREFIX}::Quick
    ${QT_PACKAGE_PREFIX}::QuickControls2
    ZLIB::ZLIB
    Threads::Threads
)

# 如果找到OpenCV，链接OpenCV库
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME}-core PUBLIC ${OpenCV_LIBS})
    target_include_directories(${PROJECT_NAME}-core PUBLIC ${OpenCV_INCLUDE_DIRS})
endif()

# 如需启用跨网关功能，请在后续重构为纯 IPMSG 帧后再恢复


# AI功能
if(ENABLE_AI_FEATURES)
    target_compile_definitions(${PROJECT_NAME}-core PRIVATE ENABLE_AI_FEATURES)

    set(_kylin_enable_rknn_search TRUE)
    if(WIN32 OR APPLE)
        set(_kylin_enable_rknn_search FALSE)
        message(STATUS "跨平台构建: 跳过 RKNN/LLM-RKNN 检测，默认禁用相关功能")
    elseif(UNIX)
        string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _kylin_cpu_lower)
        if(NOT _kylin_cpu_lower MATCHES "arm" AND NOT _kylin_cpu_lower MATCHES "aarch64")
            set(_kylin_enable_rknn_search FALSE)
            message(STATUS "检测到 CPU 架构为 ${CMAKE_SYSTEM_PROCESSOR}，默认禁用 RKNN/LLM-RKNN 依赖")
        endif()
    endif()

    if(ENABLE_NSFW)
        target_sources(${PROJECT_NAME}-core PRIVATE
            src/services/nsfw_compliance_service.cpp
        )
        message(STATUS "启用内置 NSFW 合规实现")

        if(_kylin_enable_rknn_search)
            find_library(RKNN_RT_LIB rknnrt HINTS /usr/lib)
            if(RKNN_RT_LIB)
                message(STATUS "找到RKNN运行时: ${RKNN_RT_LIB}")
                target_link_libraries(${PROJECT_NAME}-core PRIVATE ${RKNN_RT_LIB})
                target_compile_definitions(${PROJECT_NAME}-core PRIVATE HAVE_RKNN_RT)
                target_sources(${PROJECT_NAME}-core PRIVATE
                    src/services/rknn_nsfw_compliance_service.cpp
                )
                message(STATUS "启用 RKNN NSFW 合规实现")
            else()
                message(WARNING "未找到RKNN运行时库，RKNN 后端将不可用")
            endif()
        endif()
    else()
        message(STATUS "NSFW 合规服务已禁用（使用占位合规实现）")
    endif()
endif()

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME}-core PRIVATE -Wall -Wextra -g)
else()
    target_compile_options(${PROJECT_NAME}-core PRIVATE -O3)
endif()

# 应用程序目标
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32
        src/main.cpp
    )
else()
    add_executable(${PROJECT_NAME}
        src/main.cpp
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}-core
)

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 安装桌面文件
install(FILES desktop/kylin-messenger.desktop
    DESTINATION share/applications
)

# 安装图标
set(_app_icon "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.png")
if(EXISTS "${_app_icon}")
    install(FILES "${_app_icon}"
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME kylin-messenger.png
    )
else()
    message(WARNING "未找到应用图标 ${_app_icon}，打包时将跳过图标安装")
endif()

# 打包配置
set(CPACK_PACKAGE_NAME "kylin-messenger")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Kylin Messenger Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "P2P LAN Messenger with AI Features")
set(CPACK_PACKAGE_CONTACT "your.email@example.com")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kylin Messenger Team")
if(Qt6_FOUND)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6network6, libqt6gui6, zlib1g")
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5widgets5, libqt5network5, libqt5gui5, zlib1g")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

include(CPack)

# 测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 文档
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        doxygen_add_docs(docs
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            COMMENT "生成API文档"
        )
    endif()
endif()

# 打印配置摘要
message(STATUS "")
message(STATUS "========== 配置摘要 ==========")
message(STATUS "项目: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Git描述: ${PROJECT_VERSION_GIT}")
message(STATUS "构建时间: ${PROJECT_VERSION_BUILD_TIMESTAMP}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Qt版本: ${PROJECT_QT_VERSION}")
message(STATUS "AI功能: ${ENABLE_AI_FEATURES}")
message(STATUS "构建测试: ${BUILD_TESTS}")
message(STATUS "构建文档: ${BUILD_DOCS}")
message(STATUS "安装前缀: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "================================")
message(STATUS "")
