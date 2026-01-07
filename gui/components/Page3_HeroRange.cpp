#include "Page3_HeroRange.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <FL/Fl_Grid.H>
#include <algorithm>

Page3_HeroRange::Page3_HeroRange(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  // Main container grid: 3 rows (title, range grid, nav)
  auto *mainGrid = new Fl_Grid(X, Y, W, H);
  mainGrid->layout(3, 1, 5, 5);

  // Row 0: Title
  m_lblTitle = new Fl_Box(0, 0, 0, 0, "Range Editor (you)");
  m_lblTitle->labelfont(FL_BOLD);
  m_lblTitle->labelsize(28);
  m_lblTitle->align(FL_ALIGN_CENTER);
  mainGrid->widget(m_lblTitle, 0, 0);
  mainGrid->row_height(0, 60);

  // Row 1: Range grid (13×13)
  m_rangeGrid = new Fl_Grid(0, 0, 0, 0);
  m_rangeGrid->layout(13, 13, 3, 3);  // 13×13, 3px spacing

  // Create 169 hand buttons
  for (int i = 0; i < 13; ++i) {
    for (int j = 0; j < 13; ++j) {
      std::string lbl;
      Fl_Color base;

      if (i == j) {
        // Pairs (diagonal)
        lbl = RangeData::RANKS[i] + RangeData::RANKS[j];
        base = Colors::PairSelected();
      } else if (j > i) {
        // Suited (upper triangle)
        lbl = RangeData::RANKS[i] + RangeData::RANKS[j] + "s";
        base = Colors::SuitedSelected();
      } else {
        // Offsuit (lower triangle)
        lbl = RangeData::RANKS[j] + RangeData::RANKS[i] + "o";
        base = Colors::DefaultCell();
      }

      auto *btn = new CardButton(0, 0, 0, 0, base);
      btn->copy_label(lbl.c_str());
      btn->labelsize(12);
      btn->callback(cbRange, this);
      btn->clear_visible_focus();

      m_rangeGrid->widget(btn, i, j);
      m_rangeBtns.push_back(btn);
    }
  }

  // Equal weights for all rows/cols
  for (int i = 0; i < 13; ++i) {
    m_rangeGrid->row_weight(i, 1);
    m_rangeGrid->col_weight(i, 1);
  }

  m_rangeGrid->end();
  mainGrid->widget(m_rangeGrid, 1, 0);
  mainGrid->row_height(1, H - 120);  // Leave space for title and nav

  // Row 2: Navigation
  auto *navRow = new Fl_Group(0, 0, 0, 0);
  navRow->begin();
  m_btnBack = new Fl_Button(0, 0, 0, 0, "Back");
  m_btnBack->labelsize(18);

  m_btnNext = new Fl_Button(0, 0, 0, 0, "Next");
  m_btnNext->labelsize(18);
  navRow->end();

  mainGrid->widget(navRow, 2, 0);
  mainGrid->row_height(2, 50);

  mainGrid->end();
  end();

  // Force initial layout
  resize(X, Y, W, H);
}

void Page3_HeroRange::setBackCallback(Fl_Callback *cb, void *data) {
  m_btnBack->callback(cb, data);
}

void Page3_HeroRange::setNextCallback(Fl_Callback *cb, void *data) {
  m_btnNext->callback(cb, data);
}

void Page3_HeroRange::setRangeChangeCallback(std::function<void(const std::vector<std::string>&)> cb) {
  m_onRangeChange = cb;
}

void Page3_HeroRange::setSelectedRange(const std::vector<std::string>& range) {
  m_selectedRange = range;

  // Update button selection states
  for (auto *btn : m_rangeBtns) {
    std::string hand = btn->label();
    bool selected = std::find(range.begin(), range.end(), hand) != range.end();
    btn->select(selected);
  }

  if (m_onRangeChange) {
    m_onRangeChange(m_selectedRange);
  }
}

void Page3_HeroRange::clearSelection() {
  m_selectedRange.clear();
  for (auto *btn : m_rangeBtns) {
    btn->select(false);
  }
  if (m_onRangeChange) {
    m_onRangeChange(m_selectedRange);
  }
}

void Page3_HeroRange::cbRange(Fl_Widget *w, void *data) {
  ((Page3_HeroRange *)data)->handleRangeClick((CardButton *)w);
}

void Page3_HeroRange::handleRangeClick(CardButton *btn) {
  std::string hand = btn->label();

  auto it = std::find(m_selectedRange.begin(), m_selectedRange.end(), hand);
  if (it != m_selectedRange.end()) {
    // Deselect
    m_selectedRange.erase(it);
    btn->select(false);
  } else {
    // Select
    m_selectedRange.push_back(hand);
    btn->select(true);
  }

  if (m_onRangeChange) {
    m_onRangeChange(m_selectedRange);
  }
}

void Page3_HeroRange::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  // Update range grid height based on available space
  if (children() > 0) {
    auto *mainGrid = dynamic_cast<Fl_Grid*>(child(0));
    if (!mainGrid || mainGrid->children() < 3) return;

    // Adjust row 1 (range grid) height based on window size
    int rangeGridHeight = H - 120;  // Leave space for title (60) + nav (50) + margins (10)
    mainGrid->row_height(1, rangeGridHeight);
    mainGrid->resize(X, Y, W, H);

    auto *navRow = mainGrid->child(2);
    int navX = navRow->x();
    int navY = navRow->y();
    int navW = navRow->w();

    m_btnBack->resize(navX + 25, navY + 2, 120, 40);
    m_btnNext->resize(navX + navW - 145, navY + 2, 120, 40);
  }
}
