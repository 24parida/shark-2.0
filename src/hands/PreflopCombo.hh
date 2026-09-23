// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#pragma once
#include "card.h"
#include <string>
#include <utility>

using phevaluator::Card;

struct PreflopCombo {
  Card hand1;
  Card hand2;
  float probability;
  float rel_probability{0.0};

  bool operator==(const PreflopCombo &o) const {
    return (hand1 == o.hand1 && hand2 == o.hand2) ||
           (hand1 == o.hand2 && hand2 == o.hand1);
  }

  auto to_string() const -> std::string {
    return "(" + hand1.describeCard() + ", " + hand2.describeCard() + ")";
  }

  auto hand_type() const -> std::string {
    auto first = hand1.describeCard();
    auto second = hand2.describeCard();
    if (int(hand1) < int(hand2)) std::swap(first, second);
    std::string result{first[0], second[0]};
    if (first[0] != second[0]) result += first[1] == second[1] ? 's' : 'o';
    return result;
  }
};
