#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"
#include <fstream>
#include <iostream>

int main() {
  using phevaluator::Card;

  PreflopRange range1{"AA,22"};
  PreflopRange range2{"KK,QQ,AK"};

  TreeBuilderSettings settings{
      range1, range2, 2,  {Card{"3h"}, Card{"8h"}, Card{"4c"}, Card{"4d"}},
      800,    400,    10, 0.67};
  PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos,
                          settings.initial_board};

  GameTree game_tree{settings};
  ParallelDCFR trainer{prm, settings.initial_board, settings.starting_pot,
                       settings.in_position_player};

  std::unique_ptr<Node> root{game_tree.build()};
  trainer.train(root.get(), 500);

  std::ofstream file{"strat.json"};
  file << game_tree.jsonify_tree(root.get(), prm).dump(4);

  std::cout << "successfully completed" << '\n';
  return 0;
}
