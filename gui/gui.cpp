#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
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

  Fl_Group *m_pg1, *m_pg2, *m_pg3, *m_pg4, *m_pg5;

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
    std::cout << "completed";
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
