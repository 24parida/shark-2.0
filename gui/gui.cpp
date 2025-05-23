#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

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

class CardButton : public Fl_Button {
  Fl_Color m_base;
  bool m_sel = false;
  static const Fl_Color HIGHLIGHT;

public:
  CardButton(int X, int Y, int W, int H, Fl_Color baseColor)
      : Fl_Button(X, Y, W, H, ""), m_base(baseColor) {
    box(FL_ROUND_UP_BOX);
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

protected:
  int handle(int event) override {
    if (event == FL_FOCUS || event == FL_UNFOCUS)
      return 0;
    return Fl_Button::handle(event);
  }
};
const Fl_Color CardButton::HIGHLIGHT = fl_rgb_color(255, 200, 0);

class Wizard : public Fl_Window {
  struct UserInputs {
    int stackSize{}, startingPot{}, minBet{}, iterations{};
    float allInThreshold{};
    std::string potType, yourPos, theirPos;
    std::vector<std::string> board;
    std::vector<std::string> heroRange;
    std::vector<std::string> villainRange;
    float min_exploitability{};
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
  std::vector<CardButton *> m_strategyBtns;  // 13x13 grid
  Fl_Text_Display *m_infoDisplay;  // Right side info display
  Fl_Text_Buffer *m_infoBuffer;    // Buffer for info display
  std::vector<Fl_Button *> m_actionBtns;  // Bottom action buttons
  Fl_Choice *m_cardChoice;  // Turn/river card selector
  Fl_Box *m_potInfo;  // Display for pot and stack sizes
  Fl_Box *m_boardInfo;  // Display for board information
  Fl_Box *m_infoTitle;  // Info box title

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
  static void cbStrategy(Fl_Widget *w, void *d) { ((Wizard *)d)->doStrategy((CardButton *)w); }
  static void cbAction(Fl_Widget *w, void *d) { ((Wizard *)d)->doAction((Fl_Button *)w); }
  static void cbBack(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack6(); }

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
    m_pg2->hide();
    m_pg3->show();
  }

  // Page3 hero range
  void doRange(CardButton *cb) { cb->toggle(); }
  void doBack3() {
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
  }
  // Page4 villain range
  void doBack4() {
    m_pg4->hide();
    m_pg3->show();
  }

  // Page4 villain range
  void do4Next() {
    // 1) collect villain‐range selections
    m_data.villainRange.clear();
    for (auto *b : m_villainBtns)
      if (b->selected())
        m_data.villainRange.push_back(b->label());
    if (m_data.villainRange.empty()) {
      fl_message("Please select at least one hand for the villain's range.");
      return;
    }

    // 2) flip to the waiting page
    m_pg4->hide();
    m_pg5->show();
    Fl::check(); // ensure UI redraws immediately

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
        
      case Action::CALL:
        {
          int call_amount = other_wager - current_wager;
          current_stack -= call_amount;
          current_wager = other_wager;  // Match the other wager
          // Both wagers go to pot
          m_current_pot += current_wager + other_wager;
          m_p1_wager = m_p2_wager = 0;
        }
        break;
        
      case Action::BET:
        {
          current_stack -= action.amount;
          current_wager = action.amount;
        }
        break;
        
      case Action::RAISE:
        {
          int additional_amount = action.amount - current_wager;
          current_stack -= additional_amount;
          current_wager = action.amount;
        }
        break;
    }
  }

