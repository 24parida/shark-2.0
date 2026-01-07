#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>

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

  static const std::map<std::string, PositionRanges> POSITION_RANGES = {
    {"SB", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQo,K9s,KTs,KJs,KQs,QTs,QJs,QJo,JTs,T9s,98s,87s,76s,65s,54s",
      "AA,KK,QQ,JJ,TT,AKs,AKo,A5s,A4s,KTs,QTs",
      "AA,KK,AKs,A5s"
    }},
    {"BB", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K2s,K3s,K4s,K5s,K6s,K7s,K8s,K9s,KTs,KJs,KQs,KTs,KTo,KJs,KJo,Q2s,Q3s,Q4s,Q5s,Q6s,Q7s,Q8s,Q9s,QTs,QJs,QJo,J2s,J3s,J4s,J5s,J6s,J7s,J8s,J9s,JTs,T2s,T3s,T4s,T5s,T6s,T7s,T8s,T9s,98s,87s,76s,65s,54s",
      "AA,KK,QQ,AKs,AKo,A5s,A4s",
      "AA,KK,AKs,A5s"
    }},
    {"UTG", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,KTs,KJs,KQs,QTs,JTs,T9s",
      "AA,KK,QQ,JJ,TT,99,AJs,AQs,AKs,AQs,AQo,AKs,AKo,KQs",
      "AA,KK,AKs,A5s"
    }},
    {"UTG+1", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,KTs,KJs,KQs,QTs,JTs,T9s,98s",
      "AA,KK,QQ,AKs,AKo,A5s,KTs",
      "AA,KK,AKs,A5s"
    }},
    {"MP", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,QTs,JTs,T9s,98s",
      "AA,KK,QQ,JJ,AKs,AKo,A5s,KQs",
      "AA,KK,AKs,A5s"
    }},
    {"LJ", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,Q9s,JTs,T9s,98s",
      "AA,KK,QQ,AKs,AKo,A5s,QTs",
      "AA,KK,AKs,A5s"
    }},
    {"HJ", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,Q9s,QTs,JTs,T9s",
      "AA,KK,QQ,AKs,AKo,A5s,JTs",
      "AA,KK,AKs,A5s"
    }},
    {"CO", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K8s,K9s,KTs,KJs,KQs,Q9s,QTs,J9s,JTs,T9s,98s",
      "AA,KK,QQ,AKs,AKo,A5s,JTs",
      "AA,KK,AKs,A5s"
    }},
    {"BTN", {
      "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K7s,K8s,K9s,KTs,KJs,KQs,Q8s,Q9s,QTs,J8s,J9s,JTs,T7s,T8s,T9s,98s",
      "AA,KK,QQ,AKs,AKo,A5s,KTs,QTs",
      "AA,KK,AKs,A5s"
    }}
  };

  // Helper function to expand range notation into individual hands
  inline std::vector<std::string> expandRange(const std::string& range) {
    std::vector<std::string> hands;
    std::istringstream ss(range);
    std::string token;

    while (std::getline(ss, token, ',')) {
      if (token.find('-') != std::string::npos) {
        // Handle ranges like "A2s-A9s"
        size_t pos = token.find('-');
        std::string start = token.substr(0, pos);
        std::string end = token.substr(pos + 1);

        char startRank1 = start[0];
        char startRank2 = start[1];
        char endRank2 = end[1];
        bool suited = (start.find('s') != std::string::npos);

        for (char r = startRank2; r <= endRank2; r++) {
          hands.push_back(std::string(1, startRank1) + r + (suited ? "s" : "o"));
        }
      } else if (token.find('+') != std::string::npos) {
        // Handle ranges like "22+"
        std::string base = token.substr(0, token.length() - 1);
        char startRank = base[0];
        for (const auto& r : RANKS) {
          if (r[0] <= startRank) {
            hands.push_back(r + r);
          }
        }
      } else {
        // Single hand
        hands.push_back(token);
      }
    }
    return hands;
  }

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
  inline std::vector<std::string> getRangeForPosition(const std::string& position,
                                                      const std::string& potType,
                                                      bool isHero) {
    auto it = POSITION_RANGES.find(position);
    if (it == POSITION_RANGES.end()) {
      return {};
    }

    const auto& ranges = it->second;
    std::string rangeStr;

    if (potType == "Single Raise") {
      rangeStr = ranges.opening;
    } else if (potType == "3-bet") {
      rangeStr = isHero ? ranges.threeBet : ranges.opening;
    } else if (potType == "4-bet") {
      rangeStr = isHero ? ranges.fourBet : ranges.threeBet;
    }

    return expandRange(rangeStr);
  }
}
