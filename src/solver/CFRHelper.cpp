#include "Helper.hh"
#include "Solver.hh"
#include "tree/Nodes.hh"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>

void CFRHelper::compute() {
  if (m_node->get_node_type() == NodeType::ACTION_NODE) {
    action_node_utility(static_cast<ActionNode *>(m_node), m_hero_reach_probs,
                        m_villain_reach_probs);
  } else if (m_node->get_node_type() == NodeType::CHANCE_NODE) {
    chance_node_utility(static_cast<ChanceNode *>(m_node), m_hero_reach_probs,
                        m_villain_reach_probs, m_board);
  } else {
    terminal_node_utility(static_cast<TerminalNode *>(m_node),
                          m_villain_reach_probs, m_board);
  }
}

void CFRHelper::action_node_utility(
    ActionNode *const node, const std::vector<float> &hero_reach_pr,
    const std::vector<float> &villain_reach_pr) {
  const auto &strategy{node->get_current_strat()};
  const int player{node->get_player()};
  const int num_actions{node->get_num_actions()};
  std::vector<std::vector<float>> subgame_utils(num_actions);

  tbb::parallel_for(
      tbb::blocked_range<int>(0, num_actions),
      [&](const tbb::blocked_range<int> &r) {
        for (auto i = r.begin(); i < r.end(); ++i) {
          std::vector<float> new_hero_reach_probs(hero_reach_pr);
          std::vector<float> new_villain_reach_probs(villain_reach_pr);

          if (player == m_hero) {
            for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
              new_hero_reach_probs[hand] =
                  strategy[hand + i * m_num_hero_hands] * hero_reach_pr[hand];
            }
          } else {
            for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
              new_villain_reach_probs[hand] =
                  strategy[hand + i * m_num_villain_hands] *
                  villain_reach_pr[hand];
            }
          }
          CFRHelper rec{node->get_child(i),
                        m_hero,
                        m_villain,
                        m_hero_preflop_combos,
                        m_villain_preflop_combos,
                        new_hero_reach_probs,
                        new_villain_reach_probs,
                        m_board,
                        m_iteration_count,
                        m_rrm,
                        m_hero_to_villain};
          rec.compute();
          subgame_utils[i] = rec.get_result();
        }
      });

  auto *trainer{node->get_trainer()};
  if (trainer->get_current() != m_hero) {
    for (auto &i : subgame_utils) {
      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        m_result[hand] += i[hand];
      }
    }
  } else {
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
                                    const std::vector<float> &hero_reach_pr,
                                    const std::vector<float> &villain_reach_pr,
                                    const std::vector<Card> &board) {
  const auto card_weights{get_card_weights(villain_reach_pr, board)};

  std::vector<int> valid_children{};
  valid_children.reserve(52);
  for (std::size_t i{0}; i < 52; ++i) {
    if (node->get_child(i)) {
      valid_children.push_back(i);
    }
  }
  const int num_children(valid_children.size());
  std::vector<std::vector<float>> subgame_utils(num_children);

  tbb::parallel_for(
      tbb::blocked_range<int>(0, num_children),
      [&](const tbb::blocked_range<int> &r) {
        for (auto i = r.begin(); i < r.end(); ++i) {
          int card{valid_children[i]};

          std::vector<Card> new_board{board};
          new_board.push_back(card);

          std::vector<float> new_hero_reach_probs(m_num_hero_hands);
          std::vector<float> new_villain_reach_probs(m_num_villain_hands);

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

          CFRHelper rec{node->get_child(card),
                        m_hero,
                        m_villain,
                        m_hero_preflop_combos,
                        m_villain_preflop_combos,
                        new_hero_reach_probs,
                        new_villain_reach_probs,
                        new_board,
                        m_iteration_count,
                        m_rrm,
                        m_hero_to_villain};
          rec.compute();
          subgame_utils[i] = rec.get_result();
        }
      });

  for (auto &i : subgame_utils) {
    for (std::size_t h{0}; h < m_num_hero_hands; ++h) {
      m_result[h] += i[h] / static_cast<float>(num_children);
    }
  }
}

