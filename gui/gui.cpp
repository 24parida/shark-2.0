
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
#include <numeric>
#include <random>
#include <string>
#include <vector>

// CardButton: rounded box with base suit color and selection highlight
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
};
const Fl_Color CardButton::HIGHLIGHT = fl_rgb_color(255, 200, 0);

// Wizard: multi-page FLTK application
class Wizard : public Fl_Window {
  // User inputs and selections
  struct UserInputs {
    int stackSize{};
    int startingPot{};
    int minBet{};
    int iterations{};
    float allInThreshold{};
    std::string potType, yourPos, theirPos;
    std::vector<std::string> board;
    std::vector<std::string> heroRange;
    std::vector<std::string> villainRange;
  } m_data;

  // Page groups
  Fl_Group *m_pg1{nullptr}, *m_pg2{nullptr}, *m_pg3{nullptr}, *m_pg4{nullptr};

  // Page1 widgets
  Fl_Input *m_inpStack{nullptr}, *m_inpPot{nullptr}, *m_inpMinBet{nullptr},
      *m_inpIters{nullptr};
  Fl_Float_Input *m_inpAllIn{nullptr};
  Fl_Choice *m_choPotType{nullptr}, *m_choYourPos{nullptr},
      *m_choTheirPos{nullptr};
  Fl_Button *m_btn1Next{nullptr};

  // Page2 widgets
  Fl_Box *m_lblBoard{nullptr};
  std::vector<CardButton *> m_cards;
  Fl_Input *m_selDisplay{nullptr};
  Fl_Button *m_btnRand{nullptr}, *m_btn2Back{nullptr}, *m_btn2Next{nullptr};

  // Page3 widgets
  Fl_Box *m_lblRange{nullptr};
  std::vector<CardButton *> m_rangeBtns;
  Fl_Button *m_btn3Back{nullptr}, *m_btn3Next{nullptr};

  // Page4 widgets
  Fl_Box *m_lblVillain{nullptr};
  std::vector<CardButton *> m_villainBtns;
  Fl_Button *m_btn4Back{nullptr}, *m_btn4Next{nullptr};

