#include "BestResponse.hh"
#include "../Helper.hh"
#include "hands/PreflopCombo.hh"
#include "tree/Nodes.hh"
#include <iostream>

float BestResponse::get_best_response_ev(
    Node *node, int hero, int villain,
    const std::vector<PreflopCombo> &hero_combos,
    const std::vector<PreflopCombo> &villain_combos,
    const std::vector<Card> &board, const std::vector<int> &hero_to_villain) {
  m_hero = hero;
  m_villain = villain;
  m_hero_preflop_combos = hero_combos;
  m_villain_preflop_combos = villain_combos;
  m_num_hero_hands = hero_combos.size();
  m_num_villain_hands = villain_combos.size();
  m_hero_to_villain = hero_to_villain;

  float ev{0.0};

  auto preflop_combo_evs{best_response(
      node, m_prm.get_initial_reach_probs(villain, board), board)};
  auto unblocked_combo_counts{
      get_unblocked_combo_counts(hero_combos, villain_combos, board)};

  for (int i = 0; i < m_num_hero_hands; ++i) {
    if (!CardUtility::overlap(hero_combos[i], board)) {
      ev += preflop_combo_evs[i] / unblocked_combo_counts[i] *
            hero_combos[i].rel_probability;
    }
  }
  return ev;
}

void BestResponse::print_exploitability(Node *node, int iteration_count,
                                        const std::vector<Card> &board,
                                        int init_pot, int in_position_player) {

  const int p1{in_position_player == 2 ? 1 : 2};
  const int p2{in_position_player == 2 ? 2 : 1};

  auto p1_combos{m_prm.get_preflop_combos(p1)};
  auto p2_combos{m_prm.get_preflop_combos(p2)};

  std::vector<int> p1_to_p2(p1_combos.size(), -1);
  std::vector<int> p2_to_p1(p2_combos.size(), -1);

  for (int h = 0; h < p1_combos.size(); ++h) {
    auto &hc = p1_combos[h];

    for (int v = 0; v < p2_combos.size(); ++v) {
      auto &vc = p2_combos[v];

      if (hc == vc) {
        p1_to_p2[h] = v;
        break;
      }
    }
  }

  for (int h = 0; h < p2_combos.size(); ++h) {
    auto &hc = p2_combos[h];

    for (int v = 0; v < p1_combos.size(); ++v) {
      auto &vc = p1_combos[v];

      if (hc == vc) {
      }
    }
  }

  float oop_ev{get_best_response_ev(node, p1, p2, p1_combos, p2_combos, board,
                                    p1_to_p2)};
  float ip_ev{get_best_response_ev(node, p2, p1, p2_combos, p1_combos, board,
                                   p2_to_p1)};

  float exploitability{(oop_ev + ip_ev) / 2 / init_pot * 100};
  std::cout << "-------------------------------------------" << '\n';
  std::cout << "OOP BEST RESPONSE EV: " << oop_ev << '\n';
  std::cout << "IP BEST RESPONSE EV: " << ip_ev << '\n';
  std::cout << "exploitability at iteration " << iteration_count << " is "
            << exploitability << "% of the pot per hand" << '\n';
  std::cout << "-------------------------------------------" << '\n';
}

auto BestResponse::get_unblocked_combo_counts(
    const std::vector<PreflopCombo> &hero_combos,
    const std::vector<PreflopCombo> &villain_combos,
    const std::vector<Card> &board) -> std::vector<float> {
  std::vector<float> combo_counts(hero_combos.size());

  for (int hero_hand = 0; hero_hand < hero_combos.size(); ++hero_hand) {
    const PreflopCombo &combo{hero_combos[hero_hand]};

    if (CardUtility::overlap(combo, board))
      continue;

    float sum{0};
    for (const auto &villain_combo : villain_combos) {
      if (!CardUtility::overlap(combo, villain_combo) &&
          !CardUtility::overlap(villain_combo, board)) {
        sum += villain_combo.probability;
      }
    }
    combo_counts[hero_hand] = sum;
  }
  return combo_counts;
}

