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

struct BoardTest {
  std::string name;
  std::vector<std::string> cards;
};

int main() {
  using phevaluator::Card;

  std::cout << "=== Isomorphism Benchmark ===" << std::endl;
  std::cout << std::endl;

  // Standard ranges
  PreflopRange range1{"22+,ATs+,KJs+,QTs+,JTs,T9s,98s,87s,76s,AKo,AQo,AJo,KQo"};
  PreflopRange range2{"22+,A2s+,K9s+,Q9s+,J9s+,T8s+,97s+,87s,76s,65s,A8o+,KTo+,QTo+,JTo"};

  int stack = 100;
  int pot = 10;
  int min_bet = 2;

  // Test boards - some with isomorphism potential, some without
  std::vector<BoardTest> boards = {
    // Isomorphism possible (two suits with same board ranks)
    {"Ks Kh 2d 9c", {"Ks", "Kh", "2d", "9c"}},   // s/h both have K
    {"As Ah 7c 3d", {"As", "Ah", "7c", "3d"}},   // s/h both have A
    {"Qc Qd 5h 8s", {"Qc", "Qd", "5h", "8s"}},   // c/d both have Q
    {"Jh Jc 4d 6s", {"Jh", "Jc", "4d", "6s"}},   // h/c both have J

    // No isomorphism (all suits have different ranks)
    {"As Kh Qd Jc", {"As", "Kh", "Qd", "Jc"}},   // All different
    {"9s 7h 5d 3c", {"9s", "7h", "5d", "3c"}},   // Rainbow
    {"Ks Qh Jd Tc", {"Ks", "Qh", "Jd", "Tc"}},   // Broadway rainbow
    {"Ah 8d 4c 2s", {"Ah", "8d", "4c", "2s"}},   // Rainbow low

    // Monotone/Two-tone (often no isomorphism due to flush draws)
    {"Ks Qs Js 2c", {"Ks", "Qs", "Js", "2c"}},   // 3 spades
    {"Ah Kh 7d 2c", {"Ah", "Kh", "7d", "2c"}},   // 2 hearts
  };

  DCFR::debug_node_id = -1;
  DCFR::debug_hand = -1;

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "| Board           | Iso? | Children | Time (ms) | Exploitability |" << std::endl;
  std::cout << "|-----------------|------|----------|-----------|----------------|" << std::endl;

  for (const auto& test : boards) {
    std::vector<Card> board;
    for (const auto& c : test.cards) {
      board.push_back(Card{c});
    }

    TreeBuilderSettings settings{range1, range2, 2, board, stack, pot, min_bet, 0.67};
    PreflopRangeManager prm{range1.preflop_combos, range2.preflop_combos, board};
    GameTree game_tree{settings};
    BestResponse br{prm};

    std::unique_ptr<Node> root{game_tree.build()};

    // Count chance node children to verify isomorphism
    int num_children = 0;
    bool has_iso = false;
    std::function<void(Node*)> count_first_chance = [&](Node* node) {
      if (node->get_node_type() == NodeType::CHANCE_NODE) {
        ChanceNode* cn = static_cast<ChanceNode*>(node);
        for (int i = 0; i < 52; ++i) {
          if (cn->get_child(i)) num_children++;
        }
        has_iso = cn->get_isomorphism_data().has_isomorphism;
        return;
      }
      if (node->get_node_type() == NodeType::ACTION_NODE) {
        ActionNode* an = static_cast<ActionNode*>(node);
        for (int i = 0; i < an->get_num_actions(); ++i) {
          count_first_chance(an->get_child(i));
          return;
        }
      }
    };
    count_first_chance(root.get());

    ParallelDCFR trainer{prm, board, settings.starting_pot, settings.in_position_player};

    // Time 200 iterations
    auto start = std::chrono::high_resolution_clock::now();
    trainer.train(root.get(), 200, -1.0, nullptr);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    float exp = br.get_exploitability(root.get(), 200, board, pot, 2);

    std::cout << "| " << std::setw(15) << std::left << test.name
              << " | " << std::setw(4) << (has_iso ? "YES" : "NO")
              << " | " << std::setw(8) << num_children
              << " | " << std::setw(9) << duration
              << " | " << std::setw(13) << std::right << exp << "% |" << std::endl;
  }

  std::cout << std::endl;
  std::cout << "Children = number of river card subtrees (48 max, less with isomorphism)" << std::endl;
  std::cout << "Boards with isomorphism have fewer children -> faster solving" << std::endl;

  return 0;
}
