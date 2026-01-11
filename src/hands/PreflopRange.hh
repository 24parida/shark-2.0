#pragma once
#include "PreflopCombo.hh"

struct PreflopRange {
  std::vector<PreflopCombo> preflop_combos;
  int num_hands;

  PreflopRange() = delete;
  PreflopRange(std::string);
  void print() const;

private:
  void add_combo(const char rank1, const int suit1, const char rank2, const int suit2, const float weight = 1.0f);
  void add_pair(char rank, float weight = 1.0f);
  void add_suited(char rank1, char rank2, float weight = 1.0f);
  void add_offsuit(char rank1, char rank2, float weight = 1.0f);
  void add_all_unpaired(char rank1, char rank2, float weight = 1.0f);
  void parse_token(const std::string &token, float weight = 1.0f);
  void parse_range(const std::string &start, const std::string &end, float weight = 1.0f);
};