  // Callbacks
  static void cb1Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do1Next(); }
  static void cbCard(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doCard((CardButton *)w);
  }
  static void cbRand(Fl_Widget *w, void *d) { ((Wizard *)d)->doRand(); }
  static void cb2Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack2(); }
  static void cb2Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do2Next(); }
  static void cb3Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack3(); }
  static void cbRange(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doRange((CardButton *)w);
  }
  static void cb3Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do3Next(); }
  static void cb4Back(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack4(); }
  static void cb4Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do4Next(); }

  // --- Page1 to Page2 ---
  void do1Next() {
    if (!m_inpStack->value()[0] || !m_inpPot->value()[0] ||
        !m_inpMinBet->value()[0] || !m_inpAllIn->value()[0] ||
        !m_inpIters->value()[0]) {
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
    m_data.potType = m_choPotType->text();
    m_data.yourPos = m_choYourPos->text();
    m_data.theirPos = m_choTheirPos->text();
    m_pg1->hide();
    m_pg2->show();
  }

  // --- Page2 board selector ---
  void doCard(CardButton *cb) {
    int count = std::count_if(m_cards.begin(), m_cards.end(),
                              [](auto *b) { return b->selected(); });
    if (!cb->selected() && count >= 5)
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
        std::string lbl = cb->label();
        m_data.board.push_back(lbl);
        if (!out.empty())
          out += ' ';
        out += lbl;
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

  // --- Page3 hero range selector ---
  void doRange(CardButton *cb) { cb->toggle(); }
  void doBack3() {
    m_pg3->hide();
    m_pg2->show();
  }
  void do3Next() {
    m_data.heroRange.clear();
    for (auto *b : m_rangeBtns)
      if (b->selected())
        m_data.heroRange.push_back(b->label());
    m_pg3->hide();
    m_pg4->show();
  }

  // --- Page4 villain range selector ---
  void doBack4() {
    m_pg4->hide();
    m_pg3->show();
  }
  void do4Next() {
    m_data.villainRange.clear();
    for (auto *b : m_villainBtns)
      if (b->selected())
        m_data.villainRange.push_back(b->label());
    hide(); // completed
  }

public:
  Wizard(int W, int H, const char *L = 0) : Fl_Window(W, H, L) {
    position((Fl::w() - W) / 2, (Fl::h() - H) / 2);

    // Page1 setup
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
    m_btn1Next = new Fl_Button((W - 300) / 2, y, 300, 70, "Next");
    m_btn1Next->labelsize(24);
    m_btn1Next->callback(cb1Next, this);
    m_pg1->end();

    // Page2 setup
    m_pg2 = new Fl_Group(0, 0, W, H);
    m_lblBoard = new Fl_Box(0, 20, W, 50, "Init Board (3-5 Cards)");
    m_lblBoard->labelfont(FL_BOLD);
    m_lblBoard->labelsize(28);
    m_lblBoard->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
    static const std::vector<std::string> ranks = {
        "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    static const std::vector<char> suits = {'h', 'd', 'c', 's'};
    int cols = 4, rows = 13;
    int GX = 50, GY = 100, GW = W - 100, GH = H - 900;
    int bw = GW / cols - 10;
    int bh = GH / rows - 10;
    int cardH = (bh * 3) / 2;
    int rowSp = cardH + 8;
    for (int r = 0; r < rows; ++r)
      for (int s = 0; s < cols; ++s) {
        int x = GX + s * (bw + 10), y0 = GY + r * rowSp;
        Fl_Color base;
        switch (suits[s]) {
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
        std::string lbl = ranks[r] + std::string(1, suits[s]);
        cb->copy_label(lbl.c_str());
        cb->callback(cbCard, this);
        m_cards.push_back(cb);
      }
    int ySel = GY + rows * rowSp + 20;
    m_selDisplay = new Fl_Input(50, ySel, W - 400, 60);
    m_selDisplay->textsize(24);
    m_selDisplay->readonly(1);
    m_btnRand = new Fl_Button(W - 350, ySel, 320, 60, "Generate Random Flop");
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

    // Page3 setup
    m_pg3 = new Fl_Group(0, 0, W, H);
    m_lblRange = new Fl_Box(0, 20, W, 50, "Range Editor (you)");
    m_lblRange->labelfont(FL_BOLD);
    m_lblRange->labelsize(28);
    m_lblRange->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
    int RGX = 50, RGY = 100, RGW = W - 100, RGH = H - 900;
    int rbw = RGW / 13 - 10;
    int rbh = (RGH / 13 - 10) * 3 / 2;
    int rsp = rbh + 8;
    for (int i = 0; i < 13; ++i)
      for (int j = 0; j < 13; ++j) {
        int x = RGX + j * (rbw + 10), y0 = RGY + i * rsp;
        std::string lbl;
        Fl_Color base = FL_BACKGROUND_COLOR;
        if (i == j) {
          lbl = ranks[i] + ranks[j];
          base = fl_rgb_color(100, 200, 100);
        } else if (j > i) {
          lbl = ranks[i] + ranks[j] + "s";
          base = fl_rgb_color(100, 100, 200);
        } else {
          lbl = ranks[j] + ranks[i] + "o";
        }
        auto *btn = new CardButton(x, y0, rbw, rbh, base);
        btn->copy_label(lbl.c_str());
        btn->callback(cbRange, this);
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

    // Page4 setup
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
          lbl = ranks[i] + ranks[j];
          base = fl_rgb_color(100, 200, 100);
        } else if (j > i) {
          lbl = ranks[i] + ranks[j] + "s";
          base = fl_rgb_color(100, 100, 200);
        } else {
          lbl = ranks[j] + ranks[i] + "o";
        }
        auto *btn = new CardButton(x, y0, rbw, rbh, base);
        btn->copy_label(lbl.c_str());
        btn->callback(cbRange, this);
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

    end();
  }
};

int main(int argc, char **argv) {
  Wizard wiz(2000, 1375, "Wizard");
  wiz.show(argc, argv);
  return Fl::run();
}
