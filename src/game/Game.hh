#pragma once
#include <array>

namespace GameParams {
constexpr int NUM_CARDS{52};
constexpr int NUM_SUITS{4};
constexpr std::array<char, 4> suitReverseArray = {'c', 'd', 'h', 's'};
constexpr std::array<double, 5> BET_SIZES{0.5, 1.0};
constexpr std::array<double, 1> RAISE_SIZES{0.5};
} // namespace GameParams

enum class Street { FLOP = 2, TURN = 3, RIVER = 4 };
