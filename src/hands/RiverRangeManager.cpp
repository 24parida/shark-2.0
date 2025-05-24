#include "RiverRangeManager.hh"
#include "../Helper.hh"
#include "hands/RiverCombo.hh"
#include <algorithm>

auto RiverRangeManager::get_river_combos(
    const int player, const std::vector<PreflopCombo> &preflop_combos,
    const std::vector<Card> &board) -> std::vector<RiverCombo> {

  auto &river_ranges{player == 1 ? m_p1_river_ranges : m_p2_river_ranges};
  const int key{CardUtility::board_to_key(board)};

  if (auto it = river_ranges.find(key); it != river_ranges.end())
    return it->second;

  int count{0};
  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    if (!CardUtility::overlap(preflop_combos[hand], board))
      ++count;
  }

  std::vector<RiverCombo> river_combos;
  river_combos.reserve(count);

  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    const auto &preflop_combo = preflop_combos[hand];
    if (CardUtility::overlap(preflop_combo, board))
      continue;

    RiverCombo river_combo{preflop_combo.hand1, preflop_combo.hand2,
                           preflop_combo.probability, static_cast<int>(hand)};
    river_combo.rank =
        CardUtility::get_rank(river_combo.hand1, river_combo.hand2, board);
    river_combos.push_back(river_combo);
  }

  std::sort(river_combos.begin(), river_combos.end());

  river_ranges.insert({key, river_combos});
  return river_combos;
}
