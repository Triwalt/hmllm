/**
 * @file test_micro_kernel.cpp
 * @brief 微内核单元测试
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/micro_kernel.h"
#include <thread>
#include <chrono>

using namespace KylinMessenger::Core;

// 模拟服务
class MockService : public LightweightService {
public:
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, processEvent, (const Event&), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(std::string, getName, (), (const override));
    MOCK_METHOD(bool, isAvailable, (), (const override));
};

// 测试事件
class TestEvent : public Event {
public:
    explicit TestEvent(int data) : Event(Event::ServiceStarted), data_(data) {}
    int data() const { return data_; }

private:
    int data_;
};

// 测试微内核基本功能
TEST(MicroKernelTest, BasicLifecycle) {
    MicroKernel kernel;
    
    // 创建模拟服务
    auto mockService = std::make_unique<MockService>();
    EXPECT_CALL(*mockService, initialize()).WillOnce(testing::Return(true));
    EXPECT_CALL(*mockService, getName()).WillRepeatedly(testing::Return("MockService"));
    EXPECT_CALL(*mockService, isAvailable()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockService, shutdown()).Times(1);
    
    // 加载服务
    kernel.loadService(std::move(mockService), "MockService");
    
    // 启动内核
    EXPECT_TRUE(kernel.start());
    EXPECT_TRUE(kernel.isRunning());
    
    // 关闭内核
    kernel.shutdown();
    EXPECT_FALSE(kernel.isRunning());
}

// 测试事件分发
TEST(MicroKernelTest, EventDistribution) {
    MicroKernel kernel;
    
    // 创建两个模拟服务
    auto mockService1 = std::make_unique<MockService>();
    auto mockService2 = std::make_unique<MockService>();
    
    EXPECT_CALL(*mockService1, initialize()).WillOnce(testing::Return(true));
    EXPECT_CALL(*mockService2, initialize()).WillOnce(testing::Return(true));
    
    EXPECT_CALL(*mockService1, getName()).WillRepeatedly(testing::Return("Service1"));
    EXPECT_CALL(*mockService2, getName()).WillRepeatedly(testing::Return("Service2"));
    
    EXPECT_CALL(*mockService1, isAvailable()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockService2, isAvailable()).WillRepeatedly(testing::Return(true));
    
    // 期望两个服务都收到事件
    EXPECT_CALL(*mockService1, processEvent(testing::_)).Times(2);
    EXPECT_CALL(*mockService2, processEvent(testing::_)).Times(2);
    
    EXPECT_CALL(*mockService1, shutdown()).Times(1);
    EXPECT_CALL(*mockService2, shutdown()).Times(1);
    
    // 加载服务
    kernel.loadService(std::move(mockService1), "Service1");
    kernel.loadService(std::move(mockService2), "Service2");
    
    // 启动内核
    ASSERT_TRUE(kernel.start());
    
    // 发布事件
    Event event1(Event::UserOnline);
    event1.setSource("Test");
    kernel.publishEvent(event1);
    
    Event event2(Event::MessageReceived);
    event2.setSource("Test");
    kernel.publishEvent(event2);
    
    // 给事件处理一些时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 关闭内核
    kernel.shutdown();
}

// 测试服务初始化失败
TEST(MicroKernelTest, ServiceInitializationFailure) {
    MicroKernel kernel;
    
    // 创建模拟服务，初始化失败
    auto mockService = std::make_unique<MockService>();
    EXPECT_CALL(*mockService, initialize()).WillOnce(testing::Return(false));
    EXPECT_CALL(*mockService, getName()).WillRepeatedly(testing::Return("FailingService"));
    EXPECT_CALL(*mockService, isAvailable()).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*mockService, shutdown()).Times(0); // 不应该调用shutdown
    
    // 加载服务
    kernel.loadService(std::move(mockService), "FailingService");
    
    // 启动内核应该失败
    EXPECT_FALSE(kernel.start());
    EXPECT_FALSE(kernel.isRunning());
}

// 测试多线程事件处理
TEST(MicroKernelTest, MultiThreadedEventHandling) {
    MicroKernel kernel;
    
    auto mockService = std::make_unique<MockService>();
    EXPECT_CALL(*mockService, initialize()).WillOnce(testing::Return(true));
    EXPECT_CALL(*mockService, getName()).WillRepeatedly(testing::Return("ThreadTestService"));
    EXPECT_CALL(*mockService, isAvailable()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockService, shutdown()).Times(1);
    
    // 期望处理多个事件
    EXPECT_CALL(*mockService, processEvent(testing::_)).Times(testing::AtLeast(10));
    
    kernel.loadService(std::move(mockService), "ThreadTestService");
    ASSERT_TRUE(kernel.start());
    
    // 多线程发布事件
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&kernel, i]() {
            for (int j = 0; j < 10; ++j) {
                Event event(Event::UserInteraction);
                event.setSource("Thread" + std::to_string(i));
                kernel.publishEvent(event);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 给事件处理一些时间
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    kernel.shutdown();
}

// 测试服务列表
TEST(MicroKernelTest, ServiceList) {
    MicroKernel kernel;
    
    // 加载多个服务
    auto service1 = std::make_unique<MockService>();
    auto service2 = std::make_unique<MockService>();
    auto service3 = std::make_unique<MockService>();
    
    EXPECT_CALL(*service1, initialize()).WillOnce(testing::Return(true));
    EXPECT_CALL(*service2, initialize()).WillOnce(testing::Return(true));
    EXPECT_CALL(*service3, initialize()).WillOnce(testing::Return(true));
    
    EXPECT_CALL(*service1, getName()).WillRepeatedly(testing::Return("Service1"));
    EXPECT_CALL(*service2, getName()).WillRepeatedly(testing::Return("Service2"));
    EXPECT_CALL(*service3, getName()).WillRepeatedly(testing::Return("Service3"));
    
    EXPECT_CALL(*service1, isAvailable()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*service2, isAvailable()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*service3, isAvailable()).WillRepeatedly(testing::Return(true));
    
    EXPECT_CALL(*service1, shutdown()).Times(1);
    EXPECT_CALL(*service2, shutdown()).Times(1);
    EXPECT_CALL(*service3, shutdown()).Times(1);
    
    kernel.loadService(std::move(service1), "Service1");
    kernel.loadService(std::move(service2), "Service2");
    kernel.loadService(std::move(service3), "Service3");
    
    // 检查服务列表
    auto services = kernel.getLoadedServices();
    EXPECT_EQ(services.size(), 3);
    EXPECT_THAT(services, testing::UnorderedElementsAre("Service1", "Service2", "Service3"));
    
    ASSERT_TRUE(kernel.start());
    kernel.shutdown();
}

// 性能测试：事件吞吐量
TEST(PerformanceTest, EventThroughput) {
    MicroKernel kernel;
    
    class CountingService : public LightweightService {
    public:
        void processEvent(const Event& event) override {
            if (event.type() == Event::UserInteraction) {
                count_++;
            }
        }
        
        int getCount() const { return count_; }
        
        bool initialize() override { return true; }
        void shutdown() override {}
        std::string getName() const override { return "CountingService"; }
        bool isAvailable() const override { return true; }
        
    private:
        std::atomic<int> count_{0};
    };
    
    auto countingService = std::make_unique<CountingService>();
    auto* servicePtr = countingService.get();
    
    kernel.loadService(std::move(countingService), "CountingService");
    ASSERT_TRUE(kernel.start());
    
    // 发布大量事件
    const int eventCount = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < eventCount; ++i) {
        Event event(Event::UserInteraction);
        event.setSource("PerformanceTest");
        kernel.publishEvent(event);
    }
    
    // 等待处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 检查处理的事件数量
    EXPECT_GE(servicePtr->getCount(), eventCount * 0.9); // 至少处理了90%的事件
    
    // 计算吞吐量
    double throughput = static_cast<double>(servicePtr->getCount()) / (duration.count() / 1000.0);
    std::cout << "Event throughput: " << throughput << " events/second" << std::endl;
    
    kernel.shutdown();
}