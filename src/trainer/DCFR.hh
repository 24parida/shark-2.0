#pragma once
#include <vector>
#include <cstdint>

class ActionNode;

class DCFR {
  static constexpr float SCALE_FACTOR = 10000.0f;
  static constexpr float INV_SCALE_FACTOR = 1.0f / 10000.0f;

  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<int16_t> m_cummulative_regret;
  std::vector<int16_t> m_cummulative_strategy;

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
