#include "hands/PreflopRange.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"
#include "trainer/DCFR.hh"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

int main() {
  using phevaluator::Card;

  std::cout << "=== Shark Solver CLI Test ===" << std::endl;
  std::cout << "Testing against WASM-postflop reference\n" << std::endl;

  // Small test ranges to match WASM-postflop test: AA, A2s, 22
  PreflopRange range1{"AA,A2s,22"};
  PreflopRange range2{"AA,A2s,22"};

  // Board: 6h7h6d7d (turn)
  std::vector<Card> board = {Card{"6h"}, Card{"7h"}, Card{"6d"}, Card{"7d"}};

  // Match WASM-postflop settings
  int stack = 100;
  int pot = 10;
  int min_bet = 2;

  std::cout << "Ranges: AA, A2s, 22" << std::endl;
  std::cout << "Board: 6h 7h 6d 7d" << std::endl;
  std::cout << "Stack: " << stack << ", Pot: " << pot << ", Min bet: " << min_bet << std::endl;
  std::cout << std::endl;

  TreeBuilderSettings settings{range1, range2, 2, board, stack, pot, min_bet, 0.67};
  PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos, board};

  std::cout << "P1 combos: " << range1.preflop_combos.size() << std::endl;
  std::cout << "P2 combos: " << range2.preflop_combos.size() << std::endl;

  // Print all combos with probabilities
  std::cout << "\nP1 combos with probabilities:" << std::endl;
  for (size_t i = 0; i < range1.preflop_combos.size(); ++i) {
    auto& c = range1.preflop_combos[i];
    std::cout << "  [" << i << "] " << Card(c.hand1).describeCard() << Card(c.hand2).describeCard()
              << " prob=" << c.probability << std::endl;
  }

  // Print initial reach probs
  auto hero_reach = prm.get_initial_reach_probs(1, board);
  auto villain_reach = prm.get_initial_reach_probs(2, board);
  std::cout << "\nInitial reach probs (P1 hero):" << std::endl;
  for (size_t i = 0; i < hero_reach.size(); ++i) {
    auto& c = range1.preflop_combos[i];
    std::cout << "  [" << i << "] " << Card(c.hand1).describeCard() << Card(c.hand2).describeCard()
              << " reach=" << hero_reach[i] << std::endl;
  }
  std::cout << "Sum of hero reach: " << std::accumulate(hero_reach.begin(), hero_reach.end(), 0.0f) << std::endl;

  GameTree game_tree{settings};
  BestResponse br{prm};

  std::cout << std::fixed << std::setprecision(4);

  std::unique_ptr<Node> root{game_tree.build()};
  ParallelDCFR trainer{prm, board, settings.starting_pot, settings.in_position_player};

  // Set pot normalizer for regret scaling
  DCFR::pot_normalizer = static_cast<float>(settings.starting_pot);

  // Disable debug output for cleaner output
  DCFR::debug_node_id = -1;  // Disable debug
  DCFR::debug_hand = -1;

  std::cout << "\n=== TRAINING ===" << std::endl;

  // Training with progress
  auto progress_cb = [&](int iter, int total, float) {
    if (iter % 10 == 0) {
      float exp = br.get_exploitability(root.get(), iter, board, pot, 2);
      std::cout << "Iter " << iter << ": exploitability = " << exp << "%" << std::endl;
    }
  };

  trainer.train(root.get(), 100, -1.0, progress_cb);

  float final_exp = br.get_exploitability(root.get(), 100, board, pot, 2);
  std::cout << "\nFinal (100 iters): exploitability = " << final_exp << "%" << std::endl;

  // Print root node strategy for ALL hands
  if (root->get_node_type() == NodeType::ACTION_NODE) {
    ActionNode* action_node = static_cast<ActionNode*>(root.get());
    std::vector<float> avg_strat = action_node->get_average_strat();
    int num_hands = action_node->get_num_hands();
    int num_actions = action_node->get_num_actions();
    auto& combos = range1.preflop_combos;

    std::cout << "\n=== ROOT STRATEGY (P1) ===" << std::endl;
    std::cout << "Actions: ";
    for (auto& action : action_node->get_actions()) {
      const char* type_names[] = {"FOLD", "CHECK", "CALL", "BET", "RAISE"};
      std::cout << type_names[action.type];
      if (action.type == Action::BET || action.type == Action::RAISE) {
        std::cout << "(" << action.amount << ")";
      }
      std::cout << " ";
    }
    std::cout << std::endl;

    // Build board mask
    uint64_t board_mask = 0;
    for (const auto& c : board) board_mask |= (1ULL << int(c));

    std::cout << "\nAll hands:" << std::endl;
    for (int idx = 0; idx < num_hands; ++idx) {
      auto& c = combos[idx];
      uint64_t hand_mask = (1ULL << c.hand1) | (1ULL << c.hand2);

      std::string blocked = (hand_mask & board_mask) ? " [BLOCKED]" : "";

      std::cout << "  [" << idx << "] " << Card(c.hand1).describeCard()
                << Card(c.hand2).describeCard() << blocked << ": ";
      for (int a = 0; a < num_actions; ++a) {
        std::cout << std::setw(6) << (avg_strat[idx + a * num_hands] * 100) << "% ";
      }
      std::cout << std::endl;
    }

    // Focus on 22 combos specifically
    std::cout << "\n=== 22 COMBOS SPECIFICALLY ===" << std::endl;
    std::cout << "WASM-postflop reference:" << std::endl;
    std::cout << "  All 22 combos: 100% check EXCEPT:" << std::endl;
    std::cout << "  - 2h2d: 70% check" << std::endl;
    std::cout << "  - 2s2h: 95% check" << std::endl;
    std::cout << "\nOur results:" << std::endl;
    for (int idx = 0; idx < num_hands; ++idx) {
      auto& c = combos[idx];
      Card c1(c.hand1), c2(c.hand2);
      // Check if this is a 22 combo (rank 0 = 2)
      if (c1.describeCard()[0] == '2' && c2.describeCard()[0] == '2') {
        std::cout << "  " << c1.describeCard() << c2.describeCard() << ": ";
        // First action should be check
        std::cout << "Check=" << std::setw(6) << (avg_strat[idx] * 100) << "%" << std::endl;
      }
    }
  }

  return 0;
}
