#pragma once
#include <vector>

class ActionNode;

class DCFR {
  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<double> m_cummulative_regret;
  std::vector<double> m_cummulative_strategy;

public:
  DCFR() = default;
  explicit DCFR(const ActionNode *);
  auto get_average_strat() const -> std::vector<double>;
  auto get_current_strat() const -> std::vector<double>;
  void update_cum_regret_one(const std::vector<double> &action_utils,
                             const int action_index);
  void update_cum_regret_two(const std::vector<double> &utils,
                             const int iteration);
  void update_cum_strategy(const std::vector<double> &strategy,
                           const std::vector<double> &reach_probs,
                           const int iteration);
};
