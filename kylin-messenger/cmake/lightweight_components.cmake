# 轻量级组件构建配置
# 包含微内核、OpenCV NSFW检测器、网络发现和并发文件传输

# 查找OpenCV库
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "找到OpenCV: ${OpenCV_VERSION}")
    message(STATUS "OpenCV包含目录: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV库: ${OpenCV_LIBS}")
else()
    message(FATAL_ERROR "未找到OpenCV，轻量级NSFW检测器需要OpenCV库")
endif()

# 轻量级核心源文件
set(LIGHTWEIGHT_CORE_SOURCES
    src/core/micro_kernel.cpp
    src/ai/opencv_nsfw_detector.cpp
    src/network/lightweight_discovery.cpp
    src/transfer/concurrent_file_transfer.cpp
)

# 轻量级核心头文件
set(LIGHTWEIGHT_CORE_HEADERS
    include/core/micro_kernel.h
    include/ai/opencv_nsfw_detector.h
    include/network/lightweight_discovery.h
    include/transfer/concurrent_file_transfer.h
)

# 轻量级组件编译选项
if(MSVC)
    # MSVC特定选项
    set(LIGHTWEIGHT_COMPILE_OPTIONS
        /O2          # 最大优化
        /GL          # 全程序优化
        /EHsc        # 异常处理
        /MP          # 多核编译
    )
else()
    # GCC/Clang选项
    set(LIGHTWEIGHT_COMPILE_OPTIONS
        -O3          # 最高优化级别
        -march=native # 本地CPU优化
        -flto        # 链接时优化
        -funroll-loops # 循环展开
    )
endif()

# 创建轻量级核心库
add_library(kylin-lightweight-core STATIC
    ${LIGHTWEIGHT_CORE_SOURCES}
    ${LIGHTWEIGHT_CORE_HEADERS}
)

# 设置编译选项
target_compile_options(kylin-lightweight-core PRIVATE ${LIGHTWEIGHT_COMPILE_OPTIONS})

# 设置包含目录
target_include_directories(kylin-lightweight-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(kylin-lightweight-core PUBLIC
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Network
    ${OpenCV_LIBS}
    Threads::Threads
)

# 设置C++17标准
target_compile_features(kylin-lightweight-core PUBLIC cxx_std_17)

# 安装规则
install(TARGETS kylin-lightweight-core
    ARCHIVE DESTINATION lib
)

# 安装头文件
install(FILES ${LIGHTWEIGHT_CORE_HEADERS}
    DESTINATION include/kylin-lightweight
)

# 性能优化配置
option(LIGHTWEIGHT_ENABLE_LTO "启用链接时优化" ON)
option(LIGHTWEIGHT_ENABLE_NATIVE "启用本地CPU优化" ON)
option(LIGHTWEIGHT_ENABLE_OPENMP "启用OpenMP并行化" OFF)

if(LIGHTWEIGHT_ENABLE_LTO)
    set_property(TARGET kylin-lightweight-core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(LIGHTWEIGHT_ENABLE_NATIVE AND NOT MSVC)
    target_compile_options(kylin-lightweight-core PRIVATE -march=native)
endif()

if(LIGHTWEIGHT_ENABLE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(kylin-lightweight-core PUBLIC OpenMP::OpenMP_CXX)
        target_compile_definitions(kylin-lightweight-core PRIVATE LIGHTWEIGHT_OPENMP_ENABLED)
    endif()
endif()

# 内存优化配置
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(kylin-lightweight-core PRIVATE
        LIGHTWEIGHT_RELEASE_BUILD
        NDEBUG
    )

    if(NOT MSVC)
        target_link_options(kylin-lightweight-core PRIVATE -s) # 剥离符号表
    endif()
endif()

# 调试配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(kylin-lightweight-core PRIVATE
        LIGHTWEIGHT_DEBUG_BUILD
        _DEBUG
    )
endif()

# 跨平台配置
if(WIN32)
    target_compile_definitions(kylin-lightweight-core PRIVATE
        LIGHTWEIGHT_PLATFORM_WINDOWS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
elseif(APPLE)
    target_compile_definitions(kylin-lightweight-core PRIVATE
        LIGHTWEIGHT_PLATFORM_MACOS
    )
elseif(UNIX)
    target_compile_definitions(kylin-lightweight-core PRIVATE
        LIGHTWEIGHT_PLATFORM_LINUX
    )
endif()

# 模型文件配置
set(NSFW_MODEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/models/mobilenetv2_nsfw.onnx"
    CACHE PATH "NSFW检测模型文件路径")

# 检查模型文件是否存在
if(EXISTS "${NSFW_MODEL_PATH}")
    message(STATUS "找到NSFW模型文件: ${NSFW_MODEL_PATH}")
    target_compile_definitions(kylin-lightweight-core PRIVATE
        NSFW_MODEL_PATH="${NSFW_MODEL_PATH}"
    )
else()
    message(WARNING "未找到NSFW模型文件: ${NSFW_MODEL_PATH}")
    message(WARNING "请下载MobileNetV2 NSFW模型文件到指定路径")
endif()

# 性能基准测试
if(BUILD_TESTS)
    # 启用性能基准测试
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        message(STATUS "找到Google Benchmark库，启用性能测试")
        target_compile_definitions(kylin-lightweight-core PRIVATE LIGHTWEIGHT_BENCHMARK_ENABLED)
    endif()
endif()

# 导出配置
set(LIGHTWEIGHT_CORE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
set(LIGHTWEIGHT_CORE_LIBRARIES kylin-lightweight-core PARENT_SCOPE)
set(LIGHTWEIGHT_CORE_FOUND TRUE PARENT_SCOPE)

message(STATUS "轻量级核心组件配置完成")
message(STATUS "  - 源文件数量: ${LIGHTWEIGHT_CORE_SOURCES}")
message(STATUS "  - OpenCV版本: ${OpenCV_VERSION}")
message(STATUS "  - 编译优化: ${LIGHTWEIGHT_COMPILE_OPTIONS}")
message(STATUS "  - 模型路径: ${NSFW_MODEL_PATH}")