  void updateStrategyDisplay() {
    // Clear title for terminal nodes
    if (m_current_node && m_current_node->get_node_type() == NodeType::TERMINAL_NODE) {
      m_lblStrategy->copy_label("Terminal Node - Hand Complete");
      
      // Hide action buttons at terminal nodes
      for (auto *btn : m_actionBtns) {
        btn->hide();
      }
      return;
    }

    if (!m_current_node || m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

    auto *action_node = dynamic_cast<const ActionNode *>(m_current_node);
    const auto &hands = m_prm.get_preflop_combos(action_node->get_player());
    
    // Update title
    std::string title = (action_node->get_player() == 1 ? "Hero's" : "Villain's") + std::string(" Turn");
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

    // Update grid
    for (int r = 0; r < 13; r++) {
      for (int c = 0; c < 13; c++) {
        auto *btn = m_strategyBtns[r * 13 + c];
        std::string hand;
        
        if (r == c) {
          // Pocket pair
          hand = RANKS[r] + RANKS[r];
        } else if (c > r) {
          // Suited
          hand = RANKS[r] + RANKS[c] + "s";
        } else {
          // Offsuit
          hand = RANKS[c] + RANKS[r] + "o";
        }
        
        btn->copy_label(hand.c_str());
        
        // Find if this hand is in the range
        bool in_range = false;
        for (const auto &h : hands) {
          if (h.to_string() == hand) {
            in_range = true;
            break;
          }
        }
        
        if (in_range) {
          btn->color(fl_rgb_color(100, 200, 100));  // Green for available
        } else {
          btn->color(FL_BACKGROUND_COLOR);  // White for unavailable
        }
      }
    }
  }

  void updateActionButtons() {
    if (!m_current_node || m_current_node->get_node_type() != NodeType::ACTION_NODE)
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
        case Action::FOLD: label = "Fold"; break;
        case Action::CHECK: label = "Check"; break;
        case Action::CALL: label = "Call " + std::to_string(action.amount); break;
        case Action::BET: label = "Bet " + std::to_string(action.amount); break;
        case Action::RAISE: label = "Raise to " + std::to_string(action.amount); break;
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
    
    // Get all specific combos for this hand
    std::vector<std::pair<std::string, size_t>> combos;
    for (size_t i = 0; i < hands.size(); ++i) {
      const auto &h = hands[i];
      std::string hand_str = h.to_string();
      
      // Convert hand string format from "(Ah, Ad)" to "AhAd"
      hand_str = hand_str.substr(1, hand_str.length() - 2);
      hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ' '), hand_str.end());
      hand_str.erase(std::remove(hand_str.begin(), hand_str.end(), ','), hand_str.end());
      
      // Get the rank+suit format (e.g., "AKs" from "AhKh")
      std::string rank1 = hand_str.substr(0, 1);
      std::string rank2 = hand_str.substr(2, 1);
      bool suited = hand_str[1] == hand_str[3];
      std::string hand_format = rank1 + rank2 + (suited ? "s" : "o");
      if (rank1 == rank2) hand_format = rank1 + rank2;
      
      if (hand_format == hand) {
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
          std::string combo = h.hand1.describeCard() + h.hand2.describeCard();
          combos.emplace_back(combo, i);
        }
      }
    }
    
    if (combos.empty()) return;

    // Build strategy info string with better formatting and bigger text
    std::string info = "\n" + hand + " Combos:\n\n";
    for (const auto &combo : combos) {
      info += combo.first + ":\n\n";
      
      bool has_actions = false;
      std::string action_info;
      
      for (size_t i = 0; i < actions.size(); ++i) {
        const auto &action = actions[i];
        size_t strat_idx = combo.second + i * hands.size();
        
        if (strat_idx < strategy.size()) {
          float prob = strategy[strat_idx] * 100.0f;
          if (prob < 0.1f) continue;
          
          has_actions = true;
          std::string action_str;
          switch (action.type) {
            case Action::FOLD: action_str = "Fold"; break;
            case Action::CHECK: action_str = "Check"; break;
            case Action::CALL: action_str = "Call " + std::to_string(action.amount); break;
            case Action::BET: action_str = "Bet " + std::to_string(action.amount); break;
            case Action::RAISE: action_str = "Raise to " + std::to_string(action.amount); break;
          }
          action_info += "      " + action_str + ": " + std::to_string(int(prob + 0.5f)) + "%\n\n";
        }
      }
      
      if (has_actions) {
        info += action_info + "\n";
      }
    }
    
    m_infoBuffer->text(info.c_str());
    m_infoDisplay->scroll(0, 0);  // Scroll to top when updating
  }

  void doStrategy(CardButton *btn) {
    updateInfoBox(btn->label());
  }

  void doAction(Fl_Button *btn) {
    if (!m_current_node || m_current_node->get_node_type() != NodeType::ACTION_NODE)
      return;

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

    // Navigate to next node
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
      prompt += (chance_node->get_type() == ChanceNode::DEAL_TURN ? "Turn" : "River");
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
    if (!m_current_node || m_current_node->get_node_type() != NodeType::CHANCE_NODE)
      return;

    auto *chance_node = dynamic_cast<const ChanceNode *>(m_current_node);
    int selected_card = m_cardChoice->value();
    
    // Navigate to selected card's node
    m_current_node = chance_node->get_child(selected_card);
    m_cardChoice->hide();
    
    updateStrategyDisplay();
    updateActionButtons();
  }

  void doBack6() {
    m_pg6->hide();
    m_pg4->show();  // Go back to range selection
  }

