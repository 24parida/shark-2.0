#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"
#include "trainer/DCFR.hh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <functional>

int main() {
  using phevaluator::Card;

  std::cout << "=== Flop Optimization Gating Test ===" << std::endl;
  std::cout << std::endl;

  // Even larger ranges - approaching memory limits
  PreflopRange range1{"55+,A2s+,K7s+,Q8s+,J8s+,T8s+,97s+,87s,76s,A9o+,KTo+,QJo"}; // ~280+ combos
  PreflopRange range2{"33+,A2s+,K2s+,Q5s+,J7s+,T7s+,96s+,85s+,75s+,64s+,A5o+,K9o+,Q9o+,J9o+,T9o"}; // ~450+ combos

  std::cout << "Range1 combos: " << range1.num_hands << std::endl;
  std::cout << "Range2 combos: " << range2.num_hands << std::endl;
  std::cout << std::endl;

  int stack = 100;
  int pot = 10;
  int min_bet = 2;

  // ============ TEST 1: Turn solve (should NOT use flop optimizations) ============
  std::cout << "=== TEST 1: Turn Solve (4 cards) ===" << std::endl;
  {
    std::vector<Card> board{Card{"Ks"}, Card{"Qh"}, Card{"7d"}, Card{"2c"}};
    std::cout << "Board: Ks Qh 7d 2c (turn)" << std::endl;

    TreeBuilderSettings settings{range1, range2, 2, board, stack, pot, min_bet, 0.67};

    const bool is_flop_solve = (board.size() == 3);
    settings.remove_donk_bets = is_flop_solve;
    settings.raise_cap = is_flop_solve ? 3 : -1;
    DCFR::compress_strategy = is_flop_solve;

    std::cout << "is_flop_solve: " << (is_flop_solve ? "true" : "false") << std::endl;
    std::cout << "remove_donk_bets: " << (settings.remove_donk_bets ? "true" : "false") << std::endl;
    std::cout << "raise_cap: " << settings.raise_cap << std::endl;
    std::cout << "compress_strategy: " << (DCFR::compress_strategy ? "true" : "false") << std::endl;

    PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos, board};
    GameTree game_tree{settings};

    auto stats = game_tree.getTreeStats();
    std::cout << "Tree stats: " << stats.total_action_nodes << " action nodes" << std::endl;

    std::unique_ptr<Node> root{game_tree.build()};

    ParallelDCFR trainer{prm, board, settings.starting_pot, settings.in_position_player};
    BestResponse br{prm};

    std::cout << "Training 500 iterations..." << std::endl;
    trainer.train(root.get(), 500, -1.0, nullptr);

    float exp = br.get_exploitability(root.get(), 500, board, pot, 2);
    std::cout << "Exploitability: " << std::fixed << std::setprecision(4) << exp << "%" << std::endl;
    std::cout << (exp < 0.5 ? "PASS: Turn solve converged!" : "FAIL: Turn solve did not converge") << std::endl;
  }

  std::cout << std::endl;

  // ============ TEST 2: Flop tree (memory estimate only, no solve) ============
  std::cout << "=== TEST 2: Flop Tree Memory Estimate (3 cards) ===" << std::endl;
  {
    std::vector<Card> board{Card{"Ks"}, Card{"Qh"}, Card{"7d"}};
    std::cout << "Board: Ks Qh 7d (flop)" << std::endl;

    TreeBuilderSettings settings{range1, range2, 2, board, stack, pot, min_bet, 0.67};

    const bool is_flop_solve = (board.size() == 3);
    settings.remove_donk_bets = is_flop_solve;
    settings.raise_cap = is_flop_solve ? 3 : -1;
    DCFR::compress_strategy = is_flop_solve;

    std::cout << "is_flop_solve: " << (is_flop_solve ? "true" : "false") << std::endl;
    std::cout << "remove_donk_bets: " << (settings.remove_donk_bets ? "true" : "false") << std::endl;
    std::cout << "raise_cap: " << settings.raise_cap << std::endl;
    std::cout << "compress_strategy: " << (DCFR::compress_strategy ? "true" : "false") << std::endl;

    GameTree game_tree{settings};

    std::cout << "Building tree..." << std::endl;
    std::unique_ptr<Node> root{game_tree.build()};
    std::cout << "Tree built successfully!" << std::endl;

    auto stats = game_tree.getTreeStats();
    size_t mem_bytes = stats.estimateMemoryBytes();

    std::cout << std::endl;
    std::cout << "--- Tree Statistics ---" << std::endl;
    std::cout << "Flop action nodes:  " << stats.flop_action_nodes << std::endl;
    std::cout << "Turn action nodes:  " << stats.turn_action_nodes << std::endl;
    std::cout << "River action nodes: " << stats.river_action_nodes << std::endl;
    std::cout << "Total action nodes: " << stats.total_action_nodes << std::endl;
    std::cout << "Chance nodes:       " << stats.chance_nodes << std::endl;
    std::cout << "Terminal nodes:     " << stats.terminal_nodes << std::endl;
    std::cout << "P1 hands: " << stats.p1_num_hands << ", P2 hands: " << stats.p2_num_hands << std::endl;
    std::cout << std::endl;
    std::cout << "Estimated memory: " << (mem_bytes / 1024 / 1024) << " MB" << std::endl;
    std::cout << "                  " << std::fixed << std::setprecision(2)
              << (mem_bytes / 1024.0 / 1024.0 / 1024.0) << " GB" << std::endl;

    if (mem_bytes > 15ULL * 1024 * 1024 * 1024) {
      std::cout << std::endl << "WARNING: Exceeds 15GB RAM estimate" << std::endl;
    }
  }

  std::cout << std::endl;
  std::cout << "=== All tests complete ===" << std::endl;

  return 0;
}
