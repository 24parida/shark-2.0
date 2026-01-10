#pragma once
#include <vector>
#include <cstdint>
#include <cmath>

class ActionNode;

class DCFR {
  // SCALE_FACTOR for int16_t compression
  // Match wasm-postflop: direct scaling without pot normalization
  // With 1000: max value = 32.767, precision = 0.001
  static constexpr float SCALE_FACTOR = 1000.0f;
  static constexpr float INV_SCALE_FACTOR = 0.001f;

  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<int16_t> m_cummulative_regret;
  std::vector<float> m_cummulative_strategy;  // Float for small reach prob contributions

public:
  // Pot size for normalizing regrets (set by trainer)
  static inline float pot_normalizer = 1.0f;

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

    // Gamma: uses distance from nearest lower power of 4
    // nearest_pow4 = largest power of 4 <= t
    int nearest_pow4 = 0;
    if (t > 0) {
      // Count leading zeros, mask to even bit position
      int msb = 31 - __builtin_clz(static_cast<unsigned int>(t));
      nearest_pow4 = 1 << (msb & ~1);  // Round down to even power of 2 = power of 4
    }
    int t_gamma = t - nearest_pow4;
    float tf_gamma = static_cast<float>(t_gamma);
    float ratio = tf_gamma / (tf_gamma + 1.0f);
    gamma = ratio * ratio * ratio;  // (ratio)^3
  }

  // Debug: track a specific node
  static inline int debug_node_id = -1;
  static inline int debug_hand = 0;
  int m_node_id = -1;

  static inline auto compress(float value) -> int16_t {
    float scaled = value * SCALE_FACTOR;
    if (scaled > 32767.0f) return 32767;
    if (scaled < -32768.0f) return -32768;
    return static_cast<int16_t>(scaled);
  }

  static inline auto decompress(int16_t value) -> float {
    return static_cast<float>(value) * INV_SCALE_FACTOR;
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

  // New correct DCFR regret update (replaces the two-phase update)
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
