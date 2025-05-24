#include "RiverRangeManager.hh"
#include "../Helper.hh"
#include "hands/RiverCombo.hh"
#include <algorithm>
#include <array>
#include <cassert>

// Helper function to convert rank char to 0-12 index
static int char_rank_to_index(char rank) {
    switch(rank) {
        case '2': return 0;
        case '3': return 1;
        case '4': return 2;
        case '5': return 3;
        case '6': return 4;
        case '7': return 5;
        case '8': return 6;
        case '9': return 7;
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
        default: return -1;
    }
}

// Helper function to convert suit char to 0-3 index
static int char_suit_to_index(char suit) {
    switch(suit) {
        case 'h': return 0;
        case 'd': return 1;
        case 'c': return 2;
        case 's': return 3;
        default: return -1;
    }
}

auto RiverRangeManager::get_river_combos(
    const int player, const std::vector<PreflopCombo> &preflop_combos,
    const std::vector<phevaluator::Card> &board) -> std::vector<RiverCombo> {

  auto &river_ranges{player == 1 ? m_p1_river_ranges : m_p2_river_ranges};
  const int key{CardUtility::board_to_key(board)};

  if (auto it = river_ranges.find(key); it != river_ranges.end())
    return it->second;

  int count{0};
  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    if (!CardUtility::overlap(preflop_combos[hand], board))
      ++count;
  }

  std::vector<RiverCombo> river_combos;
  river_combos.reserve(count);

  for (std::size_t hand{0}; hand < preflop_combos.size(); ++hand) {
    const auto &preflop_combo = preflop_combos[hand];
    if (CardUtility::overlap(preflop_combo, board))
      continue;

    RiverCombo river_combo{preflop_combo.hand1, preflop_combo.hand2,
                           preflop_combo.probability, static_cast<int>(hand)};
    river_combo.rank =
        CardUtility::get_rank(river_combo.hand1, river_combo.hand2, board);
    river_combos.push_back(river_combo);
  }

  std::sort(river_combos.begin(), river_combos.end());

  river_ranges.insert({key, river_combos});
  return river_combos;
}

