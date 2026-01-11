// --------------------------------
// Created by Anubhav Parida.
// --------------------------------
#pragma once
#include "../hands/PreflopCombo.hh"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>

using SwapList = std::vector<std::pair<uint16_t, uint16_t>>;

struct IsomorphismData {
  std::vector<uint8_t> isomorphism_ref;
  std::vector<int> isomorphism_card;
  std::array<std::array<SwapList, 2>, 4> swap_list;
  bool has_isomorphism = false;
};

class IsomorphismComputer {
public:
  static bool is_suit_isomorphic(const std::vector<PreflopCombo>& combos,
                                  int suit1, int suit2) {
    auto swap_suit = [suit1, suit2](int card) -> int {
      int suit = card & 3;
      int rank = card >> 2;
      if (suit == suit1) {
        return (rank << 2) | suit2;
      } else if (suit == suit2) {
        return (rank << 2) | suit1;
      }
      return card;
    };

    std::array<float, 52 * 52> weights{};
    std::array<float, 52 * 52> swapped_weights{};

    for (const auto& combo : combos) {
      int c1 = combo.hand1;
      int c2 = combo.hand2;
      if (c1 > c2) std::swap(c1, c2);
      weights[c1 * 52 + c2] = combo.probability;

      int s1 = swap_suit(combo.hand1);
      int s2 = swap_suit(combo.hand2);
      if (s1 > s2) std::swap(s1, s2);
      swapped_weights[s1 * 52 + s2] = combo.probability;
    }

    for (int i = 0; i < 52 * 52; ++i) {
      if (std::abs(weights[i] - swapped_weights[i]) > 1e-6f) {
        return false;
      }
    }
    return true;
  }

  static SwapList compute_swap_list(const std::vector<PreflopCombo>& combos,
                                     int suit1, int suit2) {
    SwapList swaps;

    auto replacer = [suit1, suit2](int card) -> int {
      int suit = card & 3;
      if (suit == suit1) {
        return card - suit1 + suit2;
      } else if (suit == suit2) {
        return card + suit1 - suit2;
      }
      return card;
    };

    auto card_pair_to_index = [](int c1, int c2) -> int {
      if (c1 > c2) std::swap(c1, c2);
      return c1 * (101 - c1) / 2 + c2 - 1;
    };

    std::vector<int> reverse_table(52 * 51 / 2, -1);
    for (size_t i = 0; i < combos.size(); ++i) {
      int idx = card_pair_to_index(combos[i].hand1, combos[i].hand2);
      reverse_table[idx] = static_cast<int>(i);
    }

    for (size_t i = 0; i < combos.size(); ++i) {
      int c1 = replacer(combos[i].hand1);
      int c2 = replacer(combos[i].hand2);
      int idx = card_pair_to_index(c1, c2);
      int j = reverse_table[idx];

      if (j > static_cast<int>(i)) {
        swaps.push_back({static_cast<uint16_t>(i), static_cast<uint16_t>(j)});
      }
    }

    return swaps;
  }

  static IsomorphismData compute(const std::vector<PreflopCombo>& p1_combos,
                                  const std::vector<PreflopCombo>& p2_combos,
                                  const std::vector<Card>& board,
                                  uint64_t board_mask) {
    IsomorphismData data;

    std::array<uint16_t, 4> board_rankset{};
    for (const auto& card : board) {
      int card_val = int(card);
      int rank = card_val >> 2;
      int suit = card_val & 3;
      board_rankset[suit] |= (1 << rank);
    }

    std::array<int, 4> isomorphic_suit = {-1, -1, -1, -1};

    for (int suit1 = 1; suit1 < 4; ++suit1) {
      for (int suit2 = 0; suit2 < suit1; ++suit2) {
        if (board_rankset[suit1] == board_rankset[suit2] &&
            is_suit_isomorphic(p1_combos, suit1, suit2) &&
            is_suit_isomorphic(p2_combos, suit1, suit2)) {
          isomorphic_suit[suit1] = suit2;
          data.has_isomorphism = true;

          data.swap_list[suit1][0] = compute_swap_list(p1_combos, suit1, suit2);
          data.swap_list[suit1][1] = compute_swap_list(p2_combos, suit1, suit2);
          break;
        }
      }
    }

    if (!data.has_isomorphism) {
      return data;
    }

    int counter = 0;
    std::array<int, 52> indices;
    indices.fill(-1);

    for (int card = 0; card < 52; ++card) {
      if ((1ULL << card) & board_mask) continue;

      int suit = card & 3;

      if (isomorphic_suit[suit] >= 0) {
        int replace_suit = isomorphic_suit[suit];
        int replace_card = card - suit + replace_suit;
        data.isomorphism_ref.push_back(static_cast<uint8_t>(indices[replace_card]));
        data.isomorphism_card.push_back(card);
      } else {
        indices[card] = counter;
        counter++;
      }
    }

    return data;
  }

  template<typename T>
  static void apply_swap(T* data, size_t len, const SwapList& swaps) {
    for (const auto& [i, j] : swaps) {
      if (i < len && j < len) {
        std::swap(data[i], data[j]);
      }
    }
  }

  template<typename T>
  static void apply_swap(std::vector<T>& data, const SwapList& swaps) {
    apply_swap(data.data(), data.size(), swaps);
  }
};