auto BestResponse::best_response(Node *node,
                                 const std::vector<float> &villain_reach_probs,
                                 const std::vector<Card> &board)
    -> std::vector<float> {
  switch (node->get_node_type()) {
  case NodeType::ACTION_NODE:
    return action_best_response(static_cast<ActionNode *>(node),
                                villain_reach_probs, board);
  case NodeType::CHANCE_NODE:
    return chance_best_response(dynamic_cast<ChanceNode *>(node),
                                villain_reach_probs, board);
  case NodeType::TERMINAL_NODE:

    return terminal_best_response(dynamic_cast<TerminalNode *>(node),
                                  villain_reach_probs, board);
  }
}

auto BestResponse::action_best_response(
    ActionNode *node, const std::vector<float> &villain_reach_probs,
    const std::vector<Card> &board) -> std::vector<float> {
  if (m_hero == node->get_player()) {
    std::vector<float> max_action_evs(m_num_hero_hands);

    for (int action = 0; action < node->get_num_actions(); ++action) {
      std::vector<float> action_evs{
          best_response(node->get_child(action), villain_reach_probs, board)};

      for (int hand = 0; hand < m_num_hero_hands; ++hand) {
        if (action == 0 || action_evs[hand] > max_action_evs[hand])
          max_action_evs[hand] = action_evs[hand];
      }
    }

    return max_action_evs;
  } else {
    std::vector<float> cum_subgame_evs(m_num_hero_hands);
    auto avg_strat = node->get_average_strat();
    for (int action = 0; action < node->get_num_actions(); ++action) {
      std::vector<float> new_villain_reach_probs(m_num_villain_hands);
      for (int hand = 0; hand < m_num_villain_hands; ++hand) {
        new_villain_reach_probs[hand] =
            avg_strat[hand + action * m_num_villain_hands] *
            villain_reach_probs[hand];
      }

      std::vector<float> subgame_evs{best_response(
          node->get_child(action), new_villain_reach_probs, board)};

      for (int hand = 0; hand < m_num_hero_hands; ++hand) {
        cum_subgame_evs[hand] += subgame_evs[hand];
      }
    }
    return cum_subgame_evs;
  }
}

auto BestResponse::chance_best_response(
    ChanceNode *node, const std::vector<float> &villain_reach_probs,
    const std::vector<Card> &board) -> std::vector<float> {
  std::vector<float> preflop_combo_evs(m_num_hero_hands);
  for (int card = 0; card < 52; ++card) {
    auto child = node->get_child(card);

    if (!child)
      continue;

    auto new_board{board};
    new_board.push_back(card);

    std::vector<float> new_villain_reach_probs(m_num_villain_hands);

    for (int hand = 0; hand < m_num_villain_hands; ++hand) {
      if (!CardUtility::overlap(m_villain_preflop_combos[hand], card)) {
        new_villain_reach_probs[hand] = villain_reach_probs[hand];
      }
    }

    std::vector<float> subgame_evs{
        best_response(child, new_villain_reach_probs, new_board)};

    const int num_children{node->get_num_children()};
    for (int hand = 0; hand < m_num_hero_hands; ++hand) {
      preflop_combo_evs[hand] += subgame_evs[hand] / num_children;
    }
  }
  return preflop_combo_evs;
}

auto BestResponse::terminal_best_response(
    TerminalNode *node, const std::vector<float> &villain_reach_probs,
    const std::vector<Card> &board) -> std::vector<float> {
  if (node->get_type() == TerminalNode::TerminalType::ALLIN) {
    return all_in_best_response(node, villain_reach_probs, board);
  } else if (node->get_type() == TerminalNode::TerminalType::UNCONTESTED) {
    return uncontested_best_response(node, villain_reach_probs, board);
  } else {
    return show_down_best_response(node, villain_reach_probs, board);
  }
}

