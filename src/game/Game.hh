#pragma once
#include <array>

namespace GameParams {
constexpr int NUM_CARDS{52};
constexpr int NUM_SUITS{4};
constexpr std::array suitReverseArray = {'c', 'd', 'h', 's'};
constexpr std::array BET_SIZES{0.33f, 0.66f, 1.0f};
constexpr std::array RAISE_SIZES{0.5f, 1.0f};

} // namespace GameParams

enum class Street { FLOP = 3, TURN = 4, RIVER = 5 };
