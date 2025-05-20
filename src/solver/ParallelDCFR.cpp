#include "Solver.hh"
#include "hands/PreflopCombo.hh"
#include "tree/Nodes.hh"
#include <chrono>
#include <iostream>
#include <oneapi/tbb/task_group.h>
#include <tbb/global_control.h>

void ParallelDCFR::load_trainer_modules(Node *const node) {
  if (node->get_node_type() == NodeType::ACTION_NODE) {
    auto *action_node = static_cast<ActionNode *>(node);
    action_node->load_trainer(action_node);
    for (auto &child : action_node->get_children())
      load_trainer_modules(child.get());
  } else if (node->get_node_type() == NodeType::CHANCE_NODE) {
    auto *chance_node = static_cast<ChanceNode *>(node);
    for (int i{0}; i < 52; ++i) {
      if (!chance_node->get_child(i))
        continue;

      load_trainer_modules(chance_node->get_child(i));
    }
  }
}

void ParallelDCFR::train(Node *root, const int iterations) {
  tbb::global_control c{tbb::global_control::max_allowed_parallelism, 8};

  load_trainer_modules(root);

  const auto start = std::chrono::high_resolution_clock::now();

  auto hero_preflop_combos{m_prm.get_preflop_combos(1)};
  auto villain_preflop_combos{m_prm.get_preflop_combos(2)};
  auto hero_reach_probs{m_prm.get_initial_reach_probs(1, m_init_board)};
  auto villain_reach_probs{m_prm.get_initial_reach_probs(2, m_init_board)};

  for (int i{1}; i <= iterations; ++i) {
    cfr(1, 2, root, i, hero_preflop_combos, villain_preflop_combos,
        hero_reach_probs, villain_reach_probs);
    cfr(2, 1, root, i, villain_preflop_combos, hero_preflop_combos,
        villain_reach_probs, hero_reach_probs);

    if (i % static_cast<int>(iterations / 3) == 0 && i != 0) {
      m_brm.print_exploitability(root, i, m_init_board, m_init_pot,
                                 m_in_position_player);
    }
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
  std::cout << "process took " << duration << '\n';
}

void ParallelDCFR::cfr(const int hero, const int villain, Node *root,
                       const int iteration_count,
                       std::vector<PreflopCombo> &hero_preflop_combos,
                       std::vector<PreflopCombo> &villain_preflop_combos,
                       std::vector<float> &hero_reach_probs,
                       std::vector<float> &villain_reach_probs) {

  CFRHelper rec{root,
                hero,
                villain,
                hero_preflop_combos,
                villain_preflop_combos,
                hero_reach_probs,
                villain_reach_probs,
                m_init_board,
                iteration_count,
                m_rrm};
  rec.compute();
}
