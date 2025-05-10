#include "PreflopCombo.hh"
#include "RiverCombo.hh"
#include <unordered_map>

class RiverRangeManager {
  std::unordered_map<int, std::vector<RiverCombo>> m_p1_river_ranges;
  std::unordered_map<int, std::vector<RiverCombo>> m_p2_river_ranges;

public:
  RiverRangeManager() = default;
  auto get_river_combos(const int player,
                        const std::vector<PreflopCombo> &preflop_combos,
                        const std::vector<Card> &board)
      -> std::vector<RiverCombo>;
};
