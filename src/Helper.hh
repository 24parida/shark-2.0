#pragma once
#include "../include/omp/EquityCalculator.hh"
#include "hands/PreflopCombo.hh"
#include "phevaluator.h"
#include <cassert>

using namespace omp;

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

inline auto get_win_pct(const PreflopCombo &hero, const PreflopCombo &villain,
                        const std::vector<Card> board,
                        const float accuracy_margin = 0.01) -> float {
  assert((board.size() == 3 || board.size() == 4) &&
         "get_win_pct: expected board of size 3 or 4 (undealt turn/river)");
  EquityCalculator eq;

  const std::string hand1{hero.hand1.describeCard() +
                          hero.hand2.describeCard()};
  const std::string hand2{villain.hand1.describeCard() +
                          villain.hand2.describeCard()};
  std::vector<CardRange> ranges{hand1, hand2};

  std::string board_str{};
  for (const auto &i : board) {
    board_str += i.describeCard();
  }
  uint64_t omp_board = CardRange::getCardMask(board_str);

  // to explain the magic numbers being passed to eq.start()
  // _,_, dead_cards, accuracy (<=1%), callback_period(idk), threads (1)
  eq.start(ranges, omp_board, 0, false, accuracy_margin, nullptr, 0.2, 1);
  eq.wait();

  return eq.getResults().equity[0];
}

} // namespace CardUtility
