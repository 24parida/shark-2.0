#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "tree/GameTree.hh"
#include "solver/Solver.hh"

int main(int argc, char **argv) {
  if (argc != 2) return 1;
  std::ifstream input(argv[1]);
  if (!input) return 1;
  std::cout << std::setprecision(9);
  std::string line;
  while (std::getline(input, line)) {
    std::istringstream fields(line);
    std::vector<std::string> parts;
    std::string part;
    while (std::getline(fields, part, '|')) parts.push_back(part);
    if (parts.size() != 5) return 1;
    PreflopRange a(parts[1]), b(parts[2]);
    std::vector<Card> board;
    std::istringstream cards(parts[3]);
    while (cards >> part) board.emplace_back(part);
    TreeBuilderSettings settings(a, b, 2, board, 100, 100, 2, 1);
    settings.bet_sizing.flop = {};
    settings.bet_sizing.turn = {};
    settings.bet_sizing.river = {{1}, {}};
    GameTree tree(settings);
    auto root = tree.build();
    PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
    DCFR::compress_strategy = true;
    ParallelDCFR trainer(prm, board, 100, 2, 4);
    trainer.train(root.get(), std::stoi(parts[4]));
    BestResponse br(prm);
    std::vector<int> ab(a.num_hands, -1), ba(b.num_hands, -1);
    for (int h = 0; h < a.num_hands; ++h)
      for (int v = 0; v < b.num_hands; ++v)
        if (a.preflop_combos[h] == b.preflop_combos[v]) { ab[h] = v; ba[v] = h; }
    auto ev1 = br.get_best_response_ev(root.get(), 1, 2, a.preflop_combos, b.preflop_combos, board, ab);
    auto ev2 = br.get_best_response_ev(root.get(), 2, 1, b.preflop_combos, a.preflop_combos, board, ba);
    std::cout << parts[0] << ',' << ev1 << ',' << ev2 << ',' << (ev1 + ev2) / 2 << std::endl;
  }
}
