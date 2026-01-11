#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

namespace RangeData {
  // Range definitions for different positions
  struct PositionRanges {
    std::string opening;  // same as single-raise
    std::string threeBet;
    std::string fourBet;
  };

  static const std::vector<std::string> RANKS = {
    "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};

  static const std::vector<char> SUITS = {'h', 'd', 'c', 's'};

  // Helper to get rank index (A=0, K=1, ..., 2=12)
  inline int getRankIndex(char r) {
    static const std::string ranks = "AKQJT98765432";
    auto pos = ranks.find(r);
    return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
  }

  // Helper function to expand range notation into individual hands
  // Supports: 22+, A2s+, KTs+, A9o+, AKs, AKo, AA, 76s-54s, A5s-A2s, etc.
  inline std::vector<std::string> expandRange(const std::string& range) {
    std::vector<std::string> hands;
    std::istringstream ss(range);
    std::string token;

    while (std::getline(ss, token, ',')) {
      // Trim whitespace
      token.erase(0, token.find_first_not_of(" \t"));
      token.erase(token.find_last_not_of(" \t") + 1);
      if (token.empty()) continue;

      // Check for + notation
      if (token.back() == '+') {
        std::string base = token.substr(0, token.length() - 1);

        if (base.length() == 2 && base[0] == base[1]) {
          // Pair+ notation: "22+" means 22-AA
          int startIdx = getRankIndex(base[0]);
          for (int i = startIdx; i >= 0; i--) {
            hands.push_back(std::string(2, RANKS[i][0]));
          }
        } else if (base.length() == 3) {
          // Suited/offsuit+ notation: "A2s+" or "KTo+"
          char high = base[0];
          char low = base[1];
          char type = base[2]; // 's' or 'o'

          int highIdx = getRankIndex(high);
          int lowIdx = getRankIndex(low);

          // Generate from low to one below high (e.g., A2s+ = A2s, A3s, ... AKs)
          for (int i = lowIdx; i > highIdx; i--) {
            hands.push_back(std::string(1, high) + RANKS[i][0] + type);
          }
        }
      }
      // Check for dash range notation
      else if (token.find('-') != std::string::npos) {
        size_t pos = token.find('-');
        std::string start = token.substr(0, pos);
        std::string end = token.substr(pos + 1);

        if (start.length() >= 2 && end.length() >= 2) {
          char high1 = start[0], low1 = start[1];
          char high2 = end[0], low2 = end[1];
          bool suited = (start.length() == 3 && start[2] == 's');

          if (high1 == high2) {
            // Same high card range: "A5s-A2s" means A5s, A4s, A3s, A2s
            int startLow = getRankIndex(low1);
            int endLow = getRankIndex(low2);
            for (int i = startLow; i <= endLow; i++) {
              hands.push_back(std::string(1, high1) + RANKS[i][0] + (suited ? "s" : "o"));
            }
          } else {
            // Connector range: "76s-54s" means 76s, 65s, 54s
            int startHigh = getRankIndex(high1);
            int endHigh = getRankIndex(high2);
            int gap = getRankIndex(low1) - startHigh;

            for (int i = startHigh; i <= endHigh; i++) {
              int lowIdx = i + gap;
              if (lowIdx < 13) {
                hands.push_back(std::string(1, RANKS[i][0]) + RANKS[lowIdx][0] + (suited ? "s" : "o"));
              }
            }
          }
        }
      }
      // Single hand notation
      else {
        hands.push_back(token);
      }
    }
    return hands;
  }

  // Determine if a position is a blind (typically the 3-bettor in 3bet pots)
  inline bool isBlindPosition(const std::string& position) {
    return position == "SB" || position == "BB";
  }

  // Position ranges using compact notation
  // Format: {opening, 3bet, 4bet}
  static const std::map<std::string, PositionRanges> POSITION_RANGES = {
    // UTG: Tightest opening range
    {"UTG", {
      "66+,ATs+,A5s-A2s,KTs+,QTs+,JTs,T9s,98s,87s,76s,AJo+,KQo",  // opening
      "QQ+,AKs,AQs,AKo",  // 3bet (very tight from EP)
      "QQ+,AKs,AKo"  // 4bet
    }},

    // UTG+1: Slightly wider than UTG
    {"UTG+1", {
      "55+,ATs+,A5s-A2s,KTs+,QTs+,JTs,T9s,98s,87s,76s,AJo+,KQo",
      "QQ+,AKs,AQs,AKo",
      "QQ+,AKs,AKo"
    }},

    // MP: Middle position
    {"MP", {
      "44+,A9s+,A5s-A2s,KTs+,QTs+,JTs,T9s,98s,87s,76s,65s,ATo+,KJo+",
      "JJ+,AQs+,AKo,A5s,A4s",
      "QQ+,AKs,AKo,A5s"
    }},

    // LJ: Lojack
    {"LJ", {
      "33+,A8s+,A5s-A2s,K9s+,Q9s+,J9s+,T9s,98s,87s,76s,65s,54s,ATo+,KJo+,QJo",
      "TT+,AJs+,KQs,AKo,AQo,A5s,A4s",
      "QQ+,AKs,AKo,A5s"
    }},

    // HJ: Hijack
    {"HJ", {
      "22+,A2s+,K9s+,Q9s+,J9s+,T8s+,98s,87s,76s,65s,54s,ATo+,KTo+,QJo",
      "TT+,AJs+,KQs,AKo,AQo,A5s,A4s",
      "QQ+,AKs,AKo,AQs,A5s"
    }},

    // CO: Cutoff - fairly wide
    {"CO", {
      "22+,A2s+,K7s+,Q8s+,J8s+,T8s+,97s+,87s,76s,65s,54s,A9o+,KTo+,QTo+,JTo",
      "99+,ATs+,KJs+,QJs,AJo+,KQo,A5s,A4s",
      "JJ+,AKs,AQs,AKo,A5s,A4s"
    }},

    // BTN: Button - widest opening range
    {"BTN", {
      "22+,A2s+,K5s+,Q7s+,J7s+,T7s+,96s+,86s+,75s+,65s,54s,A7o+,K9o+,Q9o+,J9o+,T9o",
      "88+,A9s+,KTs+,QTs+,JTs,ATo+,KJo+,A5s-A2s",
      "TT+,AKs,AQs,AKo,AQo,A5s,A4s"
    }},

    // SB: Small blind open-raise range
    {"SB", {
      "22+,A2s+,K8s+,Q9s+,J9s+,T8s+,98s,87s,76s,65s,54s,A9o+,KTo+,QTo+,JTo",
      "99+,ATs+,KJs+,QJs,AJo+,KQo,A5s-A2s",
      "JJ+,AKs,AQs,AKo,A5s,A4s"
    }},

    // BB: Big blind defense range (widest since closing action)
    {"BB", {
      "22+,A2s+,K2s+,Q5s+,J7s+,T7s+,96s+,86s+,75s+,64s+,54s,A5o+,K9o+,Q9o+,J9o+,T9o",
      "88+,A9s+,KTs+,QTs+,JTs,ATo+,KQo,A5s-A2s",
      "JJ+,AKs,AQs,AKo,A5s,A4s"
    }}
  };

  // Get position index for in-position calculation
  inline int getPositionIndex(const std::string& position) {
    static const std::map<std::string, int> positionIndices = {
      {"SB", 0}, {"BB", 1}, {"UTG", 2}, {"UTG+1", 3},
      {"MP", 4}, {"LJ", 5}, {"HJ", 6}, {"CO", 7}, {"BTN", 8}
    };

    auto it = positionIndices.find(position);
    return (it != positionIndices.end()) ? it->second : 0;
  }

  // Get range for a specific position and pot type
  // Both positions get the same type of range based on pot type
  inline std::vector<std::string> getRangeForPosition(const std::string& position,
                                                      const std::string& potType,
                                                      bool /* isHero - unused, kept for API compat */) {
    auto it = POSITION_RANGES.find(position);
    if (it == POSITION_RANGES.end()) {
      return {};
    }

    const auto& ranges = it->second;
    std::string rangeStr;

    if (potType == "Single Raise") {
      rangeStr = ranges.opening;
    } else if (potType == "3-bet") {
      rangeStr = ranges.threeBet;
    } else if (potType == "4-bet") {
      rangeStr = ranges.fourBet;
    }

    return expandRange(rangeStr);
  }
}
