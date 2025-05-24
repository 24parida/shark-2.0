#include "Helper.hh"
#include "Solver.hh"
#include "tree/Nodes.hh"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <iostream>

CFRHelper::CFRHelper(Node *node, const int hero_id, const int villain_id,
                     std::vector<PreflopCombo> &hero_preflop_combos,
                     std::vector<PreflopCombo> &villain_preflop_combos,
                     std::vector<float> &hero_reach_pr,
                     std::vector<float> &villain_reach_pr, std::vector<Card> &board,
                     int iteration_count, RiverRangeManager &rrm,
                     const PreflopRangeManager &prm)
    : m_hero(hero_id), m_villain(villain_id), m_node(node),
      m_hero_reach_probs(hero_reach_pr),
      m_villain_reach_probs(villain_reach_pr), m_board(board),
      m_hero_preflop_combos(hero_preflop_combos),
      m_villain_preflop_combos(villain_preflop_combos),
      m_num_hero_hands(hero_preflop_combos.size()),
      m_num_villain_hands(villain_preflop_combos.size()),
      m_iteration_count(iteration_count), m_result(m_num_hero_hands),
      m_rrm(rrm), m_prm(prm),
      m_range_intersection(prm.compute_range_intersection(hero_id, villain_id)) {}

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
                        m_prm};
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
                        m_prm};
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
  std::vector<float> card_weights(m_num_hero_hands * 52);
  float villain_reach_sum{0.0};
  std::vector<float> villain_card_reach_sum(52);

  for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
    const int c1{m_villain_preflop_combos[hand].hand1};
    const int c2{m_villain_preflop_combos[hand].hand2};

    villain_card_reach_sum[c1] += villain_reach_pr[hand];
    villain_card_reach_sum[c2] += villain_reach_pr[hand];
    villain_reach_sum += villain_reach_pr[hand];
  }

  for (std::size_t hand{0}; hand < m_hero_preflop_combos.size(); ++hand) {
    auto &hero_combo{m_hero_preflop_combos[hand]};
    if (CardUtility::overlap(hero_combo, board))
      continue;

    float reach_prob = compute_reach_probability(hand, villain_reach_pr);

    float total_weight{0.0};
    for (int card{0}; card < 52; ++card) {
      if (CardUtility::overlap(card, board) ||
          CardUtility::overlap(hero_combo, card)) {
        continue;
      }

      const float weight = villain_reach_sum - villain_card_reach_sum[card] -
                           villain_card_reach_sum[hero_combo.hand1] -
                           villain_card_reach_sum[hero_combo.hand2] + reach_prob;
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

auto CFRHelper::get_all_in_utils(const TerminalNode *const node,
                                 const std::vector<float> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<float> {
  std::vector<float> utils(m_num_hero_hands);

  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    if (CardUtility::overlap(m_hero_preflop_combos[hand], board))
      continue;

    // Use new range intersection logic
    float reach_prob = compute_reach_probability(hand, villain_reach_pr);
    if (reach_prob <= 0.0f)
      continue;

    float total_utility = 0.0f;
    for (std::size_t v_hand{0}; v_hand < m_num_villain_hands; ++v_hand) {
      if (CardUtility::overlap(m_villain_preflop_combos[v_hand], board))
        continue;

      // Only consider villain hands that match this hero hand in the range intersection
      const auto& matching_combos = m_range_intersection.hand_to_combos[hand];
      if (std::find(matching_combos.begin(), matching_combos.end(), v_hand) != matching_combos.end()) {
        const float equity = m_rrm.get_equity(m_hero_preflop_combos[hand],
                                            m_villain_preflop_combos[v_hand], board);
        total_utility += villain_reach_pr[v_hand] * 
                        (2.0f * equity - 1.0f) * node->get_pot() / 2.0f;
      }
    }
    utils[hand] = total_utility;
  }

  return utils;
}

auto CFRHelper::get_showdown_utils(const TerminalNode *const node,
                                   const std::vector<float> &villain_reach_pr,
                                   const std::vector<Card> &board)
    -> std::vector<float> {
  std::vector<float> utils(m_num_hero_hands);

  for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
    if (CardUtility::overlap(m_hero_preflop_combos[hand], board))
      continue;

    // Use new range intersection logic
    float reach_prob = compute_reach_probability(hand, villain_reach_pr);
    if (reach_prob <= 0.0f)
      continue;

    float total_utility = 0.0f;
    for (std::size_t v_hand{0}; v_hand < m_num_villain_hands; ++v_hand) {
      if (CardUtility::overlap(m_villain_preflop_combos[v_hand], board))
        continue;

      // Only consider villain hands that match this hero hand in the range intersection
      const auto& matching_combos = m_range_intersection.hand_to_combos[hand];
      if (std::find(matching_combos.begin(), matching_combos.end(), v_hand) != matching_combos.end()) {
        const float equity = m_rrm.get_equity(m_hero_preflop_combos[hand],
                                            m_villain_preflop_combos[v_hand], board);
        total_utility += villain_reach_pr[v_hand] * 
                        (2.0f * equity - 1.0f) * node->get_pot() / 2.0f;
      }
    }
    utils[hand] = total_utility;
  }

  return utils;
}

auto CFRHelper::get_uncontested_utils(
    const TerminalNode *node, const std::vector<float> &villain_reach_pr,
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

    float reach_prob = compute_reach_probability(hand, villain_reach_pr);

    utils[hand] =
        value *
        (villain_reach_sum - sum_with_card[m_hero_preflop_combos[hand].hand1] -
         sum_with_card[m_hero_preflop_combos[hand].hand2] + reach_prob);
  }

  return utils;
}
