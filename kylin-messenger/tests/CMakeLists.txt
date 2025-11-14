cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

set(KYLIN_GTEST_ROOT "/usr/src/googletest" CACHE PATH "Path to system GoogleTest source tree")
set(_gtest_lib_dir "${KYLIN_GTEST_ROOT}/build/lib")
set(_gtest_include_dir "${KYLIN_GTEST_ROOT}/googletest/include")

set(GTEST_LIB_TARGET "")
set(GTEST_MAIN_TARGET "")

find_package(GTest QUIET)

if(GTest_FOUND AND TARGET GTest::GTest AND TARGET GTest::Main)
    get_target_property(_gtest_target_type GTest::GTest TYPE)
    if(_gtest_target_type AND NOT _gtest_target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(_gtest_imported_location GTest::GTest IMPORTED_LOCATION)
        get_filename_component(_gtest_ext "${_gtest_imported_location}" EXT)
        string(TOLOWER "${_gtest_ext}" _gtest_ext_lower)
        if(_gtest_ext_lower STREQUAL ".a" OR _gtest_ext_lower STREQUAL ".so" OR _gtest_ext_lower STREQUAL "")
            set(GTEST_LIB_TARGET GTest::GTest)
            set(GTEST_MAIN_TARGET GTest::Main)
        else()
            message(STATUS "Ignoring incompatible GoogleTest import at ${_gtest_imported_location}")
        endif()
    else()
        message(STATUS "Ignoring GoogleTest target of type ${_gtest_target_type}")
    endif()
endif()

if(NOT GTEST_LIB_TARGET OR NOT GTEST_MAIN_TARGET)
    if(EXISTS "${_gtest_lib_dir}/libgtest.a" AND EXISTS "${_gtest_lib_dir}/libgtest_main.a")
        add_library(KylinGTest::GTest STATIC IMPORTED)
        set_target_properties(KylinGTest::GTest PROPERTIES
            IMPORTED_LOCATION "${_gtest_lib_dir}/libgtest.a"
            INTERFACE_INCLUDE_DIRECTORIES "${_gtest_include_dir}"
        )

        add_library(KylinGTest::Main STATIC IMPORTED)
        set_target_properties(KylinGTest::Main PROPERTIES
            IMPORTED_LOCATION "${_gtest_lib_dir}/libgtest_main.a"
            INTERFACE_INCLUDE_DIRECTORIES "${_gtest_include_dir}"
        )

        set(GTEST_LIB_TARGET KylinGTest::GTest)
        set(GTEST_MAIN_TARGET KylinGTest::Main)
        set(KYLIN_USING_FALLBACK_GTEST TRUE)
        message(STATUS "Using fallback GoogleTest from ${_gtest_lib_dir}")
    else()
        message(WARNING "GoogleTest static libraries not found, skipping tests")
        return()
    endif()
endif()

# 选择 Qt 包前缀（与顶层保持一致）
if(TARGET Qt6::Core)
    set(QT_PACKAGE_PREFIX Qt6)
elseif(TARGET Qt5::Core)
    set(QT_PACKAGE_PREFIX Qt5)
else()
    message(FATAL_ERROR "Neither Qt6 nor Qt5 targets are available.")
endif()

if(QT_PACKAGE_PREFIX STREQUAL "Qt6")
    find_package(Qt6 COMPONENTS Test REQUIRED)
else()
    find_package(Qt5 COMPONENTS Test REQUIRED)
endif()

# 移除依赖自定义协议的测试用例（统一到 IPMSG 后不再适用）

add_executable(test_ai_service
    test_ai_service.cpp
)

target_link_libraries(test_ai_service PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${GTEST_LIB_TARGET}
    ${GTEST_MAIN_TARGET}
    Threads::Threads
)

# 轻量级核心组件测试
add_executable(test_micro_kernel
    test_micro_kernel.cpp
)

target_link_libraries(test_micro_kernel PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${GTEST_LIB_TARGET}
    ${GTEST_MAIN_TARGET}
    Threads::Threads
)

add_executable(test_opencv_nsfw_detector
    test_opencv_nsfw_detector.cpp
)

target_link_libraries(test_opencv_nsfw_detector PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${GTEST_LIB_TARGET}
    ${GTEST_MAIN_TARGET}
    Threads::Threads
)

add_executable(test_lightweight_discovery
    test_lightweight_discovery.cpp
)

target_link_libraries(test_lightweight_discovery PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Network
    ${GTEST_LIB_TARGET}
    ${GTEST_MAIN_TARGET}
    Threads::Threads
)

add_executable(test_concurrent_file_transfer
    test_concurrent_file_transfer.cpp
)

target_link_libraries(test_concurrent_file_transfer PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Network
    ${GTEST_LIB_TARGET}
    ${GTEST_MAIN_TARGET}
    Threads::Threads
)

if(KYLIN_USING_FALLBACK_GTEST)
    if(TARGET test_network_protocol)
        target_include_directories(test_network_protocol PRIVATE BEFORE ${_gtest_include_dir})
    endif()
    if(TARGET test_ai_service)
        target_include_directories(test_ai_service PRIVATE BEFORE ${_gtest_include_dir})
    endif()
    if(TARGET test_micro_kernel)
        target_include_directories(test_micro_kernel PRIVATE BEFORE ${_gtest_include_dir})
    endif()
    if(TARGET test_opencv_nsfw_detector)
        target_include_directories(test_opencv_nsfw_detector PRIVATE BEFORE ${_gtest_include_dir})
    endif()
    if(TARGET test_lightweight_discovery)
        target_include_directories(test_lightweight_discovery PRIVATE BEFORE ${_gtest_include_dir})
    endif()
    if(TARGET test_concurrent_file_transfer)
        target_include_directories(test_concurrent_file_transfer PRIVATE BEFORE ${_gtest_include_dir})
    endif()
endif()

# 注册测试
add_test(NAME AIServiceTest COMMAND test_ai_service)
add_test(NAME MicroKernelTest COMMAND test_micro_kernel)
add_test(NAME NSFWDetectorTest COMMAND test_opencv_nsfw_detector)
add_test(NAME LightweightDiscoveryTest COMMAND test_lightweight_discovery)
add_test(NAME ConcurrentFileTransferTest COMMAND test_concurrent_file_transfer)

add_executable(test_ui_smoke
    ui_smoke_test.cpp
)

target_link_libraries(test_ui_smoke PRIVATE
    ${PROJECT_NAME}-core
    ${QT_PACKAGE_PREFIX}::Core
    ${QT_PACKAGE_PREFIX}::Gui
    ${QT_PACKAGE_PREFIX}::Widgets
    ${QT_PACKAGE_PREFIX}::Test
)

add_test(NAME UISmokeTest COMMAND test_ui_smoke)

message(STATUS "Tests configured")
