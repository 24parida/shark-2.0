// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"
#include "../tree/Nodes.hh"
#include "hands/PreflopCombo.hh"
#include "trainer/DCFR.hh"

#include "BestResponse.hh"
#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <vector>
#include <functional>

using ProgressCallback = std::function<void(int, int, float)>;

class ParallelDCFR {
  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;
  BestResponse m_brm;

  std::vector<Card> m_init_board;
  int m_init_pot;
  int m_in_position_player;
  int m_thread_count;

  std::vector<int> m_p1_to_p2;
  std::vector<int> m_p2_to_p1;

public:
  ParallelDCFR(const PreflopRangeManager &prm,
               const std::vector<Card> &init_board, int init_pot,
               int in_position_player, int thread_count = 0)
      : m_prm(prm), m_brm(prm), m_init_board(init_board), m_init_pot(init_pot),
        m_in_position_player(in_position_player), m_thread_count(thread_count) {}

  void load_trainer_modules(Node *const node);
  void precompute_combo_mappings();
  void reset_cumulative_strategies(Node *const node);
  void train(Node *root, const int iterations, const float min_explot = -1.0, ProgressCallback progress_cb = nullptr);

  void cfr(const int hero, const int villain, Node *root,
           const int iteration_count,
           std::vector<PreflopCombo> &hero_preflop_combos,
           std::vector<PreflopCombo> &villain_preflop_combos,
           std::vector<float> &hero_reach_probs,
           std::vector<float> &villain_reach_probs);
};

class CFRHelper {
  int m_hero;
  int m_villain;
  Node *m_node;
  std::vector<float> &m_hero_reach_probs;
  std::vector<float> &m_villain_reach_probs;
  std::vector<Card> &m_board;
  std::vector<PreflopCombo> &m_hero_preflop_combos;
  std::vector<PreflopCombo> &m_villain_preflop_combos;
  int m_num_hero_hands;
  int m_num_villain_hands;
  int m_iteration_count;
  std::vector<float> m_result;
  DCFR m_dcfr_module;

  RiverRangeManager &m_rrm;
  std::vector<int> &m_hero_to_villain;

  std::array<std::array<int, 52>, 52> m_villain_combo_index;
  bool m_combo_index_initialized;

public:
  CFRHelper(Node *node, const int hero_id, const int villain_id,
            std::vector<PreflopCombo> &hero_preflop_combos,
            std::vector<PreflopCombo> &villain_preflop_combos,
            std::vector<float> &hero_reach_pr,
            std::vector<float> &villain_reach_pr, std::vector<Card> &board,
            int iteration_count, RiverRangeManager &rrm,
            std::vector<int> &hero_to_villain)
      : m_hero(hero_id), m_villain(villain_id), m_node(node),
        m_hero_reach_probs(hero_reach_pr),
        m_villain_reach_probs(villain_reach_pr), m_board(board),
        m_hero_preflop_combos(hero_preflop_combos),
        m_villain_preflop_combos(villain_preflop_combos),
        m_num_hero_hands(hero_preflop_combos.size()),
        m_num_villain_hands(villain_preflop_combos.size()),
        m_iteration_count(iteration_count), m_result(m_num_hero_hands),
        m_rrm(rrm), m_hero_to_villain(hero_to_villain),
        m_combo_index_initialized(false) {};

  void compute();
  auto get_result() const -> std::vector<float> { return m_result; };

  void initialize_combo_index();

  auto get_isomorphic_card_groups(const std::vector<Card> &board,
                                  const ChanceNode *node)
      -> std::vector<std::vector<int>>;

  void chance_node_utility(const ChanceNode *const node,
                           const std::vector<float> &hero_reach_pr,
                           const std::vector<float> &villain_reach_pr,
                           const std::vector<Card> &board);

  void action_node_utility(ActionNode *const node,
                           const std::vector<float> &hero_reach_pr,
                           const std::vector<float> &villain_reach_pr);

  void terminal_node_utility(const TerminalNode *const node,
                             const std::vector<float> &villain_reach_pr,
                             const std::vector<Card> &board);

  auto get_card_weights(const std::vector<float> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<float>;

  auto get_all_in_utils(const TerminalNode *const node,
                        const std::vector<float> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<float>;

  auto get_showdown_utils(const TerminalNode *const node,
                          const std::vector<float> &villain_reach_pr,
                          const std::vector<Card> &board) -> std::vector<float>;

  auto get_uncontested_utils(const TerminalNode *const node,
                             const std::vector<float> &villain_reach_pr,
                             const std::vector<Card> &board)
      -> std::vector<float>;
};
