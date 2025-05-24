#pragma once
#include "PreflopCombo.hh"
#include <cassert>
#include <string>
#include <vector>

struct RangeIntersection {
    // For each combo in range A, stores indices of all matching combos in range B
    std::vector<std::vector<int>> hand_to_combos;
    // Weight for each combo accounting for relative frequencies in both ranges
    std::vector<float> combo_weights;
};

class PreflopRangeManager {
  std::vector<PreflopCombo> m_p1_preflop_combos;
  std::vector<PreflopCombo> m_p2_preflop_combos;

public:
  PreflopRangeManager() = default;
  PreflopRangeManager(const std::vector<PreflopCombo> &p1_preflop_combos,
                      const std::vector<PreflopCombo> &p2_preflop_combos,
                      const std::vector<Card> &init_board)

      : m_p1_preflop_combos(p1_preflop_combos),
        m_p2_preflop_combos(p2_preflop_combos) {
    set_rel_probabilities(init_board);
  }

  auto get_num_hands(const int player_id) const -> int {
    assert((player_id == 1 || player_id == 2) &&
           "PreflopRangeManager get_num_hands invalid player_id");

    if (player_id == 1)
      return m_p1_preflop_combos.size();
    return m_p2_preflop_combos.size();
  }

  auto get_preflop_combos(const int player_id) const
      -> const std::vector<PreflopCombo> & {
    assert((player_id == 1 || player_id == 2) &&
           "PreflopRangeManager get_num_hands invalid player_id");

    if (player_id == 1)
      return m_p1_preflop_combos;
    return m_p2_preflop_combos;
  }

  auto get_initial_reach_probs(const int player,
                               const std::vector<Card> &board) const
      -> std::vector<float>;

  void set_rel_probabilities(const std::vector<Card> &init_board);

  // New method to compute range intersection
  RangeIntersection compute_range_intersection(int player_a, int player_b) const {
    const auto& range_a = (player_a == 1) ? m_p1_preflop_combos : m_p2_preflop_combos;
    const auto& range_b = (player_b == 1) ? m_p1_preflop_combos : m_p2_preflop_combos;
    
    RangeIntersection result;
    result.hand_to_combos.resize(range_a.size());
    result.combo_weights.resize(range_a.size());

    // For each combo in range A
    for (size_t i = 0; i < range_a.size(); ++i) {
      const auto& combo_a = range_a[i];
      
      // Find all matching combos in range B
      for (size_t j = 0; j < range_b.size(); ++j) {
        const auto& combo_b = range_b[j];
        if (combo_a == combo_b) {
          result.hand_to_combos[i].push_back(j);
          // Weight based on relative frequencies in both ranges
          result.combo_weights[i] = combo_a.rel_probability * combo_b.rel_probability;
        }
      }
    }
    
    return result;
  }
};
