#include <vector>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include "Helper.hh"
#include "tree/GameTree.hh"
#include "solver/Solver.hh"

bool is_valid_action(const Action &, const GameState &);

void require(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}

void close(double actual, double expected, const std::string &message, double tolerance = 0.0002) {
  if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
    std::ostringstream out;
    out << message << ": got " << actual << ", expected " << expected;
    throw std::runtime_error(out.str());
  }
}

std::vector<Card> cards(const std::string &text) {
  std::vector<Card> result;
  std::istringstream in(text);
  std::string card;
  while (in >> card) result.emplace_back(card);
  return result;
}

std::vector<int> mapping(const std::vector<PreflopCombo> &hero, const std::vector<PreflopCombo> &villain) {
  std::vector<int> result(hero.size(), -1);
  for (size_t h = 0; h < hero.size(); ++h)
    for (size_t v = 0; v < villain.size(); ++v)
      if (hero[h] == villain[v]) result[h] = v;
  return result;
}

std::unique_ptr<TerminalNode> terminal(TerminalNode::TerminalType type, int pot = 100) {
  auto node = std::make_unique<TerminalNode>(nullptr, type);
  node->set_pot(pot);
  node->set_last_to_act(2);
  return node;
}

std::unique_ptr<Node> full_runouts(const std::vector<Card> &board) {
  if (board.size() == 5) return terminal(TerminalNode::SHOWDOWN);
  auto node = std::make_unique<ChanceNode>(nullptr, board.size() == 3 ? ChanceNode::DEAL_TURN : ChanceNode::DEAL_RIVER);
  for (int card = 0; card < 52; ++card) {
    if (CardUtility::overlap(Card(card), board)) continue;
    auto next = board;
    next.push_back(Card(card));
    node->add_child(full_runouts(next), card);
  }
  return node;
}

double pair_value(const PreflopCombo &hero, const PreflopCombo &villain, std::vector<Card> &board) {
  if (board.size() == 5) {
    const int h = CardUtility::get_rank(hero.hand1, hero.hand2, board);
    const int v = CardUtility::get_rank(villain.hand1, villain.hand2, board);
    return h > v ? 50.0 : h < v ? -50.0 : 0.0;
  }
  double value = 0;
  int count = 0;
  for (int card = 0; card < 52; ++card) {
    if (CardUtility::overlap(Card(card), board) || CardUtility::overlap(hero, Card(card)) || CardUtility::overlap(villain, Card(card))) continue;
    board.push_back(Card(card));
    value += pair_value(hero, villain, board);
    board.pop_back();
    ++count;
  }
  return value / count;
}

std::vector<float> oracle(const std::vector<PreflopCombo> &hero, const std::vector<PreflopCombo> &villain,
                          const std::vector<float> &reach, std::vector<Card> board) {
  std::vector<float> result(hero.size());
  for (size_t h = 0; h < hero.size(); ++h) {
    if (CardUtility::overlap(hero[h], board)) continue;
    double value = 0;
    for (size_t v = 0; v < villain.size(); ++v) {
      if (CardUtility::overlap(villain[v], board) || CardUtility::overlap(hero[h], villain[v])) continue;
      value += reach[v] * pair_value(hero[h], villain[v], board);
    }
    result[h] = value;
  }
  return result;
}

float br_value(Node *node, PreflopRange &hero, PreflopRange &villain, const std::vector<Card> &board, int player = 1) {
  PreflopRangeManager prm(player == 1 ? hero.preflop_combos : villain.preflop_combos,
                          player == 1 ? villain.preflop_combos : hero.preflop_combos, board);
  BestResponse br(prm);
  return br.get_best_response_ev(node, player, 3 - player, hero.preflop_combos, villain.preflop_combos,
                                  board, mapping(hero.preflop_combos, villain.preflop_combos));
}

TreeBuilderSettings settings(const PreflopRange &a, const PreflopRange &b, const std::vector<Card> &board) {
  TreeBuilderSettings s(a, b, 2, board, 100, 100, 2, 1.0f);
  s.bet_sizing.flop = {};
  s.bet_sizing.turn = {};
  s.bet_sizing.river = {};
  return s;
}

