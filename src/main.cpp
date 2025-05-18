#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"
#include <fstream>
#include <iostream>

int main() {
  using phevaluator::Card;

  PreflopRange range1{"AA,22"};
  PreflopRange range2{"88,77"};

  TreeBuilderSettings settings{
      range1, range2, 2,  {Card{"3h"}, Card{"8h"}, Card{"4c"}, Card{"3d"}},
      800,    400,    10, 0.67};
  PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos,
                          settings.initial_board};

  GameTree game_tree{settings};
  ParallelDCFR trainer{prm, settings.initial_board};

  std::unique_ptr<Node> root{game_tree.build()};
  trainer.train(root.get(), 500);

  auto res = game_tree.jsonify_tree(root.get(), prm);
  std::ofstream out{"strategy.json"};
  out << res.dump(4);
  out.close();

  std::cout << "successfully completed" << '\n';
  return 0;
}
