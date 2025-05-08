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
} // namespace CardUtility
