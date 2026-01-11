#include "Helper.hh"
#include "Solver.hh"
#include "Isomorphism.hh"
#include "tree/Nodes.hh"
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <algorithm>
#include <array>
#include <map>
#include <cmath>
#include <iostream>

void CFRHelper::compute() {
  initialize_combo_index();

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

void CFRHelper::initialize_combo_index() {
  if (m_combo_index_initialized)
    return;

  constexpr int NC = 52;
  for (int i = 0; i < NC; ++i)
    for (int j = 0; j < NC; ++j)
      m_villain_combo_index[i][j] = -1;

  for (int v = 0; v < m_num_villain_hands; ++v) {
    auto &vc = m_villain_preflop_combos[v];
    m_villain_combo_index[vc.hand1][vc.hand2] = v;
    m_villain_combo_index[vc.hand2][vc.hand1] = v;
  }

  m_combo_index_initialized = true;
}

auto CFRHelper::get_isomorphic_card_groups(const std::vector<Card> &board,
                                            const ChanceNode *node)
    -> std::vector<std::vector<int>> {
  std::array<int, 4> suit_count{};
  for (const auto &card : board) {
    suit_count[int(card) % 4]++;
  }

  std::map<int, int> count_to_canonical;
  std::vector<int> unique_counts;
  for (int c : suit_count) {
    if (std::find(unique_counts.begin(), unique_counts.end(), c) == unique_counts.end()) {
      unique_counts.push_back(c);
    }
  }
  std::sort(unique_counts.begin(), unique_counts.end(), std::greater<int>());

  int next_canonical = 0;
  for (int count : unique_counts) {
    count_to_canonical[count] = next_canonical++;
  }

  std::array<int, 4> canonical_suit;
  for (int s = 0; s < 4; ++s) {
    canonical_suit[s] = count_to_canonical[suit_count[s]];
  }

  std::map<std::pair<int, int>, std::vector<int>> groups;
  for (int card = 0; card < 52; ++card) {
    if (!node->get_child(card))
      continue;

    Card c(card);
    int rank = int(c) / 4;
    int can_suit = canonical_suit[int(c) % 4];
    groups[{rank, can_suit}].push_back(card);
  }

  std::vector<std::vector<int>> result;
  for (auto &[key, group] : groups) {
    if (!group.empty()) {
      result.push_back(std::move(group));
    }
  }

  return result;
}

void CFRHelper::action_node_utility(
    ActionNode *const node, const std::vector<float> &hero_reach_pr,
    const std::vector<float> &villain_reach_pr) {
  const int player{node->get_player()};
  const int num_actions{node->get_num_actions()};
  const int num_hands = (player == m_hero) ? m_num_hero_hands : m_num_villain_hands;

  // Note: Can't use thread_local here due to recursive CFRHelper calls
  std::vector<float> strategy(num_hands * num_actions);
  node->get_trainer()->get_current_strat(strategy);

  std::vector<float> subgame_utils_flat(num_actions * m_num_hero_hands);
  auto get_utils = [&](int action, int hand) -> float& {
    return subgame_utils_flat[action * m_num_hero_hands + hand];
  };

  // Note: Cannot use thread_local here because these buffers are passed by reference
  // to recursive CFRHelper calls. If thread_local, the recursive call would overwrite
  // the buffer that the outer call is still using.

  tbb::parallel_for(
      tbb::blocked_range<int>(0, num_actions),
      [&](const tbb::blocked_range<int> &r) {
        // Local buffers per parallel task - each recursive call gets its own copy
        std::vector<float> hero_buffer(m_num_hero_hands);
        std::vector<float> villain_buffer(m_num_villain_hands);

        for (auto i = r.begin(); i < r.end(); ++i) {
          if (player == m_hero) {
            // Weight hero reach by strategy for cumulative strategy updates at child nodes
            for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
              hero_buffer[hand] =
                  strategy[hand + i * m_num_hero_hands] * hero_reach_pr[hand];
            }
            for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
              villain_buffer[hand] = villain_reach_pr[hand];
            }
          } else {
            for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
              hero_buffer[hand] = hero_reach_pr[hand];
            }
            for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
              villain_buffer[hand] =
                  strategy[hand + i * m_num_villain_hands] *
                  villain_reach_pr[hand];
            }
          }
          CFRHelper rec{node->get_child(i),
                        m_hero,
                        m_villain,
                        m_hero_preflop_combos,
                        m_villain_preflop_combos,
                        hero_buffer,
                        villain_buffer,
                        m_board,
                        m_iteration_count,
                        m_rrm,
                        m_hero_to_villain};
          rec.compute();
          auto result = rec.get_result();
          for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
            get_utils(i, hand) = result[hand];
          }
        }
      });

  auto *trainer{node->get_trainer()};
  if (trainer->get_current() != m_hero) {
    // Non-hero player: utilities are already strategy-weighted via reach probs
    // Just sum them up (strategy weighting happened in the recursive call)
    for (std::size_t action{0}; action < num_actions; ++action) {
      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        m_result[hand] += get_utils(action, hand);
      }
    }
  } else {
    // Hero player: compute regrets using correct DCFR formula
    // R_new = R_old * discount + instantaneous_regret
    // The two-phase approach (R_new = (R_old + inst) * discount) is WRONG

    // First compute counterfactual value (expected utility under current strategy)
    for (std::size_t action{0}; action < num_actions; ++action) {
      for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
        m_result[hand] += get_utils(action, hand) *
                          strategy[hand + action * m_num_hero_hands];
      }
    }

    // Update regrets using correct formula: R = R_old * discount + (action_util - value)
    trainer->update_regrets(subgame_utils_flat, m_result, m_iteration_count);
    trainer->update_cum_strategy(strategy, hero_reach_pr, m_iteration_count);
  }
};