void normalization() {
  auto board = cards("2c 3d 7h 8s 9c");
  auto node = terminal(TerminalNode::SHOWDOWN);
  PreflopRange a("AA"), b("KK");
  for (float scale : {1.0f, 0.25f, 0.01f}) {
    for (auto &hand : b.preflop_combos) hand.probability = scale;
    close(br_value(node.get(), a, b, board), 50, "guaranteed river win");
    close(br_value(node.get(), b, a, board, 2), -50, "guaranteed river loss");
  }
  std::mt19937 rng(7341);
  PreflopRange mixed("AA,KK,QQ,AKs");
  PreflopRange other("AA,JJ,AQs,KQo");
  for (int trial = 0; trial < 40; ++trial) {
    for (auto &h : mixed.preflop_combos) h.probability = float(1 + rng() % 100) / 100;
    for (auto &h : other.preflop_combos) h.probability = float(1 + rng() % 100) / 100;
    std::vector<float> reach;
    for (const auto &h : other.preflop_combos) reach.push_back(h.probability);
    auto values = oracle(mixed.preflop_combos, other.preflop_combos, reach, board);
    double value = 0, mass = 0;
    for (size_t h = 0; h < mixed.preflop_combos.size(); ++h) {
      value += values[h] * mixed.preflop_combos[h].probability;
      for (const auto &v : other.preflop_combos)
        if (!CardUtility::overlap(mixed.preflop_combos[h], v)) mass += mixed.preflop_combos[h].probability * v.probability;
    }
    close(br_value(node.get(), mixed, other, board), value / mass, "weighted compatible pairs");
    close(br_value(node.get(), other, mixed, board, 2), -value / mass, "zero sum");
  }
}

void chance() {
  std::mt19937 rng(412);
  for (int trial = 0; trial < 32; ++trial) {
    std::vector<int> deck(52);
    std::iota(deck.begin(), deck.end(), 0);
    std::shuffle(deck.begin(), deck.end(), rng);
    auto board = trial == 0 ? cards("2c 2d 2h 3s") : std::vector<Card>{};
    if (trial != 0) for (int c = 0; c < (trial % 4 == 0 ? 3 : 4); ++c) board.emplace_back(deck[c]);
    PreflopRange a("AA,AKs"), b("KK,KQs");
    if (trial == 0) {
      a.preflop_combos = {{Card("2s"), Card("Ac"), 1.0f}};
      b.preflop_combos = {{Card("Kc"), Card("Kd"), 1.0f}};
    }
    std::vector<float> hr(a.preflop_combos.size(), 1), vr;
    for (size_t i = 0; i < b.preflop_combos.size(); ++i) vr.push_back(float(1 + rng() % 100) / 100);
    auto expected = oracle(a.preflop_combos, b.preflop_combos, vr, board);
    auto map = mapping(a.preflop_combos, b.preflop_combos);
    auto allin = terminal(TerminalNode::ALLIN);
    auto full = full_runouts(board);
    for (Node *node : std::array<Node *, 2>{allin.get(), full.get()}) {
      RiverRangeManager rrm;
      CFRHelper helper(node, 1, 2, a.preflop_combos, b.preflop_combos, hr, vr, board, 1, rrm, map);
      helper.compute();
      auto values = helper.get_result();
      PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
      BestResponse br(prm);
      br.get_best_response_ev(node, 1, 2, a.preflop_combos, b.preflop_combos, board, map);
      auto best = br.best_response(node, vr, board);
      for (size_t h = 0; h < values.size(); ++h) {
        close(values[h], expected[h], "CFR legal-runout enumeration", 0.002);
        close(best[h], expected[h], "BR legal-runout enumeration", 0.002);
      }
    }
  }
}

