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

static const std::vector<std::string> RANKS = {
    "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
static const std::vector<char> SUITS = {'h', 'd', 'c', 's'};
constexpr float SCALE = 1.6f;

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

class CardButton : public Fl_Button {
  Fl_Color m_base;
  bool m_sel = false;
  bool m_strategy_sel = false;  // New: separate tracking for strategy selection
  static const Fl_Color HIGHLIGHT;
  static const Fl_Color UNCOLORED_BG;  // New: background color for uncolored cards
  std::vector<std::pair<Fl_Color, float>> m_strategy_colors; // Color and percentage pairs

public:
  CardButton(int X, int Y, int W, int H, Fl_Color baseColor)
      : Fl_Button(X, Y, W, H, ""), m_base(baseColor) {
    box(FL_FLAT_BOX); // Change to flat box for square shape
    labelcolor(FL_WHITE);
    labelsize(18);
    color(m_base);
    visible_focus(false);
  }

  void toggle() {
    m_sel = !m_sel;
    color(m_sel ? HIGHLIGHT : m_base);
    redraw();
  }

  void select(bool s) {
    if (s != m_sel)
      toggle();
  }

  bool selected() const { return m_sel; }

  // New: methods for strategy selection
  void setStrategySelected(bool sel) {
    if (m_strategy_sel != sel) {
      m_strategy_sel = sel;
      redraw();
    }
  }

  bool strategySelected() const { return m_strategy_sel; }

  void setStrategyColors(const std::vector<std::pair<Fl_Color, float>> &colors) {
    m_strategy_colors = colors;
    m_strategy_sel = false;  // Reset selection state when colors are updated
    redraw();
  }

protected:
  void draw() override {
    // Draw base button
    if (!m_strategy_colors.empty()) {
      int x = this->x();
      int y = this->y();
      int w = this->w();
      int h = this->h();

      // First draw the base background
      fl_color(FL_BACKGROUND_COLOR);
      fl_rectf(x, y, w, h);

      // Draw strategy color bars
      int current_y = y;
      for (const auto &[color, percentage] : m_strategy_colors) {
        if (percentage > 0.001f) { // Only draw if > 0.1%
          int bar_height = static_cast<int>(h * percentage);
          fl_color(color);
          fl_rectf(x, current_y, w, bar_height);
          current_y += bar_height;
        }
      }

      // Redraw label on top
      fl_color(labelcolor());
      fl_font(labelfont(), labelsize());
      fl_draw(label(), x, y, w, h, FL_ALIGN_CENTER);

      // Draw black border if this hand is selected in strategy display
      if (m_strategy_sel) {
        fl_color(FL_BLACK);
        fl_line_style(FL_SOLID, 4);  // Increase line thickness to 4 pixels
        
        // Draw slightly rounded rectangle
        int corner_radius = 3;  // Small radius for subtle rounding
        
        // Top-left corner
        fl_arc(x + corner_radius, y + corner_radius, corner_radius * 2, corner_radius * 2, 90, 180);
        // Top-right corner
        fl_arc(x + w - corner_radius * 3, y + corner_radius, corner_radius * 2, corner_radius * 2, 0, 90);
        // Bottom-left corner
        fl_arc(x + corner_radius, y + h - corner_radius * 3, corner_radius * 2, corner_radius * 2, 180, 270);
        // Bottom-right corner
        fl_arc(x + w - corner_radius * 3, y + h - corner_radius * 3, corner_radius * 2, corner_radius * 2, 270, 360);
        
        // Connect the corners with lines
        fl_line(x + corner_radius, y, x + w - corner_radius, y);           // Top
        fl_line(x + w, y + corner_radius, x + w, y + h - corner_radius);   // Right
        fl_line(x + corner_radius, y + h, x + w - corner_radius, y + h);   // Bottom
        fl_line(x, y + corner_radius, x, y + h - corner_radius);           // Left
        
        fl_line_style(0);  // Reset line style
      }
    } else {
      // For non-strategy buttons, draw shaded background if uncolored
      if (m_base == FL_BACKGROUND_COLOR) {
        int x = this->x();
        int y = this->y();
        int w = this->w();
        int h = this->h();
        
        // Draw shaded background
        fl_color(UNCOLORED_BG);
        fl_rectf(x, y, w, h);
      }
      Fl_Button::draw();  // Use default drawing
    }
  }

  int handle(int event) override {
    if (event == FL_FOCUS || event == FL_UNFOCUS)
      return 0;
    return Fl_Button::handle(event);
  }
};

const Fl_Color CardButton::HIGHLIGHT = fl_rgb_color(255, 200, 0);
const Fl_Color CardButton::UNCOLORED_BG = fl_rgb_color(80, 80, 80);  // Changed to match range picker gray

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
    // Clear all range selections when going back
    for (auto *btn : m_rangeBtns) {
      btn->select(false);
    }
    for (auto *btn : m_villainBtns) {
      btn->select(false);
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

    // Clear any existing villain range selections before showing page 4
    for (auto *btn : m_villainBtns) {
      btn->select(false);
    }

    m_pg3->hide();
    m_pg4->show();

    // Auto-fill villain range based on position and pot type
    autoFillRange(m_villainBtns, m_data.theirPos, false);
  }
  // Page4 villain range
  void doBack4() {
    // Clear all range selections when going back
    for (auto *btn : m_rangeBtns) {
      btn->select(false);
    }
    for (auto *btn : m_villainBtns) {
      btn->select(false);
    }
    m_pg4->hide();
    m_pg3->show();
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
    if (m_current_node &&
        m_current_node->get_node_type() == NodeType::TERMINAL_NODE) {
      m_lblStrategy->copy_label("Terminal Node - Hand Complete");

      // Hide action buttons at terminal nodes
      for (auto *btn : m_actionBtns) {
        btn->hide();
      }
      return;
    }

    if (!m_current_node ||
        m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    const auto &strategy = action_node->get_average_strat();
    const auto &actions = action_node->get_actions();

    // Update title
    std::string title =
        (action_node->get_player() == 1 ? "Hero's" : "Villain's") +
        std::string(" Turn");
    m_lblStrategy->copy_label(title.c_str());

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
          // Hand not in range - show empty box
          btn->setStrategyColors({});
          btn->color(FL_BACKGROUND_COLOR);
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

  void updateInfoBox(const std::string &hand) {
    if (!m_current_node || m_current_node->get_node_type() != NodeType::ACTION_NODE)
        return;

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &strategy = action_node->get_average_strat();
    const auto &actions = action_node->get_actions();
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    size_t num_hands = hands.size();

    // Build simplified strategy info string
    std::string info = "\n" + hand + " Strategy:\n\n";

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

    // Show strategies for each valid combo
    for (const auto &combo : combos) {
        info += combo.first + ":\n";
        
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
                info += "    " + action_str + ": " + std::to_string(int(prob + 0.5f)) + "%\n";
            }
        }
        info += "\n";
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

      // Update title based on chance type
      auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);
      std::string prompt = "Please Select ";
      prompt += (chance_node->get_type() == ChanceNode::ChanceType::DEAL_TURN
                     ? "Turn"
                     : "River");
      prompt += " Card";
      m_lblStrategy->copy_label(prompt.c_str());

      m_cardChoice->show();
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

    // Save current state before adding card
    GameState state{m_current_node, m_p1_stack, m_p2_stack,  m_current_pot,
                    m_p1_wager,     m_p2_wager, m_data.board};
    m_history.push_back(state);

    auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);

    // Get selected card text
    std::string selected_card_str = m_cardChoice->text();
    m_data.board.push_back(selected_card_str);

    // Find the correct child index by creating cards the same way the solver
    // does
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
    m_cardChoice->hide();

    updateStrategyDisplay();
    updateActionButtons();
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
    m_data.board = state.board; // This will restore the previous board state

    // Check if we're returning to a chance node
    if (m_current_node->get_node_type() == NodeType::CHANCE_NODE) {
      auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);

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
        m_cardChoice->value(0); // Select first card by default
      }

      // Update title based on chance type
      std::string prompt = "Please Select ";
      prompt +=
          (chance_node->get_type() == ChanceNode::DEAL_TURN ? "Turn" : "River");
      prompt += " Card";
      m_lblStrategy->copy_label(prompt.c_str());

      m_cardChoice->show();

      // Hide action buttons when showing card choice
      for (auto *btn : m_actionBtns) {
        btn->hide();
      }
    } else {
      m_cardChoice->hide();
      updateStrategyDisplay();
      updateActionButtons();
    }

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

