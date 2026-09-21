#define main shark_main
#include "../gui/gui.cpp"
#undef main
#include <stdexcept>

void check(bool value, const std::string &message) {
  if (!value) throw std::runtime_error(message);
}

std::string join(const std::vector<std::string> &parts) {
  std::string text;
  for (const auto &part : parts) {
    if (!text.empty()) text += ',';
    text += part;
  }
  return text;
}

void prepare(Wizard &w, const std::vector<std::string> &board, const std::string &hero = "AA", const std::string &villain = "KK", bool betting = false) {
  w.m_data.board = board;
  w.m_board = board;
  w.m_data.startingPot = 100;
  w.m_data.stackSize = 100;
  w.m_data.minBet = 2;
  w.m_data.allInThreshold = 1;
  w.m_data.heroRange = {hero};
  w.m_data.villainRange = {villain};
  w.m_pg4->setSelectedRange({villain});
  w.m_data.yourPos = "SB";
  w.m_data.theirPos = "BB";
  std::vector<Card> cards;
  for (const auto &card : board) cards.emplace_back(card);
  PreflopRange a(hero), b(villain);
  TreeBuilderSettings settings(a, b, 2, cards, 100, 100, 2, 1);
  settings.bet_sizing.flop = {};
  settings.bet_sizing.turn = {};
  settings.bet_sizing.river = {};
  if (betting) {
    settings.bet_sizing.turn.bet_sizes = {1};
    settings.bet_sizing.river.bet_sizes = {1};
  }
  GameTree tree(settings);
  w.m_root = tree.build();
  w.m_current_node = w.m_root.get();
  w.m_prm = PreflopRangeManager(a.preflop_combos, b.preflop_combos, cards);
  ParallelDCFR trainer(w.m_prm, cards, 100, 2, 1);
  trainer.load_trainer_modules(w.m_root.get());
  w.m_current_pot = 100;
  w.m_p1_stack = w.m_p2_stack = 100;
  w.m_p1_wager = w.m_p2_wager = 0;
}

void import_ranges() {
  Wizard w;
  for (auto *page : {w.m_pg3}) {
    check(PreflopRange(join(page->parseRangeString("77-55"))).num_hands == 18, "hero pair interval");
    check(PreflopRange(join(page->parseRangeString("ATs+"))).num_hands == 16, "hero suited plus");
  }
  check(PreflopRange(join(w.m_pg4->parseRangeString("77-55"))).num_hands == 18, "villain pair interval");
  check(PreflopRange(join(w.m_pg4->parseRangeString("ATs+"))).num_hands == 16, "villain suited plus");
}

void import_weights() {
  Wizard w;
  auto parsed = w.m_pg3->parseRangeString("AA:0,KK:0.25");
  w.m_pg3->setSelectedRange(parsed);
  PreflopRange range(join(w.m_pg3->getSelectedRange()));
  check(range.num_hands == 6, "zero-weight hands imported");
  for (const auto &hand : range.preflop_combos) check(hand.probability == 0.25f, "GUI import weight lost");
  for (auto *button : w.m_pg3->m_rangeBtns)
    if (std::string(button->label()) == "KK") check(button->selected(), "weighted hand not highlighted");
}

void navigation() {
  Wizard w;
  prepare(w, {"2c", "3c", "4c"});
  w.doAction("Check");
  w.doAction("Check");
  Node *chance = w.m_current_node;
  w.doCardSelected("Ah");
  check(w.m_current_node != chance, "isomorphic Ah selection ignored");
  check(w.m_current_node->get_node_type() == NodeType::ACTION_NODE, "turn action node missing");
  w.doUndo();
  check(w.m_current_node == chance, "undo did not restore chance node");
}

void edit_weights() {
  Wizard w;
  auto edit = [](auto *page) {
    page->setSelectedRange({"AcKc:0.125", "KK:0.25"});
    auto click = [&](const std::string &label) {
      auto button = std::find_if(page->m_rangeBtns.begin(), page->m_rangeBtns.end(),
                                 [&](const auto *b) { return label == b->label(); });
      check(button != page->m_rangeBtns.end(), "range button missing");
      (*button)->do_callback();
    };
    click("QQ");
    PreflopRange added(join(page->getSelectedRange()));
    check(added.num_hands == 13, "range edit changed combo count");
    for (const auto &hand : added.preflop_combos) {
      const float expected = hand.hand_type() == "AKs" ? 0.125f : hand.hand_type() == "KK" ? 0.25f : 1.0f;
      check(hand.probability == expected, "range edit lost weight");
    }
    click("QQ");
    click("KK");
    check(join(page->getSelectedRange()) == "AcKc:0.125", "range edit lost exact suits");
    click("AKs");
    check(page->getSelectedRange().empty(), "partial hand class not removed");
  };
  edit(w.m_pg3);
  edit(w.m_pg4);
}

