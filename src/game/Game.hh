#pragma once
#include <array>
#include <vector>

namespace GameParams {
constexpr int NUM_CARDS{52};
constexpr int NUM_SUITS{4};
constexpr std::array suitReverseArray = {'c', 'd', 'h', 's'};
// Legacy bet sizes (kept for backwards compatibility)
constexpr std::array BET_SIZES{0.33f, 0.66f, 1.0f};
constexpr std::array RAISE_SIZES{0.5f, 1.0f};

} // namespace GameParams

enum class Street { FLOP = 3, TURN = 4, RIVER = 5 };

// Per-street bet sizing configuration
struct StreetBetConfig {
    std::vector<float> bet_sizes;
    std::vector<float> raise_sizes;
};

struct BetSizingConfig {
    // Default: reduced sizes for flop, full sizes for turn/river
    StreetBetConfig flop{.bet_sizes = {0.5f, 1.0f}, .raise_sizes = {1.0f}};
    StreetBetConfig turn{.bet_sizes = {0.33f, 0.66f, 1.0f}, .raise_sizes = {0.5f, 1.0f}};
    StreetBetConfig river{.bet_sizes = {0.33f, 0.66f, 1.0f}, .raise_sizes = {0.5f, 1.0f}};

    const StreetBetConfig& for_street(Street s) const {
        switch(s) {
            case Street::FLOP: return flop;
            case Street::TURN: return turn;
            default: return river;
        }
    }
};
