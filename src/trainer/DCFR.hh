#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <limits>

class ActionNode;

class DCFR {
  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<int16_t> m_cummulative_regret;

  // Strategy storage: use int16 when compress_strategy=true (flop solves), float otherwise
  std::vector<int16_t> m_cummulative_strategy_i16;
  std::vector<float> m_cummulative_strategy_f32;

  // Per-node scale factors for compression (wasm-postflop style)
  float m_regret_scale = 1.0f;
  float m_strategy_scale = 1.0f;  // Only used when compress_strategy=true

public:
  // DCFR discount factors (precomputed once per iteration)
  // Based on wasm-postflop implementation
  // α = t^1.5/(t^1.5+1) for positive regrets, using t-1
  // β = 0.5 for negative regrets
  // γ = ((t-pow4)/(t-pow4+1))³ where pow4 = nearest lower power of 4
  static inline float alpha = 0.0f;
  static inline float beta = 0.5f;
  static inline float gamma = 0.0f;

  // Precompute discount factors for iteration t (call once per iteration)
  static void precompute_discounts(int t) {
    // Alpha: t^1.5/(t^1.5+1), using t-1 (so iter 1 → t=0 → alpha=0)
    int t_alpha = (t > 1) ? (t - 1) : 0;
    float tf_alpha = static_cast<float>(t_alpha);
    float pow_alpha = tf_alpha * std::sqrt(tf_alpha);  // t^1.5
    alpha = pow_alpha / (pow_alpha + 1.0f);

    // Beta: fixed at 0.5
    beta = 0.5f;

    // Gamma: (t/(t+1))^2 - NO power-of-4 reset (differs from wasm-postflop)
    // This provides smooth discounting without periodic resets
    float tf = static_cast<float>(t);
    float ratio = tf / (tf + 1.0f);
    gamma = ratio * ratio;  // (t/(t+1))^2
  }

  // Compression flag: set to true for flop solves to save memory
  // Should be set before tree construction based on initial_board.size() == 3
  static inline bool compress_strategy = true;

  // Debug: track a specific node
  static inline int debug_node_id = -1;
  static inline int debug_hand = 0;
  int m_node_id = -1;

  // wasm-postflop style encoding: finds max absolute value, returns scale
  // compressed = value * (32767 / scale)
  static float encode_signed_slice(int16_t* dst, const float* src, size_t len) {
    // Find max absolute value
    float max_abs = 0.0f;
    for (size_t i = 0; i < len; ++i) {
      float abs_val = std::abs(src[i]);
      if (abs_val > max_abs) max_abs = abs_val;
    }

    // Handle zero case
    float scale = (max_abs == 0.0f) ? 1.0f : max_abs;
    float encoder = 32767.0f / scale;

    // Encode each value
    for (size_t i = 0; i < len; ++i) {
      float scaled = src[i] * encoder;
      // Round and clamp to i16 range
      int32_t rounded = static_cast<int32_t>(std::round(scaled));
      if (rounded > 32767) rounded = 32767;
      if (rounded < -32768) rounded = -32768;
      dst[i] = static_cast<int16_t>(rounded);
    }

    return scale;
  }

  // wasm-postflop style decoding with discount baked in
  // decoded = compressed * discount * scale / 32767
  static float decode_with_discount(int16_t compressed, float scale, float pos_discount, float neg_discount) {
    float discount = (compressed >= 0) ? pos_discount : neg_discount;
    return static_cast<float>(compressed) * discount * scale / 32767.0f;
  }

public:
  DCFR() = default;
  explicit DCFR(const ActionNode *);
  auto get_current() const -> int { return m_current; }
  auto get_average_strat() const -> std::vector<float>;
  auto get_current_strat() const -> std::vector<float>;

  // In-place versions
  void get_average_strat(std::vector<float> &out) const;
  void get_current_strat(std::vector<float> &out) const;

  // wasm-postflop style DCFR regret update
  void update_regrets(const std::vector<float> &action_utils_flat,
                      const std::vector<float> &value,
                      int iteration);

  // Legacy functions (kept for compatibility but should not be used)
  void update_cum_regret_one(const std::vector<float> &action_utils,
                             const int action_index);
  void update_cum_regret_two(const std::vector<float> &utils,
                             const int iteration);
  void update_cum_regret_two(const std::vector<float> &utils,
                             const float discount_factor);
  void update_cum_strategy(const std::vector<float> &strategy,
                           const std::vector<float> &reach_probs,
                           const int iteration);
  void update_cum_strategy(const std::vector<float> &strategy,
                           const std::vector<float> &reach_probs,
                           const float discount_factor);

  void reset_cumulative_strategy();
};
