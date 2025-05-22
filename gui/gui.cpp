// file: wizard_board.cpp
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <numeric>
#include <random>
#include <string>
#include <vector>

// ─── A little button that can be “selected” ─────────────
class CardButton : public Fl_Button {
  bool m_sel = false;

public:
  CardButton(int X, int Y, int W, int H) : Fl_Button(X, Y, W, H, "") {
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND_COLOR);
  }
  void toggle() {
    m_sel = !m_sel;
    color(m_sel ? FL_SELECTION_COLOR : FL_BACKGROUND_COLOR);
    redraw();
  }
  void select(bool s) {
    if (s != m_sel)
      toggle();
  }
  bool selected() const { return m_sel; }
};

// ─── The wizard with Page 1 + Page 2 ────────────────────
class Wizard : public Fl_Window {
  // store page1 inputs
  struct Page1 {
    int stackSize{};
    int startingPot{};
    int minBet{};
    float allInThreshold{};
    int iterations{};
    std::string potType, yourPos, theirPos;
  } m_p1;

  // ── Page 1 widgets ─────────────────────────────────────
  Fl_Group *m_pg1{nullptr};
  Fl_Input *m_inpStack{nullptr}, *m_inpPot{nullptr}, *m_inpMinBet{nullptr},
      *m_inpIters{nullptr};
  Fl_Float_Input *m_inpAllIn{nullptr};
  Fl_Choice *m_choPotType{nullptr}, *m_choYourPos{nullptr},
      *m_choTheirPos{nullptr};
  Fl_Button *m_btn1Next{nullptr};

  // ── Page 2 widgets ─────────────────────────────────────
  Fl_Group *m_pg2{nullptr};
  std::vector<CardButton *> m_cards;
  Fl_Input *m_selDisplay{nullptr};
  Fl_Button *m_btnRand{nullptr}, *m_btnBack{nullptr}, *m_btn2Next{nullptr};

  // ── Callbacks ─────────────────────────────────────────
  static void cb1Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do1Next(); }
  static void cbCard(Fl_Widget *w, void *d) {
    ((Wizard *)d)->doCard((CardButton *)w);
  }
  static void cbRand(Fl_Widget *w, void *d) { ((Wizard *)d)->doRand(); }
  static void cbBack(Fl_Widget *w, void *d) { ((Wizard *)d)->doBack(); }
  static void cb2Next(Fl_Widget *w, void *d) { ((Wizard *)d)->do2Next(); }

  void do1Next() {
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
    cb->toggle();
    updateSel();
  }

  void doRand() {
    // clear
    for (auto *cb : m_cards)
      cb->select(false);
    // random 3 distinct
    std::vector<int> idx(m_cards.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::mt19937_64 rng((unsigned)std::chrono::high_resolution_clock::now()
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
    // here you can use m_selDisplay->value() as final board
    hide();
  }

  void updateSel() {
    std::string out;
    for (auto *cb : m_cards) {
      if (cb->selected()) {
        if (!out.empty())
          out += ' ';
        out += cb->label();
      }
    }
    m_selDisplay->value(out.c_str());
  }

public:
  Wizard(int W, int H, const char *L = 0) : Fl_Window(W, H, L) {
    position((Fl::w() - W) / 2, (Fl::h() - H) / 2);

    // ── Page 1 ─────────────────────────────────────────
    m_pg1 = new Fl_Group(0, 0, W, H);
    int xL = 20, xI = 140, y = 20, h = 25, sp = 10;
    int wL = 120, wI = W - xI - 20;

    new Fl_Box(xL, y, wL, h, "Stack Size:");
    m_inpStack = new Fl_Input(xI, y, wI, h);
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Starting Pot:");
    m_inpPot = new Fl_Input(xI, y, wI, h);
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Initial Min Bet:");
    m_inpMinBet = new Fl_Input(xI, y, wI, h);
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "All‑In Thresh:");
    m_inpAllIn = new Fl_Float_Input(xI, y, wI, h);
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Type of pot:");
    m_choPotType = new Fl_Choice(xI, y, wI, h);
    m_choPotType->add("Single Raise|3-bet|4-bet");
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Your pos:");
    m_choYourPos = new Fl_Choice(xI, y, wI, h);
    m_choYourPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Their pos:");
    m_choTheirPos = new Fl_Choice(xI, y, wI, h);
    m_choTheirPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
    y += h + sp;
    new Fl_Box(xL, y, wL, h, "Iterations:");
    m_inpIters = new Fl_Input(xI, y, wI, h);
    y += h + sp * 2;
    m_btn1Next = new Fl_Button((W - 100) / 2, y, 100, 30, "Next");
    m_btn1Next->callback(cb1Next, this);
    m_pg1->end();

    // ── Page 2 ─────────────────────────────────────────
    m_pg2 = new Fl_Group(0, 0, W, H);
    // grid area:
    static const std::vector<std::string> ranks = {
        "A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    static const std::vector<std::string> suits = {"h", "d", "c", "s"};
    int cols = suits.size(), rows = ranks.size();
    int GX = 20, GY = 20, GW = W - 40, GH = H - 200;
    int bw = GW / cols - 4, bh = GH / rows - 4;
    for (int r = 0; r < rows; ++r) {
      for (int s = 0; s < cols; ++s) {
        int x = GX + s * (bw + 4), y0 = GY + r * (bh + 4);
        std::string lbl = ranks[r] + suits[s];
        auto *cb = new CardButton(x, y0, bw, bh);
        cb->copy_label(lbl.c_str());
        cb->callback(cbCard, this);
        m_cards.push_back(cb);
      }
    }

    m_selDisplay = new Fl_Input(20, H - 150, W - 240, 30);
    m_selDisplay->readonly(1);

    m_btnRand =
        new Fl_Button(W - 200, H - 150, 180, 30, "Generate Random Flop");
    m_btnRand->callback(cbRand, this);

    m_btnBack = new Fl_Button(20, H - 80, 100, 40, "Back");
    m_btnBack->callback(cbBack, this);
    m_btn2Next = new Fl_Button(W - 120, H - 80, 100, 40, "Next");
    m_btn2Next->callback(cb2Next, this);
    m_pg2->end();
    m_pg2->hide();

    end();
  }
};

int main(int argc, char **argv) {
  Wizard wiz(800, 550, "Wizard");
  wiz.show(argc, argv);
  return Fl::run();
}
