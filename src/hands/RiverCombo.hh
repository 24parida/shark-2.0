#pragma once
#include "card.h"

using phevaluator::Card;

struct RiverCombo {
  Card hand1;
  Card hand2;
  int rank;
  int reach_probs_index;
  float probability;

  RiverCombo() = default;
  RiverCombo(const Card card1, const Card card2, const float prob,
             const int reach_probs_idx)
      : hand1(card1), hand2(card2), rank(0), reach_probs_index(reach_probs_idx),
        probability(prob) {}

  auto to_string() const -> std::string {
    return "(" + hand1.describeCard() + ", " + hand2.describeCard() + ")";
  }

  auto get_rank() const -> int { return rank; }
  bool operator<(const RiverCombo &other) const { return rank < other.rank; }
};
