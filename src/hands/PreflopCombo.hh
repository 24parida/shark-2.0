#pragma once
#include "card.h"

using phevaluator::Card;

struct PreflopCombo {
  Card hand1;
  Card hand2;
  double probability;
  double rel_probability{0.0};

  auto to_string() const -> std::string {
    return "(" + hand1.describeCard() + ", " + hand2.describeCard() + ")";
  }
};