int RiverRangeManager::get_hand_rank(const PreflopCombo& hand, const std::vector<phevaluator::Card>& board) const {
    // Create array to hold all cards (2 hole cards + up to 5 board cards)
    std::array<phevaluator::Card, 7> all_cards;
    size_t total_cards = 2 + board.size(); // 2 hole cards + board cards
    assert(total_cards <= 7 && "Too many cards");
    
    // Initialize hole cards
    all_cards[0] = hand.hand1;
    all_cards[1] = hand.hand2;
    
    // Copy board cards
    for (size_t i = 0; i < board.size() && i < 5; i++) {
        all_cards[i + 2] = board[i];
    }
    
    // Sort cards by rank for easier evaluation
    std::sort(all_cards.begin(), all_cards.begin() + total_cards,
              [](const phevaluator::Card& a, const phevaluator::Card& b) { 
                  return char_rank_to_index(a.describeCard()[0]) > char_rank_to_index(b.describeCard()[0]); 
              });
    
    // Initialize rank count array
    std::array<int, 13> rank_count{};
    for (size_t i = 0; i < total_cards; i++) {
        int rank = char_rank_to_index(all_cards[i].describeCard()[0]);
        assert(rank >= 0 && rank < 13 && "Invalid rank");
        rank_count[rank]++;
    }
    
    // Check for straight flush
    bool has_straight_flush = false;
    int straight_flush_high = -1;
    for (int suit = 0; suit < 4; suit++) {
        int consecutive = 0;
        int last_rank = -1;
        for (size_t i = 0; i < total_cards; i++) {
            const auto& card = all_cards[i];
            if (char_suit_to_index(card.describeCard()[1]) == suit) {
                int curr_rank = char_rank_to_index(card.describeCard()[0]);
                if (last_rank == -1 || curr_rank == last_rank - 1) {
                    consecutive++;
                    if (consecutive == 5) {
                        has_straight_flush = true;
                        straight_flush_high = curr_rank + 4;
                        break;
                    }
                } else if (curr_rank != last_rank) {
                    consecutive = 1;
                }
                last_rank = curr_rank;
            }
        }
        if (has_straight_flush) break;
    }
    if (has_straight_flush) {
        return 8000000 + straight_flush_high;
    }
    
    // Check for four of a kind
    for (int i = 12; i >= 0; i--) {
        if (rank_count[i] == 4) {
            // Find highest kicker
            for (int j = 12; j >= 0; j--) {
                if (j != i && rank_count[j] > 0) {
                    return 7000000 + i * 13 + j;
                }
            }
        }
    }
    
    // Check for full house
    int three_rank = -1;
    int pair_rank = -1;
    for (int i = 12; i >= 0; i--) {
        if (rank_count[i] >= 3 && three_rank == -1) {
            three_rank = i;
        } else if (rank_count[i] >= 2 && pair_rank == -1) {
            pair_rank = i;
        }
    }
    if (three_rank != -1 && pair_rank != -1) {
        return 6000000 + three_rank * 13 + pair_rank;
    }
    
    // Check for flush
    for (int suit = 0; suit < 4; suit++) {
        std::vector<int> flush_ranks;
        for (size_t i = 0; i < total_cards; i++) {
            const auto& card = all_cards[i];
            if (char_suit_to_index(card.describeCard()[1]) == suit) {
                flush_ranks.push_back(char_rank_to_index(card.describeCard()[0]));
            }
        }
        if (flush_ranks.size() >= 5) {
            std::sort(flush_ranks.begin(), flush_ranks.end(), std::greater<int>());
            return 5000000 + flush_ranks[0] * 13 * 13 * 13 * 13 +
                   flush_ranks[1] * 13 * 13 * 13 +
                   flush_ranks[2] * 13 * 13 +
                   flush_ranks[3] * 13 +
                   flush_ranks[4];
        }
    }
    
    // Check for straight
    int consecutive = 0;
    int straight_high = -1;
    int last_rank = -1;
    for (size_t i = 0; i < total_cards; i++) {
        int curr_rank = char_rank_to_index(all_cards[i].describeCard()[0]);
        if (last_rank == -1 || curr_rank == last_rank - 1) {
            consecutive++;
            if (consecutive == 5) {
                straight_high = curr_rank + 4;
                break;
            }
        } else if (curr_rank != last_rank) {
            consecutive = 1;
        }
        last_rank = curr_rank;
    }
    if (straight_high != -1) {
        return 4000000 + straight_high;
    }
    
    // Check for three of a kind
    if (three_rank != -1) {
        // Find two highest kickers
        int kicker1 = -1, kicker2 = -1;
        for (int i = 12; i >= 0; i--) {
            if (i != three_rank && rank_count[i] > 0) {
                if (kicker1 == -1) kicker1 = i;
                else if (kicker2 == -1) {
                    kicker2 = i;
                    break;
                }
            }
        }
        return 3000000 + three_rank * 13 * 13 + kicker1 * 13 + kicker2;
    }
    
    // Check for two pair
    int high_pair = -1, low_pair = -1;
    for (int i = 12; i >= 0; i--) {
        if (rank_count[i] >= 2) {
            if (high_pair == -1) high_pair = i;
            else if (low_pair == -1) {
                low_pair = i;
                break;
            }
        }
    }
    if (high_pair != -1 && low_pair != -1) {
        // Find highest kicker
        for (int i = 12; i >= 0; i--) {
            if (i != high_pair && i != low_pair && rank_count[i] > 0) {
                return 2000000 + high_pair * 13 * 13 + low_pair * 13 + i;
            }
        }
    }
    
    // Check for one pair
    if (high_pair != -1) {
        // Find three highest kickers
        int kicker1 = -1, kicker2 = -1, kicker3 = -1;
        for (int i = 12; i >= 0; i--) {
            if (i != high_pair && rank_count[i] > 0) {
                if (kicker1 == -1) kicker1 = i;
                else if (kicker2 == -1) kicker2 = i;
                else if (kicker3 == -1) {
                    kicker3 = i;
                    break;
                }
            }
        }
        return 1000000 + high_pair * 13 * 13 * 13 + kicker1 * 13 * 13 + kicker2 * 13 + kicker3;
    }
    
    // High card
    int kicker1 = -1, kicker2 = -1, kicker3 = -1, kicker4 = -1, kicker5 = -1;
    for (int i = 12; i >= 0; i--) {
        if (rank_count[i] > 0) {
            if (kicker1 == -1) kicker1 = i;
            else if (kicker2 == -1) kicker2 = i;
            else if (kicker3 == -1) kicker3 = i;
            else if (kicker4 == -1) kicker4 = i;
            else if (kicker5 == -1) {
                kicker5 = i;
                break;
            }
        }
    }
    return kicker1 * 13 * 13 * 13 * 13 + kicker2 * 13 * 13 * 13 + kicker3 * 13 * 13 + kicker4 * 13 + kicker5;
}
