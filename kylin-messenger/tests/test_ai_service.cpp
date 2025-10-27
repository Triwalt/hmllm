// test_ai_service.cpp - AI服务单元测试
#include <gtest/gtest.h>
#include "../include/ai_echo_service.h"
#include "../include/ai_service_factory.h"
#include <thread>
#include <chrono>

using namespace KylinMessenger;

class AIServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        service = AIServiceFactory::createService("echo");
        ASSERT_NE(service, nullptr);
        ASSERT_TRUE(service->initialize(""));
    }
    
    void TearDown() override {
        if (service) {
            service->shutdown();
        }
    }
    
    std::unique_ptr<IAIService> service;
};

TEST_F(AIServiceTest, BasicTextProcessing) {
    AIResult result = service->processText("Hello World");
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.text_output.empty());
    EXPECT_EQ(result.text_output, "Echo: Hello World");
    EXPECT_GT(result.confidence, 0.0f);
}

TEST_F(AIServiceTest, AsyncTextProcessing) {
    bool callback_called = false;
    std::string received_output;
    
    service->processTextAsync("Async Test", 
        [&](const AIResult& result) {
            callback_called = true;
            received_output = result.text_output;
        });
    
    // 等待异步回调
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_output, "Echo: Async Test");
}

TEST_F(AIServiceTest, StreamTextProcessing) {
    std::string accumulated;
    bool stream_complete = false;
    
    AIResult result = service->processTextStream("Stream Test",
        [&](const std::string& token, bool is_final) {
            accumulated += token;
            if (is_final) {
                stream_complete = true;
            }
            return true;
        });
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(stream_complete);
    EXPECT_FALSE(accumulated.empty());
}

TEST_F(AIServiceTest, SmartReplyGeneration) {
    std::vector<std::string> history = {
        "你好",
        "你好！有什么可以帮你？"
    };
    
    AIResult result = service->generateSmartReplies(history, 3);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.suggestions.empty());
    EXPECT_LE(result.suggestions.size(), 3);
    
    for (const auto& suggestion : result.suggestions) {
        EXPECT_FALSE(suggestion.empty());
    }
}

TEST_F(AIServiceTest, ContentAnalysisSentiment) {
    AIResult result = service->analyzeContent(
        "这真是太好了！我很开心！", 
        ContentAnalysisType::Sentiment);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.text_output, "positive");
}

TEST_F(AIServiceTest, ContentAnalysisSafety) {
    AIResult result = service->analyzeContent(
        "这是一条正常的消息",
        ContentAnalysisType::SafetyFilter);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.metadata["is_safe"], "true");
}

TEST_F(AIServiceTest, LanguageDetection) {
    AIResult result1 = service->analyzeContent(
        "这是中文文本",
        ContentAnalysisType::LanguageDetection);
    
    EXPECT_TRUE(result1.success);
    EXPECT_EQ(result1.text_output, "zh-CN");
    
    AIResult result2 = service->analyzeContent(
        "This is English text",
        ContentAnalysisType::LanguageDetection);
    
    EXPECT_TRUE(result2.success);
    EXPECT_EQ(result2.text_output, "en-US");
}

TEST_F(AIServiceTest, CancelOperation) {
    bool stream_cancelled = false;
    
    // 启动一个长时间运行的流式操作
    std::thread worker([&]() {
        AIResult result = service->processTextStream(
            "Long running operation",
            [](const std::string&, bool) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return true;
            });
        
        stream_cancelled = !result.success;
    });
    
    // 短暂延迟后取消
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(service->cancelOperation());
    
    worker.join();
    EXPECT_TRUE(stream_cancelled);
}

TEST_F(AIServiceTest, ServiceFactory) {
    auto services = AIServiceFactory::getRegisteredServices();
    
    EXPECT_FALSE(services.empty());
    EXPECT_TRUE(AIServiceFactory::isServiceRegistered("echo"));
    
    auto echo_service = AIServiceFactory::createService("echo");
    EXPECT_NE(echo_service, nullptr);
    
    auto invalid_service = AIServiceFactory::createService("non_existent");
    EXPECT_EQ(invalid_service, nullptr);
}

TEST_F(AIServiceTest, Capabilities) {
    AICapability caps = service->getCapabilities();
    
    EXPECT_TRUE(caps & AICapability::TextProcessing);
    EXPECT_TRUE(caps & AICapability::SmartReply);
    EXPECT_TRUE(caps & AICapability::ContentAnalysis);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
