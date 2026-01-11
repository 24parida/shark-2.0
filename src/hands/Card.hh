// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#pragma once

#include <string>
#include <cassert>

class Card {
  int m_card;

public:
  Card() : m_card(0) {}
  explicit Card(int card) : m_card(card) {}
  explicit Card(const char *str);

  auto operator==(const Card &other) const -> bool { return m_card == other.m_card; }
  auto operator!=(const Card &other) const -> bool { return !(*this == other); }

  auto describeCard() const -> std::string;
  auto get_card() const -> int { return m_card; }

  auto get_rank() const -> int {
    int rank = m_card / 4;
    assert(rank >= 0 && rank < 13 && "Invalid card rank");
    return rank;
  }

  auto get_suit() const -> int {
    int suit = m_card % 4;
    assert(suit >= 0 && suit < 4 && "Invalid card suit");
    return suit;
  }

  auto describeRank() const -> char {
    static const char RANKS[] = "23456789TJQKA";
    int rank = get_rank();
    assert(rank >= 0 && rank < 13 && "Invalid rank index");
    return RANKS[rank];
  }

  auto describeSuit() const -> char {
    static const char SUITS[] = "hdcs";
    int suit = get_suit();
    assert(suit >= 0 && suit < 4 && "Invalid suit index");
    return SUITS[suit];
  }
}; 