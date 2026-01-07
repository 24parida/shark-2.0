#include "DCFR.hh"
#include "../tree/Nodes.hh"
#include <cmath>
#include <iostream>
#include <algorithm>

DCFR::DCFR(const ActionNode *node)
    : m_num_hands(node->get_num_hands()),
      m_num_actions(node->get_num_actions()), m_current(node->get_player()),
      m_cummulative_regret(m_num_hands * m_num_actions),
      m_cummulative_strategy(m_num_hands * m_num_actions) {
  m_cummulative_regret.reserve(m_num_hands * m_num_actions);
  m_cummulative_strategy.reserve(m_num_hands * m_num_actions);
}

auto DCFR::get_average_strat() const -> std::vector<float> {
  std::vector<float> average_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += decompress(m_cummulative_strategy[hand + action * m_num_hands]);
    }

    if (total > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        average_strategy[hand + action * m_num_hands] =
            decompress(m_cummulative_strategy[hand + action * m_num_hands]) / total;
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
      const float regret{decompress(m_cummulative_regret[hand + action * m_num_hands])};
      positive_regret_sum += regret > 0 ? regret : 0;
    }

    if (positive_regret_sum > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        float regret = decompress(m_cummulative_regret[hand + action * m_num_hands]);
        if (regret > 0) {
          current_strategy[hand + action * m_num_hands] = regret / positive_regret_sum;
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

void DCFR::get_average_strat(std::vector<float> &average_strategy) const {
  // Resize only if needed
  if (average_strategy.size() != m_num_hands * m_num_actions) {
    average_strategy.resize(m_num_hands * m_num_actions);
  }

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += decompress(m_cummulative_strategy[hand + action * m_num_hands]);
    }

    if (total > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        average_strategy[hand + action * m_num_hands] =
            decompress(m_cummulative_strategy[hand + action * m_num_hands]) / total;
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        average_strategy[hand + action * m_num_hands] = 1.0 / m_num_actions;
      }
    }
  }
}

void DCFR::get_current_strat(std::vector<float> &current_strategy) const {
  // Resize only if needed
  if (current_strategy.size() != m_num_hands * m_num_actions) {
    current_strategy.resize(m_num_hands * m_num_actions);
  }

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    for (std::size_t action{0}; action < m_num_actions; ++action) {
      const float regret{decompress(m_cummulative_regret[hand + action * m_num_hands])};
      positive_regret_sum += regret > 0 ? regret : 0;
    }

    if (positive_regret_sum > 0) {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        float regret = decompress(m_cummulative_regret[hand + action * m_num_hands]);
        if (regret > 0) {
          current_strategy[hand + action * m_num_hands] = regret / positive_regret_sum;
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
}

void DCFR::update_cum_regret_one(const std::vector<float> &action_utils,
                                 const int action_index) {
  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    const std::size_t idx = hand + action_index * m_num_hands;
    float current = decompress(m_cummulative_regret[idx]);
    m_cummulative_regret[idx] = compress(current + action_utils[hand]);
  }
}
void DCFR::update_cum_regret_two(const std::vector<float> &utils,
                                 const int iteration) {
  float x{static_cast<float>(pow(iteration, 3.0f))};
  x = x / (x + 1);
  update_cum_regret_two(utils, x);
}

void DCFR::update_cum_regret_two(const std::vector<float> &utils,
                                 const float discount_factor) {
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      float regret = decompress(m_cummulative_regret[idx]) - utils[hand];
      if (regret > 0) {
        regret *= discount_factor;
      } else if (regret < 0) {
        regret *= 0.5f;
      }
      m_cummulative_regret[idx] = compress(regret);
    }
  }
}

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const int iteration) {
  float x{static_cast<float>(
      pow(static_cast<float>(iteration) / (iteration + 1), 2))};
  update_cum_strategy(strategy, reach_probs, x);
}

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const float discount_factor) {
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      float cum_strat = decompress(m_cummulative_strategy[idx]);
      cum_strat = std::fma(strategy[idx], reach_probs[hand], cum_strat);
      cum_strat *= discount_factor;
      m_cummulative_strategy[idx] = compress(cum_strat);
    }
  }
}

void DCFR::reset_cumulative_strategy() {
  std::fill(m_cummulative_strategy.begin(), m_cummulative_strategy.end(), 0);
}
