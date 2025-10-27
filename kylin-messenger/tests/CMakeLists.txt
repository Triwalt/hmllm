cmake_minimum_required(VERSION 3.16)

# 查找GoogleTest（可选）
find_package(GTest)

if(NOT GTest_FOUND)
    message(WARNING "GoogleTest not found, skipping tests")
    return()
endif()

# 测试可执行文件
add_executable(test_network_protocol
    test_network_protocol.cpp
)

target_link_libraries(test_network_protocol PRIVATE
    Qt6::Core
    Qt6::Network
    ZLIB::ZLIB
    GTest::GTest
    GTest::Main
)

add_executable(test_ai_service
    test_ai_service.cpp
)

target_link_libraries(test_ai_service PRIVATE
    Qt6::Core
    GTest::GTest
    GTest::Main
)

# 注册测试
add_test(NAME NetworkProtocolTest COMMAND test_network_protocol)
add_test(NAME AIServiceTest COMMAND test_ai_service)

message(STATUS "Tests configured")