void blocker_average() {
  Wizard w;
  prepare(w, {"2c", "3d", "7h", "8s", "9c"}, "AcKc,AdKd", "AcQc:0.75,JhJd:0.25");
  const auto &hands = w.m_prm.get_preflop_combos(1);
  auto node = std::make_unique<ActionNode>(nullptr, 1);
  node->push_action({Action::CHECK, 0});
  node->push_action({Action::BET, 100});
  node->init(hands.size());
  node->load_trainer(node.get());
  std::vector<float> strategy(hands.size() * 2), reach(hands.size(), 1);
  for (size_t i = 0; i < hands.size(); ++i)
    strategy[i + ((int(hands[i].hand1) & 3) == 0 ? hands.size() : 0)] = 1;
  node->get_trainer()->update_cum_strategy(strategy, reach, 0.0f);
  w.m_root = std::move(node);
  w.m_current_node = w.m_root.get();
  w.updateStrategyDisplay();
  w.showOverallStrategy();
  const auto &overall = w.m_overallStrategyCache.at(w.m_current_node).at(0);
  bool found = false;
  for (const auto &action : overall.actions) if (action.name == "Bet 100") {
    check(std::abs(action.prob - 0.2) < 0.0001, "average ignores opponent blockers");
    found = true;
  }
  check(found, "weighted bet missing");
  for (const auto *button : w.m_pg6->m_strategyBtns) if (std::string(button->label()) == "AKs")
    check(std::abs(button->m_strategy_colors.front().second - 0.2) < 0.0001, "grid ignores opponent blockers");
}

void original_board() {
  Wizard w;
  const std::vector<std::string> board = {"2c", "3c", "4c"};
  prepare(w, board);
  w.doAction("Check");
  w.doAction("Check");
  w.doCardSelected("5c");
  w.doBack6();
  check(w.m_data.board == board, "exploration changed solve board");
}

void cache() {
  Wizard w;
  prepare(w, {"2c", "3c", "4c"});
  w.m_data.stackSize = 2;
  w.m_data.iterations = 1;
  w.m_data.threadCount = 1;
  w.m_data.forceDonkCheck = true;
  w.do4Next();
  auto turn_actions = [&]() {
    auto *oop = static_cast<ActionNode *>(w.m_root.get());
    auto *ip = static_cast<ActionNode *>(oop->get_child(0));
    auto *chance = static_cast<ChanceNode *>(ip->get_child(0));
    return static_cast<ActionNode *>(chance->get_child(Card("5c")))->get_num_actions();
  };
  check(turn_actions() == 1, "donk check setting not active");
  w.m_data.forceDonkCheck = false;
  w.do4Next();
  check(turn_actions() > 1, "changed donk setting reused cached tree");
}

void average() {
  Wizard w;
  prepare(w, {"2c", "3d", "7h", "8s", "9c"}, "AKs", "QQ");
  auto &hands = w.m_prm.m_p1_preflop_combos;
  for (size_t i = 0; i < hands.size(); ++i) hands[i].probability = i == 0 ? 1 : 0.01f;
  auto node = std::make_unique<ActionNode>(nullptr, 1);
  node->push_action({Action::CHECK, 0});
  node->push_action({Action::BET, 100});
  node->init(hands.size());
  node->load_trainer(node.get());
  std::vector<float> strategy(hands.size() * 2), reach(hands.size(), 1);
  for (size_t i = 0; i < hands.size(); ++i) strategy[i + (i == 0 ? hands.size() : 0)] = 1;
  node->get_trainer()->update_cum_strategy(strategy, reach, 0.0f);
  w.m_root = std::move(node);
  w.m_current_node = w.m_root.get();
  w.updateStrategyDisplay();
  bool found = false;
  for (const auto *button : w.m_pg6->m_strategyBtns) {
    if (std::string(button->label()) != "AKs") continue;
    check(!button->m_strategy_colors.empty(), "missing hand grid strategy");
    check(std::abs(button->m_strategy_colors.front().second - 1.0 / 1.03) < 0.0001, "hand grid ignores weights");
    found = true;
  }
  check(found, "missing hand button");
  w.showOverallStrategy();
  const auto &overall = w.m_overallStrategyCache.at(w.m_current_node).at(0);
  for (const auto &action : overall.actions)
    if (action.name == "Bet 100") check(std::abs(action.prob - 1.0 / 1.03) < 0.0001, "overall average ignores weights");
}

void export_range() {
  Wizard w;
  prepare(w, {"2c", "3d", "7h", "8s", "9c"}, "AKs", "QQ");
  auto &hands = w.m_prm.m_p1_preflop_combos;
  for (size_t i = 0; i < hands.size(); ++i) hands[i].probability = i == 0 ? 1 : 0;
  const auto output = w.generateRangeString();
  check(output == "AcKc", "exact surviving combo lost: " + output);
}

