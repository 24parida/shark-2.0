// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#pragma once
#include "PreflopCombo.hh"
#include "RiverCombo.hh"
#include <oneapi/tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <vector>

using RiverMap = tbb::concurrent_unordered_map<int, std::vector<RiverCombo>>;

class RiverRangeManager {
  RiverMap m_p1_river_ranges;
  RiverMap m_p2_river_ranges;

public:
  RiverRangeManager() = default;
  auto get_river_combos(const int player,
                        const std::vector<PreflopCombo> &preflop_combos,
                        const std::vector<Card> &board)
      -> std::vector<RiverCombo>;
};
