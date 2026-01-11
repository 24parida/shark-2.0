// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
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

  std::vector<int16_t> m_cummulative_strategy_i16;
  std::vector<float> m_cummulative_strategy_f32;

  float m_regret_scale = 1.0f;
  float m_strategy_scale = 1.0f;

public:
  static inline float alpha = 0.0f;
  static inline float beta = 0.5f;
  static inline float gamma = 0.0f;

  static void precompute_discounts(int t) {
    int t_alpha = (t > 1) ? (t - 1) : 0;
    float tf_alpha = static_cast<float>(t_alpha);
    float pow_alpha = tf_alpha * std::sqrt(tf_alpha);
    alpha = pow_alpha / (pow_alpha + 1.0f);

    beta = 0.5f;

    float tf = static_cast<float>(t);
    float ratio = tf / (tf + 1.0f);
    gamma = ratio * ratio;
  }

  static inline bool compress_strategy = true;

  static inline int debug_node_id = -1;
  static inline int debug_hand = 0;
  int m_node_id = -1;

  static float encode_signed_slice(int16_t* dst, const float* src, size_t len) {
    float max_abs = 0.0f;
    for (size_t i = 0; i < len; ++i) {
      float abs_val = std::abs(src[i]);
      if (abs_val > max_abs) max_abs = abs_val;
    }

    float scale = (max_abs == 0.0f) ? 1.0f : max_abs;
    float encoder = 32767.0f / scale;

    for (size_t i = 0; i < len; ++i) {
      float scaled = src[i] * encoder;
      int32_t rounded = static_cast<int32_t>(std::round(scaled));
      if (rounded > 32767) rounded = 32767;
      if (rounded < -32768) rounded = -32768;
      dst[i] = static_cast<int16_t>(rounded);
    }

    return scale;
  }

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

  void get_average_strat(std::vector<float> &out) const;
  void get_current_strat(std::vector<float> &out) const;

  void update_regrets(const std::vector<float> &action_utils_flat,
                      const std::vector<float> &value,
                      int iteration);

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
