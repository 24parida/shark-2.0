#include "Page4_VillainRange.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <algorithm>

Page4_VillainRange::Page4_VillainRange(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  // Title
  m_lblTitle = new Fl_Box(X, Y + 20, W, 50, "Range Editor (villain)");
  m_lblTitle->labelfont(FL_BOLD);
  m_lblTitle->labelsize(28);
  m_lblTitle->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

  // Range grid (will be created in rebuildRangeGrid)
  rebuildRangeGrid(W, H);

  // Navigation buttons
  int navY = Y + H - 60;
  m_btnBack = new Fl_Button(X + 25, navY, 120, 40, "Back");
  m_btnBack->labelsize(18);

  m_btnNext = new Fl_Button(X + W - 145, navY, 120, 40, "Next");
  m_btnNext->labelsize(18);

  end();
}

void Page4_VillainRange::rebuildRangeGrid(int W, int H) {
  // Clear existing buttons
  for (auto *btn : m_rangeBtns) {
    remove(btn);
    delete btn;
  }
  m_rangeBtns.clear();

  int grid_size = 13;
  int available_width = W - 100;
  int available_height = H - 250;

  int base_button_size = std::min(
      (available_width - (grid_size * 10)) / grid_size,
      (available_height - (grid_size * 10)) / grid_size
  );

  int min_range_button = 60;
  int rbw = std::max(min_range_button, base_button_size);
  int rbh = rbw;
  int rsp = 10;

  int rangeGridX = x() + (W - (13 * (rbw + rsp) - rsp)) / 2;
  int rangeGridY = y() + 90;

  // Create 13x13 range grid
  for (int i = 0; i < 13; ++i) {
    for (int j = 0; j < 13; ++j) {
      int cx = rangeGridX + j * (rbw + rsp);
      int cy = rangeGridY + i * (rbh + rsp);

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

      auto *btn = new CardButton(cx, cy, rbw, rbh, base);
      btn->copy_label(lbl.c_str());
      btn->labelsize(20);
      btn->callback(cbRange, this);
      btn->clear_visible_focus();
      m_rangeBtns.push_back(btn);
    }
  }
}

void Page4_VillainRange::setBackCallback(Fl_Callback *cb, void *data) {
  m_btnBack->callback(cb, data);
}

void Page4_VillainRange::setNextCallback(Fl_Callback *cb, void *data) {
  m_btnNext->callback(cb, data);
}

void Page4_VillainRange::setRangeChangeCallback(std::function<void(const std::vector<std::string>&)> cb) {
  m_onRangeChange = cb;
}

std::vector<std::string> Page4_VillainRange::getSelectedRange() const {
  return m_selectedRange;
}

void Page4_VillainRange::setSelectedRange(const std::vector<std::string>& range) {
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

void Page4_VillainRange::clearSelection() {
  m_selectedRange.clear();
  for (auto *btn : m_rangeBtns) {
    btn->select(false);
  }
  if (m_onRangeChange) {
    m_onRangeChange(m_selectedRange);
  }
}

void Page4_VillainRange::cbRange(Fl_Widget *w, void *data) {
  ((Page4_VillainRange *)data)->handleRangeClick((CardButton *)w);
}

void Page4_VillainRange::handleRangeClick(CardButton *btn) {
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

void Page4_VillainRange::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  m_lblTitle->resize(X, Y + 20, W, 50);

  // Rebuild range grid with new dimensions
  rebuildRangeGrid(W, H);

  int navY = Y + H - 60;
  m_btnBack->resize(X + 25, navY, 120, 40);
  m_btnNext->resize(X + W - 145, navY, 120, 40);
}