void isomorphism() {
  PreflopRange a("AA,QQ"), b("KK,JJ");
  for (const auto &text : {"2c 3c 4c", "2c 3d 4h", "2c 2d 3h", "2c 3c 4c 5c", "2c 3d 4h 5s", "2c 2d 3h 3s"}) {
    auto board = cards(text);
    GameTree tree(settings(a, b, board));
    auto reduced = tree.build();
    auto full = full_runouts(board);
    PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
    ParallelDCFR trainer(prm, board, 100, 2, 1);
    trainer.load_trainer_modules(reduced.get());
    close(br_value(reduced.get(), a, b, board), br_value(full.get(), a, b, board), "reduced/full BR", 0.001);
    std::vector<float> hr(a.preflop_combos.size(), 1), vr(b.preflop_combos.size(), 1);
    auto expected = oracle(a.preflop_combos, b.preflop_combos, vr, board);
    auto map = mapping(a.preflop_combos, b.preflop_combos);
    RiverRangeManager rrm;
    CFRHelper helper(reduced.get(), 1, 2, a.preflop_combos, b.preflop_combos, hr, vr, board, 1, rrm, map);
    helper.compute();
    auto actual = helper.get_result();
    for (size_t h = 0; h < actual.size(); ++h) close(actual[h], expected[h], "isomorphic CFR hand", 0.002);
  }
}

void betting() {
  PreflopRange a("AA"), b("KK");
  TreeBuilderSettings s(a, b, 2, cards("2c 3d 7h 8s 9c"), 100, 30, 2, 0.9f);
  GameTree tree(s);
  auto state = tree.get_init_state();
  require(is_valid_action({Action::BET, 2}, state), "minimum bet rejected");
  state.apply_action({Action::BET, 10});
  state.apply_action({Action::RAISE, 40});
  require(state.minimum_raise_size == 30, "raise increment not updated");
  require(!is_valid_action({Action::RAISE, 50}, state), "undersized raise accepted");
  require(is_valid_action({Action::RAISE, 70}, state), "minimum raise rejected");
  auto root = tree.build();
  auto find = [](Node *node, Action::ActionType type, int amount) -> Node * {
    auto *action = static_cast<ActionNode *>(node);
    for (int i = 0; i < action->get_num_actions(); ++i)
      if (action->get_action(i).type == type && action->get_action(i).amount == amount) return action->get_child(i);
    throw std::runtime_error("missing legal action " + std::to_string(amount));
  };
  find(find(find(root.get(), Action::BET, 30), Action::RAISE, 75), Action::RAISE, 100);
  for (int bet = 2; bet <= 30; ++bet) {
    for (int raise = 2 * bet; raise < 100; ++raise) {
      auto next = tree.get_init_state();
      next.apply_action({Action::BET, bet});
      next.apply_action({Action::RAISE, raise});
      require(next.minimum_raise_size == raise - bet, "raise sequence minimum");
      for (int amount = raise + 1; amount <= 100; ++amount)
        require(is_valid_action({Action::RAISE, amount}, next) == (amount == 100 || amount - raise >= raise - bet), "raise legality enumeration");
    }
  }
}

void keys() {
  std::set<decltype(CardUtility::board_to_key(cards("Ac 2d 3h")))> seen;
  for (int a = 0; a < 52; ++a)
    for (int b = a + 1; b < 52; ++b)
      for (int c = b + 1; c < 52; ++c)
        require(seen.insert(CardUtility::board_to_key({Card(a), Card(b), Card(c)})).second, "flop key collision");
  auto board = cards("Ac 2d 3h 4s 5c");
  const auto key = CardUtility::board_to_key(board);
  require(key != CardUtility::board_to_key(cards("Ac 2d 3h 4s")), "board length collision");
}

void ranges() {
  require(PreflopRange("77-55").num_hands == 18, "pair interval");
  require(PreflopRange("ATs+").num_hands == 16, "suited plus");
  PreflopRange weighted("AA:0,KK:0.25");
  double total = 0;
  for (const auto &hand : weighted.preflop_combos) {
    require(hand.hand1.describeCard()[0] == 'K', "zero-weight hand retained");
    close(hand.probability, 0.25, "weight discarded");
    total += hand.probability;
  }
  close(total, 1.5, "weighted combo count");
}

