#include "DCFR.hh"
#include <cmath>

auto DCFR::get_average_strat() const -> std::vector<double> {
  std::vector<double> average_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    double total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += m_cummulative_strategy[hand + action * m_num_hands];
    }

    if (total > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        average_strategy[hand + action * m_num_hands] =
            m_cummulative_strategy[hand + action * m_num_hands] / total;
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        average_strategy[hand + action * m_num_hands] = 1.0 / m_num_actions;
      }
    }
  }

  return average_strategy;
}

auto DCFR::get_current_strat() const -> std::vector<double> {
  std::vector<double> current_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    double positive_regret_sum{0.0};

    for (std::size_t action{0}; action < m_num_actions; ++action) {
      const double regret{m_cummulative_regret[hand + action * m_num_hands]};
      positive_regret_sum += regret > 0 ? regret : 0;
    }

    if (positive_regret_sum > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] =
            m_cummulative_regret[hand + action * m_num_hands] /
            positive_regret_sum;
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] = 1.0 / m_num_actions;
      }
    }
  }

  return current_strategy;
}

void DCFR::update_cum_regret_one(const std::vector<double> &action_utils,
                                 const int action_index) {
  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    m_cummulative_regret[hand + action_index * m_num_hands] +=
        action_utils[hand];
  }
}
void DCFR::update_cum_regret_two(const std::vector<double> &utils,
                                 const int iteration) {
  double x{pow(iteration, 1.5)};
  x = x / (x + 1);

  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      m_cummulative_regret[hand + action * m_num_hands] -= utils[hand];
      if (m_cummulative_regret[hand + action * m_num_hands] > 0) {
        m_cummulative_regret[hand + action * m_num_hands] *= x;
      } else if (m_cummulative_regret[hand + action * m_num_hands] < 0) {
        m_cummulative_regret[hand + action * m_num_hands] *= 0.5;
      }
    }
  }
}

void DCFR::update_cum_strategy(const std::vector<double> &strategy,
                               const std::vector<double> &reach_probs,
                               const int iteration) {
  double x{pow(static_cast<double>(iteration) / (iteration + 1), 2)};
  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      m_cummulative_strategy[hand + action * m_num_hands] +=
          strategy[hand + action * m_num_hands] + reach_probs[hand];
      m_cummulative_strategy[hand + action * m_num_hands] *= x;
    }
  }
}
