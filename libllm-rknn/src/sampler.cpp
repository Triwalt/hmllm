/**
 * @file sampler.cpp
 * @brief Token sampling implementation
 */

#include "llm_rknn_internal.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <numeric>

namespace llm_rknn {

Sampler::Sampler(const llm_rknn_config_t& config)
    : method_(config.sampling_method)
    , temperature_(config.temperature)
    , top_k_(config.top_k)
    , top_p_(config.top_p)
    , repetition_penalty_(config.repetition_penalty)
    , last_logprob_(0.0f) {
}

int32_t Sampler::sample(const float* logits, int32_t vocab_size) {
    switch (method_) {
        case LLM_RKNN_SAMPLING_GREEDY:
            return sample_greedy(logits, vocab_size);
        case LLM_RKNN_SAMPLING_TOP_K:
            return sample_top_k(logits, vocab_size);
        case LLM_RKNN_SAMPLING_TOP_P:
            return sample_top_p(logits, vocab_size);
        case LLM_RKNN_SAMPLING_TOP_K_TOP_P:
            return sample_top_k_top_p(logits, vocab_size);
        default:
            return sample_greedy(logits, vocab_size);
    }
}

int32_t Sampler::sample_greedy(const float* logits, int32_t vocab_size) {
    // Find token with maximum logit
    int32_t max_idx = 0;
    float max_logit = logits[0];
    
    for (int32_t i = 1; i < vocab_size; i++) {
        if (logits[i] > max_logit) {
            max_logit = logits[i];
            max_idx = i;
        }
    }
    
    last_logprob_ = max_logit;
    return max_idx;
}

int32_t Sampler::sample_top_k(const float* logits, int32_t vocab_size) {
    // Create a copy of logits with indices
    std::vector<std::pair<float, int32_t>> logit_pairs;
    logit_pairs.reserve(vocab_size);
    
    for (int32_t i = 0; i < vocab_size; i++) {
        logit_pairs.push_back({logits[i], i});
    }
    
    // Partially sort to get top-k
    uint32_t k = std::min(top_k_, (uint32_t)vocab_size);
    std::partial_sort(logit_pairs.begin(), 
                     logit_pairs.begin() + k, 
                     logit_pairs.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Extract top-k logits
    std::vector<float> top_logits(k);
    std::vector<int32_t> top_indices(k);
    
    for (uint32_t i = 0; i < k; i++) {
        top_logits[i] = logit_pairs[i].first;
        top_indices[i] = logit_pairs[i].second;
    }
    
    // Apply temperature and softmax
    apply_temperature(top_logits);
    apply_softmax(top_logits);
    
    // Sample from distribution
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(top_logits.begin(), top_logits.end());
    
    int32_t sampled_idx = dist(gen);
    last_logprob_ = std::log(top_logits[sampled_idx]);
    
    return top_indices[sampled_idx];
}

int32_t Sampler::sample_top_p(const float* logits, int32_t vocab_size) {
    // Create a copy of logits with indices
    std::vector<std::pair<float, int32_t>> logit_pairs;
    logit_pairs.reserve(vocab_size);
    
    for (int32_t i = 0; i < vocab_size; i++) {
        logit_pairs.push_back({logits[i], i});
    }
    
    // Sort by logit value (descending)
    std::sort(logit_pairs.begin(), logit_pairs.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Apply temperature
    std::vector<float> sorted_logits;
    sorted_logits.reserve(vocab_size);
    for (const auto& pair : logit_pairs) {
        sorted_logits.push_back(pair.first);
    }
    apply_temperature(sorted_logits);
    apply_softmax(sorted_logits);
    
    // Find nucleus (top-p cutoff)
    float cumulative_prob = 0.0f;
    size_t nucleus_size = 0;
    
    for (size_t i = 0; i < sorted_logits.size(); i++) {
        cumulative_prob += sorted_logits[i];
        nucleus_size++;
        if (cumulative_prob >= top_p_) {
            break;
        }
    }
    
    // Sample from nucleus
    std::vector<float> nucleus_probs(sorted_logits.begin(), 
                                     sorted_logits.begin() + nucleus_size);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(nucleus_probs.begin(), nucleus_probs.end());
    
    int32_t sampled_idx = dist(gen);
    last_logprob_ = std::log(nucleus_probs[sampled_idx]);
    
    return logit_pairs[sampled_idx].second;
}

int32_t Sampler::sample_top_k_top_p(const float* logits, int32_t vocab_size) {
    // First apply top-k filtering
    std::vector<std::pair<float, int32_t>> logit_pairs;
    logit_pairs.reserve(vocab_size);
    
    for (int32_t i = 0; i < vocab_size; i++) {
        logit_pairs.push_back({logits[i], i});
    }
    
    // Partially sort to get top-k
    uint32_t k = std::min(top_k_, (uint32_t)vocab_size);
    std::partial_sort(logit_pairs.begin(), 
                     logit_pairs.begin() + k, 
                     logit_pairs.end(),
                     [](const auto& a, const auto& b) { return a.first > b.first; });
    
    logit_pairs.resize(k);
    
    // Now apply top-p on the top-k candidates
    std::vector<float> top_logits;
    top_logits.reserve(k);
    for (const auto& pair : logit_pairs) {
        top_logits.push_back(pair.first);
    }
    
    apply_temperature(top_logits);
    apply_softmax(top_logits);
    
    // Find nucleus
    float cumulative_prob = 0.0f;
    size_t nucleus_size = 0;
    
    for (size_t i = 0; i < top_logits.size(); i++) {
        cumulative_prob += top_logits[i];
        nucleus_size++;
        if (cumulative_prob >= top_p_) {
            break;
        }
    }
    
    // Sample from nucleus
    std::vector<float> nucleus_probs(top_logits.begin(), 
                                     top_logits.begin() + nucleus_size);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(nucleus_probs.begin(), nucleus_probs.end());
    
    int32_t sampled_idx = dist(gen);
    last_logprob_ = std::log(nucleus_probs[sampled_idx]);
    
    return logit_pairs[sampled_idx].second;
}

void Sampler::apply_temperature(std::vector<float>& logits) {
    if (temperature_ <= 0.0f || temperature_ == 1.0f) {
        return;
    }
    
    for (float& logit : logits) {
        logit /= temperature_;
    }
}

void Sampler::apply_softmax(std::vector<float>& logits) {
    // Find max for numerical stability
    float max_logit = *std::max_element(logits.begin(), logits.end());
    
    // Compute exp and sum
    float sum = 0.0f;
    for (float& logit : logits) {
        logit = std::exp(logit - max_logit);
        sum += logit;
    }
    
    // Normalize
    for (float& logit : logits) {
        logit /= sum;
    }
}

} // namespace llm_rknn
