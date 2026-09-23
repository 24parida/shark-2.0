#include "Page3_HeroRange.hh"
#include "../utils/RangeData.hh"
#include "../utils/RangeEditor.hh"
#include "../utils/Colors.hh"
#include <FL/Fl_Grid.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <sstream>
#include <regex>

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
      btn->labelsize(14);
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
  m_btnBack->labelfont(FL_HELVETICA_BOLD);
  m_btnBack->color(Colors::ThemeButtonBg());
  m_btnBack->labelcolor(FL_WHITE);

  m_btnImport = new Fl_Button(0, 0, 0, 0, "Import Range");
  m_btnImport->labelsize(14);
  m_btnImport->labelfont(FL_HELVETICA_BOLD);
  m_btnImport->color(Colors::ThemeButtonBg());
  m_btnImport->labelcolor(FL_WHITE);
  m_btnImport->callback(cbImport, this);

  m_btnCopy = new Fl_Button(0, 0, 0, 0, "Copy Range");
  m_btnCopy->labelsize(14);
  m_btnCopy->labelfont(FL_HELVETICA_BOLD);
  m_btnCopy->color(Colors::ThemeButtonBg());
  m_btnCopy->labelcolor(FL_WHITE);
  m_btnCopy->callback(cbCopy, this);

  m_btnNext = new Fl_Button(0, 0, 0, 0, "Next");
  m_btnNext->labelsize(18);
  m_btnNext->labelfont(FL_HELVETICA_BOLD);
  m_btnNext->color(Colors::ThemeButtonBg());
  m_btnNext->labelcolor(FL_WHITE);
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
  PreflopRange parsed(RangeEditor::join(range));
  m_selectedRange = parsed.to_strings();
  for (auto *btn : m_rangeBtns) {
    const std::string hand = btn->label();
    btn->select(std::any_of(parsed.preflop_combos.begin(), parsed.preflop_combos.end(),
                            [&](const auto &combo) { return combo.hand_type() == hand; }));
  }
  if (m_onRangeChange) m_onRangeChange(m_selectedRange);
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
  PreflopRange range(RangeEditor::join(m_selectedRange));
  const std::string hand = btn->label();
  if (btn->selected()) {
    auto &combos = range.preflop_combos;
    combos.erase(std::remove_if(combos.begin(), combos.end(),
                                [&](const auto &combo) { return combo.hand_type() == hand; }), combos.end());
  } else {
    const PreflopRange added(hand);
    range.preflop_combos.insert(range.preflop_combos.end(), added.preflop_combos.begin(), added.preflop_combos.end());
  }
  setSelectedRange(range.to_strings());
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

    m_btnBack->resize(navX + 15, navY + 2, 80, 40);
    // Center the Import Range and Copy Range buttons
    int centerX = navX + navW / 2;
    m_btnImport->resize(centerX - 130, navY + 2, 120, 40);
    m_btnCopy->resize(centerX + 10, navY + 2, 120, 40);
    m_btnNext->resize(navX + navW - 95, navY + 2, 80, 40);
  }
}

void Page3_HeroRange::cbImport(Fl_Widget *w, void *data) {
  ((Page3_HeroRange *)data)->handleImport();
}

void Page3_HeroRange::cbCopy(Fl_Widget *w, void *data) {
  ((Page3_HeroRange *)data)->handleCopy();
}

void Page3_HeroRange::handleImport() {
  // Hide the question mark icon completely
  fl_message_icon()->label("");
  fl_message_icon()->box(FL_NO_BOX);
  fl_message_icon()->hide();

  const char* input = fl_input("Enter range (PIO/WASM format):", "");
  if (input && strlen(input) > 0) {
    try {
      setSelectedRange(parseRangeString(input));
    } catch (const std::exception &error) {
      fl_alert("%s", error.what());
    }
  }
}

void Page3_HeroRange::handleCopy() {
  if (m_selectedRange.empty()) return;

  // Build comma-separated range string
  std::string rangeStr;
  for (size_t i = 0; i < m_selectedRange.size(); ++i) {
    if (i > 0) rangeStr += ",";
    rangeStr += m_selectedRange[i];
  }

  // Copy to clipboard
  Fl::copy(rangeStr.c_str(), static_cast<int>(rangeStr.length()), 1);
}

std::vector<std::string> Page3_HeroRange::parseRangeString(const std::string& rangeStr) {
  return PreflopRange(rangeStr).to_strings();
}
