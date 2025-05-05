
#include <array>
namespace GameParams {
constexpr int NUM_SUITS = 4;
constexpr std::array<char, 4> suitReverseArray = {'c', 'd', 'h', 's'};
} // namespace GameParams

enum class Street { FLOP = 2, TURN = 3, RIVER = 4 };