void convergence() {
  for (bool compressed : {false, true}) {
    DCFR::compress_strategy = compressed;
    PreflopRange a("AA,QQ"), b("KK,JJ");
    auto board = cards("2c 3d 7h 8s 9c");
    auto s = settings(a, b, board);
    s.bet_sizing.river.bet_sizes = {1.0f};
    GameTree tree(s);
    auto root = tree.build();
    PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
    ParallelDCFR trainer(prm, board, 100, 2, 4);
    trainer.train(root.get(), 2000);
    BestResponse br(prm);
    const float exploit = br.get_exploitability(root.get(), 2000, board, 100, 2);
    std::cout << "compressed=" << compressed << " exploitability=" << exploit << '\n';
    require(std::isfinite(exploit) && exploit < 0.05f, "river convergence");
    std::function<void(Node *)> check = [&](Node *node) {
      if (node->get_node_type() != NodeType::ACTION_NODE) return;
      auto *action = static_cast<ActionNode *>(node);
      auto strategy = action->get_average_strat();
      for (int h = 0; h < action->get_num_hands(); ++h) {
        double sum = 0;
        for (int a = 0; a < action->get_num_actions(); ++a) {
          float probability = strategy[h + a * action->get_num_hands()];
          require(std::isfinite(probability) && probability >= 0 && probability <= 1, "invalid strategy probability");
          sum += probability;
        }
        close(sum, 1, "strategy sum");
      }
      for (const auto &child : action->get_children()) check(child.get());
    };
    check(root.get());
  }
}

std::vector<double> direct_values(Node *node, int player, const std::vector<PreflopCombo> &hero,
                                  const std::vector<PreflopCombo> &villain, const std::vector<double> &reach,
                                  const std::vector<Card> &board, bool best_response) {
  std::vector<double> result(hero.size());
  if (node->get_node_type() == NodeType::TERMINAL_NODE) {
    const auto *term = static_cast<TerminalNode *>(node);
    for (size_t h = 0; h < hero.size(); ++h) {
      if (CardUtility::overlap(hero[h], board)) continue;
      for (size_t v = 0; v < villain.size(); ++v) {
        if (CardUtility::overlap(villain[v], board) || CardUtility::overlap(hero[h], villain[v])) continue;
        double payoff;
        if (term->get_type() == TerminalNode::UNCONTESTED) {
          payoff = player == term->get_last_to_act() ? -term->get_pot() / 2.0 : term->get_pot() / 2.0;
        } else {
          auto runout = board;
          payoff = pair_value(hero[h], villain[v], runout) * term->get_pot() / 100.0;
        }
        result[h] += payoff * reach[v];
      }
    }
    return result;
  }
  require(node->get_node_type() == NodeType::ACTION_NODE, "direct evaluator expects river tree");
  const auto *action = static_cast<ActionNode *>(node);
  const auto strategy = best_response ? action->get_average_strat() : action->get_current_strat();
  if (action->get_player() == player && best_response)
    std::fill(result.begin(), result.end(), -std::numeric_limits<double>::infinity());
  for (int a = 0; a < action->get_num_actions(); ++a) {
    auto next = reach;
    if (action->get_player() != player)
      for (size_t v = 0; v < villain.size(); ++v) next[v] *= strategy[v + a * villain.size()];
    auto values = direct_values(action->get_child(a), player, hero, villain, next, board, best_response);
    for (size_t h = 0; h < hero.size(); ++h) {
      if (action->get_player() == player) {
        if (best_response) result[h] = std::max(result[h], values[h]);
        else result[h] += strategy[h + a * hero.size()] * values[h];
      } else result[h] += values[h];
    }
  }
  return result;
}

