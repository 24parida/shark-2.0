#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Check_Button.H>
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
#include "components/CardButton.hh"

static const std::vector<std::string> RANKS = {
    "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
static const std::vector<char> SUITS = {'h', 'd', 'c', 's'};
constexpr float SCALE = 1.2f;

// Range definitions
struct PositionRanges {
    std::string opening;  // same as single-raise
    std::string threeBet;
    std::string fourBet;
};

static const std::map<std::string, PositionRanges> POSITION_RANGES = {
    {"SB", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQo,K9s,KTs,KJs,KQs,QTs,QJs,QJo,JTs,T9s,98s,87s,76s,65s,54s",  // opening
        "AA,KK,QQ,JJ,TT,AKs,AKo,A5s,A4s,KTs,QTs",  // 3bet
        "AA,KK,AKs,A5s"  // 4bet
    }},
    {"BB", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K2s,K3s,K4s,K5s,K6s,K7s,K8s,K9s,KTs,KJs,KQs,KTs,KTo,KJs,KJo,Q2s,Q3s,Q4s,Q5s,Q6s,Q7s,Q8s,Q9s,QTs,QJs,QJo,J2s,J3s,J4s,J5s,J6s,J7s,J8s,J9s,JTs,T2s,T3s,T4s,T5s,T6s,T7s,T8s,T9s,98s,87s,76s,65s,54s",
        "AA,KK,QQ,AKs,AKo,A5s,A4s",
        "AA,KK,AKs,A5s"
    }},
    {"UTG", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,KTs,KJs,KQs,QTs,JTs,T9s",
        "AA,KK,QQ,JJ,TT,99,AJs,AQs,AKs,AQs,AQo,AKs,AKo,KQs",
        "AA,KK,AKs,A5s"
    }},
    {"UTG+1", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,KTs,KJs,KQs,QTs,JTs,T9s,98s",
        "AA,KK,QQ,AKs,AKo,A5s,KTs",
        "AA,KK,AKs,A5s"
    }},
    {"MP", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,QTs,JTs,T9s,98s",
        "AA,KK,QQ,JJ,AKs,AKo,A5s,KQs",
        "AA,KK,AKs,A5s"
    }},
    {"LJ", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,Q9s,JTs,T9s,98s",
        "AA,KK,QQ,AKs,AKo,A5s,QTs",
        "AA,KK,AKs,A5s"
    }},
    {"HJ", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K9s,KTs,KJs,KQs,Q9s,QTs,JTs,T9s",
        "AA,KK,QQ,AKs,AKo,A5s,JTs",
        "AA,KK,AKs,A5s"
    }},
    {"CO", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K8s,K9s,KTs,KJs,KQs,Q9s,QTs,J9s,JTs,T9s,98s",
        "AA,KK,QQ,AKs,AKo,A5s,JTs",
        "AA,KK,AKs,A5s"
    }},
    {"BTN", {
        "AA,KK,QQ,JJ,TT,99,88,77,66,55,44,33,22,A2s,A3s,A4s,A5s,A6s,A7s,A8s,A9s,ATs,AJs,AQs,AKs,AKo,AQs,AQo,ATs,ATo,K7s,K8s,K9s,KTs,KJs,KQs,Q8s,Q9s,QTs,J8s,J9s,JTs,T7s,T8s,T9s,98s",
        "AA,KK,QQ,AKs,AKo,A5s,KTs,QTs",
        "AA,KK,AKs,A5s"
    }}
};

// Helper function to expand range notation into individual hands
std::vector<std::string> expandRange(const std::string& range) {
    std::vector<std::string> hands;
    std::istringstream ss(range);
    std::string token;
    
    while (std::getline(ss, token, ',')) {
        if (token.find('-') != std::string::npos) {
            // Handle ranges like "A2s-A9s"
            size_t pos = token.find('-');
            std::string start = token.substr(0, pos);
            std::string end = token.substr(pos + 1);
            
            char startRank1 = start[0];
            char startRank2 = start[1];
            char endRank2 = end[1];
            bool suited = (start.find('s') != std::string::npos);
            
            for (char r = startRank2; r <= endRank2; r++) {
                hands.push_back(std::string(1, startRank1) + r + (suited ? "s" : "o"));
            }
        } else if (token.find('+') != std::string::npos) {
            // Handle ranges like "22+"
            std::string base = token.substr(0, token.length() - 1);
            char startRank = base[0];
            for (const auto& r : RANKS) {
                if (r[0] <= startRank) {
                    hands.push_back(r + r);
                }
            }
        } else {
            // Single hand
            hands.push_back(token);
        }
    }
    return hands;
}

class Wizard : public Fl_Window {
  struct UserInputs {
    int stackSize{}, startingPot{}, minBet{}, iterations{};
    float allInThreshold{};
    std::string potType, yourPos, theirPos;
    std::vector<std::string> board;
    std::vector<std::string> heroRange;
    std::vector<std::string> villainRange;
    float min_exploitability{};
    bool autoImportRanges{true};  // New field for auto-import setting
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

  Fl_Group *m_pg1, *m_pg2, *m_pg3, *m_pg4, *m_pg5, *m_pg6;

  // Page1
  Fl_Input *m_inpStack, *m_inpPot, *m_inpMinBet, *m_inpIters;
  Fl_Float_Input *m_inpAllIn, *m_inpMinExploit;
  Fl_Choice *m_choPotType, *m_choYourPos, *m_choTheirPos;
  Fl_Check_Button *m_chkAutoImport;  // New checkbox
  Fl_Button *m_btn1Next;

  // Page2
  Fl_Box *m_lblBoard;
  std::vector<CardButton *> m_cards;
  Fl_Input *m_selDisplay;
  Fl_Button *m_btnRand, *m_btn2Back, *m_btn2Next;

  // Page3
  Fl_Box *m_lblRange;
  std::vector<CardButton *> m_rangeBtns;
  Fl_Button *m_btn3Back, *m_btn3Next;

  // Page4
  Fl_Box *m_lblVillain;
  std::vector<CardButton *> m_villainBtns;
  Fl_Button *m_btn4Back, *m_btn4Next;

  // Page5
  Fl_Box *m_lblWait;

  // Page6 - Strategy Display
  Fl_Box *m_lblStrategy;
  std::vector<CardButton *> m_strategyBtns; // 13x13 grid
  Fl_Text_Display *m_infoDisplay;           // Right side info display
  Fl_Text_Buffer *m_infoBuffer;             // Buffer for info display
  std::vector<Fl_Button *> m_actionBtns;    // Bottom action buttons
  Fl_Choice *m_cardChoice;                  // Turn/river card selector
  Fl_Choice *m_rankChoice;                  // New: Rank selector
  Fl_Choice *m_suitChoice;                  // New: Suit selector
  Fl_Box *m_potInfo;                        // Display for pot and stack sizes
  Fl_Box *m_boardInfo;                      // Display for board information
  Fl_Box *m_infoTitle;                      // Info box title

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

  // Add new member variables for zoom controls
  Fl_Button *m_zoomInBtn{}, *m_zoomOutBtn{};
  float m_infoTextScale = 1.0f;  // Scale factor for info text size

