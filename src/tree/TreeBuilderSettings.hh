#include "../game/Game.hh"
#include "../hands/PreflopRange.hh"
#include "card.h"

using phevaluator::Card;

struct TreeBuilderSettings {
  PreflopRange range1;
  PreflopRange range2;
  int in_position_player;
  std::vector<Card> initial_board;
  Street initial_street;
  int starting_stack;
  int starting_pot;
  int minimum_bet;
  double all_in_threshold;

  TreeBuilderSettings(const PreflopRange &o_range1,
                      const PreflopRange &o_range2,
                      const int o_in_position_player,
                      const std::vector<Card> &o_initial_board,
                      const int o_starting_stack, const int o_starting_pot,
                      const int o_min_bet, const double o_all_in_threshold)
      : range1(o_range1), range2(o_range2),
        in_position_player(o_in_position_player),
        initial_board(o_initial_board),
        initial_street(static_cast<Street>(initial_board.size())),
        starting_stack(o_starting_stack), starting_pot(o_starting_pot),
        minimum_bet(o_min_bet), all_in_threshold(o_all_in_threshold) {}
};
