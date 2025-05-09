#include "PreflopRangeManager.hh"
#include "../Helper.hh"

auto PreflopRangeManager::get_initial_reach_probs(
    const int player, const std::array<phevaluator::Card, 4> &board) const
    -> std::vector<double> {
  const auto preflop_combos{get_preflop_combos(player)};
  std::vector<double> reach_probs(preflop_combos.size());

  for (std::size_t i{0}; i < preflop_combos.size(); ++i) {
    if (CardUtility::overlap(preflop_combos[i], board)) {
      reach_probs[i] = 0;
    } else {
      reach_probs[i] = preflop_combos[i].probability;
    }
  }

  return reach_probs;
}

void PreflopRangeManager::set_rel_probabilities(
    const std::array<phevaluator::Card, 4> init_board) {
  for (int p{1}; p <= 2; ++p) {

    auto &hero_preflop_combos{p == 1 ? m_p1_preflop_combos
                                     : m_p2_preflop_combos};
    auto &villain_preflop_combos{p == 1 ? m_p2_preflop_combos
                                        : m_p1_preflop_combos};

    double rel_sum{0.0};

    for (std::size_t hero_hand{0}; hero_hand < hero_preflop_combos.size();
         ++hero_hand) {
      auto &hero_combo{hero_preflop_combos[hero_hand]};

      if (CardUtility::overlap(hero_combo, init_board)) {
        hero_combo.rel_probability = 0;
        continue;
      }

      double villain_sum{0.0};

      for (std::size_t villain_hand{0};
           villain_hand < villain_preflop_combos.size(); ++villain_hand) {
        if (CardUtility::overlap(villain_preflop_combos[villain_hand],
                                 init_board)) {
          continue;
        }
        if (CardUtility::overlap(hero_combo,
                                 villain_preflop_combos[villain_hand])) {
          continue;
        }

        villain_sum += villain_preflop_combos[villain_hand].probability;
      }

      hero_combo.rel_probability = villain_sum * hero_combo.probability;
      rel_sum += hero_combo.rel_probability;
    }

    for (std::size_t hero_hand{0}; hero_hand < hero_preflop_combos.size();
         ++hero_hand) {
      hero_preflop_combos[hero_hand].rel_probability /= rel_sum;
    }
  }
}
