#include "Action.hh"
#include "Game.hh"
#include "card.h"
#include <cassert>

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

  bool operator==(const PlayerState &other) const {
    return this->_id == other._id;
  }
};

class GameState {

  Street street;
  int pot;
  std::array<Card, 5> board;

  PlayerState p1;
  PlayerState p2;

  PlayerState current;
  PlayerState last_to_act;

  int minimum_bet_size;
  int minimum_raise_size;

public:
  GameState(const GameState &other) {
    street = other.street;
    pot = other.pot;
    board = other.board;
    p1 = other.p1;
    p2 = other.p2;

    minimum_bet_size = other.minimum_bet_size;
    minimum_raise_size = other.minimum_raise_size;

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
  void reset_last_to_act() { last_to_act = last_to_act == p1 ? p2 : p1; }
  void update_current() { current = current == p1 ? p2 : p1; }
  void init_current() { current = !p1.has_position ? p1 : p2; }
  void init_last_to_act() { last_to_act = p1.has_position ? p1 : p2; }

  bool apply_action(Action action) {
    switch (action.type) {
    case Action::FOLD:
      current.has_folded = true;
      pot -= get_call_amount();
      return true;

    case Action::CHECK:
      return current == last_to_act;

    case Action::CALL:
      current.commit_chips(action.amount);
      pot += action.amount;
      return true;

    case Action::BET:
      current.commit_chips(action.amount);
      pot += action.amount;
      minimum_raise_size = action.amount;
      reset_last_to_act();
      break;

    case Action::RAISE:
      int chips_to_commit = action.amount - current.wager;
      current.commit_chips(chips_to_commit);
      pot += chips_to_commit;
      int raise_size = action.amount - get_max_bet();
      if (raise_size > minimum_bet_size)
        minimum_bet_size = raise_size;
      reset_last_to_act();
      break;
    }

    update_current();
    return false;
  }

  void go_to_next_street() {
    assert(street != Street::RIVER &&
           "GameState: attempting to move on from river");
    street = static_cast<Street>(static_cast<int>(street) + 1);

    init_current();
    init_last_to_act();

    p1.reset_wager();
    p2.reset_wager();

    minimum_raise_size = minimum_bet_size;
  }
};
