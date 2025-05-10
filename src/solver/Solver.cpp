#include "Solver.hh"
#include "Helper.hh"
#include "hands/RiverCombo.hh"

void CFRHelper::chance_node_utility(ChanceNode *node,
                                    std::vector<double> &hero_reach_pr,
                                    std::vector<double> &villain_reach_pr,
                                    std::vector<Card> board) {}

auto CFRHelper::get_card_weights(const std::vector<double> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<double> {
  std::vector<double> card_weights(m_num_hero_hands * 52);

  double villain_reach_sum{0.0};
  std::vector<double> villain_card_reach_sum(52);

  for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
    villain_card_reach_sum[m_villain_preflop_combos[hand].hand1] +=
        villain_reach_pr[hand];
    villain_card_reach_sum[m_villain_preflop_combos[hand].hand2] +=
        villain_reach_pr[hand];
    villain_reach_sum += villain_reach_pr[hand];
  }

  for (std::size_t hand{0}; hand < m_hero_preflop_combos.size(); ++hand) {
    if (CardUtility::overlap(m_hero_preflop_combos[hand], board))
      continue;

    double total_weight{0.0};
    for (int card{0}; card < 52; ++card) {
      if (CardUtility::overlap(card, board) ||
          CardUtility::overlap(m_hero_preflop_combos[hand], card)) {
        continue;
      }

      const double weight =
          villain_reach_sum - villain_card_reach_sum[card] -
          villain_card_reach_sum[m_hero_preflop_combos[hand].hand1] -
          villain_card_reach_sum[m_hero_preflop_combos[hand].hand2] +
          villain_reach_pr[hand];
      card_weights[hand + card * m_num_hero_hands] = weight;
      total_weight += weight;
    }

    for (int card{0}; card < 52; ++card) {
      if (CardUtility::overlap(card, board) && total_weight > 0 &&
          card_weights[hand + card * m_num_hero_hands] > 0) {
        card_weights[hand + card * m_num_hero_hands] /= total_weight;
      }
    }
  }

  return card_weights;
}
auto CFRHelper::terminal_node_utility(
    const TerminalNode *node, const std::vector<double> villain_reach_pr,
    const std::vector<Card> &board) -> std::vector<double> {

  switch (node->get_type()) {
  case TerminalNode::ALLIN:
    return get_all_in_utils(node, villain_reach_pr, board);
  case TerminalNode::UNCONTESTED:
    return get_uncontested_utils(node, villain_reach_pr, board);
  case TerminalNode::SHOWDOWN:
    return get_showdown_utils(node, villain_reach_pr, board);
  }
}

// will have to adjust for flop subgame -> probably just use per hand evs
auto CFRHelper::get_all_in_utils(const TerminalNode *node,
                                 const std::vector<double> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<double> {
  std::vector<double> preflop_combo_evs(m_num_hero_hands);
  const auto villain_preflop_combos{m_prm.get_preflop_combos(m_villain)};

  for (int card{0}; card < 52; ++card) {
    if (CardUtility::overlap(card, board))
      continue;

    std::vector<Card> new_board{board};
    new_board.push_back(card);

    std::vector<double> new_villain_reach_probs(m_num_villain_hands);
    for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
      if (!CardUtility::overlap(villain_preflop_combos[hand], new_board)) {
        new_villain_reach_probs[hand] = villain_reach_pr[hand];
      }
    }

    const auto subgame_evs{
        get_showdown_utils(node, new_villain_reach_probs, new_board)};
    for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
      preflop_combo_evs[hand] += subgame_evs[hand] / 44;
    }
  }

  return preflop_combo_evs;
}

auto CFRHelper::get_showdown_utils(const TerminalNode *node,
                                   const std::vector<double> &villain_reach_pr,
                                   const std::vector<Card> &board)
    -> std::vector<double> {
  std::vector<RiverCombo> hero_river_combos{
      m_rrm.get_river_combos(m_hero, m_hero_preflop_combos, board)};
  std::vector<RiverCombo> villain_river_combos{
      m_rrm.get_river_combos(m_hero, m_villain_preflop_combos, board)};

  std::vector<double> utils(m_num_hero_hands);

  double win_sum{0.0};
  std::vector<double> card_win_sum(52);
  double value{node->get_pot() / 2.0};

  std::size_t j{0};
  for (std::size_t i{0}; i < hero_river_combos.size(); ++i) {
    const auto hero_combo{hero_river_combos[i]};
    const auto villain_combo{villain_river_combos[j]};

    while (hero_combo.rank < villain_combo.rank) {
      win_sum += villain_reach_pr[villain_combo.reach_probs_index];
      card_win_sum[villain_combo.hand1] +=
          villain_reach_pr[villain_combo.reach_probs_index];
      card_win_sum[villain_combo.hand2] +=
          villain_reach_pr[villain_combo.reach_probs_index];
      j++;
    }

    utils[hero_combo.reach_probs_index] =
        value * (win_sum - card_win_sum[hero_combo.hand1] -
                 card_win_sum[hero_combo.hand2]);
  }

  double lose_sum{0.0};
  std::vector<double> card_lose_sum(52);
  j = villain_river_combos.size() - 1;
  for (std::size_t i{hero_river_combos.size()}; i != 0; i--) {
    const auto hero_combo{hero_river_combos[i]};
    const auto villain_combo{villain_river_combos[j]};

    while (hero_combo.rank > villain_combo.rank) {

      lose_sum += villain_reach_pr[villain_combo.reach_probs_index];
      card_lose_sum[villain_combo.hand1] +=
          villain_reach_pr[villain_combo.reach_probs_index];
      card_lose_sum[villain_combo.hand2] +=
          villain_reach_pr[villain_combo.reach_probs_index];
      j--;
    }

    utils[hero_combo.reach_probs_index] -=
        value * (lose_sum - card_lose_sum[hero_combo.hand1] -
                 card_lose_sum[hero_combo.hand2]);
  }

  return utils;
}

auto CFRHelper::get_uncontested_utils(
    const TerminalNode *node, const std::vector<double> &villain_reach_pr,
    const std::vector<Card> &board) -> std::vector<double> {
  double villain_reach_sum{0.0};
  std::vector<double> sum_with_card(52);

  for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
    sum_with_card[m_villain_preflop_combos[hand].hand1] +=
        villain_reach_pr[hand];
    sum_with_card[m_villain_preflop_combos[hand].hand2] +=
        villain_reach_pr[hand];

    villain_reach_sum += villain_reach_pr[hand];
  }

  const double value = (m_hero == node->get_last_to_act())
                           ? (-node->get_pot() / 2.0)
                           : (node->get_pot() / 2.0);
  std::vector<double> utils(m_num_hero_hands);
  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    if (CardUtility::overlap(m_hero_preflop_combos[hand], board))
      continue;

    utils[hand] = value * (villain_reach_sum -
                           sum_with_card[m_hero_preflop_combos[hand].hand1] -
                           sum_with_card[m_hero_preflop_combos[hand].hand2] +
                           villain_reach_pr[hand]);
  }

  return utils;
}
