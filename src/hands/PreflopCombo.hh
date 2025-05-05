#include "card.h"

using phevaluator::Card;

struct PreflopCombo {
  double probability;
  double rel_probability{0.0};
  Card hand1;
  Card hand2;

  std::string to_string() const {
    return "(" + hand1.describeCard() + ", " + hand2.describeCard() + ")";
  }
};
