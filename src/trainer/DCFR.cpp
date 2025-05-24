#include "DCFR.hh"
#include "../tree/Nodes.hh"
#include <cmath>
#include <iostream>

DCFR::DCFR(const ActionNode *node)
    : m_num_hands(node->get_num_hands()),
      m_num_actions(node->get_num_actions()), m_current(node->get_player()),
      m_cummulative_regret(m_num_hands * m_num_actions),
      m_cummulative_strategy(m_num_hands * m_num_actions) {}

auto DCFR::get_average_strat() const -> std::vector<float> {
  std::vector<float> average_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
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

auto DCFR::get_current_strat() const -> std::vector<float> {
  std::vector<float> current_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    for (std::size_t action{0}; action < m_num_actions; ++action) {
      const float regret{m_cummulative_regret[hand + action * m_num_hands]};
      positive_regret_sum += regret > 0 ? regret : 0;
    }

    if (positive_regret_sum > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        if (m_cummulative_regret[hand + action * m_num_hands] > 0) {
          current_strategy[hand + action * m_num_hands] =
              m_cummulative_regret[hand + action * m_num_hands] /
              positive_regret_sum;
        } else {
          current_strategy[hand + action * m_num_hands] = 0;
        }
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] = 1.0 / m_num_actions;
      }
    }
  }

  return current_strategy;
}

void DCFR::update_cum_regret_one(const std::vector<float> &action_utils,
                                 const int action_index) {
  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    m_cummulative_regret[hand + action_index * m_num_hands] +=
        action_utils[hand];
  }
}
void DCFR::update_cum_regret_two(const std::vector<float> &utils,
                                 const int iteration) {
  float x{static_cast<float>(pow(iteration, 1.5f))};
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

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const int iteration) {
  float x{static_cast<float>(
      pow(static_cast<float>(iteration) / (iteration + 1), 2))};
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      m_cummulative_strategy[hand + action * m_num_hands] +=
          strategy[hand + action * m_num_hands] * reach_probs[hand];
    }
  }

  for (auto &cs : m_cummulative_strategy) {
    cs *= x;
  }
}
