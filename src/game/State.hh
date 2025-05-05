#include "Game.hh"
#include "card.h"

using phevaluator::Card;

struct PlayerState {
  bool has_position;
  bool has_folded{false};

  int stack;
  int wager{0};
  int _id;

  int hash_code() { return _id; }
  void reset_wager() { wager = 0; };
  void all_in() { stack = 0; }
  void fold() { has_folded = true; }
  void commit_chips(int amount) {
    wager += amount;
    stack -= amount;
  }
};

struct GameState {

  Street street;
  int pot;
  std::array<Card, 5> board;

  PlayerState p1;
  PlayerState p2;

  PlayerState current;
  PlayerState last_to_act;

  int minimum_bet_size;
  int maximum_bet_size;

  GameState(const GameState &other) {
    street = other.street;
    pot = other.pot;
    board = other.board;
    p1 = other.p1;
    p2 = other.p2;

    minimum_bet_size = other.minimum_bet_size;
    maximum_bet_size = other.maximum_bet_size;

    current = other.current._id == 1 ? p1 : p2;
    last_to_act = other.last_to_act._id == 1 ? p1 : p2;
  };

  void set_turn(const Card card) {
    board[static_cast<size_t>(Street::TURN)] = card;
  }
  void set_river(const Card card) {
    board[static_cast<size_t>(Street::RIVER)] = card;
  }
  void set_pot(const int amt) { pot = amt; }
  int get_max_bet() const { return p1.wager > p2.wager ? p1.wager : p2.wager; }
  int get_call_amount() const { return get_max_bet() - current.wager; }
  bool is_uncontested() const { return p1.has_folded || p2.has_folded; }
  bool both_all_in() const { return p1.stack == 0 && p2.stack == 0; }

  bool apply_action() {}
};
