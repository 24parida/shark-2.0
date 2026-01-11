#include "../hands/PreflopRangeManager.hh"
#include "../hands/RiverRangeManager.hh"
#include "../tree/Nodes.hh"
#include "hands/PreflopCombo.hh"
class BestResponse {
  PreflopRangeManager m_prm;
  RiverRangeManager m_rrm;

  std::vector<PreflopCombo> m_hero_preflop_combos;
  std::vector<PreflopCombo> m_villain_preflop_combos;
  std::vector<int> m_hero_to_villain;
  int m_hero;
  int m_villain;
  int m_num_hero_hands;
  int m_num_villain_hands;

public:
  BestResponse(const PreflopRangeManager &prm) : m_prm(prm) {};
  float get_best_response_ev(Node *node, int hero, int villain,
                             const std::vector<PreflopCombo> &hero_combos,
                             const std::vector<PreflopCombo> &villain_combos,
                             const std::vector<Card> &board,
                             const std::vector<int> &hero_to_villain);

  float get_exploitability(Node *node, int iteration_count,
                           const std::vector<Card> &board, int init_pot,
                           int in_position_player);

  auto best_response(Node *node, const std::vector<float> &villain_reach_probs,
                     const std::vector<Card> &board) -> std::vector<float>;

  auto action_best_response(ActionNode *node,
                            const std::vector<float> &villain_reach_probs,
                            const std::vector<Card> &board)
      -> std::vector<float>;

  auto chance_best_response(ChanceNode *node,
                            const std::vector<float> &villain_reach_probs,
                            const std::vector<Card> &board)
      -> std::vector<float>;

  auto terminal_best_response(TerminalNode *node,
                              const std::vector<float> &villain_reach_probs,
                              const std::vector<Card> &board)
      -> std::vector<float>;

  auto all_in_best_response(TerminalNode *node,
                            const std::vector<float> &villain_reach_probs,
                            const std::vector<Card> &board)
      -> std::vector<float>;

  auto show_down_best_response(TerminalNode *node,
                               const std::vector<float> &villain_reach_probs,
                               const std::vector<Card> &board)
      -> std::vector<float>;

  auto uncontested_best_response(TerminalNode *node,
                                 const std::vector<float> &villain_reach_probs,
                                 const std::vector<Card> &board)
      -> std::vector<float>;
};
