/**
 * @file ai_service.h
 * @brief Abstract AI Service Interface for Kylin Messenger
 * 
 * This file defines the abstract interface for AI services in Kylin Messenger.
 * All AI features (LLM chat, image tagging, smart replies, etc.) must implement
 * this interface to ensure modularity and easy integration with the NPU.
 * 
 * @version 1.0.0
 * @date 2025-10-09
 * @copyright Apache License 2.0
 */

#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>
#include <QImage>
#include <QVariant>

namespace KylinMessenger {

enum class AICapability : std::uint32_t {
    None            = 0x00,
    TextProcessing  = 0x01,
    ImageProcessing = 0x02,
    Conversation    = 0x04,
    RealTime        = 0x08,
    ObjectDetection = 0x10,
    TextGeneration  = 0x20,
    ContentAnalysis = 0x40,
    SmartReply      = 0x80
};

inline AICapability operator|(AICapability lhs, AICapability rhs)
{
    return static_cast<AICapability>(static_cast<std::uint32_t>(lhs) |
                                     static_cast<std::uint32_t>(rhs));
}

inline AICapability& operator|=(AICapability& lhs, AICapability rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline bool operator&(AICapability lhs, AICapability rhs)
{
    return (static_cast<std::uint32_t>(lhs) &
            static_cast<std::uint32_t>(rhs)) != 0;
}

enum class ContentAnalysisType {
    Sentiment,
    SafetyFilter,
    LanguageDetection
};

/**
 * @brief AI processing result structure
 */
struct AIResult {
    bool success;
    float confidence;
    std::string error_message;
    std::string text_output;
    std::vector<std::string> suggestions;
    QVariantMap metadata;
    
    AIResult() : success(false), confidence(0.0f) {}
};

using AICallback = std::function<void(const AIResult& result)>;

using AIStreamCallback = std::function<bool(const std::string& partial_output,
                                            bool is_final)>;

/**
 * @brief Abstract base class for AI services
 * 
 * This interface provides a unified way to interact with different AI models
 * running on the NPU. Implementations handle the specific RKNN model loading
 * and inference details.
 */
class IAIService {
public:
    virtual ~IAIService() = default;
    
    /**
     * @brief Initialize the AI service
     * 
     * @param model_path Path to the RKNN model file
     * @param config_path Optional path to configuration file
     * @return true if initialization succeeded
     */
    virtual bool initialize(const std::string& model_path,
                            const std::string& config_path = "") = 0;
    
    /**
     * @brief Shutdown and cleanup the AI service
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if the service is ready for inference
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Get the service name
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get the service description
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Get supported capabilities
     */
    virtual AICapability getCapabilities() const = 0;
    
    /**
     * @brief Process text input
     * 
     * @param input Input text to process
     * @param context Optional conversation context
     * @return AIResult containing the processing result
     */
    virtual AIResult processText(const std::string& input,
                                 const std::string& context = "") = 0;
    
    /**
     * @brief Process text with streaming output
     * 
     * @param input Input text to process
     * @param callback Callback function for receiving streaming output
     * @param context Optional conversation context
     * @return AIResult with final status
     */
    virtual AIResult processTextStream(const std::string& input,
                                       AIStreamCallback callback,
                                       const std::string& context = "") = 0;
    
    /**
     * @brief Process image input
     * 
     * @param image Input image to process
     * @param task Task description (e.g., "detect", "tag", "caption")
     * @return AIResult containing the processing result
     */
    virtual AIResult processImage(const QImage& image,
                                  const std::string& task = "detect") = 0;

    virtual void processTextAsync(const std::string& input,
                                  AICallback callback,
                                  const std::string& context = "") = 0;

    virtual void processImageAsync(const QImage& image,
                                   AICallback callback,
                                   const std::string& task = "detect") = 0;
    
    /**
     * @brief Generate smart reply suggestions
     * 
     * @param message_history Recent message history
     * @param num_suggestions Number of suggestions to generate
     * @return AIResult with suggestions in the suggestions field
     */
    virtual AIResult generateSmartReplies(const std::vector<std::string>& message_history,
                                         int num_suggestions = 3) = 0;
    
    /**
     * @brief Analyze content for filtering
     * 
     * @param content Content to analyze
     * @return AIResult with metadata containing filter decision
     */
    virtual AIResult analyzeContent(const std::string& content,
                                    ContentAnalysisType type = ContentAnalysisType::Sentiment) = 0;
    
    /**
     * @brief Reset conversation context
     */
    virtual void resetContext() = 0;
    virtual bool cancelOperation() = 0;
};

} // namespace KylinMessenger

#endif // AI_SERVICE_H
