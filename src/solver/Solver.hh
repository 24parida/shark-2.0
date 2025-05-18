#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"
#include "../tree/Nodes.hh"
#include "hands/PreflopCombo.hh"
#include "trainer/DCFR.hh"

#include <oneapi/tbb/global_control.h>
#include <oneapi/tbb/parallel_for.h>
#include <vector>

class ParallelDCFR {
  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;

  std::vector<Card> m_init_board;
  int m_in_position_player;

public:
  ParallelDCFR(const PreflopRangeManager &prm,
               const std::vector<Card> &init_board)
      : m_prm(prm), m_init_board(init_board) {}

  void load_trainer_modules(Node *const node);
  void train(Node *root, const int iterations);

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

public:
  CFRHelper(Node *node, const int hero_id, const int villain_id,
            std::vector<PreflopCombo> &hero_preflop_combos,
            std::vector<PreflopCombo> &villain_preflop_combos,
            std::vector<float> &hero_reach_pr,
            std::vector<float> &villain_reach_pr, std::vector<Card> &board,
            int iteration_count, RiverRangeManager &rrm)
      : m_hero(hero_id), m_villain(villain_id), m_node(node),
        m_hero_reach_probs(hero_reach_pr),
        m_villain_reach_probs(villain_reach_pr), m_board(board),
        m_hero_preflop_combos(hero_preflop_combos),
        m_villain_preflop_combos(villain_preflop_combos),
        m_num_hero_hands(hero_preflop_combos.size()),
        m_num_villain_hands(villain_preflop_combos.size()),
        m_iteration_count(iteration_count), m_result(m_num_hero_hands),
        m_rrm(rrm) {};

  void compute();
  auto get_result() const -> std::vector<float> { return m_result; };

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