void compound_swaps() {
  Wizard w;
  prepare(w, {"2c", "3c", "4c"}, "AA", "KK", true);
  w.doAction("Check");
  w.doAction("Check");
  Node *chance = w.m_current_node;
  for (int card = 0; card < 52; ++card) {
    const auto label = Card(card).describeCard();
    if (std::find(w.m_data.board.begin(), w.m_data.board.end(), label) != w.m_data.board.end()) continue;
    w.doCardSelected(label);
    check(w.m_current_node != chance, "legal turn card ignored: " + label);
    check(w.m_board.back() == label, "displayed card changed suit");
    w.doUndo();
    check(w.m_current_node == chance && w.m_suits == std::array<int, 4>{0, 1, 2, 3}, "turn undo retained suit swap");
  }
  w.doCardSelected("Ah");
  check(w.m_suits == std::array<int, 4>{0, 2, 1, 3}, "turn representative mapping");
  auto probability = [](int a, int b) {
    if (a < b) std::swap(a, b);
    return 0.1f + 0.05f * ((a & 3) + 2 * (b & 3));
  };
  auto seed = [&]() {
    auto *node = static_cast<ActionNode *>(w.m_current_node);
    const auto &hands = w.m_prm.get_preflop_combos(node->get_player());
    check(node->get_num_actions() == 2, "fixture needs check and bet");
    std::vector<float> strategy(hands.size() * 2), reach(hands.size(), 1);
    for (size_t i = 0; i < hands.size(); ++i) {
      strategy[i] = probability(hands[i].hand1, hands[i].hand2);
      strategy[i + hands.size()] = 1 - strategy[i];
    }
    node->get_trainer()->update_cum_strategy(strategy, reach, 0.0f);
  };
  seed();
  w.doAction("Check");
  w.doAction("Check");
  w.doCardSelected("Ks");
  check(w.m_suits == std::array<int, 4>{0, 3, 1, 2}, "river swap did not compose with turn swap");
  seed();
  const auto reach = w.computeComboReach(1);
  const auto &hands = w.m_prm.get_preflop_combos(1);
  const std::array<int, 4> turn_map{0, 2, 1, 3};
  const std::array<int, 4> river_map{0, 3, 1, 2};
  const std::vector<Card> board{Card("2c"), Card("3c"), Card("4c"), Card("Ah"), Card("Ks")};
  for (size_t i = 0; i < hands.size(); ++i) {
    const int a = hands[i].hand1, b = hands[i].hand2;
    const float expected = CardUtility::overlap(hands[i], board) ? 0 : probability((a & ~3) | turn_map[a & 3], (b & ~3) | turn_map[b & 3]);
    check(std::abs(reach[i] - expected) < 0.0001, "history reinterpreted using river suit swap");
  }
  PreflopRange exported(w.generateRangeString());
  check(exported.num_hands == 3, "export lost board blockers");
  for (const auto &hand : exported.preflop_combos) {
    const auto found = std::find(hands.begin(), hands.end(), hand);
    check(found != hands.end(), "export changed physical cards");
    check(std::abs(hand.probability - reach[found - hands.begin()]) < 0.0001, "export lost action history weight");
  }
  w.handleHandSelect("AA");
  const auto &displayed = w.m_pg6->m_comboDisplay->m_combos;
  check(displayed.size() == 3, "river display has wrong blocker filtering");
  for (const auto &combo : displayed) {
    const int a = Card(combo.combo.substr(0, 2)), b = Card(combo.combo.substr(2, 2));
    const float expected = probability((a & ~3) | river_map[a & 3], (b & ~3) | river_map[b & 3]);
    bool found = false;
    for (const auto &action : combo.actions) if (action.name == "Check") {
      check(std::abs(action.prob - expected) < 0.0001, "strategy shown for wrong physical suit");
      found = true;
    }
    check(found, "check strategy missing");
  }
  w.doUndo();
  check(w.m_suits == turn_map, "river undo lost turn mapping");
  w.doUndo();
  w.doUndo();
  w.doUndo();
  check(w.m_current_node == chance && w.m_suits == std::array<int, 4>{0, 1, 2, 3}, "compound undo failed");
  check(w.m_data.board == std::vector<std::string>{"2c", "3c", "4c"}, "original board changed");
}

int main(int argc, char **argv) {
  const std::map<std::string, std::function<void()>> tests = {
    {"import", import_ranges}, {"weights", import_weights}, {"navigation", navigation},
    {"board", original_board}, {"cache", cache}, {"average", average}, {"export", export_range},
    {"compound_swaps", compound_swaps}, {"edit_weights", edit_weights}, {"blocker_average", blocker_average}
  };
  try {
    check(argc == 2 && tests.count(argv[1]), "unknown GUI test");
    tests.at(argv[1])();
    std::cout << "PASS " << argv[1] << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL " << error.what() << '\n';
    return 1;
  }
}
