#pragma once
#include <array>

namespace GameParams {
constexpr int NUM_CARDS{52};
constexpr int NUM_SUITS{4};
constexpr std::array suitReverseArray = {'c', 'd', 'h', 's'};
// constexpr std::array BET_SIZES{0.25, 0.5, 0.75, 1.0};
// constexpr std::array RAISE_SIZES{0.5, 1.0};

constexpr std::array BET_SIZES{0.5, 1.0};
constexpr std::array RAISE_SIZES{0.5};
} // namespace GameParams

enum class Street { FLOP = 3, TURN = 4, RIVER = 5 };
