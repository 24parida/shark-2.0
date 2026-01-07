#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>
#include <sstream>

// Solver headers
#include "hands/PreflopRange.hh"
#include "hands/PreflopRangeManager.hh"
#include "solver/Solver.hh"
#include "tree/GameTree.hh"
#include "tree/Nodes.hh"

// GUI components
#include "components/Page1_Settings.hh"
#include "components/Page2_Board.hh"
#include "components/Page3_HeroRange.hh"
#include "components/Page4_VillainRange.hh"
#include "components/Page5_Progress.hh"
#include "components/Page6_Strategy.hh"
#include "utils/RangeData.hh"
#include "utils/MemoryUtil.hh"

static const std::vector<std::string> RANKS = {
    "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
static const std::vector<char> SUITS = {'h', 'd', 'c', 's'};

class Wizard : public Fl_Window {
  struct UserInputs {
    int stackSize{}, startingPot{}, minBet{}, iterations{}, threadCount{};
    float allInThreshold{};
    std::string potType, yourPos, theirPos;
    std::vector<std::string> board;
    std::vector<std::string> heroRange;
    std::vector<std::string> villainRange;
    float min_exploitability{};
    bool autoImportRanges{true};
  } m_data;

  Node *m_current_node;
  PreflopRangeManager m_prm;
  std::unique_ptr<Node> m_root;

  // Track game state
  int m_current_pot;
  int m_p1_stack;
  int m_p2_stack;
  int m_p1_wager;
  int m_p2_wager;

  // Page components
  Page1_Settings *m_pg1;
  Page2_Board *m_pg2;
  Page3_HeroRange *m_pg3;
  Page4_VillainRange *m_pg4;
  Page5_Progress *m_pg5;
  Page6_Strategy *m_pg6;

  // Add undo history tracking
  struct GameState {
    Node *node;
    int p1_stack;
    int p2_stack;
    int current_pot;
    int p1_wager;
    int p2_wager;
    std::vector<std::string> board;
  };
  std::vector<GameState> m_history;

  // Callbacks for page navigation
  static void cb1Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do1Next(); }
  static void cb2Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack2(); }
  static void cb2Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do2Next(); }
  static void cb3Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack3(); }
  static void cb3Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do3Next(); }
  static void cb4Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack4(); }
  static void cb4Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do4Next(); }

  // Page1 -> Page2
  void do1Next() {
    // Validate all fields are filled
    if (m_pg1->getStackSize() <= 0 || m_pg1->getStartingPot() <= 0 ||
        m_pg1->getMinBet() <= 0 || m_pg1->getAllInThreshold() <= 0 ||
        m_pg1->getIterations() <= 0) {
      fl_message("Please fill out all fields with valid values.");
      return;
    }

    // Check positions differ
    std::string yourPos = m_pg1->getYourPosition();
    std::string theirPos = m_pg1->getTheirPosition();
    if (yourPos == theirPos) {
      fl_message("Positions must differ.");
      return;
    }

    // Save settings
    m_data.stackSize = m_pg1->getStackSize();
    m_data.startingPot = m_pg1->getStartingPot();
    m_data.minBet = m_pg1->getMinBet();
    m_data.allInThreshold = m_pg1->getAllInThreshold();
    m_data.iterations = m_pg1->getIterations();
    m_data.threadCount = m_pg1->getThreadCount();
    m_data.min_exploitability = m_pg1->getMinExploitability();
    m_data.potType = m_pg1->getPotType();
    m_data.yourPos = yourPos;
    m_data.theirPos = theirPos;
    m_data.autoImportRanges = m_pg1->getAutoImport();

    m_pg1->hide();
    m_pg2->show();
  }

  // Page2 actions
  void doBack2() {
    m_pg3->clearSelection();
    m_pg4->clearSelection();
    m_pg2->hide();
    m_pg1->show();
  }

  void do2Next() {
    auto selectedCards = m_pg2->getSelectedCards();
    if (selectedCards.size() < 3 || selectedCards.size() > 5) {
      fl_message("Select 3-5 cards.");
      return;
    }

    m_data.board = selectedCards;
    m_pg3->clearSelection();

    m_pg2->hide();
    m_pg3->show();

    // Auto-fill hero range based on position and pot type
    if (m_data.autoImportRanges) {
      auto range = RangeData::getRangeForPosition(m_data.yourPos, m_data.potType, true);
      m_pg3->setSelectedRange(range);
    }
  }

  // Page3 hero range
  void doBack3() {
    m_pg3->hide();
    m_pg2->show();
  }

  void do3Next() {
    m_data.heroRange = m_pg3->getSelectedRange();

    if (m_data.heroRange.empty()) {
      fl_message("Please select at least one hand for your range.");
      return;
    }

    m_pg3->hide();
    m_pg4->show();

    // Auto-fill villain range based on position and pot type
    if (m_data.autoImportRanges) {
      auto range = RangeData::getRangeForPosition(m_data.theirPos, m_data.potType, false);
      m_pg4->setSelectedRange(range);
    }
  }

  // Page4 villain range
  void doBack4() {
    m_pg4->hide();
    m_pg3->show();

    // Restore previous hero range selections if auto-import is enabled
    if (m_data.autoImportRanges) {
      auto range = RangeData::getRangeForPosition(m_data.yourPos, m_data.potType, true);
      m_pg3->setSelectedRange(range);
    }
  }

  // Page4 villain range
  void do4Next() {
    m_data.villainRange = m_pg4->getSelectedRange();

    if (m_data.villainRange.empty()) {
      fl_message("Please select at least one hand for the villain's range.");
      return;
    }

    m_pg4->hide();
    m_pg5->show();
    Fl::check();

    // Build tree and train
    runTraining();
  }

  void runTraining() {
    // Helper to turn vector<string> → comma‑list
    auto join = [](const std::vector<std::string> &v) {
      std::string s;
      for (size_t i = 0; i < v.size(); ++i) {
        if (i)
          s += ",";
        s += v[i];
      }
      return s;
    };

    // Build PreflopRange from your & villain selections
    PreflopRange range1{join(m_data.heroRange)};
    PreflopRange range2{join(m_data.villainRange)};

    // Convert board labels into Cards
    std::vector<Card> board;
    for (auto &lbl : m_data.board)
      board.emplace_back(lbl.c_str());

    // Figure out who is in‐position
    int heroPos = RangeData::getPositionIndex(m_data.yourPos);
    int villainPos = RangeData::getPositionIndex(m_data.theirPos);
    int ip = (heroPos > villainPos ? 1 : 2);

    // Assemble settings
    TreeBuilderSettings settings{range1,
                                 range2,
                                 ip,
                                 board,
                                 m_data.stackSize,
                                 m_data.startingPot,
                                 m_data.minBet,
                                 m_data.allInThreshold};

    // Build manager + tree
    m_prm = PreflopRangeManager(range1.preflop_combos, range2.preflop_combos,
                                settings.initial_board);
    GameTree game_tree{settings};

    // Estimate memory
    auto stats = game_tree.getTreeStats();
    size_t estimatedMemory = stats.estimateMemoryBytes();
    size_t availableMemory = MemoryUtil::getAvailableMemory();

    m_pg5->setMemoryEstimate(estimatedMemory, availableMemory);
    m_pg5->setStatus("Building game tree...");
    Fl::check();

    if (!m_pg5->isMemoryOk()) {
      fl_alert("Warning: Estimated memory usage exceeds available memory.\n"
               "The solver may run slowly or fail.");
    }

    // Build tree
    m_root = game_tree.build();

    m_pg5->setStatus("Training solver...");
    m_pg5->reset();
    Fl::check();

    // Create trainer with thread count and progress callback
    ParallelDCFR trainer{m_prm, settings.initial_board, settings.starting_pot,
                         settings.in_position_player, m_data.threadCount};

    // Progress callback
    auto progress_cb = [this](int current, int total, float exploit) {
      m_pg5->setIteration(current, total);
      m_pg5->setProgress(current, total);
      if (exploit >= 0) {
        m_pg5->setExploitability(exploit);
      }
      Fl::check();
    };

    // Run training
    trainer.train(m_root.get(), m_data.iterations, m_data.min_exploitability, progress_cb);

    // Initialize pot and stack tracking
    m_current_pot = m_data.startingPot;
    m_p1_stack = m_p2_stack = m_data.stackSize;
    m_p1_wager = m_p2_wager = 0;

    // Store current node and show strategy display
    m_current_node = m_root.get();
    m_pg5->hide();
    m_pg6->show();
    updateStrategyDisplay();
    Fl::check();
  }

  void updatePotAndStacks(const Action &action, int player) {
    // Update wagers based on action
    int &current_wager = (player == 1) ? m_p1_wager : m_p2_wager;
    int &other_wager = (player == 1) ? m_p2_wager : m_p1_wager;
    int &current_stack = (player == 1) ? m_p1_stack : m_p2_stack;
    int &other_stack = (player == 1) ? m_p2_stack : m_p1_stack;

    switch (action.type) {
    case Action::FOLD:
      // Pot and all wagers go to other player
      other_stack += m_current_pot + current_wager + other_wager;
      m_current_pot = 0;
      m_p1_wager = m_p2_wager = 0;
      break;

    case Action::CHECK:
      if (current_wager == other_wager && current_wager > 0) {
        // If both players have wagered equally, move wagers to pot
        m_current_pot += current_wager + other_wager;
        m_p1_wager = m_p2_wager = 0;
      }
      break;

    case Action::CALL: {
      int call_amount = other_wager - current_wager;
      current_stack -= call_amount;
      current_wager = other_wager; // Match the other wager
      // Both wagers go to pot
      m_current_pot += current_wager + other_wager;
      m_p1_wager = m_p2_wager = 0;
    } break;

    case Action::BET: {
      current_stack -= action.amount;
      current_wager = action.amount;
    } break;

    case Action::RAISE: {
      int additional_amount = action.amount - current_wager;
      current_stack -= additional_amount;
      current_wager = action.amount;
    } break;
    }
  }

  void updateStrategyDisplay() {
    if (m_current_node &&
        m_current_node->get_node_type() == NodeType::TERMINAL_NODE) {
      m_pg6->setTitle("Terminal Node - Hand Complete");
      return;
    }

    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE) {

      // If we're at a chance node, show the card selection view
      if (m_current_node && m_current_node->get_node_type() == NodeType::CHANCE_NODE) {
        auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);
        std::string prompt = "Select ";
        prompt += (chance_node->get_type() == ChanceNode::ChanceType::DEAL_TURN ? "Turn" : "River");
        prompt += " Card";
        m_pg6->setTitle(prompt);

        // TODO: Update Page6_Strategy to handle chance node card selection
      }
      return;
    }

    // For action nodes, show strategy
    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    const auto &strategy = action_node->get_average_strat();
    const auto &actions = action_node->get_actions();

    std::string title = (action_node->get_player() == 1 ? "Hero's" : "Villain's") + std::string(" Turn");
    m_pg6->setTitle(title);

    // Update board info
    std::string board = "Board: ";
    for (const auto &card : m_data.board) {
      board += card + " ";
    }
    m_pg6->setBoardInfo(board);

    // Update pot/stack info
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_pg6->setPotInfo(info);

    // Create map to store aggregated strategies for each hand type
    std::map<std::string, std::vector<float>> handTypeStrategies;
    std::map<std::string, int> handTypeCounts;

    // First pass: Aggregate all strategies for each hand type
    size_t num_hands = hands.size();
    for (size_t i = 0; i < num_hands; ++i) {
      const auto &h = hands[i];
      std::string hand_str = h.to_string();

      // Convert hand string format from "(Ah, Ad)" to "AhAd"
      hand_str = hand_str.substr(1, hand_str.length() - 2);
      hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ' '),
                     hand_str.end());
      hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ','),
                     hand_str.end());

      // Get the rank+suit format (e.g., "AKs" from "AhKh")
      std::string rank1 = hand_str.substr(0, 1);
      std::string rank2 = hand_str.substr(2, 1);
      bool suited = hand_str[1] == hand_str[3];
      std::string hand_format = rank1 + rank2 + (suited ? "s" : "o");
      if (rank1 == rank2)
        hand_format = rank1 + rank2;

      // Check if combo overlaps with board
      bool overlaps = false;
      for (const auto &board_card : m_data.board) {
        Card card(board_card.c_str());
        if (h.hand1 == card || h.hand2 == card) {
          overlaps = true;
          break;
        }
      }

      if (!overlaps) {
        // Initialize strategy vector if needed
        if (handTypeStrategies.find(hand_format) == handTypeStrategies.end()) {
          handTypeStrategies[hand_format] =
              std::vector<float>(actions.size(), 0.0f);
          handTypeCounts[hand_format] = 0;
        }

        // Add this combo's strategy - using correct indexing
        for (size_t a = 0; a < actions.size(); ++a) {
          size_t strat_idx = i + a * num_hands;
          if (strat_idx < strategy.size()) {
            handTypeStrategies[hand_format][a] += strategy[strat_idx];
          }
        }
        handTypeCounts[hand_format]++;
      }
    }

    // Build strategy map for Page6_Strategy
    std::map<std::string, std::map<std::string, float>> strategyMap;
    for (const auto &[hand, stratVec] : handTypeStrategies) {
      int count = handTypeCounts[hand];
      std::map<std::string, float> actionProbs;

      for (size_t i = 0; i < actions.size(); ++i) {
        const auto &action = actions[i];
        float prob = stratVec[i] / count;

        std::string actionStr;
        switch (action.type) {
        case Action::FOLD:
          actionStr = "Fold";
          break;
        case Action::CHECK:
          actionStr = "Check";
          break;
        case Action::CALL:
          actionStr = "Call " + std::to_string(action.amount);
          break;
        case Action::BET:
          actionStr = "Bet " + std::to_string(action.amount);
          break;
        case Action::RAISE:
          actionStr = "Raise to " + std::to_string(action.amount);
          break;
        }

        if (prob > 0.001f) {
          actionProbs[actionStr] = prob;
        }
      }

      strategyMap[hand] = actionProbs;
    }

    // Update Page6 strategy grid
    m_pg6->updateStrategyGrid(strategyMap);

    // Set available actions
    std::vector<std::string> actionLabels;
    for (const auto &action : actions) {
      std::string label;
      switch (action.type) {
      case Action::FOLD:
        label = "Fold";
        break;
      case Action::CHECK:
        label = "Check";
        break;
      case Action::CALL:
        label = "Call " + std::to_string(action.amount);
        break;
      case Action::BET:
        label = "Bet " + std::to_string(action.amount);
        break;
      case Action::RAISE:
        label = "Raise to " + std::to_string(action.amount);
        break;
      }
      actionLabels.push_back(label);
    }
    m_pg6->setActions(actionLabels);
  }

  void doAction(const std::string &actionStr) {
    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

    // Save current state before action
    GameState state{m_current_node, m_p1_stack, m_p2_stack,  m_current_pot,
                    m_p1_wager,     m_p2_wager, m_data.board};
    m_history.push_back(state);

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &actions = action_node->get_actions();

    // Find which action was clicked
    size_t action_idx = 0;
    for (; action_idx < actions.size(); ++action_idx) {
      std::string label;
      const auto &action = actions[action_idx];
      switch (action.type) {
      case Action::FOLD:
        label = "Fold";
        break;
      case Action::CHECK:
        label = "Check";
        break;
      case Action::CALL:
        label = "Call " + std::to_string(action.amount);
        break;
      case Action::BET:
        label = "Bet " + std::to_string(action.amount);
        break;
      case Action::RAISE:
        label = "Raise to " + std::to_string(action.amount);
        break;
      }
      if (label == actionStr)
        break;
    }

    if (action_idx >= actions.size())
      return;

    // Update pot and stacks based on action
    updatePotAndStacks(actions[action_idx], action_node->get_player());

    // Update display
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_pg6->setPotInfo(info);

    // Navigate to next node
    m_current_node = action_node->get_child(action_idx);

    if (m_current_node->get_node_type() == NodeType::CHANCE_NODE) {
      // TODO: Handle chance node card selection
      updateStrategyDisplay();
    } else {
      updateStrategyDisplay();
    }
  }

  void doBack6() {
    // Reset all game state
    m_current_node = nullptr;
    m_p1_stack = m_data.stackSize;
    m_p2_stack = m_data.stackSize;
    m_current_pot = m_data.startingPot;
    m_p1_wager = m_p2_wager = 0;

    // Reset board to original flop/turn selection
    std::vector<std::string> original_board;
    for (size_t i = 0; i < std::min(size_t(4), m_data.board.size()); i++) {
      original_board.push_back(m_data.board[i]);
    }
    m_data.board = original_board;

    // Clear history
    m_history.clear();

    m_pg6->hide();
    m_pg4->show(); // Go back to range selection
  }

  void doUndo() {
    if (m_history.empty())
      return;

    // Restore previous state
    auto state = m_history.back();
    m_history.pop_back();

    m_current_node = state.node;
    m_p1_stack = state.p1_stack;
    m_p2_stack = state.p2_stack;
    m_current_pot = state.current_pot;
    m_p1_wager = state.p1_wager;
    m_p2_wager = state.p2_wager;
    m_data.board = state.board;

    // Update displays
    updateStrategyDisplay();

    // Update pot/stack display
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_pg6->setPotInfo(info);

    // Update board display
    std::string board = "Board: ";
    for (const auto &card : m_data.board) {
      board += card + " ";
    }
    m_pg6->setBoardInfo(board);
  }

