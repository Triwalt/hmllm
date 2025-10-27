/**
 * @file tokenizer_sentencepiece.cpp
 * @brief SentencePiece tokenizer implementation
 */

#include "tokenizer_sentencepiece.h"
#include <cstdio>

namespace llm_rknn {

SentencePieceTokenizer::SentencePieceTokenizer()
    : bos_id_(-1)
    , eos_id_(-1)
    , pad_id_(-1) {
}

bool SentencePieceTokenizer::load(const std::string& model_path) {
    const auto status = processor_.Load(model_path);
    
    if (!status.ok()) {
        fprintf(stderr, "[Tokenizer] Failed to load model: %s\n", status.ToString().c_str());
        return false;
    }
    
    // Get special token IDs
    bos_id_ = processor_.bos_id();
    eos_id_ = processor_.eos_id();
    pad_id_ = processor_.pad_id();
    
    // Some models don't define PAD, use EOS as fallback
    if (pad_id_ < 0) {
        pad_id_ = eos_id_;
    }
    
    printf("[Tokenizer] SentencePiece model loaded successfully\n");
    printf("[Tokenizer] Vocabulary size: %d\n", processor_.GetPieceSize());
    
    return true;
}

std::vector<int32_t> SentencePieceTokenizer::encode(const std::string& text, bool add_bos) {
    std::vector<int> ids;
    processor_.Encode(text, &ids);
    
    std::vector<int32_t> result;
    
    if (add_bos && bos_id_ >= 0) {
        result.push_back(bos_id_);
    }
    
    for (int id : ids) {
        result.push_back(static_cast<int32_t>(id));
    }
    
    return result;
}

std::string SentencePieceTokenizer::decode(const std::vector<int32_t>& tokens) {
    std::vector<int> ids;
    for (int32_t token : tokens) {
        // Skip special tokens
        if (token == bos_id_ || token == eos_id_ || token == pad_id_) {
            continue;
        }
        ids.push_back(static_cast<int>(token));
    }
    
    std::string result;
    processor_.Decode(ids, &result);
    
    return result;
}

std::string SentencePieceTokenizer::decode_token(int32_t token_id) {
    // Skip special tokens
    if (token_id == bos_id_ || token_id == eos_id_ || token_id == pad_id_) {
        return "";
    }
    
    std::string result;
    processor_.Decode(std::vector<int>{static_cast<int>(token_id)}, &result);
    
    return result;
}

int32_t SentencePieceTokenizer::vocab_size() const {
    return processor_.GetPieceSize();
}

int32_t SentencePieceTokenizer::bos_token_id() const {
    return bos_id_;
}

int32_t SentencePieceTokenizer::eos_token_id() const {
    return eos_id_;
}

int32_t SentencePieceTokenizer::pad_token_id() const {
    return pad_id_;
}

} // namespace llm_rknn
