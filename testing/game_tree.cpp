#include "game/Game.hh"
#include "hands/PreflopRange.hh"
#include "tree/GameTree.hh"
#include <fstream>
#include <iostream>

int main() {
  using phevaluator::Card;
  PreflopRange range1{"AA,22"};
  PreflopRange range2{"88,77"};
  TreeBuilderSettings settings{
      .range1{range1},
      .range2{range2},
      .in_position_player = 2,
      .initial_street = Street::TURN,
      .initial_board{Card{"3h"}, Card{"8h"}, Card{"4c"}, Card{"3d"}},
      .starting_stack = 800,
      .starting_pot = 400,
      .minimum_bet = 10,
      .all_in_threshold = 0.67};

  GameTree game_tree{settings};
  std::unique_ptr<Node> root{game_tree.build()};

  std::ofstream out{"tree.json"};
  json result{game_tree.jsonify_tree(root.get())};
  out << result.dump(4);
  out.close();

  std::cout << "successfully completed" << '\n';
  return 0;
}