auto CFRHelper::get_card_weights(const std::vector<float> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<float> {
  constexpr int NC = 52;

  int combo_index[NC][NC];
  for (int i = 0; i < NC; ++i)
    for (int j = 0; j < NC; ++j)
      combo_index[i][j] = -1;

  for (int v = 0; v < m_num_villain_hands; ++v) {
    auto &vc = m_villain_preflop_combos[v];
    combo_index[vc.hand1][vc.hand2] = v;
    combo_index[vc.hand2][vc.hand1] = v;
  }

  float p_total = 0.0f;
  std::array<float, NC> p_card{};
  for (int v = 0; v < m_num_villain_hands; ++v) {
    const auto &vc = m_villain_preflop_combos[v];
    if (CardUtility::overlap(vc, board))
      continue;
    float pr = villain_reach_pr[v];
    p_total += pr;
    p_card[vc.hand1] += pr;
    p_card[vc.hand2] += pr;
  }

  std::array<bool, NC> board_mask{};
  for (Card c : board)
    board_mask[c] = true;

  std::vector<float> card_weights(m_num_hero_hands * NC, 0.0f);
  for (size_t h = 0; h < m_num_hero_hands; ++h) {
    const auto &hc = m_hero_preflop_combos[h];
    int h1 = hc.hand1, h2 = hc.hand2;
    if (board_mask[h1] || board_mask[h2])
      continue;

    int v_self = m_hero_to_villain[h];
    float self_pr = (v_self >= 0 ? villain_reach_pr[v_self] : 0.0f);
    float S_h = p_total - p_card[h1] - p_card[h2] + self_pr;

    float total_w = 0.0f;
    for (int c = 0; c < NC; ++c) {
      if (board_mask[c] || c == h1 || c == h2)
        continue;

      float excl = 0.0f;
      int idx = combo_index[c][h1];
      if (idx >= 0)
        excl += villain_reach_pr[idx];
      idx = combo_index[c][h2];
      if (idx >= 0)
        excl += villain_reach_pr[idx];

      float w = S_h - (p_card[c] - excl);
      card_weights[h + c * m_num_hero_hands] = w;
      total_w += w;
    }

    if (total_w > 0.0f) {
      for (int c = 0; c < NC; ++c) {
        float &cw = card_weights[h + c * m_num_hero_hands];
        if (cw > 0.0f)
          cw /= total_w;
      }
    }
  }

  return card_weights;
}

void CFRHelper::terminal_node_utility(
    const TerminalNode *const node, const std::vector<float> &villain_reach_pr,
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
                                 const std::vector<float> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<float> {
  assert(board.size() <= 5 && "get_all_in_utils unexpected all in board size");
  if (board.size() == 5) {
    return get_showdown_utils(node, villain_reach_pr, board);
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
        new_villain_reach_probs[hand] = villain_reach_pr[hand];
    }

    const auto subgame_evs{
        get_all_in_utils(node, new_villain_reach_probs, new_board)};

    const float normalizing_sum{static_cast<float>(52 - 2 - 2 - board.size())};
    for (int hand = 0; hand < m_num_hero_hands; ++hand)
      preflop_combo_evs[hand] += subgame_evs[hand] / normalizing_sum;
  }

  return preflop_combo_evs;
}

auto CFRHelper::get_showdown_utils(const TerminalNode *node,
                                   const std::vector<float> &villain_reach_pr,
                                   const std::vector<Card> &board)
    -> std::vector<float> {
  const std::vector<RiverCombo> hero_river_combos{
      m_rrm.get_river_combos(m_hero, m_hero_preflop_combos, board)};
  const std::vector<RiverCombo> villain_river_combos{
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
      assert(j < villain_river_combos.size() && j >= 0 && "forward pass error");
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

  float lose_sum{0.0};
  std::vector<float> card_lose_sum(52);
  j = static_cast<int>(villain_river_combos.size()) - 1;
  for (int i{static_cast<int>(hero_river_combos.size()) - 1}; i >= 0; i--) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j >= 0 && hero_combo.rank < villain_river_combos[j].rank) {
      assert(j < villain_river_combos.size() && j >= 0 &&
             "backward pass error");
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
    const TerminalNode *node, const std::vector<float> &villain_reach_pr,
    const std::vector<Card> &board) -> std::vector<float> {
  const bool hero_last = (m_hero == node->get_last_to_act());
  const float value =
      hero_last ? -0.5f * node->get_pot() : 0.5f * node->get_pot();

  float p_total = 0.0f;
  std::array<float, 52> sum_with_card{};
  for (size_t v = 0; v < m_num_villain_hands; ++v) {
    const auto &vc = m_villain_preflop_combos[v];
    if (CardUtility::overlap(vc, board))
      continue;
    float pr = villain_reach_pr[v];
    p_total += pr;
    sum_with_card[vc.hand1] += pr;
    sum_with_card[vc.hand2] += pr;
  }

  std::vector<float> utils(m_num_hero_hands, 0.0f);
  for (size_t h = 0; h < m_num_hero_hands; ++h) {
    const auto &hc = m_hero_preflop_combos[h];
    if (CardUtility::overlap(hc, board))
      continue;

    int h1 = hc.hand1, h2 = hc.hand2;
    int v_self = m_hero_to_villain[h];
    float self_pr = (v_self >= 0 ? villain_reach_pr[v_self] : 0.0f);

    float p_disjoint =
        p_total - sum_with_card[h1] - sum_with_card[h2] + self_pr;

    utils[h] = value * p_disjoint;
  }

  return utils;
}
