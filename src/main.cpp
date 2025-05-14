#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include <iostream>

int main() {
  using phevaluator::Card;
  // PreflopRange range1{
  //     "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,AK,AQ,AJ,AT,A9,A8,A7,A6,A5,A4,A3,"
  //     "A2,KQ,KJ,KT,K9,K8,K7,K6,K5,K4,K3,K2,QJ,QT,Q9,Q8,Q7,Q6,Q5,Q4,Q3,Q2,JT,J9,"
  //     "J8,J7,J6,J5,J4,J3,J2,T9,T8,T7,T6,T5,T4,T3,T2,98,97,96,95,94,93,92,87,86,"
  //     "85,84,83,82,76,75,74,73,72,65,64,63,62,54,53,52,43,42,32"};
  //
  // PreflopRange range2{
  //     "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,AK,AQ,AJ,AT,A9,A8,A7,A6,A5,A4,A3,"
  //     "A2,KQ,KJ,KT,K9,K8,K7,K6,K5,K4,K3,K2,QJ,QT,Q9,Q8,Q7,Q6,Q5,Q4,Q3,Q2,JT,J9,"
  //     "J8,J7,J6,J5,J4,J3,J2,T9,T8,T7,T6,T5,T4,T3,T2,98,97,96,95,94,93,92,87,86,"
  //     "85,84,83,82,76,75,74,73,72,65,64,63,62,54,53,52,43,42,32"};

  PreflopRange range1{
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,AK,AQ,AJ,AT,A9,A8,A7,A6,A5,A4,A3,"
      "A2,KQ,KJ,KT,K9,K8,K7,K6,K5,K4,K3,K2,QJ,QT,Q9,Q8,Q7,Q6,Q5,Q4,Q3,Q2,JT,J9,"
      "J8,J7,J6,J5,J4,J3,J2,T9,T8,T7,T6,T5,T4,T3,T2,98,97,96,95,94,93,92,87,86,"
      "85,84,83,82,76,75,74,73,72,65,64,63,62,54,53,52,43,42,32"};

  PreflopRange range2{
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,AK,AQ,AJ,AT,A9,A8,A7,A6,A5,A4,A3,"
      "A2,KQ,KJ,KT,K9,K8,K7,K6,K5,K4,K3,K2,QJ,QT,Q9,Q8,Q7,Q6,Q5,Q4,Q3,Q2,JT,J9,"
      "J8,J7,J6,J5,J4,J3,J2,T9,T8,T7,T6,T5,T4,T3,T2,98,97,96,95,94,93,92,87,86,"
      "85,84,83,82,76,75,74,73,72,65,64,63,62,54,53,52,43,42,32"};
  TreeBuilderSettings settings{
      range1, range2, 2,  {Card{"3h"}, Card{"8h"}, Card{"4c"}, Card{"3d"}},
      800,    400,    10, 0.67};
  PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos,
                          settings.initial_board};

  GameTree game_tree{settings};
  ParallelDCFR trainer{prm, settings.initial_board};

  std::unique_ptr<Node> root{game_tree.build()};
  trainer.train(root.get(), 500);

  std::cout << "successfully completed" << '\n';
  return 0;
}
