#include "Solver.hh"
#include "hands/PreflopCombo.hh"
#include "tree/Nodes.hh"
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

void ParallelDCFR::train(Node *root, const int iterations,
                         const float min_exploit) {
  tbb::global_control c{tbb::global_control::max_allowed_parallelism,
                        std::thread::hardware_concurrency()};

  load_trainer_modules(root);

  auto hero_preflop_combos{m_prm.get_preflop_combos(1)};
  auto villain_preflop_combos{m_prm.get_preflop_combos(2)};
  auto hero_reach_probs{m_prm.get_initial_reach_probs(1, m_init_board)};
  auto villain_reach_probs{m_prm.get_initial_reach_probs(2, m_init_board)};

  for (int i{1}; i <= iterations; ++i) {
    cfr(1, 2, root, i, hero_preflop_combos, villain_preflop_combos,
        hero_reach_probs, villain_reach_probs);
    cfr(2, 1, root, i, villain_preflop_combos, hero_preflop_combos,
        villain_reach_probs, hero_reach_probs);

    if (i % static_cast<int>(iterations / 5) == 0 && i != 0) {
      float exploit = m_brm.get_exploitability(
          root, i, m_init_board, m_init_pot, m_in_position_player);
      if (exploit < min_exploit)
        return;
    }
  }
}

void ParallelDCFR::cfr(const int hero, const int villain, Node *root,
                       const int iteration_count,
                       std::vector<PreflopCombo> &hero_preflop_combos,
                       std::vector<PreflopCombo> &villain_preflop_combos,
                       std::vector<float> &hero_reach_probs,
                       std::vector<float> &villain_reach_probs) {
  std::vector<int> hero_to_villain(hero_preflop_combos.size(), -1);
  for (int h = 0; h < hero_preflop_combos.size(); ++h) {
    auto &hc = hero_preflop_combos[h];

    for (int v = 0; v < villain_preflop_combos.size(); ++v) {
      auto &vc = villain_preflop_combos[v];

      if (hc == vc) {
        hero_to_villain[h] = v;
        break;
      }
    }
  }

  CFRHelper rec{root,
                hero,
                villain,
                hero_preflop_combos,
                villain_preflop_combos,
                hero_reach_probs,
                villain_reach_probs,
                m_init_board,
                iteration_count,
                m_rrm,
                hero_to_villain};
  rec.compute();
}
