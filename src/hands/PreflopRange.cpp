// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#include "PreflopRange.hh"
#include "../game/Game.hh"
#include "card.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>

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

  PreflopCombo combo{
      .hand1{std::string{rank1} + GameParams::suitReverseArray[suit1]},
      .hand2{std::string{rank2} + GameParams::suitReverseArray[suit2]},
      .probability = weight,
  };
  if (int(combo.hand1) < int(combo.hand2)) std::swap(combo.hand1, combo.hand2);
  auto existing = std::find(preflop_combos.begin(), preflop_combos.end(), combo);
  if (existing == preflop_combos.end()) preflop_combos.push_back(combo);
  else existing->probability = weight;
}

void PreflopRange::add_pair(char rank, float weight) {
  for (int suit1 = 0; suit1 < 4; ++suit1) {
    for (int suit2 = suit1 + 1; suit2 < 4; ++suit2) {
      add_combo(rank, suit1, rank, suit2, weight);
    }
  }
}

void PreflopRange::add_suited(char rank1, char rank2, float weight) {
  for (int suit = 0; suit < 4; ++suit) {
    add_combo(rank1, suit, rank2, suit, weight);
  }
}

void PreflopRange::add_offsuit(char rank1, char rank2, float weight) {
  for (int suit1 = 0; suit1 < 4; ++suit1) {
    for (int suit2 = 0; suit2 < 4; ++suit2) {
      if (suit1 != suit2) {
        add_combo(rank1, suit1, rank2, suit2, weight);
      }
    }
  }
}

void PreflopRange::add_all_unpaired(char rank1, char rank2, float weight) {
  add_suited(rank1, rank2, weight);
  add_offsuit(rank1, rank2, weight);
}

