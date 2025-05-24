#pragma once
#include "PreflopCombo.hh"
#include "RiverCombo.hh"
#include <oneapi/tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <vector>

using RiverMap = tbb::concurrent_unordered_map<int, std::vector<RiverCombo>>;

class RiverRangeManager {
  static RiverRangeManager *m_instance;

public:
  static auto getInstance() -> RiverRangeManager *;

  RiverRangeManager() = default;
  virtual ~RiverRangeManager() = default;

  auto get_river_combos(const int player,
                        const std::vector<PreflopCombo> &preflop_combos,
                        const std::vector<phevaluator::Card> &board)
      -> std::vector<RiverCombo>;

  // Calculate equity between two hands on a given board
  float get_equity(const PreflopCombo& hero_hand, 
                  const PreflopCombo& villain_hand,
                  const std::vector<phevaluator::Card>& board) const {
    // For showdown, we just need to compare hand ranks
    int hero_rank = get_hand_rank(hero_hand, board);
    int villain_rank = get_hand_rank(villain_hand, board);
    
    if (hero_rank > villain_rank) return 1.0f;
    if (hero_rank < villain_rank) return 0.0f;
    return 0.5f;  // Split pot on equal ranks
  }

private:
  RiverMap m_p1_river_ranges;
  RiverMap m_p2_river_ranges;

  // Helper to get hand rank
  int get_hand_rank(const PreflopCombo& hand, const std::vector<phevaluator::Card>& board) const;
};
