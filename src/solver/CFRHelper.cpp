#include "Helper.hh"
#include "Solver.hh"
#include "hands/PreflopCombo.hh"
#include "hands/RiverCombo.hh"
#include "tree/Nodes.hh"

void CFRHelper::compute(tbb::task_group &tg) {
  if (m_node->get_node_type() == NodeType::ACTION_NODE) {
    action_node_utility(static_cast<ActionNode *>(m_node), m_hero_reach_probs,
                        m_villain_reach_probs, tg);
  } else if (m_node->get_node_type() == NodeType::CHANCE_NODE) {
    chance_node_utility(static_cast<ChanceNode *>(m_node), m_hero_reach_probs,
                        m_villain_reach_probs, m_board, tg);
  } else {
    terminal_node_utility(static_cast<TerminalNode *>(m_node),
                          m_villain_reach_probs, m_board);
  }
}

void CFRHelper::action_node_utility(ActionNode *const node,
                                    const std::vector<double> &hero_reach_pr,
                                    const std::vector<double> &villain_reach_pr,
                                    tbb::task_group &tg) {
  const auto &strategy{node->get_current_strat()};
  const int player{node->get_player()};
  const int num_actions{node->get_num_actions()};
  std::vector<std::vector<double>> subgame_utils(num_actions);

  for (std::size_t action{0}; action < num_actions; ++action) {
    tg.run([&, action]() {
      std::vector<double> new_hero_reach_probs(hero_reach_pr);
      std::vector<double> new_villain_reach_probs(villain_reach_pr);

      if (player == m_hero) {
        for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
          new_hero_reach_probs[hand] =
              strategy[hand + action * m_num_hero_hands] * hero_reach_pr[hand];
        }
      } else {
        for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
          new_villain_reach_probs[hand] =
              strategy[hand + action * m_num_villain_hands] *
              villain_reach_pr[hand];
        }
      }
      CFRHelper rec{node->get_child(action),
                    m_hero,
                    m_villain,
                    m_hero_preflop_combos,
                    m_villain_preflop_combos,
                    new_hero_reach_probs,
                    new_villain_reach_probs,
                    m_board,
                    m_iteration_count,
                    m_rrm};
      rec.compute(tg);
      subgame_utils[action] = rec.get_result();
    });
  }
  tg.wait();

  if (player != m_hero) {
    for (auto &i : subgame_utils) {
      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        m_result[hand] += i[hand];
      }
    }
  } else {
    auto *trainer{node->get_trainer()};
    for (std::size_t action{0}; action < num_actions; ++action) {
      trainer->update_cum_regret_one(subgame_utils[action], action);
      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        m_result[hand] += subgame_utils[action][hand] *
                          strategy[hand + action * m_num_hero_hands];
      }
    }

    trainer->update_cum_regret_two(m_result, m_iteration_count);
    trainer->update_cum_strategy(strategy, hero_reach_pr, m_iteration_count);
  }
};