public:
  Wizard(int W, int H, const char *L = 0) : Fl_Window(W, H, L) {
    position((Fl::w() - W) / 2, (Fl::h() - H) / 2);
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
    y += h + sp * 2;

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
    int cols = 4, rows = 13, GX = 50, GY = 100, GW = W - 100, GH = H - 900;
    int bw = int((static_cast<float>(GW) / cols - 10)),
        bh = int((static_cast<float>(GH) / rows - 10) * SCALE),
        cardH = (bh * 3) / 2, rowSp = cardH + 8;
    for (int r = 0; r < rows; ++r)
      for (int s = 0; s < cols; ++s) {
        int x = GX + s * (bw + 10), y0 = GY + r * rowSp;
        Fl_Color base;
        switch (SUITS[s]) {
        case 'h':
          base = fl_rgb_color(180, 30, 30);
          break;
        case 'd':
          base = fl_rgb_color(30, 30, 180);
          break;
        case 'c':
          base = fl_rgb_color(30, 180, 30);
          break;
        default:
          base = fl_rgb_color(20, 20, 20);
        }
        auto *cb = new CardButton(x, y0, bw, cardH, base);
        std::string lbl = RANKS[r] + std::string(1, SUITS[s]);
        cb->copy_label(lbl.c_str());
        cb->callback(cbCard, this);
        m_cards.push_back(cb);
      }
    m_selDisplay = new Fl_Input(50, GY + rows * rowSp + 20, W - 400, 60);
    m_selDisplay->textsize(24);
    m_selDisplay->readonly(1);
    m_btnRand = new Fl_Button(W - 350, m_selDisplay->y(), 320, 60,
                              "Generate Random Flop");
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

    for (int i = 0; i < 13; ++i)
      for (int j = 0; j < 13; ++j) {
        int x = RGX + j * (rbw + 10), y0 = RGY + i * rsp;
        std::string lbl;
        Fl_Color base = FL_BACKGROUND_COLOR;
        if (i == j) {
          lbl = RANKS[i] + RANKS[j];
          base = fl_rgb_color(100, 200, 100);
        } else if (j > i) {
          lbl = RANKS[i] + RANKS[j] + "s";
          base = fl_rgb_color(100, 100, 200);
        } else {
          lbl = RANKS[j] + RANKS[i] + "o";
        }
        auto *btn = new CardButton(x, y0, rbw, rbh, base);
        btn->copy_label(lbl.c_str());
        btn->callback(cbRange, this);
        btn->clear_visible_focus();
        m_rangeBtns.push_back(btn);
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
    for (int i = 0; i < 13; ++i)
      for (int j = 0; j < 13; ++j) {
        int x = RGX + j * (rbw + 10), y0 = RGY + i * rsp;
        std::string lbl;
        Fl_Color base = FL_BACKGROUND_COLOR;
        if (i == j) {
          lbl = RANKS[i] + RANKS[j];
          base = fl_rgb_color(100, 200, 100);
        } else if (j > i) {
          lbl = RANKS[i] + RANKS[j] + "s";
          base = fl_rgb_color(100, 100, 200);
        } else {
          lbl = RANKS[j] + RANKS[i] + "o";
        }
        auto *btn = new CardButton(x, y0, rbw, rbh, base);
        btn->copy_label(lbl.c_str());
        btn->callback(cbRange, this);
        btn->clear_visible_focus();
        m_villainBtns.push_back(btn);
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

    // Title area with more spacing
    int titleY = 30;
    int titleH = 25;
    int infoSpacing = 20;
    
    // Player turn - large bold title
    m_lblStrategy = new Fl_Box(0, titleY, W, titleH);
    m_lblStrategy->labelsize(28);
    m_lblStrategy->labelfont(FL_HELVETICA_BOLD);
    m_lblStrategy->align(FL_ALIGN_CENTER);

    // Board info - medium size
    m_boardInfo = new Fl_Box(0, titleY + titleH + 5, W, titleH);
    m_boardInfo->labelsize(16);
    m_boardInfo->labelfont(FL_HELVETICA);
    m_boardInfo->align(FL_ALIGN_CENTER);

    // Stack and pot info - smaller size
    m_potInfo = new Fl_Box(0, titleY + 2*titleH + 10, W, titleH);
    m_potInfo->labelsize(14);
    m_potInfo->labelfont(FL_HELVETICA);
    m_potInfo->align(FL_ALIGN_CENTER);

    int headerHeight = titleY + 3*titleH + 30;

    // Grid on left side (2/3 of width)
    int gridX = 40;
    int gridY = headerHeight;
    int gridW = (W * 2) / 3 - 80;
    int gridH = H - gridY - 100;
    
    // Calculate cell size for circular buttons - 7.5% smaller
    int cellSize = std::min(gridW / 13, gridH / 13);
    cellSize = static_cast<int>(cellSize * 0.925);  // 7.5% smaller instead of 0.85
    int cellPadding = 6;
    
    // Center the grid
    gridX = (((W * 2) / 3) - (13 * (cellSize + cellPadding))) / 2;
    
    // Create grid buttons
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

    // Strategy info box on right side
    int infoX = (W * 2) / 3 + 20;
    int infoY = headerHeight;
    int infoW = (W / 3) - 40;
    int infoH = H - infoY - 100;
    
    // Info box title
    m_infoTitle = new Fl_Box(infoX, infoY - 30, infoW, 25, "Hand Analysis");
    m_infoTitle->labelsize(24);
    m_infoTitle->labelfont(FL_HELVETICA_BOLD);
    m_infoTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Create text buffer and display for scrollable info
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

    // Action buttons at bottom
    int btnY = H - 70;
    int btnW = 100;
    int btnH = 35;
    int btnSpacing = 15;
    
    // Back button on far left with more spacing
    int backBtnW = 70;
    int backBtnH = 30;
    int backBtnY = btnY + (btnH - backBtnH)/2;
    auto *backBtn = new Fl_Button(40, backBtnY, backBtnW, backBtnH, "Back");
    backBtn->labelsize(12);
    backBtn->labelfont(FL_HELVETICA_BOLD);
    backBtn->box(FL_FLAT_BOX);
    backBtn->color(fl_rgb_color(200, 200, 200));
    backBtn->callback(cbBack, this);

    // Action buttons on right
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

    // Card choice dropdown - position to the left of action buttons
    int dropdownX = actionBtnX - btnW * 2 - btnSpacing;
    m_cardChoice = new Fl_Choice(dropdownX, btnY, btnW * 1.5, btnH);
    m_cardChoice->labelsize(14);
    m_cardChoice->labelfont(FL_HELVETICA);
    m_cardChoice->callback([](Fl_Widget* w, void* v) {
        ((Wizard*)v)->doCardSelect();
    }, this);
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
