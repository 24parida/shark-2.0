#include "../include/card.h"
#include "../include/omp/EquityCalculator.hh"
#include <chrono>
#include <iostream>

using namespace omp;
using namespace std;
int main() {
  using phevaluator::Card;

  EquityCalculator eq;
  vector<CardRange> ranges{"Ac9c", "8s8c"};
  uint64_t board = CardRange::getCardMask("2c4c5h");

  auto start = std::chrono::high_resolution_clock::now();
  Card h1{"Ac"};
  Card h2{"9c"};
  Card v1{"8s"};
  Card v2{"8c"};
  std::vector<Card> card_board{{"2c"}, {"4c"}, {"5h"}};

  std::string hand1{h1.describeCard() + h2.describeCard()};
  std::string hand2{v1.describeCard() + v2.describeCard()};
  ranges = {hand1, hand2};
  std::string board_str{};
  for (const auto &i : card_board) {
    board_str += i.describeCard();
  }
  board = CardRange::getCardMask(board_str);

  eq.start(ranges, board, 0, false, 0.01, nullptr, 0.2, 1);
  eq.wait();
  auto r = eq.getResults();

  auto end = std::chrono::high_resolution_clock::now();
  auto duration_1 =
      std::chrono::duration_cast<std::chrono::seconds>(start - end);

  std::cout << r.equity[0] << " " << r.equity[1] << std::endl;
  std::cout << duration_1.count() << '\n';
  return 0;
}
