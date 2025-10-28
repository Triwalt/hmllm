cmake_minimum_required(VERSION 3.16)

project(kylin-messenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 构建选项
option(ENABLE_AI_FEATURES "Enable AI features" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_DOCS "Build API documentation" OFF)

# 查找Qt：优先 Qt6，若不可用则回退 Qt5（便于 Ubuntu 20.04 开发调试）
set(QT_REQUIRED_COMPONENTS Core Widgets Network Gui)
find_package(Qt6 COMPONENTS ${QT_REQUIRED_COMPONENTS})
if(Qt6_FOUND)
    message(STATUS "Using Qt6: ${Qt6_VERSION}")
    set(QT_PACKAGE_PREFIX Qt6)
else()
    find_package(Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
    message(WARNING "Qt6 not found. Falling back to Qt5: ${Qt5Core_VERSION}")
    set(QT_PACKAGE_PREFIX Qt5)
endif()

# 查找ZLIB (用于CRC32校验)
find_package(ZLIB REQUIRED)

# 公共源文件
set(CORE_SOURCES
    src/main_window.cpp
    src/chat_window.cpp
    src/network_protocol.cpp
    src/network_manager.cpp
    src/ai_echo_service.cpp
    src/ai_service_factory.cpp
)

# 头文件
set(HEADERS
    include/main_window.h
    include/chat_window.h
    include/network_protocol.h
    include/network_manager.h
    include/ai_service.h
    include/ai_echo_service.h
    include/ai_service_factory.h
)

# 共享核心库（供可执行文件与单元测试复用）
add_library(${PROJECT_NAME}-core
    ${CORE_SOURCES}
    ${HEADERS}
)

target_include_directories(${PROJECT_NAME}-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}-core
    PUBLIC
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Widgets
    ${QT_PACKAGE_PREFIX}::Network
    ${QT_PACKAGE_PREFIX}::Gui
    ZLIB::ZLIB
)

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

    if(_kylin_enable_rknn_search)
        # 可选: 查找RKNN运行时
        find_library(RKNN_RT_LIB rknnrt HINTS /usr/lib)
        if(RKNN_RT_LIB)
            message(STATUS "找到RKNN运行时: ${RKNN_RT_LIB}")
            target_link_libraries(${PROJECT_NAME}-core PRIVATE ${RKNN_RT_LIB})
            target_compile_definitions(${PROJECT_NAME}-core PRIVATE HAVE_RKNN_RT)
        else()
            message(WARNING "未找到RKNN运行时库，AI功能可能受限")
        endif()

        # 可选: 查找libllm-rknn
        find_library(LLM_RKNN_LIB llm-rknn HINTS /usr/lib /usr/local/lib)
        if(LLM_RKNN_LIB)
            message(STATUS "找到libllm-rknn: ${LLM_RKNN_LIB}")
            target_link_libraries(${PROJECT_NAME}-core PRIVATE ${LLM_RKNN_LIB})
            target_compile_definitions(${PROJECT_NAME}-core PRIVATE HAVE_LLM_RKNN)
        else()
            message(WARNING "未找到libllm-rknn，LLM功能将不可用")
        endif()
    endif()
endif()

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME}-core PRIVATE -Wall -Wextra -g)
else()
    target_compile_options(${PROJECT_NAME}-core PRIVATE -O3)
endif()

# 应用程序目标
add_executable(${PROJECT_NAME}
    src/main.cpp
)

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
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++标准: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Qt版本: ${Qt6_VERSION}")
message(STATUS "AI功能: ${ENABLE_AI_FEATURES}")
message(STATUS "构建测试: ${BUILD_TESTS}")
message(STATUS "构建文档: ${BUILD_DOCS}")
message(STATUS "安装前缀: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "================================")
message(STATUS "")
