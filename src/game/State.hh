#include "Action.hh"
#include "Game.hh"
#include "card.h"
#include <cassert>
#include <memory>

using phevaluator::Card;

struct PlayerState {
  bool has_position;
  bool has_folded;

  int stack;
  int wager;
  int _id;

  PlayerState() = default;

  PlayerState(const int id, const bool position, int stack_size) {
    _id = id;
    has_position = position;
    stack = stack_size;
    has_folded = false;
    wager = 0;
  }

  int hash_code() { return _id; }
  void reset_wager() { wager = 0; };
  void all_in() { stack = 0; }
  void fold() { has_folded = true; }
  void commit_chips(const int amount) {
    wager += amount;
    stack -= amount;
  }
  bool operator==(const PlayerState &other) const { return _id == other._id; }
};

struct GameState {

  Street street;
  int pot;
  std::vector<Card> board;

  std::shared_ptr<PlayerState> p1;
  std::shared_ptr<PlayerState> p2;

  std::shared_ptr<PlayerState> current;
  std::shared_ptr<PlayerState> last_to_act;

  int minimum_bet_size;
  int minimum_raise_size;

  // Per-street aggressor tracking for donk bet removal
  // -1 = no aggressor (checked through), 1/2 = player ID
  int flop_aggressor = -1;
  int turn_aggressor = -1;

  // Raise counter for raise cap (reset each street)
  int street_raise_count = 0;

  GameState() = default;
  GameState(const GameState &other)
      : street(other.street), pot(other.pot), board(other.board),
        p1(std::make_shared<PlayerState>(*other.p1)),
        p2(std::make_shared<PlayerState>(*other.p2)),
        minimum_bet_size(other.minimum_bet_size),
        minimum_raise_size(other.minimum_raise_size),
        flop_aggressor(other.flop_aggressor),
        turn_aggressor(other.turn_aggressor),
        street_raise_count(other.street_raise_count) {
    current = other.current->_id == 1 ? p1 : p2;
    last_to_act = other.last_to_act->_id == 1 ? p1 : p2;
  };

  void set_turn(const Card card) {
    assert(board.size() == 3 &&
           "GameState set_turn: attempting to set an already set turn");
    board.push_back(card);
  }
  void set_river(const Card card) {
    assert(board.size() == 4 &&
           "GameState set_river: attempting to set an already set river");
    board.push_back(card);
  }
  void set_pot(const int amt) { pot = amt; }
  int get_max_bet() const {
    return p1->wager > p2->wager ? p1->wager : p2->wager;
  }
  int get_call_amount() const { return get_max_bet() - current->wager; }
  bool is_uncontested() const { return p1->has_folded || p2->has_folded; }
  bool both_all_in() const { return p1->stack == 0 && p2->stack == 0; }
  void reset_last_to_act() { last_to_act = last_to_act == p1 ? p2 : p1; }
  void update_current() { current = current == p1 ? p2 : p1; }
  void init_current() { current = !p1->has_position ? p1 : p2; }
  void init_last_to_act() { last_to_act = p1->has_position ? p1 : p2; }

  bool apply_action(const Action action) {
    switch (action.type) {
    case Action::FOLD:
      current->has_folded = true;
      pot -= get_call_amount();
      return true;
      break;

    case Action::CHECK:
      if (current == last_to_act)
        return true;
      break;

    case Action::CALL:
      current->commit_chips(action.amount);
      pot += action.amount;
      return true;
      break;

    case Action::BET:
      current->commit_chips(action.amount);
      pot += action.amount;
      minimum_raise_size = action.amount;
      // Track aggressor for donk bet removal
      if (street == Street::FLOP) flop_aggressor = current->_id;
      else if (street == Street::TURN) turn_aggressor = current->_id;
      reset_last_to_act();
      break;

    case Action::RAISE: {
      const int chips_to_commit = action.amount - current->wager;
      current->commit_chips(chips_to_commit);
      pot += chips_to_commit;
      const int raise_size = action.amount - get_max_bet();
      if (raise_size > minimum_raise_size)
        minimum_raise_size = raise_size;
      // Track aggressor for donk bet removal
      if (street == Street::FLOP) flop_aggressor = current->_id;
      else if (street == Street::TURN) turn_aggressor = current->_id;
      // Track raise count for raise cap
      ++street_raise_count;
      reset_last_to_act();
      break;
    }
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

    p1->reset_wager();
    p2->reset_wager();

    minimum_raise_size = minimum_bet_size;
    street_raise_count = 0;  // Reset raise counter for new street
  }
};
