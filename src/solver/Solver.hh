#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"
#include "../tree/Nodes.hh"
#include "hands/PreflopCombo.hh"
#include "trainer/DCFR.hh"

#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_group.h>
#include <vector>

class ParallelDCFR {
  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;

  std::vector<Card> m_init_board;
  int m_init_pot;
  int m_in_position_player;

public:
  ParallelDCFR(const PreflopRangeManager &prm,
               const std::vector<Card> &init_board, const int init_pot,
               const int in_position_player)
      : m_prm(prm), m_init_board(init_board), m_init_pot(init_pot),
        m_in_position_player(in_position_player) {}

  void load_trainer_modules(Node *const node);
  void train(Node *root, const int iterations);
  void cfr(const int hero, const int villain, Node *root,
           const int iteration_count, tbb::task_group &tg);
};

class CFRHelper {
  int m_hero;
  int m_villain;
  Node *m_node;
  std::vector<double> m_hero_reach_probs;
  std::vector<double> m_villain_reach_probs;
  std::vector<Card> m_board;
  std::vector<PreflopCombo> m_hero_preflop_combos;
  std::vector<PreflopCombo> m_villain_preflop_combos;
  int m_num_hero_hands;
  int m_num_villain_hands;
  int m_iteration_count;
  std::vector<double> m_result;
  DCFR m_dcfr_module;

  RiverRangeManager m_rrm;

public:
  CFRHelper(Node *node, const int hero_id, const int villain_id,
            const std::vector<PreflopCombo> &hero_preflop_combos,
            const std::vector<PreflopCombo> &villain_preflop_combos,
            const std::vector<double> &hero_reach_pr,
            const std::vector<double> &villain_reach_pr,
            const std::vector<Card> &board, int iteration_count,
            const RiverRangeManager &rrm)
      : m_hero(hero_id), m_villain(villain_id), m_node(node),
        m_hero_reach_probs(hero_reach_pr),
        m_villain_reach_probs(villain_reach_pr), m_board(board),
        m_hero_preflop_combos(hero_preflop_combos),
        m_villain_preflop_combos(villain_preflop_combos),
        m_num_hero_hands(hero_preflop_combos.size()),
        m_num_villain_hands(villain_preflop_combos.size()),
        m_iteration_count(iteration_count), m_result(m_num_hero_hands),
        m_rrm(rrm) {};

  void compute(tbb::task_group &tg);
  void complete();
  auto get_result() const -> std::vector<double> { return m_result; };

  void chance_node_utility(const ChanceNode *const node,
                           const std::vector<double> &hero_reach_pr,
                           const std::vector<double> &villain_reach_pr,
                           const std::vector<Card> &board, tbb::task_group &tg);

  void action_node_utility(ActionNode *const node,
                           const std::vector<double> &hero_reach_pr,
                           const std::vector<double> &villain_reach_pr,
                           tbb::task_group &tg);

  void terminal_node_utility(const TerminalNode *const node,
                             const std::vector<double> &villain_reach_pr,
                             const std::vector<Card> &board);

  auto get_card_weights(const std::vector<double> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<double>;

  auto get_all_in_utils(const TerminalNode *const node,
                        const std::vector<double> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<double>;

  auto get_showdown_utils(const TerminalNode *const node,
                          const std::vector<double> &villain_reach_pr,
                          const std::vector<Card> &board)
      -> std::vector<double>;

  auto get_uncontested_utils(const TerminalNode *const node,
                             const std::vector<double> &villain_reach_pr,
                             const std::vector<Card> &board)
      -> std::vector<double>;
};
