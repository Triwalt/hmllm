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

# 查找Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

# 查找ZLIB (用于CRC32校验)
find_package(ZLIB REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 源文件
set(SOURCES
    src/main.cpp
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

# 可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ZLIB::ZLIB
)

# AI功能
if(ENABLE_AI_FEATURES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_AI_FEATURES)
    
    # 可选: 查找RKNN运行时
    find_library(RKNN_RT_LIB rknnrt HINTS /usr/lib)
    if(RKNN_RT_LIB)
        message(STATUS "找到RKNN运行时: ${RKNN_RT_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${RKNN_RT_LIB})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_RKNN_RT)
    else()
        message(WARNING "未找到RKNN运行时库，AI功能可能受限")
    endif()
    
    # 可选: 查找libllm-rknn
    find_library(LLM_RKNN_LIB llm-rknn HINTS /usr/lib /usr/local/lib)
    if(LLM_RKNN_LIB)
        message(STATUS "找到libllm-rknn: ${LLM_RKNN_LIB}")
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LLM_RKNN_LIB})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LLM_RKNN)
    else()
        message(WARNING "未找到libllm-rknn，LLM功能将不可用")
    endif()
endif()

# 编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -g)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 安装桌面文件
install(FILES desktop/kylin-messenger.desktop
    DESTINATION share/applications
)

# 安装图标
install(FILES resources/icons/app_icon.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME kylin-messenger.png
)

# 打包配置
set(CPACK_PACKAGE_NAME "kylin-messenger")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Kylin Messenger Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "P2P LAN Messenger with AI Features")
set(CPACK_PACKAGE_CONTACT "your.email@example.com")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kylin Messenger Team")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6network6, zlib1g")
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
