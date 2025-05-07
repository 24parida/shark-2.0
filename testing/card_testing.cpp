#include "card.h"
#include <iostream>

int main() {
  using phevaluator::Card;
  for (int i{0}; i < 52; i++) {
    std::cout << Card(i).describeCard() << '\n';
  }

  std::cout << "---------------" << '\n';

  Card a{0};
  Card b{1};
  bool a_b{a == b};

  Card c{"Ah"};
  Card d{"Ah"};
  bool c_d{c == d};

  std::cout << a_b << '\n';
  std::cout << c_d << '\n';
  std::cout << "---------------";
  return 0;
}
