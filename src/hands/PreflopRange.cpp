#include "PreflopRange.hh"
#include "../game/Game.hh"
#include "card.h"
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
    tokens.push_back(token);
  }
  return tokens;
}

bool isValidRank(const char rank) {
  return (rank == '1' || rank == '2' || rank == '3' || rank == '4' ||
          rank == '5' || rank == '6' || rank == '7' || rank == '8' ||
          rank == '9' || rank == 'T' || rank == 'J' || rank == 'Q' ||
          rank == 'K' || rank == 'A');
}

void PreflopRange::add_combo(const char rank1, const int suit1,
                             const char rank2, const int suit2,
                             const double weight = 1.0) {

  assert(suit1 >= 0 && suit1 < 4 && "PreflopRange suit1 out of range 1-3");
  assert(suit2 >= 0 && suit2 < 4 && "PrefopRange suit2 out of range 1-3");
  assert(isValidRank(rank1) && "PreflopRange rank1 invalid");
  assert(isValidRank(rank2) && "PreflopRange rank2 invalid");
  assert(!(suit1 == suit2 && rank1 == rank2) &&
         "PreflopRange attempting to add a suited pair");

  preflop_combos.push_back({
      .probability = weight,
      .hand1{std::string{rank1} + GameParams::suitReverseArray[suit1]},
      .hand2{std::string{rank2} + GameParams::suitReverseArray[suit2]},
  });
}

PreflopRange::PreflopRange(std::string string_range) : preflop_combos{} {
  using phevaluator::Card;
  std::vector<std::string> holeCardCombos{split(string_range, ',')};
  for (auto const &combo : holeCardCombos) {
    char rank1 = combo[0];
    char rank2 = combo[1];

    if (combo.length() == 2) {
      for (int suit1{0}; suit1 < GameParams::NUM_SUITS; ++suit1) {
        for (int suit2{0}; suit2 < GameParams::NUM_SUITS; ++suit2) {
          if (rank1 == rank2 && suit1 >= suit2)
            continue;
          add_combo(rank1, suit1, rank2, suit2);
        }
      }
      continue;
    }

    assert(combo.length() == 3 && "PreflopRange incorrect combo length");

    switch (combo[2]) {
    case 'o':
      for (int suit1{0}; suit1 < GameParams::NUM_SUITS; suit1++) {
        for (int suit2{0}; suit2 < GameParams::NUM_SUITS; suit2++) {
          if (suit1 == suit2)
            continue;
          add_combo(rank1, suit1, rank2, suit2);
        }
      }
      break;
    case 's':
      for (int suit{0}; suit < GameParams::NUM_SUITS; suit++) {
        add_combo(rank1, suit, rank2, suit);
      }
      break;
    default:
      assert("PreflopRange: incorrect initialization @ combo[2]");
    }
  }

  num_hands = preflop_combos.size();
}

void PreflopRange::print() const {
  for (const auto &i : preflop_combos) {
    std::cout << i.to_string() << ", ";
  }
}
