#include "Solver.hh"
#include "tree/Nodes.hh"
#include <chrono>
#include <iostream>
#include <oneapi/tbb/task_group.h>
#include <tbb/global_control.h>

void ParallelDCFR::load_trainer_modules(Node *const node) {
  if (node->get_node_type() == NodeType::ACTION_NODE) {
    auto *action_node = static_cast<ActionNode *>(node);
    action_node->load_trainer(action_node);
    for (int i{0}; i < action_node->get_num_actions(); ++i) {
      load_trainer_modules(action_node->get_child(i));
    }
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
  tbb::global_control c{tbb::global_control::max_allowed_parallelism, 1};
  tbb::task_group tg;

  load_trainer_modules(root);

  const auto start = std::chrono::high_resolution_clock::now();

  for (int i{0}; i < iterations; ++i) {
    cfr(1, 2, root, i, tg);
    cfr(2, 1, root, i, tg);
  }

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(start - end).count();
  std::cout << "process took " << duration << '\n';
}

void ParallelDCFR::cfr(const int hero, const int villain, Node *root,
                       const int iteration_count, tbb::task_group &tg) {

  const auto &hero_preflop_combos{m_prm.get_preflop_combos(hero)};
  const auto &villain_preflop_combos{m_prm.get_preflop_combos(villain)};

  const auto &hero_reach_probs{
      m_prm.get_initial_reach_probs(hero, m_init_board)};
  const auto &villain_reach_probs{
      m_prm.get_initial_reach_probs(villain, m_init_board)};

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
  rec.compute(tg);
}
