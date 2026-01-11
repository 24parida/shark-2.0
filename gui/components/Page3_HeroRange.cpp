#include "Page3_HeroRange.hh"
#include "../utils/RangeData.hh"
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
    std::vector<std::string> parsed = parseRangeString(input);
    if (!parsed.empty()) {
      setSelectedRange(parsed);
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
  std::vector<std::string> result;

  // Remove whitespace and split by comma
  std::string cleaned;
  for (char c : rangeStr) {
    if (!std::isspace(c)) cleaned += c;
  }

  std::stringstream ss(cleaned);
  std::string token;

  while (std::getline(ss, token, ',')) {
    if (token.empty()) continue;

    // Handle weight suffix (e.g., "AKo:0.5" -> just "AKo")
    size_t colonPos = token.find(':');
    if (colonPos != std::string::npos) {
      token = token.substr(0, colonPos);
    }

    // Handle range notation (e.g., "77+" or "ATs-A5s" or "KQo-K9o")
    if (token.find('+') != std::string::npos) {
      // Pair+ notation (e.g., "77+" means 77,88,99,TT,JJ,QQ,KK,AA)
      std::string base = token.substr(0, token.find('+'));
      if (base.length() >= 2 && base[0] == base[1]) {
        int startIdx = -1;
        for (int i = 0; i < 13; ++i) {
          if (RangeData::RANKS[i][0] == base[0]) {
            startIdx = i;
            break;
          }
        }
        if (startIdx >= 0) {
          for (int i = startIdx; i >= 0; --i) {
            result.push_back(RangeData::RANKS[i] + RangeData::RANKS[i]);
          }
        }
      }
    } else if (token.find('-') != std::string::npos) {
      // Range notation (e.g., "ATs-A5s" or "77-55")
      size_t dashPos = token.find('-');
      std::string start = token.substr(0, dashPos);
      std::string end = token.substr(dashPos + 1);

      // Check if it's suited/offsuit range
      bool isSuited = (start.back() == 's');
      bool isOffsuit = (start.back() == 'o');

      if (start.length() >= 2 && end.length() >= 2) {
        char highCard = start[0];
        int startKicker = -1, endKicker = -1;

        for (int i = 0; i < 13; ++i) {
          if (RangeData::RANKS[i][0] == start[1]) startKicker = i;
          if (RangeData::RANKS[i][0] == end[1]) endKicker = i;
        }

        if (startKicker >= 0 && endKicker >= 0) {
          int lo = std::min(startKicker, endKicker);
          int hi = std::max(startKicker, endKicker);
          for (int i = lo; i <= hi; ++i) {
            std::string hand = std::string(1, highCard) + RangeData::RANKS[i];
            if (isSuited) hand += "s";
            else if (isOffsuit) hand += "o";
            result.push_back(hand);
          }
        }
      }
    } else {
      // Single hand (e.g., "AA", "AKs", "AKo")
      result.push_back(token);
    }
  }

  return result;
}
