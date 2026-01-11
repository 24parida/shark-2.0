// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#pragma once
#include "card.h"

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
};
