#pragma once
#include "PreflopCombo.hh"

struct PreflopRange {
  std::vector<PreflopCombo> preflop_combos;
  int num_hands;

  PreflopRange() = delete;
  PreflopRange(std::string);
  void add_combo(const char, const int, const char, const int, const float);
  void print() const;
};