double direct_ev(Node *node, int player, const PreflopRange &hero, const PreflopRange &villain,
                 const std::vector<Card> &board) {
  std::vector<double> reach;
  for (const auto &v : villain.preflop_combos) reach.push_back(v.probability);
  auto values = direct_values(node, player, hero.preflop_combos, villain.preflop_combos, reach, board, true);
  double total = 0, mass = 0;
  for (size_t h = 0; h < hero.preflop_combos.size(); ++h) {
    const auto &hand = hero.preflop_combos[h];
    total += hand.probability * values[h];
    if (CardUtility::overlap(hand, board)) continue;
    for (const auto &v : villain.preflop_combos)
      if (!CardUtility::overlap(v, board) && !CardUtility::overlap(hand, v)) mass += hand.probability * v.probability;
  }
  return total / mass;
}

void random_strategy(Node *node, std::mt19937 &rng) {
  if (node->get_node_type() != NodeType::ACTION_NODE) return;
  auto *action = static_cast<ActionNode *>(node);
  std::vector<float> strategy(action->get_num_hands() * action->get_num_actions());
  for (int h = 0; h < action->get_num_hands(); ++h) {
    float total = 0;
    for (int a = 0; a < action->get_num_actions(); ++a) total += strategy[h + a * action->get_num_hands()] = 1 + rng() % 100;
    for (int a = 0; a < action->get_num_actions(); ++a) strategy[h + a * action->get_num_hands()] /= total;
  }
  action->get_trainer()->update_cum_strategy(strategy, std::vector<float>(action->get_num_hands(), 1), 0.0f);
  action->get_trainer()->update_regrets(strategy, std::vector<float>(action->get_num_hands(), 0), 1);
  for (const auto &child : action->get_children()) random_strategy(child.get(), rng);
}

void response() {
  std::mt19937 rng(51792);
  for (int trial = 0; trial < 24; ++trial) {
    auto board = trial % 2 ? cards("Ac Kd 7h 8s 9c") : cards("2c 3d 7h 8s 9c");
    PreflopRange a("AA,QQ,AKs"), b("KK,JJ,AKs");
    for (auto &h : a.preflop_combos) h.probability = (1 + rng() % 100) / 100.0f;
    for (auto &h : b.preflop_combos) h.probability = (1 + rng() % 100) / 100.0f;
    auto s = settings(a, b, board);
    s.bet_sizing.river.bet_sizes = {0.5f, 1.0f};
    s.bet_sizing.river.raise_sizes = {0.5f, 1.0f};
    GameTree tree(s);
    auto root = tree.build();
    PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
    ParallelDCFR trainer(prm, board, 100, 2, 1);
    trainer.load_trainer_modules(root.get());
    random_strategy(root.get(), rng);
    close(br_value(root.get(), a, b, board), direct_ev(root.get(), 1, a, b, board), "independent OOP best response", 0.001);
    close(br_value(root.get(), b, a, board, 2), direct_ev(root.get(), 2, b, a, board), "independent IP best response", 0.001);
    std::vector<float> hr, vr;
    for (const auto &h : a.preflop_combos) hr.push_back(h.probability);
    for (const auto &v : b.preflop_combos) vr.push_back(v.probability);
    auto expected = direct_values(root.get(), 1, a.preflop_combos, b.preflop_combos,
                                  std::vector<double>(vr.begin(), vr.end()), board, false);
    auto map = mapping(a.preflop_combos, b.preflop_combos);
    RiverRangeManager rrm;
    CFRHelper helper(root.get(), 1, 2, a.preflop_combos, b.preflop_combos, hr, vr, board, 1, rrm, map);
    helper.compute();
    const auto actual = helper.get_result();
    for (size_t h = 0; h < actual.size(); ++h) close(actual[h], expected[h], "CFR strategy value", 0.002);
  }
}

