#pragma once
#include "card.h"
#include "hands/PreflopCombo.hh"

namespace CardUtility {
template <std::size_t N>
inline bool overlap(const Card &card,
                    const std::array<phevaluator::Card, N> &board) {
  for (const auto &i : board) {
    if (i == card)
      return true;
  }
  return false;
}

template <std::size_t N>
inline bool overlap(const PreflopCombo &combo,
                    const std::array<phevaluator::Card, N> &board) {
  for (const auto &i : board) {
    if (i == combo.hand1 || i == combo.hand2)
      return true;
  }
  return false;
}

inline bool overlap(const PreflopCombo &combo1, const PreflopCombo &combo2) {
  return (combo1.hand1 == combo2.hand1 || combo1.hand1 == combo2.hand2 ||
          combo1.hand2 == combo2.hand1 || combo1.hand2 == combo2.hand2);
}

} // namespace CardUtility