void PreflopRange::parse_token(const std::string &token, float weight) {
  if (token.empty()) return;

  size_t dash_pos = token.find('-');
  if (dash_pos != std::string::npos && dash_pos > 0) {
    std::string start = token.substr(0, dash_pos);
    std::string end = token.substr(dash_pos + 1);
    parse_range(start, end, weight);
    return;
  }

  bool has_plus = (!token.empty() && token.back() == '+');
  std::string base = has_plus ? token.substr(0, token.length() - 1) : token;

  if (base.length() == 4 && !has_plus) {
    const std::string suits = "cdhs";
    auto suit1 = suits.find(base[1]);
    auto suit2 = suits.find(base[3]);
    if (!isValidRank(base[0]) || !isValidRank(base[2]) ||
        suit1 == std::string::npos || suit2 == std::string::npos ||
        base.substr(0, 2) == base.substr(2, 2))
      throw std::invalid_argument("Invalid hand: " + token);
    add_combo(base[0], suit1, base[2], suit2, weight);
    return;
  }

  if (base.length() < 2 || base.length() > 3)
    throw std::invalid_argument("Invalid hand: " + token);

  char rank1 = base[0];
  char rank2 = base[1];
  int idx1 = rankToIndex(rank1);
  int idx2 = rankToIndex(rank2);

  if (idx1 < 0 || idx2 < 0)
    throw std::invalid_argument("Invalid hand: " + token);

  bool is_pair = (rank1 == rank2);
  char type = 'a';
  if (base.length() >= 3) {
    type = base[2];
    if (is_pair || (type != 's' && type != 'o'))
      throw std::invalid_argument("Invalid hand: " + token);
  }

  if (has_plus) {
    if (is_pair) {
      for (int r = idx1; r < NUM_RANKS; ++r) {
        add_pair(RANKS[r], weight);
      }
    } else {
      int high_idx = std::max(idx1, idx2);
      int low_idx = std::min(idx1, idx2);
      char high_rank = RANKS[high_idx];

      int gap = high_idx - low_idx;
      int end = gap == 1 ? NUM_RANKS - gap : high_idx;
      for (int r = low_idx; r < end; ++r) {
        if (gap == 1) high_rank = RANKS[r + gap];
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

void PreflopRange::parse_range(const std::string &start, const std::string &end, float weight) {
  if (start.length() < 2 || end.length() < 2 || start.length() > 3 || end.length() > 3)
    throw std::invalid_argument("Invalid interval: " + start + "-" + end);

  char start_r1 = start[0], start_r2 = start[1];
  char end_r1 = end[0], end_r2 = end[1];

  int start_idx1 = rankToIndex(start_r1);
  int start_idx2 = rankToIndex(start_r2);
  int end_idx1 = rankToIndex(end_r1);
  int end_idx2 = rankToIndex(end_r2);

  if (start_idx1 < 0 || start_idx2 < 0 || end_idx1 < 0 || end_idx2 < 0)
    throw std::invalid_argument("Invalid interval: " + start + "-" + end);

  bool is_pair = (start_r1 == start_r2) && (end_r1 == end_r2);

  char type = 'a';
  if (start.length() >= 3) type = start[2];
  const char end_type = end.length() == 3 ? end[2] : 'a';
  if (type != end_type || (type != 'a' && type != 's' && type != 'o') ||
      (is_pair && type != 'a'))
    throw std::invalid_argument("Invalid interval: " + start + "-" + end);

  if (is_pair) {
    int high = std::max(start_idx1, end_idx1);
    int low = std::min(start_idx1, end_idx1);
    for (int r = low; r <= high; ++r) {
      add_pair(RANKS[r], weight);
    }
  } else if (start_idx1 - start_idx2 == end_idx1 - end_idx2) {
    const int gap = start_idx1 - start_idx2;
    if (gap <= 0) throw std::invalid_argument("Invalid interval: " + start + "-" + end);
    for (int high = std::min(start_idx1, end_idx1); high <= std::max(start_idx1, end_idx1); ++high) {
      if (type == 's') add_suited(RANKS[high], RANKS[high - gap], weight);
      else if (type == 'o') add_offsuit(RANKS[high], RANKS[high - gap], weight);
      else add_all_unpaired(RANKS[high], RANKS[high - gap], weight);
    }
  } else {
    if (start_r1 != end_r1)
      throw std::invalid_argument("Invalid interval: " + start + "-" + end);
    int high_card = std::max({start_idx1, start_idx2, end_idx1, end_idx2});
    char high_rank = RANKS[high_card];

    int kicker1 = (start_idx1 == high_card) ? start_idx2 : start_idx1;
    int kicker2 = (end_idx1 == high_card) ? end_idx2 : end_idx1;

    int low_kicker = std::min(kicker1, kicker2);
    int high_kicker = std::max(kicker1, kicker2);

    for (int k = low_kicker; k <= high_kicker; ++k) {
      if (k == high_card) continue;
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
  for (auto token = tokens.rbegin(); token != tokens.rend(); ++token) {
    std::string trimmed = *token;
    trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(),
                                [](unsigned char c) { return std::isspace(c); }), trimmed.end());

    if (trimmed.empty()) continue;

    size_t colon_pos = trimmed.find(':');
    float weight = 1.0f;
    if (colon_pos != std::string::npos) {
      const std::string text = trimmed.substr(colon_pos + 1);
      if (text.find_first_not_of("0123456789.eE+-") != std::string::npos)
        throw std::invalid_argument("Invalid weight: " + text);
      size_t consumed = 0;
      weight = std::stof(text, &consumed);
      if (consumed != text.size() || !std::isfinite(weight) || weight < 0 || weight > 1)
        throw std::invalid_argument("Invalid weight: " + text);
      trimmed = trimmed.substr(0, colon_pos);
    }

    parse_token(trimmed, weight);
  }

  preflop_combos.erase(std::remove_if(preflop_combos.begin(), preflop_combos.end(),
                                    [](const auto &hand) { return hand.probability == 0; }), preflop_combos.end());
  num_hands = preflop_combos.size();
}

auto PreflopRange::to_strings() const -> std::vector<std::string> {
  std::map<std::string, std::vector<const PreflopCombo *>> groups;
  for (const auto &hand : preflop_combos)
    if (hand.probability > 0) groups[hand.hand_type()].push_back(&hand);
  std::vector<std::string> result;
  auto append = [&](std::string name, float weight) {
    if (weight != 1) {
      std::ostringstream out;
      out << std::setprecision(std::numeric_limits<float>::max_digits10) << weight;
      name += ':' + out.str();
    }
    result.push_back(std::move(name));
  };
  for (const auto &[type, hands] : groups) {
    const size_t count = type.size() == 2 ? 6 : type.back() == 's' ? 4 : 12;
    const float weight = hands.front()->probability;
    if (hands.size() == count && std::all_of(hands.begin(), hands.end(),
                                           [weight](const auto *hand) { return hand->probability == weight; })) {
      append(type, weight);
    } else {
      for (const auto *hand : hands) {
        const Card first = int(hand->hand1) > int(hand->hand2) ? hand->hand1 : hand->hand2;
        const Card second = int(hand->hand1) > int(hand->hand2) ? hand->hand2 : hand->hand1;
        append(first.describeCard() + second.describeCard(), hand->probability);
      }
    }
  }
  return result;
}

void PreflopRange::print() const {
  for (const auto &i : preflop_combos) {
    std::cout << i.to_string() << ", ";
  }
}
