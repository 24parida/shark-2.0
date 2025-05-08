#pragma once

#include <vector>

class Node;

class DCFR {
  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<double> m_cummulative_regret;
  std::vector<double> m_cummulative_strategy;
  Node *node_ref;

public:
  DCFR() = default;
  DCFR(const int player, const int num_hands, const int num_actions)
      : m_num_hands(num_hands), m_num_actions(num_actions), m_current(player),
        m_cummulative_regret(num_actions * num_hands),
        m_cummulative_strategy(num_actions * num_hands) {}
  auto get_average_strat() -> std::vector<double>;
  auto get_current_strat() -> std::vector<double>;
};
