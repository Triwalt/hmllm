// ai_echo_service.cpp - Echo AI服务实现
#include "ai_echo_service.h"
#include <QThread>
#include <QDebug>
#include <algorithm>
#include <random>
#include <thread>

namespace KylinMessenger {

EchoAIService::EchoAIService()
    : m_initialized(false)
    , m_cancel_requested(false)
{
}

EchoAIService::~EchoAIService()
{
    shutdown();
}

bool EchoAIService::initialize(const std::string& model_path,
                               const std::string& config_path)
{
    qInfo() << "初始化Echo AI服务（测试模式）";
    qInfo() << "模型路径:" << QString::fromStdString(model_path);
    Q_UNUSED(config_path);
    
    m_initialized = true;
    return true;
}

void EchoAIService::shutdown()
{
    if (m_initialized) {
        qInfo() << "关闭Echo AI服务";
        m_initialized = false;
    }
}

bool EchoAIService::isReady() const
{
    return m_initialized;
}

std::string EchoAIService::getName() const
{
    return "Echo Test Service";
}

std::string EchoAIService::getDescription() const
{
    return "Simple echo AI service used for testing";
}

AICapability EchoAIService::getCapabilities() const
{
    AICapability caps = AICapability::TextProcessing;
    caps |= AICapability::SmartReply;
    caps |= AICapability::ContentAnalysis;
    return caps;
}

AIResult EchoAIService::processText(const std::string& input,
                                    const std::string& context)
{
    AIResult result;
    
    if (!m_initialized) {
        result.success = false;
        result.error_message = "服务未初始化";
        return result;
    }
    Q_UNUSED(context);
    
    // 模拟处理延迟
    QThread::msleep(100);
    
    result.success = true;
    result.text_output = "Echo: " + input;
    result.confidence = 1.0f;
    
    // 添加元数据
    result.metadata.insert(QStringLiteral("service"), QStringLiteral("echo"));
    result.metadata.insert(QStringLiteral("input_length"), static_cast<int>(input.length()));
    
    return result;
}

AIResult EchoAIService::processImage(const QImage& image,
                                     const std::string& task)
{
    AIResult result;
    
    if (!m_initialized) {
        result.success = false;
        result.error_message = "服务未初始化";
        return result;
    }
    Q_UNUSED(task);
    
    result.success = true;
    result.text_output = QString("图像尺寸: %1x%2 像素")
        .arg(image.width())
        .arg(image.height())
        .toStdString();
    
    result.confidence = 1.0f;
    
    // 添加图像信息
    result.metadata.insert(QStringLiteral("width"), image.width());
    result.metadata.insert(QStringLiteral("height"), image.height());
    result.metadata.insert(QStringLiteral("format"), static_cast<int>(image.format()));
    
    return result;
}

AIResult EchoAIService::processTextStream(
    const std::string& input,
    AIStreamCallback stream_callback,
    const std::string& context)
{
    AIResult result;
    
    if (!m_initialized) {
        result.success = false;
        result.error_message = "服务未初始化";
        return result;
    }
    Q_UNUSED(context);
    
    m_cancel_requested = false;
    
    // 模拟流式输出
    std::string response = "Echo (流式): " + input;
    
    for (size_t i = 0; i < response.length(); ++i) {
        if (m_cancel_requested) {
            result.success = false;
            result.error_message = "操作已取消";
            return result;
        }
        
        std::string token(1, response[i]);
        
        stream_callback(token, i == response.length() - 1);
        
        // 模拟延迟
        QThread::msleep(50);
    }
    
    result.success = true;
    result.text_output = response;
    result.confidence = 1.0f;
    
    return result;
}

void EchoAIService::processTextAsync(const std::string& input,
                                     AICallback callback,
                                     const std::string& context)
{
    std::thread([this, input, callback, context]() {
        AIResult result = processText(input, context);
        callback(result);
    }).detach();
}

void EchoAIService::processImageAsync(const QImage& image,
                                      AICallback callback,
                                      const std::string& task)
{
    std::thread([this, image, callback, task]() {
        AIResult result = processImage(image, task);
        callback(result);
    }).detach();
}

AIResult EchoAIService::generateSmartReplies(
    const std::vector<std::string>& conversation_history,
    int num_suggestions)
{
    AIResult result;
    
    if (!m_initialized) {
        result.success = false;
        result.error_message = "服务未初始化";
        return result;
    }
    
    result.suggestions = generateMockReplies(conversation_history);
    
    // 限制数量
    if (result.suggestions.size() > static_cast<size_t>(num_suggestions)) {
        result.suggestions.resize(num_suggestions);
    }
    
    result.success = true;
    result.confidence = 0.8f;
    
    return result;
}

AIResult EchoAIService::analyzeContent(
    const std::string& content,
    ContentAnalysisType type)
{
    AIResult result;
    
    if (!m_initialized) {
        result.success = false;
        result.error_message = "服务未初始化";
        return result;
    }
    
    result.success = true;
    
    switch (type) {
        case ContentAnalysisType::Sentiment:
            result.text_output = analyzeSentiment(content);
            result.confidence = 0.75f;
            break;
        case ContentAnalysisType::SafetyFilter:
            result.metadata.insert(QStringLiteral("is_safe"), true);
            result.metadata.insert(QStringLiteral("reason"), QStringLiteral("通过基本检查"));
            result.confidence = 0.9f;
            break;
        case ContentAnalysisType::LanguageDetection: {
            bool has_chinese = false;
            for (unsigned char c : content) {
                if (c > 127) {
                    has_chinese = true;
                    break;
                }
            }
            result.text_output = has_chinese ? "zh-CN" : "en-US";
            result.confidence = 0.85f;
            break;
        }
    }
    
    return result;
}

bool EchoAIService::cancelOperation()
{
    m_cancel_requested = true;
    return true;
}

void EchoAIService::resetContext()
{
    m_cancel_requested = false;
}

// ============================================================================
// 私有辅助函数
// ============================================================================

std::vector<std::string> EchoAIService::generateMockReplies(
    const std::vector<std::string>& history)
{
    std::vector<std::string> replies;
    
    if (history.empty()) {
        replies = {"你好！", "有什么可以帮你？", "😊"};
        return replies;
    }
    
    // 获取最后一条消息
    std::string last_msg = history.back();
    
    // 转换为小写进行匹配
    std::string lower_msg = last_msg;
    std::transform(lower_msg.begin(), lower_msg.end(), 
                   lower_msg.begin(), ::tolower);
    
    // 基于关键词生成回复
    if (lower_msg.find("你好") != std::string::npos ||
        lower_msg.find("hello") != std::string::npos ||
        lower_msg.find("hi") != std::string::npos) {
        replies = {"你好！", "Hi!", "很高兴见到你"};
    }
    else if (lower_msg.find("？") != std::string::npos ||
             lower_msg.find("?") != std::string::npos) {
        replies = {"让我想想...", "好问题", "我需要查一下"};
    }
    else if (lower_msg.find("谢谢") != std::string::npos ||
             lower_msg.find("thank") != std::string::npos) {
        replies = {"不客气！", "很高兴能帮到你", "随时欢迎"};
    }
    else if (lower_msg.find("再见") != std::string::npos ||
             lower_msg.find("bye") != std::string::npos) {
        replies = {"再见！", "保重", "下次见"};
    }
    else {
        // 默认回复
        replies = {"好的", "明白了", "收到"};
    }
    
    return replies;
}

std::string EchoAIService::analyzeSentiment(const std::string& text)
{
    // 简单的情感分析
    int positive_score = 0;
    int negative_score = 0;
    
    // 正面词汇
    std::vector<std::string> positive_words = {
        "好", "棒", "赞", "喜欢", "开心", "高兴", "满意",
        "good", "great", "nice", "happy", "love", "excellent"
    };
    
    // 负面词汇
    std::vector<std::string> negative_words = {
        "差", "烂", "讨厌", "难过", "生气", "失望", "糟糕",
        "bad", "terrible", "hate", "sad", "angry", "awful"
    };
    
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), 
                   lower_text.begin(), ::tolower);
    
    for (const auto& word : positive_words) {
        if (lower_text.find(word) != std::string::npos) {
            positive_score++;
        }
    }
    
    for (const auto& word : negative_words) {
        if (lower_text.find(word) != std::string::npos) {
            negative_score++;
        }
    }
    
    if (positive_score > negative_score) {
        return "positive";
    } else if (negative_score > positive_score) {
        return "negative";
    } else {
        return "neutral";
    }
}

} // namespace KylinMessenger