void CFRHelper::chance_node_utility(const ChanceNode *node,
                                    const std::vector<double> &hero_reach_pr,
                                    const std::vector<double> &villain_reach_pr,
                                    const std::vector<Card> &board,
                                    tbb::task_group &tg) {
  const auto card_weights{get_card_weights(villain_reach_pr, board)};
  const int num_children(node->get_num_children());
  std::vector<std::vector<double>> subgame_utils(num_children);

  int count{0};

  for (int card{0}; card < 52; ++card) {
    Node *child{node->get_child(card)};
    if (!child)
      continue;

    tg.run([&, count, card, card_weights]() {
      std::vector<Card> new_board{board};
      new_board.push_back(card);

      std::vector<double> new_hero_reach_probs(m_num_hero_hands);
      std::vector<double> new_villain_reach_probs(m_num_villain_hands);

      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        if (!CardUtility::overlap(m_hero_preflop_combos[hand], card)) {
          new_hero_reach_probs[hand] =
              hero_reach_pr[hand] *
              card_weights[hand + card * m_num_hero_hands];
        }
      }

      for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
        if (!CardUtility::overlap(m_villain_preflop_combos[hand], card)) {
          new_villain_reach_probs[hand] = villain_reach_pr[hand];
        }
      }

      CFRHelper rec{child,
                    m_hero,
                    m_villain,
                    m_hero_preflop_combos,
                    m_villain_preflop_combos,
                    new_hero_reach_probs,
                    new_villain_reach_probs,
                    new_board,
                    m_iteration_count,
                    m_rrm};
      rec.compute(tg);
      subgame_utils[count] = rec.get_result();
    });
    ++count;
  }
  tg.wait();

  for (auto &i : subgame_utils) {
    for (std::size_t h{0}; h < m_num_hero_hands; ++h) {
      m_result[h] += i[h] / static_cast<double>(num_children);
    }
  }
}

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
      if (!CardUtility::overlap(card, board) && total_weight > 0 &&
          card_weights[hand + card * m_num_hero_hands] > 0) {
        card_weights[hand + card * m_num_hero_hands] /= total_weight;
      }
    }
  }

  return card_weights;
}

void CFRHelper::terminal_node_utility(
    const TerminalNode *const node, const std::vector<double> &villain_reach_pr,
    const std::vector<Card> &board) {

  switch (node->get_type()) {
  case TerminalNode::ALLIN:
    m_result = get_all_in_utils(node, villain_reach_pr, board);
    break;
  case TerminalNode::UNCONTESTED:
    m_result = get_uncontested_utils(node, villain_reach_pr, board);
    break;
  case TerminalNode::SHOWDOWN:
    m_result = get_showdown_utils(node, villain_reach_pr, board);
    break;
  }
}

auto CFRHelper::get_all_in_utils(const TerminalNode *node,
                                 const std::vector<double> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<double> {
  std::vector<double> preflop_combo_evs(m_num_hero_hands);

  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    const PreflopCombo &hero_combo{m_hero_preflop_combos[hand]};

    if (CardUtility::overlap(hero_combo, board))
      continue;

    double ev_sum{0.0};

    for (std::size_t v{0}; v < m_num_villain_hands; ++v) {
      const PreflopCombo &villain_combo{m_villain_preflop_combos[v]};
      const double villain_reach{villain_reach_pr[v]};

      if (villain_reach == 0 || CardUtility::overlap(villain_combo, board) ||
          CardUtility::overlap(hero_combo, board)) {
        continue;
      }

      double win_pct{
          CardUtility::get_win_pct(hero_combo, villain_combo, board)};
      const double util{(2 * win_pct - 1) * (node->get_pot() / 2.0)};
      ev_sum += util * villain_reach;
    }

    preflop_combo_evs[hand] = ev_sum;
  }

  return preflop_combo_evs;
}

auto CFRHelper::get_showdown_utils(const TerminalNode *node,
                                   const std::vector<double> &villain_reach_pr,
                                   const std::vector<Card> &board)
    -> std::vector<double> {
  const std::vector<RiverCombo> &hero_river_combos{
      m_rrm.get_river_combos(m_hero, m_hero_preflop_combos, board)};
  const std::vector<RiverCombo> &villain_river_combos{
      m_rrm.get_river_combos(m_villain, m_villain_preflop_combos, board)};

  std::vector<double> utils(m_num_hero_hands);

  double win_sum{0.0};
  const double value{node->get_pot() / 2.0};
  std::vector<double> card_win_sum(52);

  int j{0};
  for (std::size_t i{0}; i < hero_river_combos.size(); ++i) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j < villain_river_combos.size() &&
           hero_combo.rank < villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};
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
  j = static_cast<int>(villain_river_combos.size()) - 1;
  for (int i{static_cast<int>(hero_river_combos.size()) - 1}; i >= 0; i--) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j >= 0 && hero_combo.rank > villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};

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
