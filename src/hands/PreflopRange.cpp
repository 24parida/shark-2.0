#include "PreflopRange.hh"
#include "../game/Game.hh"
#include "card.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> split(const std::string &input, const char delim) {
  std::vector<std::string> tokens;
  std::stringstream ss(input);
  std::string token;
  while (std::getline(ss, token, delim)) {
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

// Rank characters in order: 2, 3, 4, 5, 6, 7, 8, 9, T, J, Q, K, A
static const char RANKS[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'};
static const int NUM_RANKS = 13;

int rankToIndex(char rank) {
  for (int i = 0; i < NUM_RANKS; ++i) {
    if (RANKS[i] == rank) return i;
  }
  return -1;
}

bool isValidRank(const char rank) {
  return rankToIndex(rank) >= 0;
}

void PreflopRange::add_combo(const char rank1, const int suit1,
                             const char rank2, const int suit2,
                             const float weight) {

  assert(suit1 >= 0 && suit1 < 4 && "PreflopRange suit1 out of range 0-3");
  assert(suit2 >= 0 && suit2 < 4 && "PreflopRange suit2 out of range 0-3");
  assert(isValidRank(rank1) && "PreflopRange rank1 invalid");
  assert(isValidRank(rank2) && "PreflopRange rank2 invalid");
  assert(!(suit1 == suit2 && rank1 == rank2) &&
         "PreflopRange attempting to add a suited pair");

  preflop_combos.push_back({
      .hand1{std::string{rank1} + GameParams::suitReverseArray[suit1]},
      .hand2{std::string{rank2} + GameParams::suitReverseArray[suit2]},
      .probability = weight,
  });
}

// Add a pair (e.g., "AA", "KK")
void PreflopRange::add_pair(char rank, float weight) {
  for (int suit1 = 0; suit1 < 4; ++suit1) {
    for (int suit2 = suit1 + 1; suit2 < 4; ++suit2) {
      add_combo(rank, suit1, rank, suit2, weight);
    }
  }
}

// Add suited hand (e.g., "AKs")
void PreflopRange::add_suited(char rank1, char rank2, float weight) {
  for (int suit = 0; suit < 4; ++suit) {
    add_combo(rank1, suit, rank2, suit, weight);
  }
}

// Add offsuit hand (e.g., "AKo")
void PreflopRange::add_offsuit(char rank1, char rank2, float weight) {
  for (int suit1 = 0; suit1 < 4; ++suit1) {
    for (int suit2 = 0; suit2 < 4; ++suit2) {
      if (suit1 != suit2) {
        add_combo(rank1, suit1, rank2, suit2, weight);
      }
    }
  }
}

// Add all combos for unpaired hand (suited + offsuit)
void PreflopRange::add_all_unpaired(char rank1, char rank2, float weight) {
  add_suited(rank1, rank2, weight);
  add_offsuit(rank1, rank2, weight);
}

// Parse a single combo/range token like "AA", "AKs", "22+", "ATs+", "JJ-99", "ATs-A6s"
void PreflopRange::parse_token(const std::string &token, float weight) {
  if (token.empty()) return;

  // Check for range notation with '-' (e.g., "JJ-99", "ATs-A6s")
  size_t dash_pos = token.find('-');
  if (dash_pos != std::string::npos && dash_pos > 0) {
    std::string start = token.substr(0, dash_pos);
    std::string end = token.substr(dash_pos + 1);
    parse_range(start, end, weight);
    return;
  }

  // Check for '+' notation (e.g., "22+", "ATs+")
  bool has_plus = (!token.empty() && token.back() == '+');
  std::string base = has_plus ? token.substr(0, token.length() - 1) : token;

  if (base.length() < 2) return;

  char rank1 = base[0];
  char rank2 = base[1];
  int idx1 = rankToIndex(rank1);
  int idx2 = rankToIndex(rank2);

  if (idx1 < 0 || idx2 < 0) return;

  // Determine type: pair, suited, offsuit, or all
  bool is_pair = (rank1 == rank2);
  char type = 'a'; // 'a' = all, 's' = suited, 'o' = offsuit
  if (base.length() >= 3) {
    type = base[2];
  }

  if (has_plus) {
    if (is_pair) {
      // "22+" means 22, 33, 44, ..., AA
      for (int r = idx1; r < NUM_RANKS; ++r) {
        add_pair(RANKS[r], weight);
      }
    } else {
      // "ATs+" means ATs, AJs, AQs, AKs (gap decreases)
      // Keep rank1 fixed, increase rank2 until it reaches rank1-1
      int high_idx = std::max(idx1, idx2);
      int low_idx = std::min(idx1, idx2);
      char high_rank = RANKS[high_idx];

      for (int r = low_idx; r < high_idx; ++r) {
        if (type == 's') {
          add_suited(high_rank, RANKS[r], weight);
        } else if (type == 'o') {
          add_offsuit(high_rank, RANKS[r], weight);
        } else {
          add_all_unpaired(high_rank, RANKS[r], weight);
        }
      }
    }
  } else {
    // Single hand
    if (is_pair) {
      add_pair(rank1, weight);
    } else if (type == 's') {
      add_suited(rank1, rank2, weight);
    } else if (type == 'o') {
      add_offsuit(rank1, rank2, weight);
    } else {
      add_all_unpaired(rank1, rank2, weight);
    }
  }
}

// Parse a range like "JJ-99" or "ATs-A6s"
void PreflopRange::parse_range(const std::string &start, const std::string &end, float weight) {
  if (start.length() < 2 || end.length() < 2) return;

  char start_r1 = start[0], start_r2 = start[1];
  char end_r1 = end[0], end_r2 = end[1];

  int start_idx1 = rankToIndex(start_r1);
  int start_idx2 = rankToIndex(start_r2);
  int end_idx1 = rankToIndex(end_r1);
  int end_idx2 = rankToIndex(end_r2);

  if (start_idx1 < 0 || start_idx2 < 0 || end_idx1 < 0 || end_idx2 < 0) return;

  bool is_pair = (start_r1 == start_r2) && (end_r1 == end_r2);

  char type = 'a';
  if (start.length() >= 3) type = start[2];

  if (is_pair) {
    // Pair range like "JJ-99"
    int high = std::max(start_idx1, end_idx1);
    int low = std::min(start_idx1, end_idx1);
    for (int r = low; r <= high; ++r) {
      add_pair(RANKS[r], weight);
    }
  } else {
    // Unpaired range like "ATs-A6s" (same high card, different kickers)
    // Or "KQs-KTs" etc.
    int high_card = std::max({start_idx1, start_idx2, end_idx1, end_idx2});
    char high_rank = RANKS[high_card];

    // Get the kicker range
    int kicker1 = (start_idx1 == high_card) ? start_idx2 : start_idx1;
    int kicker2 = (end_idx1 == high_card) ? end_idx2 : end_idx1;

    int low_kicker = std::min(kicker1, kicker2);
    int high_kicker = std::max(kicker1, kicker2);

    for (int k = low_kicker; k <= high_kicker; ++k) {
      if (k == high_card) continue; // Skip if it would make a pair
      if (type == 's') {
        add_suited(high_rank, RANKS[k], weight);
      } else if (type == 'o') {
        add_offsuit(high_rank, RANKS[k], weight);
      } else {
        add_all_unpaired(high_rank, RANKS[k], weight);
      }
    }
  }
}

PreflopRange::PreflopRange(std::string string_range) : preflop_combos{} {
  std::vector<std::string> tokens = split(string_range, ',');
  for (const auto &token : tokens) {
    // Trim whitespace
    std::string trimmed = token;
    while (!trimmed.empty() && std::isspace(trimmed.front())) trimmed.erase(0, 1);
    while (!trimmed.empty() && std::isspace(trimmed.back())) trimmed.pop_back();

    if (trimmed.empty()) continue;

    // Strip weight notation like "AKs:0.5" - solver doesn't support weighted combos
    // Just ignore the weight and use 1.0 for all hands
    size_t colon_pos = trimmed.find(':');
    if (colon_pos != std::string::npos) {
      trimmed = trimmed.substr(0, colon_pos);
    }

    parse_token(trimmed, 1.0f);
  }

  num_hands = preflop_combos.size();
}

void PreflopRange::print() const {
  for (const auto &i : preflop_combos) {
    std::cout << i.to_string() << ", ";
  }
}
