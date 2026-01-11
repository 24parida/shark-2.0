#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"
#include "trainer/DCFR.hh"
#include <iostream>
#include <iomanip>
#include <chrono>

int main() {
  using phevaluator::Card;

  std::cout << "=== Flop Solve Benchmark ===" << std::endl;

  // Same ranges as before
  PreflopRange range1{"55+,A2s+,K7s+,Q8s+,J8s+,T8s+,97s+,87s,76s,A9o+,KTo+,QJo"};
  PreflopRange range2{"33+,A2s+,K2s+,Q5s+,J7s+,T7s+,96s+,85s+,75s+,64s+,A5o+,K9o+,Q9o+,J9o+,T9o"};

  std::cout << "Range1: " << range1.num_hands << " combos" << std::endl;
  std::cout << "Range2: " << range2.num_hands << " combos" << std::endl;

  // Flop board (3 cards)
  std::vector<Card> board{Card{"Ks"}, Card{"Qh"}, Card{"7d"}};
  std::cout << "Board: Ks Qh 7d (flop)" << std::endl;

  int stack = 100, pot = 10, min_bet = 2;

  // Flop optimizations
  TreeBuilderSettings settings{range1, range2, 2, board, stack, pot, min_bet, 0.67};
  settings.remove_donk_bets = true;
  settings.raise_cap = 3;

  DCFR::compress_strategy = true;

  std::cout << "\nBuilding tree..." << std::endl;
  auto t0 = std::chrono::high_resolution_clock::now();

  PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos, board};
  GameTree game_tree{settings};

  std::unique_ptr<Node> root{game_tree.build()};

  auto stats = game_tree.getTreeStats();
  std::cout << "Action nodes: " << stats.total_action_nodes << std::endl;
  std::cout << "Est. memory: " << (stats.estimateMemoryBytes() / 1024 / 1024) << " MB" << std::endl;

  auto t1 = std::chrono::high_resolution_clock::now();
  std::cout << "Tree built in " << std::chrono::duration<double>(t1 - t0).count() << "s\n" << std::endl;

  // Train
  int iterations = 100;
  std::cout << "Training " << iterations << " iterations..." << std::endl;

  int thread_count = 14;
  std::cout << "Using " << thread_count << " threads" << std::endl;

  ParallelDCFR trainer{prm, board, pot, 2, thread_count};
  BestResponse br{prm};

  trainer.train(root.get(), iterations, -1.0,
    [&](int i, int total, float exploit) {
      if (i % 20 == 0 || i == total) {
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - t1).count();
        std::cout << "Iter " << i << "/" << total << " - "
                  << std::fixed << std::setprecision(1) << elapsed << "s";
        if (exploit >= 0) std::cout << " - Exploit: " << std::setprecision(2) << exploit << "%";
        std::cout << std::endl;
      }
    });

  auto t2 = std::chrono::high_resolution_clock::now();

  // Final stats
  float final_exploit = br.get_exploitability(root.get(), iterations, board, pot, 2);
  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "Final exploitability: " << std::fixed << std::setprecision(3) << final_exploit << "%" << std::endl;
  std::cout << "Training time: " << std::setprecision(1) << std::chrono::duration<double>(t2 - t1).count() << "s" << std::endl;
  std::cout << "Total time: " << std::chrono::duration<double>(t2 - t0).count() << "s" << std::endl;

  return 0;
}