public:
  Wizard(int W, int H, const char *L = 0) : Fl_Window(W, H, L) {
    position((Fl::w() - W) / 2, (Fl::h() - H) / 2);
    
    // Enable the window's title bar and close button
    border(1);

    // Page1
    m_pg1 = new Fl_Group(0, 0, W, H);
    
    int xL = 50, xI = 300, y = 50, h = 60, sp = 25;
    int wL = 250, wI = W - xI - 100;
    
    auto addLbl = [&](const char *t) {
      auto f = new Fl_Box(xL, y, wL, h, t);
      f->labelsize(24);
    };
    
    auto addInp = [&](Fl_Input *&w) {
      w = new Fl_Input(xI, y, wI, h);
      w->textsize(24);
    };
    
    auto addFlt = [&](Fl_Float_Input *&w) {
      w = new Fl_Float_Input(xI, y, wI, h);
      w->textsize(24);
    };
    
    auto addCh = [&](Fl_Choice *&w) {
      w = new Fl_Choice(xI, y, wI, h);
      w->textsize(24);
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
    m_chkAutoImport = new Fl_Check_Button(xL, y, W - 100, 60, "Auto-import ranges based on positions");  // Height 60 instead of 80 or 40
    m_chkAutoImport->labelsize(30);  // Size 30 instead of 40 or 20
    m_chkAutoImport->value(1);  // Checked by default
    y += 60 + sp;  // Adjusted spacing for medium checkbox

    m_btn1Next = new Fl_Button((W - 300) / 2, y, 300, 70, "Next");
    m_btn1Next->labelsize(24);
    m_btn1Next->callback(cb1Next, this);
    m_pg1->end();

    // Page2
    m_pg2 = new Fl_Group(0, 0, W, H);
    
    m_lblBoard = new Fl_Box(0, 20, W, 50, "Init Board (3-5 Cards)");
    m_lblBoard->labelfont(FL_BOLD);
    m_lblBoard->labelsize(28);
    m_lblBoard->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    // Card grid layout
    int cols = 4, rows = 13, GX = 50, GY = 100, GW = W - 100, GH = H - 900;
    int bw = int((static_cast<float>(GW) / cols - 10)),
        bh = int((static_cast<float>(GH) / rows - 10) * SCALE),
        cardH = (bh * 3) / 2, rowSp = cardH + 8;

    // Create card grid
    for (int r = 0; r < rows; ++r) {
        for (int s = 0; s < cols; ++s) {
            int x = GX + s * (bw + 10), y0 = GY + r * rowSp;
            Fl_Color base;
            switch (SUITS[s]) {
                case 'h': base = fl_rgb_color(180, 30, 30); break;
                case 'd': base = fl_rgb_color(30, 30, 180); break;
                case 'c': base = fl_rgb_color(30, 180, 30); break;
                default:  base = fl_rgb_color(20, 20, 20);
            }
            auto *cb = new CardButton(x, y0, bw, cardH, base);
            std::string lbl = RANKS[r] + std::string(1, SUITS[s]);
            cb->copy_label(lbl.c_str());
            cb->callback(cbCard, this);
            m_cards.push_back(cb);
        }
    }

    m_selDisplay = new Fl_Input(50, GY + rows * rowSp + 20, W - 400, 60);
    m_selDisplay->textsize(24);
    m_selDisplay->readonly(1);

    m_btnRand = new Fl_Button(W - 350, m_selDisplay->y(), 320, 60, "Generate Random Flop");
    m_btnRand->labelsize(24);
    m_btnRand->callback(cbRand, this);

    int yBtn = H - 200;
    m_btn2Back = new Fl_Button(50, yBtn, 300, 100, "Back");
    m_btn2Back->labelsize(24);
    m_btn2Back->callback(cb2Back, this);

    m_btn2Next = new Fl_Button(W - 350, yBtn, 300, 100, "Next");
    m_btn2Next->labelsize(24);
    m_btn2Next->callback(cb2Next, this);

    m_pg2->end();
    m_pg2->hide();

    // Page3
    m_pg3 = new Fl_Group(0, 0, W, H);
    
    m_lblRange = new Fl_Box(0, 20, W, 50, "Range Editor (you)");
    m_lblRange->labelfont(FL_BOLD);
    m_lblRange->labelsize(28);
    m_lblRange->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    int RGX = 50, RGY = 100, RGW = W - 100, RGH = H - 900;
    int rbw = int((static_cast<float>(RGW) / 13 - 10)),
        rbh = int(((static_cast<float>(RGH) / 13 - 10) * 3 / 2) * SCALE),
        rsp = rbh + 8;

    for (int i = 0; i < 13; ++i) {
        for (int j = 0; j < 13; ++j) {
            int x = RGX + j * (rbw + 10), y0 = RGY + i * rsp;
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
                base = fl_rgb_color(80, 80, 80);  // Changed from FL_BACKGROUND_COLOR to a medium gray
            }
            auto *btn = new CardButton(x, y0, rbw, rbh, base);
            btn->copy_label(lbl.c_str());
            btn->callback(cbRange, this);
            btn->clear_visible_focus();
            m_rangeBtns.push_back(btn);
        }
    }

    m_btn3Back = new Fl_Button(50, yBtn, 300, 100, "Back");
    m_btn3Back->labelsize(24);
    m_btn3Back->callback(cb3Back, this);

    m_btn3Next = new Fl_Button(W - 350, yBtn, 300, 100, "Next");
    m_btn3Next->labelsize(24);
    m_btn3Next->callback(cb3Next, this);

    m_pg3->end();
    m_pg3->hide();

    // Page4
    m_pg4 = new Fl_Group(0, 0, W, H);
    
    m_lblVillain = new Fl_Box(0, 20, W, 50, "Range Editor (villain)");
    m_lblVillain->labelfont(FL_BOLD);
    m_lblVillain->labelsize(28);
    m_lblVillain->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

    for (int i = 0; i < 13; ++i) {
        for (int j = 0; j < 13; ++j) {
            int x = RGX + j * (rbw + 10), y0 = RGY + i * rsp;
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
                base = fl_rgb_color(80, 80, 80);  // Changed from FL_BACKGROUND_COLOR to a medium gray
            }
            auto *btn = new CardButton(x, y0, rbw, rbh, base);
            btn->copy_label(lbl.c_str());
            btn->callback(cbRange, this);
            btn->clear_visible_focus();
            m_villainBtns.push_back(btn);
        }
    }

    m_btn4Back = new Fl_Button(50, yBtn, 300, 100, "Back");
    m_btn4Back->labelsize(24);
    m_btn4Back->callback(cb4Back, this);

    m_btn4Next = new Fl_Button(W - 350, yBtn, 300, 100, "Next");
    m_btn4Next->labelsize(24);
    m_btn4Next->callback(cb4Next, this);

    m_pg4->end();
    m_pg4->hide();

    // Page5
    m_pg5 = new Fl_Group(0, 0, W, H);
    
    m_lblWait = new Fl_Box(0, 0, W, H, "Please wait...");
    m_lblWait->labelsize(36);
    m_lblWait->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
    
    m_pg5->end();
    m_pg5->hide();

    // Page6 - Strategy Display
    m_pg6 = new Fl_Group(0, 0, W, H);
    m_pg6->box(FL_FLAT_BOX);
    m_pg6->color(fl_rgb_color(240, 240, 240));  // Light gray background

    // Title area
    int titleY = 30;
    int titleH = 25;

    m_lblStrategy = new Fl_Box(0, titleY, W, titleH);
    m_lblStrategy->labelsize(28);
    m_lblStrategy->labelfont(FL_HELVETICA_BOLD);
    m_lblStrategy->align(FL_ALIGN_CENTER);

    m_boardInfo = new Fl_Box(0, titleY + titleH + 5, W, titleH);
    m_boardInfo->labelsize(16);
    m_boardInfo->labelfont(FL_HELVETICA);
    m_boardInfo->align(FL_ALIGN_CENTER);

    m_potInfo = new Fl_Box(0, titleY + 2 * titleH + 10, W, titleH);
    m_potInfo->labelsize(14);
    m_potInfo->labelfont(FL_HELVETICA);
    m_potInfo->align(FL_ALIGN_CENTER);

    int headerHeight = titleY + 3 * titleH + 30;

    // Strategy grid
    int gridX = 40;
    int gridY = headerHeight;
    int gridW = (W * 2) / 3 - 80;
    int gridH = H - gridY - 100;

    int cellSize = std::min(gridW / 13, gridH / 13);
    cellSize = static_cast<int>(cellSize * 0.925);
    int cellPadding = 6;

    gridX = (((W * 2) / 3) - (13 * (cellSize + cellPadding))) / 2;

    // Create strategy grid buttons
    for (int r = 0; r < 13; r++) {
        for (int c = 0; c < 13; c++) {
            int x = gridX + c * (cellSize + cellPadding);
            int y = gridY + r * (cellSize + cellPadding);

            auto *btn = new CardButton(x, y, cellSize, cellSize, FL_BACKGROUND_COLOR);
            btn->box(FL_ROUND_DOWN_BOX);
            btn->labelsize(14);
            btn->labelfont(FL_HELVETICA_BOLD);
            btn->callback(cbStrategy, this);
            m_strategyBtns.push_back(btn);
        }
    }

    // Info display
    int infoX = (W * 2) / 3 + 20;
    int infoY = headerHeight;
    int infoW = (W / 3) - 40;
    int infoH = H - infoY - 100;

    m_infoTitle = new Fl_Box(infoX, infoY - 30, infoW, 25, "Hand Analysis");
    m_infoTitle->labelsize(24);
    m_infoTitle->labelfont(FL_HELVETICA_BOLD);
    m_infoTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    m_infoBuffer = new Fl_Text_Buffer();
    m_infoDisplay = new Fl_Text_Display(infoX, infoY, infoW, infoH);
    m_infoDisplay->buffer(m_infoBuffer);
    m_infoDisplay->textsize(22);
    m_infoDisplay->textfont(FL_HELVETICA);
    m_infoDisplay->box(FL_FLAT_BOX);
    m_infoDisplay->color(fl_rgb_color(230, 230, 230));
    m_infoDisplay->selection_color(fl_rgb_color(200, 200, 200));
    m_infoDisplay->scrollbar_width(15);
    m_infoDisplay->scrollbar_align(FL_ALIGN_RIGHT);

    // Action buttons
    int btnY = H - 70;
    int btnW = 100;
    int btnH = 35;
    int btnSpacing = 15;

    int backBtnW = 70;
    int backBtnH = 30;
    int backBtnY = btnY + (btnH - backBtnH) / 2;

    auto *backBtn = new Fl_Button(40, backBtnY, backBtnW, backBtnH, "Back");
    backBtn->labelsize(12);
    backBtn->labelfont(FL_HELVETICA_BOLD);
    backBtn->box(FL_FLAT_BOX);
    backBtn->color(fl_rgb_color(200, 200, 200));
    backBtn->callback(cbBack, this);

    auto *undoBtn = new Fl_Button(40 + backBtnW + 10, backBtnY, backBtnW, backBtnH, "Undo");
    undoBtn->labelsize(12);
    undoBtn->labelfont(FL_HELVETICA_BOLD);
    undoBtn->box(FL_FLAT_BOX);
    undoBtn->color(fl_rgb_color(200, 200, 200));
    undoBtn->callback(cbUndo, this);

    int actionBtnX = W - (4 * btnW + 3 * btnSpacing + 40);
    for (int i = 0; i < 4; i++) {
        auto *btn = new Fl_Button(actionBtnX + i * (btnW + btnSpacing), btnY, btnW, btnH);
        btn->labelsize(14);
        btn->labelfont(FL_HELVETICA_BOLD);
        btn->box(FL_FLAT_BOX);
        btn->color(fl_rgb_color(220, 220, 220));
        btn->callback(cbAction, this);
        m_actionBtns.push_back(btn);
    }

    m_cardChoice = new Fl_Choice(actionBtnX - btnW * 2 - btnSpacing, btnY, btnW * 1.5, btnH);
    m_cardChoice->labelsize(14);
    m_cardChoice->labelfont(FL_HELVETICA);
    m_cardChoice->callback([](Fl_Widget *w, void *v) { ((Wizard *)v)->doCardSelect(); }, this);
    m_cardChoice->hide();

    m_pg6->end();
    m_pg6->hide();

    end();
  }
};

int main(int argc, char **argv) {
  fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE * 3);
  fl_message_hotspot(1);
  Wizard wiz(2000, 1375, "Wizard");
  wiz.show(argc, argv);
  return Fl::run();
}
