/**
 * @file tokenizer_sentencepiece.h
 * @brief SentencePiece tokenizer implementation
 */

#ifndef TOKENIZER_SENTENCEPIECE_H
#define TOKENIZER_SENTENCEPIECE_H

#include "llm_rknn_internal.h"
#include <sentencepiece_processor.h>

namespace llm_rknn {

/**
 * @brief SentencePiece tokenizer implementation
 */
class SentencePieceTokenizer : public ITokenizer {
public:
    SentencePieceTokenizer();
    ~SentencePieceTokenizer() override = default;
    
    /**
     * @brief Load tokenizer from file
     */
    bool load(const std::string& model_path);
    
    /**
     * @brief Encode text to token IDs
     */
    std::vector<int32_t> encode(const std::string& text, bool add_bos = true) override;
    
    /**
     * @brief Decode token IDs to text
     */
    std::string decode(const std::vector<int32_t>& tokens) override;
    
    /**
     * @brief Decode a single token
     */
    std::string decode_token(int32_t token_id) override;
    
    /**
     * @brief Get vocabulary size
     */
    int32_t vocab_size() const override;
    
    /**
     * @brief Get special token IDs
     */
    int32_t bos_token_id() const override;
    int32_t eos_token_id() const override;
    int32_t pad_token_id() const override;
    
private:
    sentencepiece::SentencePieceProcessor processor_;
    int32_t bos_id_;
    int32_t eos_id_;
    int32_t pad_id_;
};

} // namespace llm_rknn

#endif // TOKENIZER_SENTENCEPIECE_H
