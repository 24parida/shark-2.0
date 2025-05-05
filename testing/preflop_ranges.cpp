#include "hands/PreflopRange.hh"
#include <iostream>

int main() {
  PreflopRange range1{"AA,9Ts,32o"};
  range1.print();
  std::cout << '\n';
  std::cout << "-----------";
  std::cout << '\n';

  for (const auto &i : range1.preflop_combos) {
    std::cout << i.to_string() << "\n";
    std::cout << i.probability << "\n";
    std::cout << i.rel_probability << "\n";
    std::cout << "\n";
  }

  return 0;
}