  // Callbacks
  static void cb1Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do1Next(); }
  static void cbCard(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doCard((CardButton *)w);
  }
  static void cbRand(Fl_Widget *w, void *d) { ((Wizard *)d)->doRand(); }
  static void cb2Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack2(); }
  static void cb2Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do2Next(); }
  static void cb3Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack3(); }
  static void cb3Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do3Next(); }
  static void cbRange(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doRange((CardButton *)w);
  }
  static void cb4Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack4(); }
  static void cb4Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do4Next(); }
  static void cbStrategy(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doStrategy((CardButton *)w);
  }
  static void cbAction(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doAction((Fl_Button *)w);
  }
  static void cbBack(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack6(); }
  static void cbUndo(Fl_Widget *w, void *d) { ((Wizard *)d)->doUndo(); }
  static void cbZoomIn(Fl_Widget*, void* v) { ((Wizard*)v)->zoomIn(); }
  static void cbZoomOut(Fl_Widget*, void* v) { ((Wizard*)v)->zoomOut(); }

  // Helper function to auto-fill range buttons based on position and pot type
  void autoFillRange(const std::vector<CardButton*>& buttons, const std::string& position, bool isHero) {
    // Only auto-fill if the feature is enabled
    if (!m_data.autoImportRanges) return;

    // First clear all selections
    for (auto* btn : buttons) {
      btn->select(false);
    }

    // Get the appropriate range based on pot type
    const auto& ranges = POSITION_RANGES.at(position);
    std::string rangeStr;
    
    if (m_data.potType == "Single Raise") {
      rangeStr = ranges.opening;
    } else if (m_data.potType == "3-bet") {
      rangeStr = isHero ? ranges.threeBet : ranges.opening;
    } else if (m_data.potType == "4-bet") {
      rangeStr = isHero ? ranges.fourBet : ranges.threeBet;
    }

    // Expand the range notation into individual hands
    auto hands = expandRange(rangeStr);

    // Select the buttons corresponding to these hands
    for (auto* btn : buttons) {
      if (std::find(hands.begin(), hands.end(), btn->label()) != hands.end()) {
        btn->select(true);
      }
    }
  }

  // Page1 -> Page2
  void do1Next() {
    if (!*m_inpStack->value() || !*m_inpPot->value() ||
        !*m_inpMinBet->value() || !*m_inpAllIn->value() ||
        !*m_inpIters->value() || !*m_inpMinExploit->value()) {
      fl_message("Please fill out all fields.");
      return;
    }
    if (m_choYourPos->value() == m_choTheirPos->value()) {
      fl_message("Positions must differ.");
      return;
    }
    m_data.stackSize = std::atoi(m_inpStack->value());
    m_data.startingPot = std::atoi(m_inpPot->value());
    m_data.minBet = std::atoi(m_inpMinBet->value());
    m_data.allInThreshold = std::atof(m_inpAllIn->value());
    m_data.iterations = std::atoi(m_inpIters->value());
    m_data.min_exploitability = std::atof(m_inpMinExploit->value());

    m_data.potType = m_choPotType->text();
    m_data.yourPos = m_choYourPos->text();
    m_data.theirPos = m_choTheirPos->text();
    m_data.autoImportRanges = m_chkAutoImport->value();  // Save checkbox state

    m_pg1->hide();
    m_pg2->show();
  }

  // Page2 actions
  void doCard(CardButton *cb) {
    int c = std::count_if(m_cards.begin(), m_cards.end(),
                          [](auto *b) { return b->selected(); });
    if (!cb->selected() && c >= 5)
      return;
    cb->toggle();
    updateBoardSel();
  }
  void doRand() {
    for (auto *cb : m_cards)
      cb->select(false);
    std::vector<int> idx(m_cards.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now()
                         .time_since_epoch()
                         .count());
    std::shuffle(idx.begin(), idx.end(), rng);
    for (int i = 0; i < 3; ++i)
      m_cards[idx[i]]->select(true);
    updateBoardSel();
  }
  void updateBoardSel() {
    m_data.board.clear();
    std::string out;
    for (auto *cb : m_cards)
      if (cb->selected()) {
        std::string l = cb->label();
        m_data.board.push_back(l);
        if (!out.empty())
          out += ' ';
        out += l;
      }
    m_selDisplay->value(out.c_str());
  }
  void doBack2() {
    // Clear all range selections when going back
    for (auto *btn : m_rangeBtns) {
      btn->select(false);
    }
    for (auto *btn : m_villainBtns) {
      btn->select(false);
    }
    m_pg2->hide();
    m_pg1->show();
  }
  void do2Next() {
    int c = std::count_if(m_cards.begin(), m_cards.end(),
                          [](auto *b) { return b->selected(); });
    if (c < 3 || c > 5) {
      fl_message("Select 3-5 cards.");
      return;
    }

    // Clear any existing range selections before showing page 3
    for (auto *btn : m_rangeBtns) {
      btn->select(false);
    }
    
    m_pg2->hide();
    m_pg3->show();
    
    // Auto-fill hero range based on position and pot type
    autoFillRange(m_rangeBtns, m_data.yourPos, true);
  }

  // Page3 hero range
  void doRange(CardButton *cb) {
    cb->toggle();
  }
  void doBack3() {
    // Store current hero range selections before going back
    std::vector<bool> hero_selections;
    for (auto *btn : m_rangeBtns) {
        hero_selections.push_back(btn->selected());
    }

    m_pg3->hide();
    m_pg2->show();
  }
  // Page3 hero range
  void do3Next() {
    m_data.heroRange.clear();
    for (auto *b : m_rangeBtns)
      if (b->selected())
        m_data.heroRange.push_back(b->label());

    if (m_data.heroRange.empty()) {
      fl_message("Please select at least one hand for your range.");
      return;
    }

    m_pg3->hide();
    m_pg4->show();

    // Auto-fill villain range based on position and pot type
    if (m_data.autoImportRanges) {
        autoFillRange(m_villainBtns, m_data.theirPos, false);
    }
  }
  // Page4 villain range
  void doBack4() {
    // Store current villain range selections before going back
    std::vector<bool> villain_selections;
    for (auto *btn : m_villainBtns) {
        villain_selections.push_back(btn->selected());
    }

    m_pg4->hide();
    m_pg3->show();

    // Restore previous hero range selections if auto-import is enabled
    if (m_data.autoImportRanges) {
        autoFillRange(m_rangeBtns, m_data.yourPos, true);
    }
  }

  // Page4 villain range
  void do4Next() {
    m_data.villainRange.clear();
    for (auto *b : m_villainBtns)
      if (b->selected())
        m_data.villainRange.push_back(b->label());

    if (m_data.villainRange.empty()) {
      fl_message("Please select at least one hand for the villain's range.");
      return;
    }

    m_pg4->hide();
    m_pg5->show();
    Fl::check();

    // 3) helper to turn vector<string> → comma‑list
    auto join = [](const std::vector<std::string> &v) {
      std::string s;
      for (size_t i = 0; i < v.size(); ++i) {
        if (i)
          s += ",";
        s += v[i];
      }
      return s;
    };

    // 4) build PreflopRange from your & villain selections
    PreflopRange range1{join(m_data.heroRange)};
    PreflopRange range2{join(m_data.villainRange)};

    // 5) convert board labels into Cards
    std::vector<Card> board;
    for (auto &lbl : m_data.board)
      board.emplace_back(lbl.c_str());

    // 6) figure out who is in‐position (choice index)
    int heroPos = m_choYourPos->value();
    int villainPos = m_choTheirPos->value();
    int ip = (heroPos > villainPos ? 1 : 2);

    // 7) assemble settings
    TreeBuilderSettings settings{range1,
                                 range2,
                                 ip,
                                 board,
                                 m_data.stackSize,
                                 m_data.startingPot,
                                 m_data.minBet,
                                 m_data.allInThreshold};

    // 8) build manager + tree + trainer
    m_prm = PreflopRangeManager(range1.preflop_combos, range2.preflop_combos,
                                settings.initial_board);
    GameTree game_tree{settings};
    ParallelDCFR trainer{m_prm, settings.initial_board, settings.starting_pot,
                         settings.in_position_player};

    // 9) run it
    m_root = game_tree.build();
    trainer.train(m_root.get(), m_data.iterations, m_data.min_exploitability);

    // Initialize pot and stack tracking
    m_current_pot = m_data.startingPot;
    m_p1_stack = m_p2_stack = m_data.stackSize;
    m_p1_wager = m_p2_wager = 0;

    // Store current node and show strategy display
    m_current_node = m_root.get();
    m_pg5->hide();
    m_pg6->show();
    updateStrategyDisplay();
    updateActionButtons();
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
    // Always hide the card selection dropdowns by default
    m_rankChoice->hide();
    m_suitChoice->hide();

    if (m_current_node &&
        m_current_node->get_node_type() == NodeType::TERMINAL_NODE) {
      m_lblStrategy->copy_label("Terminal Node - Hand Complete");
      m_lblStrategy->labelsize(static_cast<int>(28 * m_scale));

      // Hide action buttons at terminal nodes
      for (auto *btn : m_actionBtns) {
        btn->hide();
      }
      return;
    }

    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE) {
      
      // If we're at a chance node, show the card selection view
      if (m_current_node && m_current_node->get_node_type() == NodeType::CHANCE_NODE) {
        // Hide strategy grid
        for (auto *btn : m_strategyBtns) {
          btn->hide();
        }
        
        // Hide action buttons
        for (auto *btn : m_actionBtns) {
          btn->hide();
        }
        
        // Hide info display and title
        m_infoDisplay->hide();
        m_infoTitle->hide();

        auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);
        std::string prompt = "Select ";
        prompt += (chance_node->get_type() == ChanceNode::ChanceType::DEAL_TURN ? "Turn" : "River");
        prompt += " Card";
        m_lblStrategy->copy_label(prompt.c_str());
        m_lblStrategy->labelsize(static_cast<int>(36 * m_scale));  // Larger text for prompt

        // Center and show the dropdowns with increased size
        int dropdownWidth = static_cast<int>(150 * m_scale);  // Smaller width for each dropdown
        int dropdownHeight = static_cast<int>(50 * m_scale);
        int dropdownSpacing = static_cast<int>(20 * m_scale);
        int totalWidth = (2 * dropdownWidth) + dropdownSpacing;
        int dropdownX = (this->w() - totalWidth) / 2;
        int dropdownY = (this->h() - dropdownHeight) / 2;
        
        // Position and show rank dropdown
        m_rankChoice->resize(dropdownX, dropdownY, dropdownWidth, dropdownHeight);
        m_rankChoice->textsize(static_cast<int>(24 * m_scale));
        m_rankChoice->clear();
        // Add empty first option
        m_rankChoice->add("Select Rank");
        for (const auto& rank : RANKS) {
            m_rankChoice->add(rank.c_str());
        }
        m_rankChoice->value(0);  // Select the "Select Rank" option
        m_rankChoice->show();
        
        // Position and show suit dropdown
        m_suitChoice->resize(dropdownX + dropdownWidth + dropdownSpacing, dropdownY, dropdownWidth, dropdownHeight);
        m_suitChoice->textsize(static_cast<int>(24 * m_scale));
        m_suitChoice->clear();
        // Add empty first option
        m_suitChoice->add("Select Suit");
        for (const auto& suit : SUITS) {
            std::string suit_str(1, suit);
            m_suitChoice->add(suit_str.c_str());
        }
        m_suitChoice->value(0);  // Select the "Select Suit" option
        m_suitChoice->show();
        
        // Update board info position to be above dropdowns
        m_boardInfo->position(0, dropdownY - static_cast<int>(60 * m_scale));
        m_boardInfo->labelsize(static_cast<int>(24 * m_scale));
        
        // Update pot info position to be below dropdowns
        m_potInfo->position(0, dropdownY + dropdownHeight + static_cast<int>(20 * m_scale));
        m_potInfo->labelsize(static_cast<int>(24 * m_scale));
      }
      return;
    }

    // For action nodes, show proper title and enable strategy display
    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    const auto &strategy = action_node->get_average_strat();
    const auto &actions = action_node->get_actions();

    std::string title = (action_node->get_player() == 1 ? "Hero's" : "Villain's") + std::string(" Turn");
    m_lblStrategy->copy_label(title.c_str());
    m_lblStrategy->labelsize(static_cast<int>(28 * m_scale));

    // Show strategy grid and make buttons clickable
    for (auto *btn : m_strategyBtns) {
      btn->show();
      btn->activate();
    }

    // Show info display and title
    m_infoDisplay->show();
    m_infoTitle->show();

    // Update board info
    std::string board = "Board: ";
    for (const auto &card : m_data.board) {
      board += card + " ";
    }
    m_boardInfo->copy_label(board.c_str());

    // Update pot/stack info
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_potInfo->copy_label(info.c_str());

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
          size_t strat_idx = i + a * num_hands; // Correct indexing: hand_idx +
                                                // action_idx * num_hands
          if (strat_idx < strategy.size()) {
            handTypeStrategies[hand_format][a] += strategy[strat_idx];
          }
        }
        handTypeCounts[hand_format]++;
      }
    }

    // Update grid - now show all hands as boxes
    for (int r = 0; r < 13; r++) {
      for (int c = 0; c < 13; c++) {
        auto *btn = m_strategyBtns[r * 13 + c];
        std::string hand;

        if (r == c) {
          hand = RANKS[r] + RANKS[r]; // Pocket pair
        } else if (c > r) {
          hand = RANKS[r] + RANKS[c] + "s"; // Suited
        } else {
          hand = RANKS[c] + RANKS[r] + "o"; // Offsuit
        }

        btn->copy_label(hand.c_str());
        btn->box(FL_FLAT_BOX); // Always show as box

        // Check if this hand is in the current range
        if (handTypeStrategies.find(hand) != handTypeStrategies.end()) {
          // Average the strategies
          std::vector<float> avgStrategy = handTypeStrategies[hand];
          int count = handTypeCounts[hand];
          for (float &prob : avgStrategy) {
            prob /= count;
          }

          // Convert strategies to colors
          std::vector<std::pair<Fl_Color, float>> colors;

          // Aggregate probabilities by action type
          float checkProb = 0.0f;
          float foldProb = 0.0f;
          std::map<int, float> betSizeProbs;  // Map bet size to probability

          for (size_t i = 0; i < actions.size(); ++i) {
            const auto &action = actions[i];
            float prob = avgStrategy[i];

            switch (action.type) {
            case Action::CHECK:
            case Action::CALL:
                checkProb += prob;
                break;
            case Action::FOLD:
                foldProb += prob;
                break;
            case Action::BET:
            case Action::RAISE:
                betSizeProbs[action.amount] += prob;
                break;
            }
          }

          // Add colors in order (they will be drawn from top to bottom)
          if (foldProb > 0.001f)
            colors.emplace_back(fl_rgb_color(255, 50, 50), foldProb); // Red for fold

          if (checkProb > 0.001f)
            colors.emplace_back(fl_rgb_color(50, 255, 50), checkProb); // Green for check/call

          // Sort bet sizes
          std::vector<std::pair<int, float>> sortedBets(betSizeProbs.begin(), betSizeProbs.end());
          std::sort(sortedBets.begin(), sortedBets.end());

          // Create color gradient for different bet sizes
          for (const auto &[betSize, prob] : sortedBets) {
            if (prob > 0.001f) {
                // Calculate color based on bet size
                // Smaller bets: lighter blue (50,50,255)
                // Medium bets: darker blue (20,20,180)
                // Larger bets: purple (100,20,180)
                
                // Find the relative position of this bet size
                float maxBet = sortedBets.back().first;
                float relativeSize = betSize / static_cast<float>(maxBet);
                
                // Create color gradient
                int r = static_cast<int>(50 + relativeSize * 50);  // 50 -> 100
                int g = static_cast<int>(50 - relativeSize * 30);  // 50 -> 20
                int b = static_cast<int>(255 - relativeSize * 75); // 255 -> 180
                
                colors.emplace_back(fl_rgb_color(r, g, b), prob);
            }
          }

          btn->setStrategyColors(colors);
        } else {
          // Hand not in range - show gray background
          btn->setStrategyColors({});
          btn->color(fl_rgb_color(80, 80, 80));
        }
      }
    }
  }

  void updateActionButtons() {
    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &actions = action_node->get_actions();

    // Hide all buttons first
    for (auto *btn : m_actionBtns)
      btn->hide();

    // Show and update available action buttons
    for (size_t i = 0; i < actions.size() && i < m_actionBtns.size(); ++i) {
      const auto &action = actions[i];
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
      m_actionBtns[i]->copy_label(label.c_str());
      m_actionBtns[i]->show();
    }
  }

  void updateInfoDisplaySettings() {
    // Calculate base text size with scaling
    int textSize = static_cast<int>(22 * m_scale * m_infoTextScale);
    
    // Set text size
    m_infoDisplay->textsize(textSize);
    
    // Adjust line spacing based on text size
    int lineSpacing = static_cast<int>(textSize * 1.5);
    m_infoDisplay->linenumber_width(lineSpacing);
    
    // Set text font
    m_infoDisplay->textfont(FL_HELVETICA);
    
    // Reset scroll position
    m_infoDisplay->scroll(0, 0);
    
    // Force redraw
    m_infoDisplay->redraw();
  }

  void updateInfoBox(const std::string &hand) {
    if (!m_current_node || m_current_node->get_node_type() != NodeType::ACTION_NODE)
        return;

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &strategy = action_node->get_average_strat();
    const auto &actions = action_node->get_actions();
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    size_t num_hands = hands.size();

    // Build simplified strategy info string with added spacing
    std::string info = "\n    " + hand + " Strategy:\n\n";  // Added leading spaces

    // Get all specific combos for this hand
    std::vector<std::pair<std::string, size_t>> combos;
    for (size_t i = 0; i < hands.size(); ++i) {
        const auto &h = hands[i];
        std::string hand_str = h.to_string();
        
        hand_str = hand_str.substr(1, hand_str.length() - 2);
        hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ' '), hand_str.end());
        hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ','), hand_str.end());
        
        std::string rank1 = hand_str.substr(0, 1);
        std::string rank2 = hand_str.substr(2, 1);
        bool suited = hand_str[1] == hand_str[3];
        std::string hand_format = rank1 + rank2 + (suited ? "s" : "o");
        if (rank1 == rank2)
            hand_format = rank1 + rank2;
        
        if (hand_format == hand) {
            bool overlaps = false;
            for (const auto &board_card : m_data.board) {
                Card card(board_card.c_str());
                if (h.hand1 == card || h.hand2 == card) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) {
                std::string combo = h.hand1.describeCard() + h.hand2.describeCard();
                combos.emplace_back(combo, i);
            }
        }
    }

    // Show strategies for each valid combo with improved spacing
    for (const auto &combo : combos) {
        info += "    " + combo.first + ":\n";  // Added leading spaces
        
        float total = 0.0f;
        for (size_t i = 0; i < actions.size(); ++i) {
            size_t strat_idx = combo.second + i * num_hands;
            if (strat_idx < strategy.size()) {
                total += strategy[strat_idx];
            }
        }
        
        for (size_t i = 0; i < actions.size(); ++i) {
            const auto &action = actions[i];
            size_t strat_idx = combo.second + i * num_hands;
            
            if (strat_idx < strategy.size()) {
                float prob = (total > 0) ? (strategy[strat_idx] / total * 100.0f) : (100.0f / actions.size());
                if (prob < 0.1f) continue;  // Skip very low probability actions
                
                std::string action_str;
                switch (action.type) {
                    case Action::FOLD: action_str = "Fold"; break;
                    case Action::CHECK: action_str = "Check"; break;
                    case Action::CALL: action_str = "Call " + std::to_string(action.amount); break;
                    case Action::BET: action_str = "Bet " + std::to_string(action.amount); break;
                    case Action::RAISE: action_str = "Raise to " + std::to_string(action.amount); break;
                }
                info += "        " + action_str + ": " + std::to_string(int(prob + 0.5f)) + "%\n";  // Added more leading spaces
            }
        }
        info += "\n";  // Extra line between combos
    }

    m_infoBuffer->text(info.c_str());
    m_infoDisplay->scroll(0, 0);
  }

  void doStrategy(CardButton *btn) { 
    // Clear previous selection
    for (auto *b : m_strategyBtns) {
        b->setStrategySelected(false);
    }
    // Set new selection
    btn->setStrategySelected(true);
    updateInfoBox(btn->label()); 
  }

  void doAction(Fl_Button *btn) {
    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

    // Save current state before action
    GameState state{m_current_node, m_p1_stack, m_p2_stack,  m_current_pot,
                    m_p1_wager,     m_p2_wager, m_data.board};
    m_history.push_back(state);

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &actions = action_node->get_actions();

    // Find which action button was clicked
    size_t btn_idx = 0;
    for (; btn_idx < m_actionBtns.size(); ++btn_idx) {
      if (m_actionBtns[btn_idx] == btn)
        break;
    }

    if (btn_idx >= actions.size())
      return;

    // Update pot and stacks based on action
    updatePotAndStacks(actions[btn_idx], action_node->get_player());

    // Update display BEFORE transitioning to next node
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_potInfo->copy_label(info.c_str());
    m_potInfo->redraw();

    // Navigate to next node using the action index
    m_current_node = action_node->get_child(btn_idx);

    if (m_current_node->get_node_type() == NodeType::CHANCE_NODE) {
      // Show card selection dropdown
      m_cardChoice->clear();

      // Add valid turn/river cards
      std::set<std::string> used_cards;
      for (const auto &card : m_data.board) {
        used_cards.insert(card);
      }

      // Add all possible cards that aren't on the board
      for (const auto &rank : RANKS) {
        for (const auto &suit : SUITS) {
          std::string card = rank + std::string(1, suit);
          if (used_cards.find(card) == used_cards.end()) {
            m_cardChoice->add(card.c_str());
          }
        }
      }

      if (m_cardChoice->size() > 0) {
        m_cardChoice->value(0);  // Select first card by default
      }

      updateStrategyDisplay();  // This will now handle showing the card selection view
    } else {
      m_cardChoice->hide();
      updateStrategyDisplay();
      updateActionButtons();
    }
  }

  void doCardSelect() {
    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::CHANCE_NODE)
      return;

    // Check if either dropdown is still showing placeholder text
    if (m_rankChoice->value() <= 0 || m_suitChoice->value() <= 0) {
      return;  // Don't proceed if either hasn't been selected
    }

    // Save current state before adding card
    GameState state{m_current_node, m_p1_stack, m_p2_stack,  m_current_pot,
                    m_p1_wager,     m_p2_wager, m_data.board};
    m_history.push_back(state);

    auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);

    // Get selected rank and suit (now guaranteed to not be placeholder text)
    std::string selected_rank = m_rankChoice->text();
    std::string selected_suit = m_suitChoice->text();
    std::string selected_card_str = selected_rank + selected_suit;
    m_data.board.push_back(selected_card_str);

    // Find the correct child index by creating cards the same way the solver does
    int found_idx = -1;
    for (int i = 0; i < 52; i++) {
      Card test_card{i}; // Create card the same way solver does
      if (test_card.describeCard() == selected_card_str) {
        found_idx = i;
        break;
      }
    }

    if (found_idx == -1) {
      std::cerr << "Error: Could not find matching card index for "
                << selected_card_str << std::endl;
      return;
    }

    // Navigate to selected card's node
    m_current_node = chance_node->get_child(found_idx);

    // Hide card selection UI
    m_rankChoice->hide();
    m_suitChoice->hide();

    // Reset display state
    m_lblStrategy->labelsize(static_cast<int>(28 * m_scale));  // Reset to normal size

    // Show strategy grid
    for (auto *btn : m_strategyBtns) {
      btn->show();
    }

    // Show info display and title
    m_infoDisplay->show();
    m_infoTitle->show();

    // Update the display with the new state
    updateStrategyDisplay();
    updateActionButtons();

    // Force a redraw of the window
    this->redraw();
    Fl::check();
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

    // Hide card selection dropdowns by default
    m_rankChoice->hide();
    m_suitChoice->hide();

    // Show all elements that might have been hidden
    m_infoDisplay->show();
    m_infoTitle->show();
    for (auto *btn : m_strategyBtns) {
      btn->show();
    }

    // Update displays based on current node type
    updateStrategyDisplay();
    updateActionButtons();

    // Update pot/stack display
    std::string info = "Hero: " + std::to_string(m_p1_stack) +
                       " | Villain: " + std::to_string(m_p2_stack) +
                       " | Pot: " + std::to_string(m_current_pot);
    m_potInfo->copy_label(info.c_str());
    m_potInfo->redraw();

    // Update board display
    std::string board = "Board: ";
    for (const auto &card : m_data.board) {
      board += card + " ";
    }
    m_boardInfo->copy_label(board.c_str());
    m_boardInfo->redraw();
  }

  void zoomIn() {
    if (m_infoTextScale < 2.0f) {  // Maximum 2x zoom
      m_infoTextScale += 0.1f;
      updateInfoDisplaySettings();
    }
  }

  void zoomOut() {
    if (m_infoTextScale > 0.5f) {  // Minimum 0.5x zoom
      m_infoTextScale -= 0.1f;
      updateInfoDisplaySettings();
    }
  }

  // Revert to original design dimensions
  static constexpr float TARGET_WIDTH = 2000.0f;
  static constexpr float TARGET_HEIGHT = 1375.0f;
  float m_scale;