public:
  Wizard(const char *L = 0) : Fl_Window(100, 100, L) {
    init();
  }

private:
  void init() {
    // Get primary screen work area
    int sx, sy, sw, sh;
    Fl::screen_work_area(sx, sy, sw, sh, 0);

    // Calculate window size (80% of screen)
    int new_w = static_cast<int>(sw * 0.8);
    int new_h = static_cast<int>(sh * 0.8);

    // Resize and center window
    size(new_w, new_h);
    position((sw - new_w) / 2 + sx, (sh - new_h) / 2 + sy);

    // Set minimum window size to prevent UI elements from overlapping
    size_range(650, 550);  // min width, min height (no max limits)

    // Enable the window's title bar
    border(1);

    // Create page components
    m_pg1 = new Page1_Settings(0, 0, new_w, new_h);
    m_pg1->setNextCallback(cb1Next, this);

    m_pg2 = new Page2_Board(0, 0, new_w, new_h);
    m_pg2->setBackCallback(cb2Back, this);
    m_pg2->setNextCallback(cb2Next, this);
    m_pg2->hide();

    m_pg3 = new Page3_HeroRange(0, 0, new_w, new_h);
    m_pg3->setBackCallback(cb3Back, this);
    m_pg3->setNextCallback(cb3Next, this);
    m_pg3->hide();

    m_pg4 = new Page4_VillainRange(0, 0, new_w, new_h);
    m_pg4->setBackCallback(cb4Back, this);
    m_pg4->setNextCallback(cb4Next, this);
    m_pg4->hide();

    m_pg5 = new Page5_Progress(0, 0, new_w, new_h);
    m_pg5->hide();

    m_pg6 = new Page6_Strategy(0, 0, new_w, new_h);
    m_pg6->setActionCallback([this](const std::string &action) { doAction(action); });
    m_pg6->setBackCallback([this]() { doBack6(); });
    m_pg6->setUndoCallback([this]() { doUndo(); });
    m_pg6->hide();

    resizable(this);
    end();
  }
};

int main(int argc, char **argv) {
  fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE * 2);
  fl_message_hotspot(1);
  Wizard wiz("Shark Poker Solver");
  wiz.show(argc, argv);
  return Fl::run();
}
