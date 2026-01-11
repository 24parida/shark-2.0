// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#include "BestResponse.hh"
#include "../Helper.hh"
#include "hands/PreflopCombo.hh"
#include "tree/Nodes.hh"
#include "Isomorphism.hh"
#include <iostream>
#include <array>

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

  const uint64_t board_mask = CardUtility::board_to_mask(board);

  double num_combinations = 0.0;
  for (int h = 0; h < m_num_hero_hands; ++h) {
    if (CardUtility::overlap_mask(hero_combos[h], board_mask))
      continue;
    for (int v = 0; v < m_num_villain_hands; ++v) {
      if (CardUtility::overlap_mask(villain_combos[v], board_mask))
        continue;
      if (!CardUtility::overlap(hero_combos[h], villain_combos[v])) {
        num_combinations += static_cast<double>(hero_combos[h].probability) *
                           static_cast<double>(villain_combos[v].probability);
      }
    }
  }

  auto preflop_combo_evs{best_response(
      node, m_prm.get_initial_reach_probs(villain, board), board)};

  double weighted_cfv_sum = 0.0;
  for (int i = 0; i < m_num_hero_hands; ++i) {
    if (!CardUtility::overlap_mask(hero_combos[i], board_mask)) {
      weighted_cfv_sum += static_cast<double>(preflop_combo_evs[i]) *
                          static_cast<double>(hero_combos[i].probability);
    }
  }

  float ev = static_cast<float>(weighted_cfv_sum / num_combinations);

  return ev;
}

