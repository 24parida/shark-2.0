#include "PreflopCombo.hh"
#include <cassert>

class PreflopRangeManager {
  std::vector<PreflopCombo> m_p1_preflop_combos;
  std::vector<PreflopCombo> m_p2_preflop_combos;

public:
  PreflopRangeManager(const std::vector<PreflopCombo> &p1_preflop_combos,
                      const std::vector<PreflopCombo> &p2_preflop_combos,
                      const std::vector<Card> &init_board)

      : m_p1_preflop_combos(p1_preflop_combos),
        m_p2_preflop_combos(p2_preflop_combos) {
    set_rel_probabilities(init_board);
  }
  auto get_num_hands(const int player_id) const -> int {
    assert((player_id == 1 || player_id == 2) &&
           "PreflopRangeManager get_num_hands invalid player_id");

    if (player_id == 1)
      return m_p1_preflop_combos.size();
    return m_p2_preflop_combos.size();
  }

  auto get_preflop_combos(const int player_id) const
      -> std::vector<PreflopCombo> {
    assert((player_id == 1 || player_id == 2) &&
           "PreflopRangeManager get_num_hands invalid player_id");

    if (player_id == 1)
      return m_p1_preflop_combos;
    return m_p2_preflop_combos;
  }

  auto get_initial_reach_probs(const int player,
                               const std::vector<Card> &board) const
      -> std::vector<double>;

  void set_rel_probabilities(const std::vector<Card> &init_board);
};
