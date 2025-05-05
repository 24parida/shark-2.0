#pragma once

#include <vector>

class DCFR {
  int m_num_hands;
  int m_num_actions;
  int m_current;
  std::vector<double> m_cummulative_regret;
  std::vector<double> m_cummulative_strategy;

  DCFR(const int player, const int num_hands, const int num_actions)
      : m_current(player), m_num_actions(num_actions), m_num_hands(num_hands),
        m_cummulative_regret(num_actions * num_hands),
        m_cummulative_strategy(num_actions * num_hands) {}
};
