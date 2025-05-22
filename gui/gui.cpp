
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

// Card button with suit-based base color and highlight
class CardButton : public Fl_Button {
  Fl_Color m_base;
  bool m_sel = false;
  static const Fl_Color HIGHLIGHT;

public:
  CardButton(int X, int Y, int W, int H, Fl_Color baseColor)
      : Fl_Button(X, Y, W, H, ""), m_base(baseColor) {
    box(FL_ROUND_UP_BOX);
    labelsize(18);
    labelcolor(FL_WHITE);
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

class Wizard : public Fl_Window {
  struct Page1 {
    int stackSize{}, startingPot{}, minBet{}, iterations{};
    float allInThreshold{};
    std::string potType, yourPos, theirPos;
  } m_p1;
  std::vector<std::string> m_board;
  Fl_Group *m_pg1{nullptr}, *m_pg2{nullptr};
  Fl_Input *m_inpStack{nullptr}, *m_inpPot{nullptr}, *m_inpMinBet{nullptr},
      *m_inpIters{nullptr};
  Fl_Float_Input *m_inpAllIn{nullptr};
  Fl_Choice *m_choPotType{nullptr}, *m_choYourPos{nullptr},
      *m_choTheirPos{nullptr};
  Fl_Button *m_btn1Next{nullptr}, *m_btnBack{nullptr}, *m_btnRand{nullptr},
      *m_btn2Next{nullptr};
  Fl_Box *m_lblBoard{nullptr};
  Fl_Input *m_selDisplay{nullptr};
  std::vector<CardButton *> m_cards;

  static void cb1Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do1Next(); }
  static void cbCard(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doCard((CardButton *)w);
  }
  static void cbRand(Fl_Widget *w, void *d) { ((Wizard *)d)->doRand(); }
  static void cbBack(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack(); }
  static void cb2Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do2Next(); }

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
    m_p1.stackSize = std::atoi(m_inpStack->value());
    m_p1.startingPot = std::atoi(m_inpPot->value());
    m_p1.minBet = std::atoi(m_inpMinBet->value());
    m_p1.allInThreshold = std::atof(m_inpAllIn->value());
    m_p1.iterations = std::atoi(m_inpIters->value());
    m_p1.potType = m_choPotType->text();
    m_p1.yourPos = m_choYourPos->text();
    m_p1.theirPos = m_choTheirPos->text();
    m_pg1->hide();
    m_pg2->show();
  }

  void doCard(CardButton *cb) {
    int c = std::count_if(m_cards.begin(), m_cards.end(),
                          [](auto *b) { return b->selected(); });
    if (!cb->selected() && c >= 5)
      return;
    cb->toggle();
    updateSel();
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
    updateSel();
  }

  void doBack() {
    m_pg2->hide();
    m_pg1->show();
  }

  void do2Next() {
    m_board.clear();
    for (auto *cb : m_cards)
      if (cb->selected())
        m_board.push_back(cb->label());
    int c = m_board.size();
    if (c < 3 || c > 5) {
      fl_message("Select 3-5 cards.");
      return;
    }
    hide();
  }

  void updateSel() {
    std::string out;
    for (auto *cb : m_cards)
      if (cb->selected()) {
        if (!out.empty())
          out += ' ';
        out += cb->label();
      }
    m_selDisplay->value(out.c_str());
  }

public:
  Wizard(int W, int H, const char *L = 0) : Fl_Window(W, H, L) {
    position((Fl::w() - W) / 2, (Fl::h() - H) / 2);

    // Page 1
    m_pg1 = new Fl_Group(0, 0, W, H);
    int xL = 50, xI = 300, y = 50, h = 60, sp = 25;
    int wL = 200, wI = W - xI - 100;
    auto lb = [&](const char *t) {
      auto f = new Fl_Box(xL, y, wL, h, t);
      f->labelsize(22);
    };
    auto in = [&](Fl_Input *&w) {
      w = new Fl_Input(xI, y, wI, h);
      w->textsize(22);
    };
    auto flt = [&](Fl_Float_Input *&w) {
      w = new Fl_Float_Input(xI, y, wI, h);
      w->textsize(22);
    };
    auto ch = [&](Fl_Choice *&w) {
      w = new Fl_Choice(xI, y, wI, h);
      w->textsize(22);
    };
    lb("Stack Size:");
    in(m_inpStack);
    y += h + sp;
    lb("Starting Pot:");
    in(m_inpPot);
    y += h + sp;
    lb("Initial Min Bet:");
    in(m_inpMinBet);
    y += h + sp;
    lb("All-In Thresh:");
    flt(m_inpAllIn);
    y += h + sp;
    lb("Type of pot:");
    ch(m_choPotType);
    m_choPotType->add("Single Raise|3-bet|4-bet");
    m_choPotType->value(0);
    y += h + sp;
    lb("Your pos:");
    ch(m_choYourPos);
    m_choYourPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    m_choYourPos->value(0);
    y += h + sp;
    lb("Their pos:");
    ch(m_choTheirPos);
    m_choTheirPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    m_choTheirPos->value(1);
    y += h + sp;
    lb("Iterations:");
    in(m_inpIters);
    y += h + sp * 2;
    m_btn1Next = new Fl_Button((W - 300) / 2, y, 300, 70, "Next");
    m_btn1Next->labelsize(24);
    m_btn1Next->callback(cb1Next, this);
    m_pg1->end();

    // Page 2
    m_pg2 = new Fl_Group(0, 0, W, H);
    m_lblBoard = new Fl_Box(0, 20, W, 50, "Init Board (3-5 Cards)");
    m_lblBoard->labelfont(FL_BOLD);
    m_lblBoard->labelsize(28);
    m_lblBoard->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
    static const std::vector<std::string> ranks = {
        "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    static const std::vector<std::string> suits = {"h", "d", "c", "s"};
    int cols = suits.size(), rows = ranks.size();
    int GX = 50, GY = 100, GW = W - 100, GH = H - 800;
    int bw = GW / cols - 8, bh = GH / rows - 8;
    // card height at 1.5x original
    int cardH = (bh * 3) / 2;
    int rowSp = cardH + 8;
    for (int r = 0; r < rows; ++r)
      for (int s = 0; s < cols; ++s) {
        int x = GX + s * (bw + 8), y0 = GY + r * rowSp;
        Fl_Color base;
        switch (suits[s][0]) {
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
        std::string lbl = ranks[r] + suits[s];
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
    m_btnBack = new Fl_Button(50, yBtn, 300, 100, "Back");
    m_btnBack->labelsize(24);
    m_btnBack->callback(cbBack, this);
    m_btn2Next = new Fl_Button(W - 350, yBtn, 300, 100, "Next");
    m_btn2Next->labelsize(24);
    m_btn2Next->callback(cb2Next, this);
    m_pg2->end();
    m_pg2->hide();
    end();
  }
};

int main(int argc, char **argv) {
  Wizard wiz(2000, 1375, "Wizard");
  wiz.show(argc, argv);
  return Fl::run();
}
