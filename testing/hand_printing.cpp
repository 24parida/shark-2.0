#include "card.h"
#include <iostream>

int main() {
  using phevaluator::Card;
  Card a{"9h"};
  std::cout << a.describeCard() << "\n";
  return 0;
}
