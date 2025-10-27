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
#include <QImage>
#include <QVariant>

namespace KylinMessenger {

/**
 * @brief AI service capability flags
 */
enum class AICapability {
    TEXT_PROCESSING = 0x01,      ///< Can process text input
    IMAGE_PROCESSING = 0x02,     ///< Can process image input
    CONVERSATION = 0x04,         ///< Can maintain conversation context
    REAL_TIME = 0x08,            ///< Can provide real-time streaming output
    OBJECT_DETECTION = 0x10,     ///< Can detect objects in images
    TEXT_GENERATION = 0x20,      ///< Can generate text
    CLASSIFICATION = 0x40        ///< Can classify content
};

/**
 * @brief AI processing result structure
 */
struct AIResult {
    bool success;                           ///< Whether the operation succeeded
    std::string error_message;              ///< Error message if failed
    std::string text_output;                ///< Text output from AI
    std::vector<std::string> suggestions;   ///< Multiple suggestions (for smart reply)
    QVariantMap metadata;                   ///< Additional metadata (tags, confidence, etc.)
    
    AIResult() : success(false) {}
};

/**
 * @brief Callback type for streaming AI output
 * 
 * @param partial_output The incremental output from AI
 * @param is_final Whether this is the final output
 */
using AIStreamCallback = std::function<void(const std::string& partial_output, bool is_final)>;

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
    virtual int getCapabilities() const = 0;
    
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
    virtual AIResult processTextStreaming(const std::string& input,
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
    virtual AIResult analyzeContent(const std::string& content) = 0;
    
    /**
     * @brief Reset conversation context
     */
    virtual void resetContext() = 0;
};

/**
 * @brief Factory class for creating AI service instances
 */
class AIServiceFactory {
public:
    /**
     * @brief Create an AI service instance by name
     * 
     * @param service_name Name of the service ("echo", "llm_chat", "image_tagger", etc.)
     * @return Pointer to the created service, or nullptr if not found
     */
    static std::unique_ptr<IAIService> createService(const std::string& service_name);
    
    /**
     * @brief Get list of available AI services
     */
    static std::vector<std::string> getAvailableServices();
};

} // namespace KylinMessenger

#endif // AI_SERVICE_H
