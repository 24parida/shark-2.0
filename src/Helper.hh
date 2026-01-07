#pragma once
#include "hands/PreflopCombo.hh"
#include "phevaluator.h"
#include <cassert>
#include <cstdint>

namespace CardUtility {
inline uint64_t card_to_mask(const Card &card) {
  return 1ULL << int(card);
}

inline uint64_t board_to_mask(const std::vector<Card> &board) {
  uint64_t mask = 0;
  for (const auto &c : board) {
    mask |= card_to_mask(c);
  }
  return mask;
}

inline bool overlap(const PreflopCombo &combo, const Card &card) {
  return combo.hand1 == card || combo.hand2 == card;
}

inline bool overlap(const Card &card, const std::vector<Card> &board) {
  uint64_t card_mask = card_to_mask(card);
  uint64_t board_mask = board_to_mask(board);
  return (card_mask & board_mask) != 0;
}

inline bool overlap(const PreflopCombo &combo, const std::vector<Card> &board) {
  uint64_t combo_mask = card_to_mask(combo.hand1) | card_to_mask(combo.hand2);
  uint64_t board_mask = board_to_mask(board);
  return (combo_mask & board_mask) != 0;
}

inline bool overlap(const PreflopCombo &combo1, const PreflopCombo &combo2) {
  return (combo1.hand1 == combo2.hand1 || combo1.hand1 == combo2.hand2 ||
          combo1.hand2 == combo2.hand1 || combo1.hand2 == combo2.hand2);
}

// Fast overlap check using precomputed board mask
inline bool overlap_mask(const PreflopCombo &combo, uint64_t board_mask) {
  uint64_t combo_mask = (1ULL << int(combo.hand1)) | (1ULL << int(combo.hand2));
  return (combo_mask & board_mask) != 0;
}

// this sucks
inline int board_to_key(const std::vector<Card> &board) {
  assert((board.size() >= 3 && board.size() <= 5) &&
         "CardUtility: board_to_key incorrect board size");
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
  return -1 * phevaluator::EvaluateCards(board[0], board[1], board[2], board[3],
                                         board[4], card1, card2)
                  .value();
}

} // namespace CardUtility
