#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"
#include "../tree/Nodes.hh"
#include "hands/PreflopCombo.hh"
#include "trainer/DCFR.hh"

enum class ResultType { CHANCE_NODE, ACTION_NODE };

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

  void load_trainer_modules(Node *node);
  void train(Node *node, const int iterations);
  auto cfr(const int hero, const int villain, Node *root, int iteration_count)
      -> std::vector<double>;
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
  std::vector<CFRHelper> m_cfr_helpers;
  DCFR m_dcfr_module;
  ResultType m_result_type;

  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;

public:
  CFRHelper(Node *node, const int hero_id, const int villain_id,
            const std::vector<PreflopCombo> &hero_preflop_combos,
            const std::vector<PreflopCombo> &villain_preflop_combos,
            const std::vector<double> &hero_reach_pr,
            const std::vector<double> &villain_reach_pr,
            const std::vector<Card> &board, int iteration_count,
            PreflopRangeManager prm, RiverRangeManager rrm)
      : m_hero(hero_id), m_villain(villain_id), m_node(node),
        m_hero_reach_probs(hero_reach_pr),
        m_villain_reach_probs(villain_reach_pr), m_board(board),
        m_hero_preflop_combos(hero_preflop_combos),
        m_villain_preflop_combos(villain_preflop_combos),
        m_iteration_count(iteration_count), m_prm(prm), m_rrm(rrm) {};

  void compute();
  void complete();

  void chance_node_utility(ChanceNode *node,
                           const std::vector<double> &hero_reach_pr,
                           const std::vector<double> &villain_reach_pr,
                           const std::vector<Card> &board);

  auto get_card_weights(const std::vector<double> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<double>;

  auto terminal_node_utility(const TerminalNode *node,
                             const std::vector<double> &villain_reach_pr,
                             const std::vector<Card> &board)
      -> std::vector<double>;
  auto get_all_in_utils(const TerminalNode *node,
                        const std::vector<double> &villain_reach_pr,
                        const std::vector<Card> &board) -> std::vector<double>;

  auto get_showdown_utils(const TerminalNode *node,
                          const std::vector<double> &villain_reach_pr,
                          const std::vector<Card> &board)
      -> std::vector<double>;

  auto get_uncontested_utils(const TerminalNode *node,
                             const std::vector<double> &villain_reach_pr,
                             const std::vector<Card> &board)
      -> std::vector<double>;
};
