#include "RiverRangeManager.hh"
#include "../Helper.hh"
#include "hands/RiverCombo.hh"
#include <algorithm>
#include <unordered_map>

auto RiverRangeManager::get_river_combos(
    const int player, const std::vector<PreflopCombo> &preflop_combos,
    const std::vector<Card> &board) -> std::vector<RiverCombo> {

  std::unordered_map<int, std::vector<RiverCombo>> &river_ranges{
      player == 1 ? m_p1_river_ranges : m_p2_river_ranges};
  const int key{CardUtility::board_to_key(board)};

  if (river_ranges.count(key) > 0) {
    return river_ranges[key];
  }

  int count{0};
  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    if (!CardUtility::overlap(preflop_combos[hand], board))
      ++count;
  }

  int index{0};
  std::vector<RiverCombo> river_combos(count);

  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    auto preflop_combo = preflop_combos[hand];
    if (CardUtility::overlap(preflop_combo, board))
      continue;

    RiverCombo river_combo{preflop_combo.hand1, preflop_combo.hand2,
                           preflop_combo.probability, static_cast<int>(hand)};
    river_combo.rank =
        CardUtility::get_rank(river_combo.hand1, river_combo.hand2, board);
    river_combos[index++] = river_combo;
  }

  std::sort(river_combos.begin(), river_combos.end(),
            [](const RiverCombo &a, const RiverCombo &b) {
              return a.get_rank() < b.get_rank();
            });

  river_ranges[key] = river_combos;
  return river_combos;
}
