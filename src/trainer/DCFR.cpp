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
      m_cummulative_strategy(m_num_hands * m_num_actions),
      m_regret_scale(1.0f) {
  m_node_id = g_node_counter++;
}

auto DCFR::get_average_strat() const -> std::vector<float> {
  std::vector<float> average_strategy(m_num_hands * m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += m_cummulative_strategy[hand + action * m_num_hands];
    }

    if (total > 0) {
      // VECTORIZED: GCC emits divps (packed divide) processing 4 floats/iter
      // Assembly: divps %xmm1, %xmm0; movups %xmm0, (%rax)
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

  // Decode regrets without discount (for strategy computation, use scale 1.0)
  // We just need relative magnitudes for regret matching
  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    // First pass: decode and compute sum of positive regrets
    thread_local std::vector<float> cached_regrets;
    cached_regrets.resize(m_num_actions);

    for (std::size_t action{0}; action < m_num_actions; ++action) {
      // Decode without discount: compressed * scale / 32767
      int16_t compressed = m_cummulative_regret[hand + action * m_num_hands];
      float decoded = static_cast<float>(compressed) * m_regret_scale / 32767.0f;
      cached_regrets[action] = decoded;
      if (decoded > 0) {
        positive_regret_sum += decoded;
      }
    }

    if (positive_regret_sum > 0) {
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
  if (average_strategy.size() != m_num_hands * m_num_actions) {
    average_strategy.resize(m_num_hands * m_num_actions);
  }

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float total{0.0};
    for (std::size_t action{0}; action < m_num_actions; ++action) {
      total += m_cummulative_strategy[hand + action * m_num_hands];
    }

    if (total > 0) {
      // VECTORIZED: GCC emits divps (packed divide) processing 4 floats/iter
      // Assembly: divps %xmm1, %xmm0; movups %xmm0, (%rax)
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
  if (current_strategy.size() != m_num_hands * m_num_actions) {
    current_strategy.resize(m_num_hands * m_num_actions);
  }

  thread_local std::vector<float> cached_regrets;
  cached_regrets.resize(m_num_actions);

  for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
    float positive_regret_sum{0.0};

    for (std::size_t action{0}; action < m_num_actions; ++action) {
      int16_t compressed = m_cummulative_regret[hand + action * m_num_hands];
      float decoded = static_cast<float>(compressed) * m_regret_scale / 32767.0f;
      cached_regrets[action] = decoded;
      if (decoded > 0) {
        positive_regret_sum += decoded;
      }
    }

    if (positive_regret_sum > 0) {
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

// wasm-postflop style DCFR regret update
// 1. Decode old regrets with discount baked in
// 2. Add instantaneous regrets (action_utils - value)
// 3. Re-encode with new scale
void DCFR::update_regrets(const std::vector<float> &action_utils_flat,
                          const std::vector<float> &value,
                          int iteration) {
  const size_t total_size = m_num_hands * m_num_actions;

  // Thread-local buffer for float regrets
  thread_local std::vector<float> new_regrets;
  new_regrets.resize(total_size);

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

  // wasm-postflop style: decode with discount, add instantaneous regret
  // alpha_decoder = alpha * scale / 32767
  // beta_decoder = beta * scale / 32767
  float alpha_decoder = alpha * m_regret_scale / 32767.0f;
  float beta_decoder = beta * m_regret_scale / 32767.0f;

  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;

      // Decode with discount baked in (wasm-postflop style)
      int16_t compressed = m_cummulative_regret[idx];
      float discount = (compressed >= 0) ? alpha_decoder : beta_decoder;
      float discounted_old = static_cast<float>(compressed) * discount;

      // Add instantaneous regret
      float inst_regret = action_utils_flat[idx] - value[hand];
      new_regrets[idx] = discounted_old + inst_regret;

      // Debug: show regret update for tracked hand
      if (is_debug && hand == debug_hand) {
        float old_decoded = static_cast<float>(compressed) * m_regret_scale / 32767.0f;
        std::cout << "    a" << action << ": inst_reg=" << inst_regret
                  << " old=" << old_decoded << " disc=" << discounted_old
                  << " new=" << new_regrets[idx] << std::endl;
      }
    }
  }

  // Re-encode all regrets and get new scale
  m_regret_scale = encode_signed_slice(m_cummulative_regret.data(), new_regrets.data(), total_size);
}

// Legacy functions (kept for compatibility)
void DCFR::update_cum_regret_one(const std::vector<float> &action_utils,
                                 const int action_index) {
  // Not used in new implementation
  (void)action_utils;
  (void)action_index;
}

void DCFR::update_cum_regret_two(const std::vector<float> &utils,
                                 const int iteration) {
  (void)utils;
  (void)iteration;
}

void DCFR::update_cum_regret_two(const std::vector<float> &utils,
                                 const float discount_factor) {
  (void)utils;
  (void)discount_factor;
}

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const int iteration) {
  // Use precomputed gamma from precompute_discounts()
  (void)iteration;

  // VECTORIZED: GCC emits mulps+addps (fused multiply-add pattern) for 4 floats/iter
  // Assembly: mulps %xmm2, %xmm0; mulps %xmm1, %xmm3; addps %xmm3, %xmm0; movups %xmm0, (%rax)
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      float discounted_old = m_cummulative_strategy[idx] * gamma;
      float new_contribution = strategy[idx] * reach_probs[hand];
      m_cummulative_strategy[idx] = discounted_old + new_contribution;
    }
  }
}

void DCFR::update_cum_strategy(const std::vector<float> &strategy,
                               const std::vector<float> &reach_probs,
                               const float discount_factor) {
  // VECTORIZED: GCC emits mulps+addps (fused multiply-add pattern) for 4 floats/iter
  // Assembly: mulps %xmm2, %xmm0; mulps %xmm1, %xmm3; addps %xmm3, %xmm0; movups %xmm0, (%rax)
  for (std::size_t action{0}; action < m_num_actions; ++action) {
    for (std::size_t hand{0}; hand < m_num_hands; ++hand) {
      const std::size_t idx = hand + action * m_num_hands;
      float discounted_old = m_cummulative_strategy[idx] * discount_factor;
      float new_contribution = strategy[idx] * reach_probs[hand];
      m_cummulative_strategy[idx] = discounted_old + new_contribution;
    }
  }
}

void DCFR::reset_cumulative_strategy() {
  std::fill(m_cummulative_strategy.begin(), m_cummulative_strategy.end(), 0.0f);
}
