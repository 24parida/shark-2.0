#include "../game/Game.hh"
#include "../hands/PreflopRange.hh"
#include "card.h"

using phevaluator::Card;

struct TreeBuilderSettings {
  PreflopRange range1;
  PreflopRange range2;
  int in_position_player;
  Street initial_street;
  std::array<Card, 4> initial_board;

  int starting_stack;
  int starting_pot;

  int minimum_bet;
  double all_in_threshold;
};
