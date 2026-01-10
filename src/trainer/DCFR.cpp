#include "DCFR.hh"
#include "../tree/Nodes.hh"
#include <cmath>
#include <iostream>
#include <algorithm>

static int g_node_counter = 0;

DCFR::DCFR(const ActionNode *node)
    : m_num_hands(node->get_num_hands()),
      m_num_actions(node->get_num_actions()), m_current(node->get_player()),
      m_cummulative_regret(m_num_hands * m_num_actions),
      m_cummulative_strategy(m_num_hands * m_num_actions) {
  m_node_id = g_node_counter++;
}

auto DCFR::get_average_strat() const -> std::vector<float> {
  std::vector<float> average_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += m_cummulative_strategy[hand + action * m_num_hands];  // Direct float access
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

  // Thread-local cache to avoid double decompress
  thread_local std::vector<float> cached_regrets;
  cached_regrets.resize(m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    // Single pass: decompress and cache, compute sum
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      cached_regrets[action] = decompress(m_cummulative_regret[hand + action * m_num_hands]);
      if (cached_regrets[action] > 0) {
        positive_regret_sum += cached_regrets[action];
      }
    }

    if (positive_regret_sum > 0) {
      // Use cached values
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] =
            cached_regrets[action] > 0 ? cached_regrets[action] / positive_regret_sum : 0;
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] = 1.0f / m_num_actions;
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
      total += m_cummulative_strategy[hand + action * m_num_hands];  // Direct float access
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
}

void DCFR::get_current_strat(std::vector<float> &current_strategy) const {
  // Resize only if needed
  if (current_strategy.size() != m_num_hands * m_num_actions) {
    current_strategy.resize(m_num_hands * m_num_actions);
  }

  // Thread-local cache to avoid double decompress
  thread_local std::vector<float> cached_regrets;
  cached_regrets.resize(m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    // Single pass: decompress and cache, compute sum
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      cached_regrets[action] = decompress(m_cummulative_regret[hand + action * m_num_hands]);
      if (cached_regrets[action] > 0) {
        positive_regret_sum += cached_regrets[action];
      }
    }

    if (positive_regret_sum > 0) {
      // Use cached values
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] =
            cached_regrets[action] > 0 ? cached_regrets[action] / positive_regret_sum : 0;
      }
    } else {
      for (std::size_t action{0}; action < m_num_actions; ++action) {
        current_strategy[hand + action * m_num_hands] = 1.0f / m_num_actions;
      }
    }
  }
}

// DCFR with λ=3: Discount old regrets before adding new
// α = t³/(t³+1) for positive regrets
// β = 0.5 for negative regrets
void DCFR::update_regrets(const std::vector<float> &action_utils_flat,
                          const std::vector<float> &value,
                          int iteration) {
  // Debug output for tracked node
  bool is_debug = (m_node_id == debug_node_id && debug_node_id >= 0);
  if (is_debug) {
    int h = debug_hand;
    std::cout << "  [Node " << m_node_id << "] Iter " << iteration
              << " hand=" << h << " EV=" << value[h];
    for (int a = 0; a < m_num_actions; ++a) {
      std::cout << " u" << a << "=" << action_utils_flat[h + a * m_num_hands];
    }
    std::cout << std::endl;
  }

  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;

      float old_cum = decompress(m_cummulative_regret[idx]);

      // DCFR: discount old regret based on sign (NO floor here - floor is in get_current_strat)
      float coef = (old_cum > 0) ? alpha : beta;
      float discounted_old = old_cum * coef;

      // Compute instantaneous regret (no pot normalization - match wasm-postflop)
      float inst_regret = action_utils_flat[idx] - value[hand];

      // Store new cumulative regret (CAN be negative - floor happens in strategy computation)
      float new_cum = discounted_old + inst_regret;

      m_cummulative_regret[idx] = compress(new_cum);

      // Debug: show regret update for tracked hand
      if (is_debug && hand == debug_hand) {
        std::cout << "    a" << action << ": inst_reg=" << inst_regret
                  << " old=" << old_cum << " disc=" << discounted_old
                  << " new=" << new_cum << std::endl;
      }
    }
  }
}

// Legacy functions (kept for compatibility)
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
  // Use precomputed gamma from precompute_discounts()
  // γ = (t/(t+1))² - already computed once per iteration
  (void)iteration;  // Unused - gamma is precomputed

  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      // Use precomputed gamma for discounting
      float discounted_old = m_cummulative_strategy[idx] * gamma;
      float new_contribution = strategy[idx] * reach_probs[hand];
      m_cummulative_strategy[idx] = discounted_old + new_contribution;
    }
  }
}

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const float discount_factor) {
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      // Use float directly (no compression)
      float discounted_old = m_cummulative_strategy[idx] * discount_factor;
      float new_contribution = strategy[idx] * reach_probs[hand];
      m_cummulative_strategy[idx] = discounted_old + new_contribution;
    }
  }
}

void DCFR::reset_cumulative_strategy() {
  std::fill(m_cummulative_strategy.begin(), m_cummulative_strategy.end(), 0.0f);
}