void CFRHelper::chance_node_utility(const ChanceNode *node,
                                    const std::vector<float> &hero_reach_pr,
                                    const std::vector<float> &villain_reach_pr,
                                    const std::vector<Card> &board) {
  const uint64_t board_mask = CardUtility::board_to_mask(board);
  const auto& iso_data = node->get_isomorphism_data();

  // Build list of representative cards (those with children in the tree)
  std::vector<int> rep_cards;
  rep_cards.reserve(52);
  for (int card = 0; card < 52; ++card) {
    if (!((1ULL << card) & board_mask) && node->get_child(card)) {
      rep_cards.push_back(card);
    }
  }

  const int num_rep_cards = static_cast<int>(rep_cards.size());
  if (num_rep_cards == 0) return;

  // chance_factor = total valid cards (representatives + isomorphic)
  const int num_iso_cards = static_cast<int>(iso_data.isomorphism_card.size());
  const int chance_factor = num_rep_cards + num_iso_cards;
  const float reach_scale = 1.0f / static_cast<float>(chance_factor);

  // Allocate storage for representative card utilities
  std::vector<float> cfv_actions(num_rep_cards * m_num_hero_hands);
  auto get_utils = [&](int card_idx, int hand) -> float& {
    return cfv_actions[card_idx * m_num_hero_hands + hand];
  };

  // Process representative cards in parallel
  tbb::parallel_for(
      tbb::blocked_range<int>(0, num_rep_cards),
      [&](const tbb::blocked_range<int> &r) {
        std::vector<float> hero_buffer(m_num_hero_hands);
        std::vector<float> villain_buffer(m_num_villain_hands);
        std::vector<Card> board_buffer;
        board_buffer.reserve(6);

        for (auto i = r.begin(); i < r.end(); ++i) {
          int card = rep_cards[i];

          board_buffer = board;
          board_buffer.push_back(card);

          std::fill(hero_buffer.begin(), hero_buffer.end(), 0.0f);
          std::fill(villain_buffer.begin(), villain_buffer.end(), 0.0f);

          for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
            if (!CardUtility::overlap(m_hero_preflop_combos[hand], card)) {
              hero_buffer[hand] = hero_reach_pr[hand];
            }
          }

          for (std::size_t hand{0}; hand < m_num_villain_hands; ++hand) {
            if (!CardUtility::overlap(m_villain_preflop_combos[hand], card)) {
              villain_buffer[hand] = villain_reach_pr[hand] * reach_scale;
            }
          }

          CFRHelper rec{node->get_child(card),
                        m_hero,
                        m_villain,
                        m_hero_preflop_combos,
                        m_villain_preflop_combos,
                        hero_buffer,
                        villain_buffer,
                        board_buffer,
                        m_iteration_count,
                        m_rrm,
                        m_hero_to_villain};
          rec.compute();
          auto result = rec.get_result();
          for (std::size_t hand{0}; hand < m_num_hero_hands; ++hand) {
            get_utils(i, hand) = result[hand];
          }
        }
      });

  // Sum up CFV from representative cards (using f64 for precision like wasm-postflop)
  std::vector<double> result_f64(m_num_hero_hands, 0.0);
  for (int i = 0; i < num_rep_cards; ++i) {
    for (std::size_t h{0}; h < m_num_hero_hands; ++h) {
      result_f64[h] += static_cast<double>(get_utils(i, h));
    }
  }

  // Process isomorphic cards using swap-add-swap pattern (exactly like wasm-postflop)
  // For each isomorphic card, get its representative's CFV, apply swap, add, swap back
  for (std::size_t i = 0; i < iso_data.isomorphism_ref.size(); ++i) {
    int iso_card = iso_data.isomorphism_card[i];
    int iso_suit = iso_card & 3;
    int rep_action_idx = iso_data.isomorphism_ref[i];

    // Get swap list for this player (hero)
    const auto& swap_list = iso_data.swap_list[iso_suit][m_hero - 1];

    // Get pointer to the representative's CFV row
    float* tmp = &cfv_actions[rep_action_idx * m_num_hero_hands];

    // Apply swap (transforms from rep suit perspective to isomorphic suit perspective)
    IsomorphismComputer::apply_swap(tmp, m_num_hero_hands, swap_list);

    // Add to result
    for (std::size_t h{0}; h < m_num_hero_hands; ++h) {
      result_f64[h] += static_cast<double>(tmp[h]);
    }

    // Apply swap again to restore (swap is its own inverse)
    IsomorphismComputer::apply_swap(tmp, m_num_hero_hands, swap_list);
  }

  // Convert back to f32
  for (std::size_t h{0}; h < m_num_hero_hands; ++h) {
    m_result[h] = static_cast<float>(result_f64[h]);
  }
}

