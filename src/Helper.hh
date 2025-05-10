#pragma once
#include "hands/PreflopCombo.hh"
#include "phevaluator.h"
#include <cassert>

namespace CardUtility {
inline bool overlap(const PreflopCombo &combo, const Card &card) {
  return combo.hand1 == card || combo.hand2 == card;
}

inline bool overlap(const Card &card, const std::vector<Card> &board) {
  for (const auto &i : board) {
    if (i == card)
      return true;
  }
  return false;
}

inline bool overlap(const PreflopCombo &combo, const std::vector<Card> &board) {
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

// this sucks
inline int board_to_key(const std::vector<Card> &board) {
  assert((board.size() >= 3 && board.size() <= 5) &&
         "CardUtility: board_to_key incorrecy board size");
  if (board.size() == 3) {
    return 100000000 * static_cast<int>(board[0]) +
           1000000 * static_cast<int>(board[1]) +
           10000 * static_cast<int>(board[2]);
  } else if (board.size() == 3) {
    return 100000000 * static_cast<int>(board[0]) +
           1000000 * static_cast<int>(board[1]) +
           10000 * static_cast<int>(board[2]) +
           100 * static_cast<int>(board[3]);
  } else {
    return 100000000 * static_cast<int>(board[0]) +
           1000000 * static_cast<int>(board[1]) +
           10000 * static_cast<int>(board[2]) +
           100 * static_cast<int>(board[3]) + static_cast<int>(board[4]);
  }
}

inline auto get_rank(const Card &card1, const Card &card2,
                     const std::vector<Card> &board) -> int {
  assert(board.size() == 5 &&
         "Helper get_rank: board is of incorrect size (!=5)");
  return phevaluator::EvaluateCards(board[1], board[2], board[2], board[3],
                                    board[4], card1, card2)
      .value();
}

} // namespace CardUtility