void trained_isomorphism() {
  for (const auto &text : {"2c 3c 4c", "2c 3c 4c 5d", "2c 2d 3h 3s"}) {
    auto board = cards(text);
    PreflopRange a("AA,QQ"), b("KK,JJ");
    std::array<double, 2> result;
    for (int reduced = 0; reduced < 2; ++reduced) {
      auto s = settings(a, b, board);
      s.use_isomorphism = reduced;
      s.bet_sizing.river.bet_sizes = {1};
      if (board.size() == 4) s.bet_sizing.turn.bet_sizes = {1};
      GameTree tree(s);
      auto root = tree.build();
      PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
      ParallelDCFR trainer(prm, board, 100, 2, reduced ? 4 : 1);
      trainer.train(root.get(), 400);
      result[reduced] = br_value(root.get(), a, b, board);
      const auto exploit = (result[reduced] + br_value(root.get(), b, a, board, 2)) / 2;
      require(exploit >= -0.001 && exploit < 0.1, "multistreet convergence");
      std::cout << text << " reduced=" << reduced << " BR=" << result[reduced] << " exploit=" << exploit << '\n';
    }
    close(result[0], result[1], "trained full/reduced value", 0.01);
  }
}

void range_roundtrip() {
  for (const auto &input : {"AcKc", "AcKc:0.125,AdKd:0.875", "77-55,ATs+:0.5", "AA:0.25,AA", "98s+", "98s-65s", "AQo-86o", "AA:0,AA", "22+,AKs,AKo"}) {
    PreflopRange original(input);
    std::string text;
    for (const auto &token : original.to_strings()) text += token + ',';
    PreflopRange copy(text);
    require(original.num_hands == copy.num_hands, "roundtrip combo count");
    for (const auto &hand : original.preflop_combos) {
      auto found = std::find(copy.preflop_combos.begin(), copy.preflop_combos.end(), hand);
      require(found != copy.preflop_combos.end(), "roundtrip lost suit");
      close(found->probability, hand.probability, "roundtrip weight", 0);
    }
  }
  require(PreflopRange("98s+").num_hands == 24, "connector plus");
  require(PreflopRange("98s-65s").num_hands == 16, "connector interval");
  require(PreflopRange("AQo-86o").num_hands == 84, "gapper interval");
  require(PreflopRange("AA,AA").num_hands == 6, "duplicate combos");
  require(PreflopRange("AA:0,AA").num_hands == 0, "overlapping weight precedence");
  for (const auto &input : {"AA:nan", "AA:inf", "AA:-0.1", "AA:1.1", "AA:0.2x", "Aa", "AAo", "AcAc", "AKs-", "-", "AA:"}) {
    bool rejected = false;
    try { PreflopRange range(input); }
    catch (const std::exception &) { rejected = true; }
    require(rejected, std::string("invalid range accepted: ") + input);
  }
}

void tiny_weights() {
  PreflopRange a("AdKd:0.0000001"), b("QQ");
  auto board = cards("2c 3c 4c");
  std::array<double, 2> value;
  for (int reduced = 0; reduced < 2; ++reduced) {
    auto s = settings(a, b, board);
    s.use_isomorphism = reduced;
    GameTree tree(s);
    auto root = tree.build();
    PreflopRangeManager prm(a.preflop_combos, b.preflop_combos, board);
    ParallelDCFR trainer(prm, board, 100, 2, 1);
    trainer.load_trainer_modules(root.get());
    value[reduced] = br_value(root.get(), a, b, board);
  }
  close(value[1], value[0], "small asymmetric weights cannot share suits");
}

int main(int argc, char **argv) {
  const std::vector<std::pair<std::string, std::function<void()>>> tests = {
    {"normalization", normalization}, {"chance", chance}, {"isomorphism", isomorphism},
    {"betting", betting}, {"keys", keys}, {"ranges", ranges}, {"convergence", convergence},
    {"response", response}, {"trained_isomorphism", trained_isomorphism}, {"range_roundtrip", range_roundtrip},
    {"tiny_weights", tiny_weights}
  };
  int failed = 0;
  for (const auto &[name, test] : tests) {
    if (argc > 1 && name != argv[1]) continue;
    try { test(); std::cout << "PASS " << name << '\n'; }
    catch (const std::exception &error) { ++failed; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
  }
  return failed ? 1 : 0;
}