float BestResponse::get_exploitability(Node *node, int iteration_count,
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
        p2_to_p1[h] = v;
        break;
      }
    }
  }

  float oop_ev{get_best_response_ev(node, p1, p2, p1_combos, p2_combos, board,
                                    p1_to_p2)};
  float ip_ev{get_best_response_ev(node, p2, p1, p2_combos, p1_combos, board,
                                   p2_to_p1)};

  float exploitability_chips = std::max(0.0f, (oop_ev + ip_ev) * 0.5f);
  float exploitability_pct = exploitability_chips / init_pot * 100.0f;

  return exploitability_pct;
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
    return chance_best_response(static_cast<ChanceNode *>(node),
                                villain_reach_probs, board);
  case NodeType::TERMINAL_NODE:
    return terminal_best_response(static_cast<TerminalNode *>(node),
                                  villain_reach_probs, board);
  }
  return std::vector<float>(m_num_hero_hands, 0.0f);
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
    std::vector<float> avg_strat(m_num_villain_hands * node->get_num_actions());
    node->get_trainer()->get_average_strat(avg_strat);

    std::vector<float> new_villain_reach_probs(m_num_villain_hands);

    for (int action = 0; action < node->get_num_actions(); ++action) {
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
  const uint64_t board_mask = CardUtility::board_to_mask(board);
  const auto& iso_data = node->get_isomorphism_data();

  int num_rep_cards = 0;
  for (int card = 0; card < 52; ++card) {
    if (!((1ULL << card) & board_mask) && node->get_child(card)) {
      num_rep_cards++;
    }
  }
  const int num_iso_cards = static_cast<int>(iso_data.isomorphism_card.size());
  const int chance_factor = num_rep_cards + num_iso_cards;
  const float reach_scale = 1.0f / static_cast<float>(chance_factor);

  std::vector<float> preflop_combo_evs(m_num_hero_hands, 0.0f);
  std::vector<float> new_villain_reach_probs(m_num_villain_hands);
  auto new_board{board};
  new_board.reserve(board.size() + 1);

  for (int card = 0; card < 52; ++card) {
    if ((1ULL << card) & board_mask)
      continue;

    Node* child = node->get_child(card);
    bool is_isomorphic = (child == nullptr);

    if (is_isomorphic) {
      int suit = card & 3;
      int rank = card >> 2;

      for (int rep_suit = 0; rep_suit < suit; ++rep_suit) {
        int rep_card = (rank << 2) | rep_suit;
        if (node->get_child(rep_card)) {
          child = node->get_child(rep_card);
          break;
        }
      }

      if (!child) continue;
    }

    new_board.resize(board.size());
    new_board.push_back(card);

    std::fill(new_villain_reach_probs.begin(), new_villain_reach_probs.end(), 0.0f);
    for (int hand = 0; hand < m_num_villain_hands; ++hand) {
      if (!CardUtility::overlap(m_villain_preflop_combos[hand], card)) {
        new_villain_reach_probs[hand] = villain_reach_probs[hand] * reach_scale;
      }
    }

    std::vector<float> subgame_evs{
        best_response(child, new_villain_reach_probs, new_board)};

    if (is_isomorphic) {
      int suit = card & 3;
      const auto& swap_list = iso_data.swap_list[suit][m_hero - 1];
      IsomorphismComputer::apply_swap(subgame_evs, swap_list);
    }

    for (int hand = 0; hand < m_num_hero_hands; ++hand) {
      preflop_combo_evs[hand] += subgame_evs[hand];
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

  std::vector<float> preflop_combo_evs(m_num_hero_hands, 0.0f);
  const uint64_t board_mask = CardUtility::board_to_mask(board);

  std::vector<float> new_villain_reach_probs(m_num_villain_hands);
  auto new_board{board};
  new_board.reserve(board.size() + 1);

  int chance_factor = 0;
  for (int card = 0; card < 52; ++card) {
    const uint64_t card_mask = 1ULL << card;
    if (!(card_mask & board_mask)) chance_factor++;
  }
  const float reach_scale = 1.0f / static_cast<float>(chance_factor);

  for (int card = 0; card < 52; ++card) {
    const uint64_t card_mask = 1ULL << card;
    if (card_mask & board_mask)
      continue;

    new_board.resize(board.size());
    new_board.push_back(card);

    std::fill(new_villain_reach_probs.begin(), new_villain_reach_probs.end(), 0.0f);
    for (int hand = 0; hand < m_num_villain_hands; ++hand) {
      if (!CardUtility::overlap(m_villain_preflop_combos[hand], card))
        new_villain_reach_probs[hand] = villain_reach_probs[hand] * reach_scale;
    }

    const auto subgame_evs{
        all_in_best_response(node, new_villain_reach_probs, new_board)};

    for (int hand = 0; hand < m_num_hero_hands; ++hand) {
      preflop_combo_evs[hand] += subgame_evs[hand];
    }
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

  float win_sum{0.0f};
  const float value{static_cast<float>(node->get_pot() / 2.0)};
  std::array<float, 52> card_win_sum{};

  int j{0};
  for (std::size_t i{0}; i < hero_river_combos.size(); ++i) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j < villain_river_combos.size() &&
           hero_combo.rank > villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};
      const float reach = villain_reach_probs[villain_combo.reach_probs_index];
      win_sum += reach;
      card_win_sum[villain_combo.hand1] += reach;
      card_win_sum[villain_combo.hand2] += reach;
      j++;
    }

    utils[hero_combo.reach_probs_index] =
        value * (win_sum - card_win_sum[hero_combo.hand1] -
                 card_win_sum[hero_combo.hand2]);
  }

  float lose_sum{0.0f};
  std::array<float, 52> card_lose_sum{};
  j = static_cast<int>(villain_river_combos.size()) - 1;
  for (int i{static_cast<int>(hero_river_combos.size()) - 1}; i >= 0; i--) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j >= 0 && hero_combo.rank < villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};
      const float reach = villain_reach_probs[villain_combo.reach_probs_index];
      lose_sum += reach;
      card_lose_sum[villain_combo.hand1] += reach;
      card_lose_sum[villain_combo.hand2] += reach;
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
  float villain_reach_sum{0.0f};
  std::array<float, 52> sum_with_card{};
  const uint64_t board_mask = CardUtility::board_to_mask(board);

  for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
    if (CardUtility::overlap_mask(m_villain_preflop_combos[hand], board_mask))
      continue;

    const float reach = villain_reach_pr[hand];
    sum_with_card[m_villain_preflop_combos[hand].hand1] += reach;
    sum_with_card[m_villain_preflop_combos[hand].hand2] += reach;
    villain_reach_sum += reach;
  }

  const float value = (m_hero == node->get_last_to_act())
                          ? (-node->get_pot() / 2.0f)
                          : (node->get_pot() / 2.0f);
  std::vector<float> utils(m_num_hero_hands);
  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    if (CardUtility::overlap_mask(m_hero_preflop_combos[hand], board_mask))
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
