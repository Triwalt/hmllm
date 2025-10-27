// ai_echo_service.h - 简单的回声AI服务（用于测试）
#ifndef KYLIN_MESSENGER_AI_ECHO_SERVICE_H
#define KYLIN_MESSENGER_AI_ECHO_SERVICE_H

#include "ai_service.h"

namespace KylinMessenger {

/**
 * @brief 回声AI服务 - 简单的测试实现
 * 
 * 这是一个简单的AI服务实现，用于测试AI服务框架。
 * 它会将输入文本回显，并添加一些简单的智能回复建议。
 */
class EchoAIService : public IAIService
{
public:
    EchoAIService();
    virtual ~EchoAIService() override;
    
    // IAIService接口实现
    virtual bool initialize(const std::string& model_path) override;
    virtual void shutdown() override;
    virtual bool isInitialized() const override;
    
    virtual AICapability getCapabilities() const override;
    
    virtual AIResult processText(const std::string& input) override;
    virtual AIResult processImage(const QImage& image) override;
    
    virtual void processTextAsync(
        const std::string& input,
        AICallback callback) override;
    
    virtual void processImageAsync(
        const QImage& image,
        AICallback callback) override;
    
    virtual AIResult processTextStream(
        const std::string& input,
        AIStreamCallback stream_callback) override;
    
    virtual AIResult generateSmartReplies(
        const std::vector<std::string>& conversation_history,
        int num_suggestions = 3) override;
    
    virtual AIResult analyzeContent(
        const std::string& content,
        ContentAnalysisType type) override;
    
    virtual bool cancelOperation() override;
    
private:
    bool m_initialized;
    std::atomic<bool> m_cancel_requested;
    
    // 辅助函数
    std::vector<std::string> generateMockReplies(
        const std::vector<std::string>& history);
    
    std::string analyzeSentiment(const std::string& text);
};

} // namespace KylinMessenger

#endif // KYLIN_MESSENGER_AI_ECHO_SERVICE_H