public:
  Wizard(const char *L = 0) : Fl_Window(100, 100, L) {
    init();
  }

private:
  void init() {
    // Get primary screen work area (accounts for taskbar/menubar)
    int sx, sy, sw, sh;
    Fl::screen_work_area(sx, sy, sw, sh, 0);  // 0 for primary screen

    // Calculate initial scale based on screen size
    float height_scale = (sh * 0.90f) / TARGET_HEIGHT;
    float width_scale = (sw * 0.95f) / TARGET_WIDTH;
    float base_scale = std::min(height_scale, width_scale);

    // Apply additional scaling based on screen size, but with upper limit for large screens
    float screen_size_factor = std::sqrt((float)(sw * sh) / (1920.0f * 1080.0f));  // 1.0 at 1080p
    
    // Cap the screen size factor for large monitors
    if (sh > 1440) {  // For screens taller than 1440p
        screen_size_factor = std::min(screen_size_factor, 1.1f);
    }
    
    float scale_multiplier = std::min(1.0f, std::max(0.6f, 0.9f * screen_size_factor));
    m_scale = base_scale * scale_multiplier;

    // Ensure minimum usable scale
    m_scale = std::max(m_scale, 0.45f);

    // Additional upper limit for very large screens
    m_scale = std::min(m_scale, 0.85f);  // Cap maximum scale

    // Calculate new window dimensions
    int new_w = static_cast<int>(TARGET_WIDTH * m_scale);
    int new_h = static_cast<int>(TARGET_HEIGHT * m_scale);

    // Final check to ensure window fits on screen
    if (new_w > sw || new_h > sh) {
        float extra_scale = std::min((float)sw / new_w, (float)sh / new_h);
        m_scale *= extra_scale * 0.95f;
        new_w = static_cast<int>(TARGET_WIDTH * m_scale);
        new_h = static_cast<int>(TARGET_HEIGHT * m_scale);
    }

    // Resize and center window
    size(new_w, new_h);
    position((sw - new_w) / 2 + sx, (sh - new_h) / 2 + sy);

    // Enable the window's title bar and close button
    border(1);

    // Page1
    m_pg1 = new Fl_Group(0, 0, new_w, new_h);
    
    // Scale all dimensions based on m_scale
    int xL = static_cast<int>(50 * m_scale);
    int xI = static_cast<int>(300 * m_scale);
    int y = static_cast<int>(50 * m_scale);
    int h = static_cast<int>(60 * m_scale);
    int sp = static_cast<int>(25 * m_scale);
    int wL = static_cast<int>(250 * m_scale);
    int wI = new_w - xI - static_cast<int>(100 * m_scale);
    
    auto addLbl = [&](const char *t) {
      auto f = new Fl_Box(xL, y, wL, h, t);
      f->labelsize(static_cast<int>(24 * m_scale));
    };
    
    auto addInp = [&](Fl_Input *&w) {
      w = new Fl_Input(xI, y, wI, h);
      w->textsize(static_cast<int>(24 * m_scale));
    };
    
    auto addFlt = [&](Fl_Float_Input *&w) {
      w = new Fl_Float_Input(xI, y, wI, h);
      w->textsize(static_cast<int>(24 * m_scale));
    };
    
    auto addCh = [&](Fl_Choice *&w) {
      w = new Fl_Choice(xI, y, wI, h);
      w->textsize(static_cast<int>(24 * m_scale));
    };

    // Create all page 1 elements
    addLbl("Stack Size:");
    addInp(m_inpStack);
    y += h + sp;
    addLbl("Starting Pot:");
    addInp(m_inpPot);
    y += h + sp;
    addLbl("Initial Min Bet:");
    addInp(m_inpMinBet);
    y += h + sp;
    addLbl("All-In Thresh:");
    addFlt(m_inpAllIn);
    m_inpAllIn->value("0.67");
    y += h + sp;
    addLbl("Type of pot:");
    addCh(m_choPotType);
    m_choPotType->add("Single Raise|3-bet|4-bet");
    m_choPotType->value(0);
    y += h + sp;
    addLbl("Your pos:");
    addCh(m_choYourPos);
    m_choYourPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    m_choYourPos->value(0);
    y += h + sp;
    addLbl("Their pos:");
    addCh(m_choTheirPos);
    m_choTheirPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    m_choTheirPos->value(1);
    y += h + sp;
    addLbl("Iterations:");
    addInp(m_inpIters);
    y += h + sp * 2;
    m_inpIters->value("100");
    addLbl("Min Exploitability (%):");
    addFlt(m_inpMinExploit);
    m_inpMinExploit->value("1.0");
    y += h + sp;

    // Add auto-import checkbox with medium size
    m_chkAutoImport = new Fl_Check_Button(xL, y, new_w - static_cast<int>(100 * m_scale), 
                                         static_cast<int>(60 * m_scale), 
                                         "Auto-import ranges based on positions");
    m_chkAutoImport->labelsize(static_cast<int>(30 * m_scale));
    m_chkAutoImport->value(1);
    y += static_cast<int>(60 * m_scale) + sp;

    m_btn1Next = new Fl_Button((new_w - 225) / 2, y, 225, 52, "Next");
    m_btn1Next->labelsize(18);
    m_btn1Next->callback(cb1Next, this);
    m_pg1->end();

    // Page2
    m_pg2 = new Fl_Group(0, 0, new_w, new_h);
    
    // Title section
    int titleHeight = static_cast<int>(50 * m_scale);
    m_lblBoard = new Fl_Box(0, 15, new_w, titleHeight, "Init Board (3-5 Cards)");
    m_lblBoard->labelfont(FL_BOLD);
    m_lblBoard->labelsize(static_cast<int>(21 * m_scale));
    m_lblBoard->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    // Calculate space needed for bottom elements
    int bottomInputHeight = static_cast<int>(30 * m_scale);  // Height for input field
    int bottomInputMargin = static_cast<int>(20 * m_scale);  // Margin around input field
    int navButtonHeight = static_cast<int>(40 * m_scale);    // Height for nav buttons
    int navButtonMargin = static_cast<int>(20 * m_scale);    // Margin around nav buttons
    
    // Total space needed at bottom
    int bottomReservedSpace = bottomInputHeight + bottomInputMargin * 2 + 
                            navButtonHeight + navButtonMargin * 2;

    // Card grid layout
    int cols = 4, rows = 13;
    
    // Calculate available space for grid
    int topMargin = titleHeight + static_cast<int>(20 * m_scale);  // Space after title
    int availableHeight = new_h - topMargin - bottomReservedSpace;
    int availableWidth = new_w - static_cast<int>(100 * m_scale);  // Side margins
    
    // Calculate spacing between buttons - scale with window size but keep reasonable
    int spacing = static_cast<int>(8 * m_scale);  // Slightly increased base spacing
    
    // Calculate maximum possible button size based on available space
    int maxButtonWidth = (availableWidth - (cols - 1) * spacing) / cols;
    int maxButtonHeight = (availableHeight - (rows - 1) * spacing) / rows;
    
    // Use the smaller dimension to ensure square buttons that fit
    int buttonSize = std::min(maxButtonWidth, maxButtonHeight);
    
    // Apply scaling limits - increased minimum and maximum sizes
    int minButtonSize = static_cast<int>(45 * m_scale);  // Increased from 35
    int maxButtonSize = static_cast<int>(65 * m_scale);  // Increased from 45
    buttonSize = std::min(maxButtonSize, std::max(minButtonSize, buttonSize));
    
    // Center the card grid
    int totalGridWidth = cols * buttonSize + (cols - 1) * spacing;
    int totalGridHeight = rows * buttonSize + (rows - 1) * spacing;
    int GX = (new_w - totalGridWidth) / 2;
    int GY = topMargin + (availableHeight - totalGridHeight) / 2;

    // Create card grid with square buttons
    for (int r = 0; r < rows; ++r) {
        for (int s = 0; s < cols; ++s) {
            int x = GX + s * (buttonSize + spacing);
            int y = GY + r * (buttonSize + spacing);
            Fl_Color base;
            switch (SUITS[s]) {
                case 'h': base = fl_rgb_color(180, 30, 30); break;
                case 'd': base = fl_rgb_color(30, 30, 180); break;
                case 'c': base = fl_rgb_color(30, 180, 30); break;
                default:  base = fl_rgb_color(20, 20, 20);
            }
            auto *cb = new CardButton(x, y, buttonSize, buttonSize, base);
            std::string lbl = RANKS[r] + std::string(1, SUITS[s]);
            cb->copy_label(lbl.c_str());
            cb->labelsize(static_cast<int>(16 * m_scale));  // Increased label size
            cb->callback(cbCard, this);
            m_cards.push_back(cb);
        }
    }

    // Position input display and random button at bottom
    int inputY = new_h - bottomReservedSpace + bottomInputMargin;
    int inputWidth = static_cast<int>(new_w * 0.6);
    int randButtonWidth = static_cast<int>(200 * m_scale);  // Increased from 160
    int inputSpacing = static_cast<int>(20 * m_scale);
    int totalWidth = inputWidth + randButtonWidth + inputSpacing;
    int startX = (new_w - totalWidth) / 2;

    // Make input field taller and text larger
    int inputFieldHeight = static_cast<int>(40 * m_scale);  // Increased from 30
    m_selDisplay = new Fl_Input(startX, inputY, inputWidth, inputFieldHeight);
    m_selDisplay->textsize(static_cast<int>(18 * m_scale));  // Increased from 12
    m_selDisplay->readonly(1);

    // Make random flop button match input field height and larger text
    m_btnRand = new Fl_Button(m_selDisplay->x() + m_selDisplay->w() + inputSpacing, 
                             inputY, randButtonWidth, inputFieldHeight, 
                             "Generate Random Flop");
    m_btnRand->labelsize(static_cast<int>(18 * m_scale));  // Increased from 12
    m_btnRand->callback(cbRand, this);

    // Position back/next buttons at bottom with larger text
    int navButtonWidth = static_cast<int>(150 * m_scale);
    int navY = new_h - navButtonHeight - navButtonMargin;

    m_btn2Back = new Fl_Button(25, navY, navButtonWidth, navButtonHeight, "Back");
    m_btn2Back->labelsize(static_cast<int>(18 * m_scale));  // Increased from 12
    m_btn2Back->callback(cb2Back, this);

    m_btn2Next = new Fl_Button(new_w - navButtonWidth - 25, navY, 
                              navButtonWidth, navButtonHeight, "Next");
    m_btn2Next->labelsize(static_cast<int>(18 * m_scale));  // Increased from 12
    m_btn2Next->callback(cb2Next, this);

    m_pg2->end();
    m_pg2->hide();

    // Page3
    m_pg3 = new Fl_Group(0, 0, new_w, new_h);
    
    m_lblRange = new Fl_Box(0, 20, new_w, 50, "Range Editor (you)");
    m_lblRange->labelfont(FL_BOLD);
    m_lblRange->labelsize(28);
    m_lblRange->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    // Range grid - ensure buttons are large enough
    int min_range_button = static_cast<int>(60 * m_scale);  // Increased from 45
    
    // Calculate range button sizes with guaranteed minimum
    int grid_size = 13;
    int available_width = new_w - 100;
    int available_height = new_h - 250;
    
    int base_button_size = std::min(
        (available_width - (grid_size * 10)) / grid_size,
        (available_height - (grid_size * 10)) / grid_size
    );
    
    // Ensure minimum size for range buttons
    int rbw = std::max(min_range_button, base_button_size);
    int rbh = rbw;  // Keep square
    
    int rsp = static_cast<int>(10 * m_scale);  // Consistent spacing for range grid
    
    // Calculate grid position
    int rangeGridX = (new_w - (13 * (rbw + rsp) - rsp)) / 2;  // Fixed spacing calculation
    int rangeGridY = 90;
    
    // Create range buttons with guaranteed minimum sizes
    for (int i = 0; i < 13; ++i) {
        for (int j = 0; j < 13; ++j) {
            int x = rangeGridX + j * (rbw + rsp);
            int y0 = rangeGridY + i * (rbh + rsp);
            std::string lbl;
            Fl_Color base;
            if (i == j) {
                lbl = RANKS[i] + RANKS[j];
                base = fl_rgb_color(100, 200, 100);
            } else if (j > i) {
                lbl = RANKS[i] + RANKS[j] + "s";
                base = fl_rgb_color(100, 100, 200);
            } else {
                lbl = RANKS[j] + RANKS[i] + "o";
                base = fl_rgb_color(80, 80, 80);
            }
            auto *btn = new CardButton(x, y0, rbw, rbh, base);
            btn->copy_label(lbl.c_str());
            btn->labelsize(static_cast<int>(20 * m_scale));  // Consistent larger size
            btn->callback(cbRange, this);
            btn->clear_visible_focus();
            m_rangeBtns.push_back(btn);
        }
    }

    // Position back/next buttons for range pages with proper sizing
    int rangeNavButtonHeight = static_cast<int>(40 * m_scale);
    int rangeNavButtonWidth = static_cast<int>(120 * m_scale);
    int rangeNavMargin = static_cast<int>(20 * m_scale);
    int rangeNavY = new_h - rangeNavButtonHeight - rangeNavMargin;

    // Range page navigation buttons with larger text
    m_btn3Back = new Fl_Button(25, rangeNavY, rangeNavButtonWidth, rangeNavButtonHeight, "Back");
    m_btn3Back->labelsize(static_cast<int>(18 * m_scale));  // Increased from 16
    m_btn3Back->callback(cb3Back, this);

    m_btn3Next = new Fl_Button(new_w - rangeNavButtonWidth - 25, rangeNavY, rangeNavButtonWidth, rangeNavButtonHeight, "Next");
    m_btn3Next->labelsize(static_cast<int>(18 * m_scale));  // Increased from 16
    m_btn3Next->callback(cb3Next, this);

    m_pg3->end();
    m_pg3->hide();

    // Page4 - Use same button sizes and text sizes as Page3 for consistency
    m_pg4 = new Fl_Group(0, 0, new_w, new_h);
    
    m_lblVillain = new Fl_Box(0, 20, new_w, 50, "Range Editor (villain)");
    m_lblVillain->labelfont(FL_BOLD);
    m_lblVillain->labelsize(28);
    m_lblVillain->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    // Create villain range buttons with same sizes as hero range
    for (int i = 0; i < 13; ++i) {
        for (int j = 0; j < 13; ++j) {
            int x = rangeGridX + j * (rbw + rsp);
            int y0 = rangeGridY + i * (rbh + rsp);
            std::string lbl;
            Fl_Color base;
            if (i == j) {
                lbl = RANKS[i] + RANKS[j];
                base = fl_rgb_color(100, 200, 100);
            } else if (j > i) {
                lbl = RANKS[i] + RANKS[j] + "s";
                base = fl_rgb_color(100, 100, 200);
            } else {
                lbl = RANKS[j] + RANKS[i] + "o";
                base = fl_rgb_color(80, 80, 80);
            }
            auto *btn = new CardButton(x, y0, rbw, rbh, base);
            btn->copy_label(lbl.c_str());
            btn->labelsize(static_cast<int>(20 * m_scale));  // Same size as hero range
            btn->callback(cbRange, this);
            btn->clear_visible_focus();
            m_villainBtns.push_back(btn);
        }
    }

    m_btn4Back = new Fl_Button(25, rangeNavY, rangeNavButtonWidth, rangeNavButtonHeight, "Back");
    m_btn4Back->labelsize(static_cast<int>(18 * m_scale));  // Same size as hero range
    m_btn4Back->callback(cb4Back, this);

    m_btn4Next = new Fl_Button(new_w - rangeNavButtonWidth - 25, rangeNavY, rangeNavButtonWidth, rangeNavButtonHeight, "Next");
    m_btn4Next->labelsize(static_cast<int>(18 * m_scale));  // Same size as hero range
    m_btn4Next->callback(cb4Next, this);

    m_pg4->end();
    m_pg4->hide();

    // Page5
    m_pg5 = new Fl_Group(0, 0, new_w, new_h);
    
    m_lblWait = new Fl_Box(0, 0, new_w, new_h, "Please wait...");
    m_lblWait->labelsize(36);
    m_lblWait->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
    
    m_pg5->end();
    m_pg5->hide();

    // Page6 - Strategy Display
    m_pg6 = new Fl_Group(0, 0, new_w, new_h);
    m_pg6->box(FL_FLAT_BOX);
    m_pg6->color(fl_rgb_color(240, 240, 240));

    // Title area - INCREASE TEXT SIZES AND ADJUST SPACING
    int titleY = 15;
    int titleH = static_cast<int>(30 * m_scale);  // Increased from 12 to accommodate larger text

    // Strategy text (Hero's/Villain's Turn)
    m_lblStrategy = new Fl_Box(0, titleY, new_w, titleH);
    m_lblStrategy->labelsize(static_cast<int>(28 * m_scale));
    m_lblStrategy->labelfont(FL_HELVETICA_BOLD);
    m_lblStrategy->align(FL_ALIGN_CENTER);

    // Board info text
    int boardY = titleY + titleH + static_cast<int>(10 * m_scale);
    int boardH = static_cast<int>(25 * m_scale);
    m_boardInfo = new Fl_Box(0, boardY, new_w, boardH);
    m_boardInfo->labelsize(static_cast<int>(16 * m_scale));
    m_boardInfo->labelfont(FL_HELVETICA);
    m_boardInfo->align(FL_ALIGN_CENTER);

    // Pot/stack info text
    int potY = boardY + boardH + static_cast<int>(5 * m_scale);
    int potH = static_cast<int>(25 * m_scale);
    m_potInfo = new Fl_Box(0, potY, new_w, potH);
    m_potInfo->labelsize(static_cast<int>(14 * m_scale));
    m_potInfo->labelfont(FL_HELVETICA);
    m_potInfo->align(FL_ALIGN_CENTER);

    int headerHeight = potY + potH + static_cast<int>(15 * m_scale);

    // Strategy grid
    int gridX = 20;
    int gridY = headerHeight;
    int gridW = (new_w * 2) / 3 - 40;
    
    // Reserve space at bottom for buttons
    int bottomButtonHeight = static_cast<int>(60 * m_scale);  // Space for buttons
    int rangeGridBottomMargin = static_cast<int>(20 * m_scale);  // Margin below grid
    
    // Adjust grid height to leave room for buttons
    int gridH = new_h - gridY - bottomButtonHeight - rangeGridBottomMargin;

    int cellSize = std::min(gridW / 13, gridH / 13);
    cellSize = static_cast<int>(cellSize * 0.925);
    int cellPadding = 3;

    gridX = (((new_w * 2) / 3) - (13 * (cellSize + cellPadding))) / 2;

    // Create strategy grid buttons
    for (int r = 0; r < 13; r++) {
        for (int c = 0; c < 13; c++) {
            int x = gridX + c * (cellSize + cellPadding);
            int y = gridY + r * (cellSize + cellPadding);

            auto *btn = new CardButton(x, y, cellSize, cellSize, fl_rgb_color(80, 80, 80));  // Default to gray
            btn->box(FL_ROUND_DOWN_BOX);
            btn->labelsize(14);
            btn->labelfont(FL_HELVETICA_BOLD);
            btn->callback(cbStrategy, this);
            m_strategyBtns.push_back(btn);
        }
    }

    // Info display
    int infoX = (new_w * 2) / 3 + 10;
    int infoY = headerHeight;
    int infoW = (new_w / 3) - 20;
    int infoH = new_h - infoY - 50;

    // Info title with proper spacing
    int infoTitleH = static_cast<int>(30 * m_scale);  // Increased height for title
    m_infoTitle = new Fl_Box(infoX, infoY, infoW - static_cast<int>(80 * m_scale), infoTitleH, "Hand Analysis");
    m_infoTitle->labelsize(static_cast<int>(24 * m_scale));
    m_infoTitle->labelfont(FL_HELVETICA_BOLD);
    m_infoTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Add zoom control buttons
    int zoomButtonSize = static_cast<int>(30 * m_scale);
    int zoomX = infoX + infoW - 2 * (zoomButtonSize + 5);
    
    m_zoomOutBtn = new Fl_Button(zoomX, infoY, zoomButtonSize, zoomButtonSize, "-");
    m_zoomOutBtn->labelsize(static_cast<int>(18 * m_scale));
    m_zoomOutBtn->labelfont(FL_HELVETICA_BOLD);
    m_zoomOutBtn->box(FL_FLAT_BOX);
    m_zoomOutBtn->color(fl_rgb_color(220, 220, 220));
    m_zoomOutBtn->callback(cbZoomOut, this);

    m_zoomInBtn = new Fl_Button(zoomX + zoomButtonSize + 5, infoY, zoomButtonSize, zoomButtonSize, "+");
    m_zoomInBtn->labelsize(static_cast<int>(18 * m_scale));
    m_zoomInBtn->labelfont(FL_HELVETICA_BOLD);
    m_zoomInBtn->box(FL_FLAT_BOX);
    m_zoomInBtn->color(fl_rgb_color(220, 220, 220));
    m_zoomInBtn->callback(cbZoomIn, this);

    // Adjust info display position to account for title
    m_infoBuffer = new Fl_Text_Buffer();
    m_infoDisplay = new Fl_Text_Display(infoX, infoY + infoTitleH + 5, infoW, infoH - infoTitleH - 5);
    m_infoDisplay->buffer(m_infoBuffer);
    m_infoDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);  // Enable word wrapping
    m_infoDisplay->box(FL_FLAT_BOX);
    m_infoDisplay->color(fl_rgb_color(230, 230, 230));
    m_infoDisplay->selection_color(fl_rgb_color(200, 200, 200));
    m_infoDisplay->scrollbar_width(7);
    m_infoDisplay->scrollbar_align(FL_ALIGN_RIGHT);
    updateInfoDisplaySettings();  // Initialize display settings

    // Action buttons - Position them below the grid with proper spacing
    // Calculate button sizes based on available width and reasonable proportions
    int totalAvailableWidth = new_w - 100;  // Leave 50px margin on each side
    int numButtons = 4;  // Number of action buttons
    
    // Reserve space for back/undo buttons on the left
    int backUndoWidth = static_cast<int>(300 * m_scale);  // Fixed width for back/undo section
    int actionButtonsWidth = totalAvailableWidth - backUndoWidth - static_cast<int>(50 * m_scale);  // Space between sections
    
    // Calculate action button sizes
    int buttonWidth = std::min(
        static_cast<int>(250 * m_scale),  // Max width
        std::max(static_cast<int>(120 * m_scale),  // Min width
                actionButtonsWidth / numButtons - static_cast<int>(15 * m_scale))  // Leave spacing between buttons
    );
    
    int buttonHeight = static_cast<int>(buttonWidth * 0.4);  // Height is 40% of width
    buttonHeight = std::min(static_cast<int>(50 * m_scale),  // Max height
                          std::max(static_cast<int>(35 * m_scale),  // Min height
                                 buttonHeight));

    // Position buttons at bottom with proper margin
    int bottomMargin = static_cast<int>(20 * m_scale);
    int btnY = new_h - buttonHeight - bottomMargin;

    // Back/Undo buttons - make them proportional but smaller than action buttons
    int backButtonWidth = static_cast<int>(backUndoWidth * 0.45);  // 45% of reserved space
    int backButtonHeight = buttonHeight;
    int backButtonSpacing = static_cast<int>(10 * m_scale);
    
    auto *backBtn = new Fl_Button(50, btnY, backButtonWidth, backButtonHeight, "Back");
    backBtn->labelsize(static_cast<int>(16 * m_scale));
    backBtn->labelfont(FL_HELVETICA_BOLD);
    backBtn->box(FL_FLAT_BOX);
    backBtn->color(fl_rgb_color(200, 200, 200));
    backBtn->callback(cbBack, this);

    auto *undoBtn = new Fl_Button(50 + backButtonWidth + backButtonSpacing, 
                                 btnY, backButtonWidth, backButtonHeight, "Undo");
    undoBtn->labelsize(static_cast<int>(16 * m_scale));
    undoBtn->labelfont(FL_HELVETICA_BOLD);
    undoBtn->box(FL_FLAT_BOX);
    undoBtn->color(fl_rgb_color(200, 200, 200));
    undoBtn->callback(cbUndo, this);

    // Position action buttons on the right side with even spacing
    int actionStartX = 50 + backUndoWidth + static_cast<int>(50 * m_scale);  // Start after back/undo section
    int actionSpacing = (actionButtonsWidth - (numButtons * buttonWidth)) / (numButtons - 1);
    
    for (int i = 0; i < numButtons; i++) {
        auto *btn = new Fl_Button(actionStartX + i * (buttonWidth + actionSpacing), 
                                 btnY, buttonWidth, buttonHeight);
        btn->labelsize(static_cast<int>(16 * m_scale));
        btn->labelfont(FL_HELVETICA_BOLD);
        btn->box(FL_FLAT_BOX);
        btn->color(fl_rgb_color(220, 220, 220));
        btn->callback(cbAction, this);
        m_actionBtns.push_back(btn);
    }

    // Position card choice dropdown between back/undo and action buttons with increased visibility
    int dropdownX = 50 + (2 * backButtonWidth) + backButtonSpacing + static_cast<int>(30 * m_scale);
    int dropdownWidth = actionStartX - dropdownX - static_cast<int>(30 * m_scale);
    int dropdownHeight = static_cast<int>(40 * m_scale);  // Increased height for better visibility
    
    m_cardChoice = new Fl_Choice(dropdownX, btnY + (buttonHeight - dropdownHeight)/2, 
                                dropdownWidth, dropdownHeight);
    m_cardChoice->labelsize(static_cast<int>(18 * m_scale));  // Increased font size
    m_cardChoice->textsize(static_cast<int>(18 * m_scale));   // Increased text size
    m_cardChoice->textfont(FL_HELVETICA_BOLD);  // Make text bold
    m_cardChoice->box(FL_UP_BOX);  // More visible box style
    m_cardChoice->color(fl_rgb_color(240, 240, 240));  // Lighter background
    m_cardChoice->callback([](Fl_Widget *w, void *v) { ((Wizard *)v)->doCardSelect(); }, this);
    m_cardChoice->hide();

    // Adjust info display height to match grid
    m_infoDisplay->size(m_infoDisplay->w(), gridH - m_infoTitle->h() - 5);

    // Create rank and suit choice dropdowns
    m_rankChoice = new Fl_Choice(0, 0, 100, 30);  // Position and size will be set in updateStrategyDisplay
    m_rankChoice->labelsize(static_cast<int>(18 * m_scale));
    m_rankChoice->textsize(static_cast<int>(18 * m_scale));
    m_rankChoice->textfont(FL_HELVETICA_BOLD);
    m_rankChoice->box(FL_UP_BOX);
    m_rankChoice->color(fl_rgb_color(240, 240, 240));
    m_rankChoice->callback([](Fl_Widget *w, void *v) { ((Wizard *)v)->doCardSelect(); }, this);
    m_rankChoice->hide();

    m_suitChoice = new Fl_Choice(0, 0, 100, 30);  // Position and size will be set in updateStrategyDisplay
    m_suitChoice->labelsize(static_cast<int>(18 * m_scale));
    m_suitChoice->textsize(static_cast<int>(18 * m_scale));
    m_suitChoice->textfont(FL_HELVETICA_BOLD);
    m_suitChoice->box(FL_UP_BOX);
    m_suitChoice->color(fl_rgb_color(240, 240, 240));
    m_suitChoice->callback([](Fl_Widget *w, void *v) { ((Wizard *)v)->doCardSelect(); }, this);
    m_suitChoice->hide();

    m_pg6->end();
    m_pg6->hide();

    resizable(this);
    end();
  }
};

int main(int argc, char **argv) {
  fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE * 2);
  fl_message_hotspot(1);
  Wizard wiz("Wizard");  // No need to specify size here anymore
  wiz.show(argc, argv);
  return Fl::run();
}