auto BestResponse::all_in_best_response(
    TerminalNode *node, const std::vector<float> &villain_reach_probs,
    const std::vector<Card> &board) -> std::vector<float> {
  if (board.size() == 5) {
    return show_down_best_response(node, villain_reach_probs, board);
  }

  std::vector<float> preflop_combo_evs(m_num_hero_hands);
  for (int card = 0; card < 52; ++card) {
    if (CardUtility::overlap(card, board))
      continue;

    auto new_board{board};
    new_board.push_back(card);

    std::vector<float> new_villain_reach_probs(m_num_villain_hands);
    for (int hand = 0; hand < m_num_villain_hands; ++hand) {
      if (!CardUtility::overlap(m_villain_preflop_combos[hand], card))
        new_villain_reach_probs[hand] = villain_reach_probs[hand];
    }

    const auto subgame_evs{
        all_in_best_response(node, new_villain_reach_probs, new_board)};

    const float normalizing_sum{static_cast<float>(52 - 2 - 2 - board.size())};
    for (int hand = 0; hand < m_num_hero_hands; ++hand)
      preflop_combo_evs[hand] += subgame_evs[hand] / normalizing_sum;
  }

  return preflop_combo_evs;
}

auto BestResponse::show_down_best_response(
    TerminalNode *node, const std::vector<float> &villain_reach_probs,
    const std::vector<Card> &board) -> std::vector<float> {
  const std::vector<RiverCombo> &hero_river_combos{
      m_rrm.get_river_combos(m_hero, m_hero_preflop_combos, board)};
  const std::vector<RiverCombo> &villain_river_combos{
      m_rrm.get_river_combos(m_villain, m_villain_preflop_combos, board)};

  std::vector<float> utils(m_num_hero_hands);

  float win_sum{0.0};
  const float value{static_cast<float>(node->get_pot() / 2.0)};
  std::vector<float> card_win_sum(52);

  int j{0};
  for (std::size_t i{0}; i < hero_river_combos.size(); ++i) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j < villain_river_combos.size() &&
           hero_combo.rank > villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};
      win_sum += villain_reach_probs[villain_combo.reach_probs_index];
      card_win_sum[villain_combo.hand1] +=
          villain_reach_probs[villain_combo.reach_probs_index];
      card_win_sum[villain_combo.hand2] +=
          villain_reach_probs[villain_combo.reach_probs_index];
      j++;
    }

    utils[hero_combo.reach_probs_index] =
        value * (win_sum - card_win_sum[hero_combo.hand1] -
                 card_win_sum[hero_combo.hand2]);
  }

  float lose_sum{0.0};
  std::vector<float> card_lose_sum(52);
  j = static_cast<int>(villain_river_combos.size()) - 1;
  for (int i{static_cast<int>(hero_river_combos.size()) - 1}; i >= 0; i--) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j >= 0 && hero_combo.rank < villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};

      lose_sum += villain_reach_probs[villain_combo.reach_probs_index];
      card_lose_sum[villain_combo.hand1] +=
          villain_reach_probs[villain_combo.reach_probs_index];
      card_lose_sum[villain_combo.hand2] +=
          villain_reach_probs[villain_combo.reach_probs_index];
      j--;
    }

    utils[hero_combo.reach_probs_index] -=
        value * (lose_sum - card_lose_sum[hero_combo.hand1] -
                 card_lose_sum[hero_combo.hand2]);
  }

  return utils;
}

auto BestResponse::uncontested_best_response(
    TerminalNode *node, const std::vector<float> &villain_reach_pr,
    const std::vector<Card> &board) -> std::vector<float> {
  float villain_reach_sum{0.0};
  std::vector<float> sum_with_card(52);

  for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
    sum_with_card[m_villain_preflop_combos[hand].hand1] +=
        villain_reach_pr[hand];
    sum_with_card[m_villain_preflop_combos[hand].hand2] +=
        villain_reach_pr[hand];

    villain_reach_sum += villain_reach_pr[hand];
  }

  const float value = (m_hero == node->get_last_to_act())
                          ? (-node->get_pot() / 2.0f)
                          : (node->get_pot() / 2.0f);
  std::vector<float> utils(m_num_hero_hands);
  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    if (CardUtility::overlap(m_hero_preflop_combos[hand], board))
      continue;

    int v{m_hero_to_villain[hand]};
    float v_weight = v >= 0 ? villain_reach_pr[v] : 0.0f;

    utils[hand] =
        value *
        (villain_reach_sum - sum_with_card[m_hero_preflop_combos[hand].hand1] -
         sum_with_card[m_hero_preflop_combos[hand].hand2] + v_weight);
  }

  return utils;
}