auto CFRHelper::get_card_weights(const std::vector<float> &villain_reach_pr,
                                 const std::vector<Card> &board)
    -> std::vector<float> {
  constexpr int NC = 52;

  float p_total = 0.0f;
  std::array<float, NC> p_card{};
  for (int v = 0; v < m_num_villain_hands; ++v) {
    const auto &vc = m_villain_preflop_combos[v];
    if (CardUtility::overlap(vc, board))
      continue;
    float pr = villain_reach_pr[v];
    p_total += pr;
    p_card[int(vc.hand1)] += pr;
    p_card[int(vc.hand2)] += pr;
  }

  uint64_t board_mask_bits = CardUtility::board_to_mask(board);

  std::vector<float> card_weights(m_num_hero_hands * NC, 0.0f);
  for (size_t h = 0; h < m_num_hero_hands; ++h) {
    const auto &hc = m_hero_preflop_combos[h];
    int h1 = int(hc.hand1), h2 = int(hc.hand2);
    uint64_t hero_mask = (1ULL << h1) | (1ULL << h2);
    if ((hero_mask & board_mask_bits) != 0)
      continue;

    int v_self = m_hero_to_villain[h];
    float self_pr = (v_self >= 0 ? villain_reach_pr[v_self] : 0.0f);
    float S_h = p_total - p_card[h1] - p_card[h2] + self_pr;

    uint64_t unavailable = board_mask_bits | hero_mask;

    thread_local std::vector<int> available_cards;
    available_cards.clear();
    for (int c = 0; c < NC; ++c) {
      if (!((1ULL << c) & unavailable)) {
        available_cards.push_back(c);
      }
    }

    float total_w = 0.0f;
    for (int c : available_cards) {
      float excl = 0.0f;
      int idx = m_villain_combo_index[h1][c];
      if (idx >= 0)
        excl += villain_reach_pr[idx];
      idx = m_villain_combo_index[h2][c];
      if (idx >= 0)
        excl += villain_reach_pr[idx];

      float w = S_h - (p_card[c] - excl);
      card_weights[h + c * m_num_hero_hands] = w;
      total_w += w;
    }

    if (total_w > 0.0f) {
      float inv_total = 1.0f / total_w;
      for (int c : available_cards) {
        card_weights[h + c * m_num_hero_hands] *= inv_total;
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
  std::vector<int> card_counts(m_num_hero_hands, 0);  // Count valid cards per hero hand

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

    // Only add for hero hands that don't block this card
    for (int hand = 0; hand < m_num_hero_hands; ++hand) {
      if (!CardUtility::overlap(m_hero_preflop_combos[hand], card)) {
        preflop_combo_evs[hand] += subgame_evs[hand];
        card_counts[hand]++;
      }
    }
  }

  // Normalize by actual number of valid cards per hero hand
  for (int hand = 0; hand < m_num_hero_hands; ++hand) {
    if (card_counts[hand] > 0) {
      preflop_combo_evs[hand] /= static_cast<float>(card_counts[hand]);
    }
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

  float win_sum{0.0f};
  const float value{static_cast<float>(node->get_pot() / 2.0)};
  std::array<float, 52> card_win_sum{};

  int j{0};
  for (std::size_t i{0}; i < hero_river_combos.size(); ++i) {
    const auto &hero_combo{hero_river_combos[i]};

    while (j < villain_river_combos.size() &&
           hero_combo.rank > villain_river_combos[j].rank) {
      const auto &villain_combo{villain_river_combos[j]};
      const float reach = villain_reach_pr[villain_combo.reach_probs_index];
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
      const float reach = villain_reach_pr[villain_combo.reach_probs_index];
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
    const float pr = villain_reach_pr[v];
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
