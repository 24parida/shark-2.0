#include "Page6_Strategy.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <FL/Fl.H>

Page6_Strategy::Page6_Strategy(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  box(FL_FLAT_BOX);
  color(Colors::LightBg());

  // Title area
  int titleY = Y + 15;
  int titleH = 30;

  m_lblStrategy = new Fl_Box(X, titleY, W, titleH);
  m_lblStrategy->labelsize(28);
  m_lblStrategy->labelfont(FL_HELVETICA_BOLD);
  m_lblStrategy->align(FL_ALIGN_CENTER);

  // Board info
  int boardY = titleY + titleH + 10;
  int boardH = 25;
  m_boardInfo = new Fl_Box(X, boardY, W, boardH);
  m_boardInfo->labelsize(16);
  m_boardInfo->labelfont(FL_HELVETICA);
  m_boardInfo->align(FL_ALIGN_CENTER);

  // Pot info
  int potY = boardY + boardH + 5;
  int potH = 25;
  m_potInfo = new Fl_Box(X, potY, W, potH);
  m_potInfo->labelsize(14);
  m_potInfo->labelfont(FL_HELVETICA);
  m_potInfo->align(FL_ALIGN_CENTER);

  int headerHeight = potY + potH + 15;

  // Strategy grid will be built in rebuildStrategyGrid
  rebuildStrategyGrid(W, H);

  // Info panel on right side
  int infoX = X + (W * 2) / 3 + 10;
  int infoY = Y + headerHeight;
  int infoW = (W / 3) - 20;
  int infoH = H - (infoY - Y) - 50;

  // Info title
  int infoTitleH = 30;
  m_infoTitle = new Fl_Box(infoX, infoY, infoW - 80, infoTitleH, "Hand Analysis");
  m_infoTitle->labelsize(24);
  m_infoTitle->labelfont(FL_HELVETICA_BOLD);
  m_infoTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

  // Zoom buttons
  int zoomButtonSize = 30;
  int zoomX = infoX + infoW - 2 * (zoomButtonSize + 5);

  m_zoomOutBtn = new Fl_Button(zoomX, infoY, zoomButtonSize, zoomButtonSize, "-");
  m_zoomOutBtn->labelsize(18);
  m_zoomOutBtn->labelfont(FL_HELVETICA_BOLD);
  m_zoomOutBtn->color(Colors::ButtonBg());
  m_zoomOutBtn->callback(cbZoomOut, this);

  m_zoomInBtn = new Fl_Button(zoomX + zoomButtonSize + 5, infoY, zoomButtonSize, zoomButtonSize, "+");
  m_zoomInBtn->labelsize(18);
  m_zoomInBtn->labelfont(FL_HELVETICA_BOLD);
  m_zoomInBtn->color(Colors::ButtonBg());
  m_zoomInBtn->callback(cbZoomIn, this);

  // Info text display
  m_infoBuffer = new Fl_Text_Buffer();
  m_infoDisplay = new Fl_Text_Display(infoX, infoY + infoTitleH + 5, infoW, infoH - infoTitleH - 5);
  m_infoDisplay->buffer(m_infoBuffer);
  m_infoDisplay->textsize(14);
  m_infoDisplay->color(Colors::InfoBg());
  m_infoDisplay->selection_color(Colors::InfoSelBg());
  m_infoDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

  // Back and Undo buttons (will be positioned at bottom)
  m_backBtn = new Fl_Button(X + 25, Y + H - 55, 120, 40, "Back");
  m_backBtn->labelsize(18);
  m_backBtn->color(Colors::InfoSelBg());
  m_backBtn->callback(cbBack, this);

  m_undoBtn = new Fl_Button(X + 155, Y + H - 55, 120, 40, "Undo");
  m_undoBtn->labelsize(18);
  m_undoBtn->color(Colors::InfoSelBg());
  m_undoBtn->callback(cbUndo, this);

  // Card selector (initially hidden)
  m_cardChoice = new Fl_Choice(X + 285, Y + H - 55, 120, 40);
  m_cardChoice->color(Colors::LightBg());

  m_rankChoice = new Fl_Choice(X + 415, Y + H - 55, 80, 40);
  m_rankChoice->color(Colors::LightBg());

  m_suitChoice = new Fl_Choice(X + 505, Y + H - 55, 80, 40);
  m_suitChoice->color(Colors::LightBg());

  end();
}

Page6_Strategy::~Page6_Strategy() {
  delete m_infoBuffer;
}

void Page6_Strategy::rebuildStrategyGrid(int W, int H) {
  // Clear existing buttons
  for (auto *btn : m_strategyBtns) {
    remove(btn);
    delete btn;
  }
  m_strategyBtns.clear();

  int headerHeight = 120;
  int gridX = x() + 20;
  int gridY = y() + headerHeight;
  int gridW = (W * 2) / 3 - 40;
  int bottomButtonHeight = 60;
  int rangeGridBottomMargin = 20;
  int gridH = H - (gridY - y()) - bottomButtonHeight - rangeGridBottomMargin;

  int cellSize = std::min(gridW / 13, gridH / 13);
  cellSize = static_cast<int>(cellSize * 0.925);
  int cellPadding = 3;

  gridX = x() + (((W * 2) / 3) - (13 * (cellSize + cellPadding))) / 2;

  // Create 13x13 strategy grid
  for (int r = 0; r < 13; r++) {
    for (int c = 0; c < 13; c++) {
      int cx = gridX + c * (cellSize + cellPadding);
      int cy = gridY + r * (cellSize + cellPadding);

      auto *btn = new CardButton(cx, cy, cellSize, cellSize, Colors::UncoloredCard());
      btn->box(FL_ROUND_DOWN_BOX);
      btn->labelsize(14);
      btn->labelfont(FL_HELVETICA_BOLD);
      btn->callback(cbStrategy, this);
      m_strategyBtns.push_back(btn);
    }
  }
}

void Page6_Strategy::setActionCallback(std::function<void(const std::string&)> cb) {
  m_onAction = cb;
}

void Page6_Strategy::setBackCallback(std::function<void()> cb) {
  m_onBack = cb;
}

void Page6_Strategy::setUndoCallback(std::function<void()> cb) {
  m_onUndo = cb;
}

void Page6_Strategy::setTitle(const std::string& title) {
  m_lblStrategy->copy_label(title.c_str());
  redraw();
}

void Page6_Strategy::setBoardInfo(const std::string& board) {
  m_boardInfo->copy_label(board.c_str());
  redraw();
}

void Page6_Strategy::setPotInfo(const std::string& pot) {
  m_potInfo->copy_label(pot.c_str());
  redraw();
}

void Page6_Strategy::setInfoText(const std::string& text) {
  m_infoBuffer->text(text.c_str());
  redraw();
}

void Page6_Strategy::setActions(const std::vector<std::string>& actions) {
  // Clear existing action buttons
  for (auto *btn : m_actionBtns) {
    remove(btn);
    delete btn;
  }
  m_actionBtns.clear();

  // Create new action buttons (positioned at bottom)
  int btnWidth = 100;
  int btnHeight = 40;
  int spacing = 10;
  int startX = x() + 600;
  int startY = y() + h() - 55;

  for (size_t i = 0; i < actions.size(); ++i) {
    auto *btn = new Fl_Button(startX + i * (btnWidth + spacing), startY, btnWidth, btnHeight);
    btn->copy_label(actions[i].c_str());
    btn->labelsize(16);
    btn->color(Colors::ButtonBg());
    btn->callback(cbAction, this);
    m_actionBtns.push_back(btn);
  }

  redraw();
}

void Page6_Strategy::updateStrategyGrid(const std::map<std::string, std::map<std::string, float>>& strategies) {
  // Update each button with strategy colors
  for (auto *btn : m_strategyBtns) {
    std::string hand = btn->label();
    if (hand.empty()) continue;

    auto it = strategies.find(hand);
    if (it != strategies.end()) {
      const auto& actionProbs = it->second;

      std::vector<std::pair<Fl_Color, float>> colors;
      for (const auto& [action, prob] : actionProbs) {
        // Assign colors based on action type
        Fl_Color color;
        if (action.find("fold") != std::string::npos || action.find("Fold") != std::string::npos) {
          color = Colors::FoldColor();
        } else if (action.find("call") != std::string::npos || action.find("Check") != std::string::npos) {
          color = Colors::CallColor();
        } else {
          color = Colors::BetColor(0, 1);  // Default bet color
        }
        colors.emplace_back(color, prob);
      }

      btn->setStrategyColors(colors);
    }
  }
  redraw();
}

void Page6_Strategy::selectHand(const std::string& hand) {
  for (auto *btn : m_strategyBtns) {
    btn->setStrategySelected(btn->label() == hand);
  }
  redraw();
}

void Page6_Strategy::zoomIn() {
  m_infoTextScale = std::min(2.0f, m_infoTextScale + 0.1f);
  updateInfoTextSize();
}

void Page6_Strategy::zoomOut() {
  m_infoTextScale = std::max(0.5f, m_infoTextScale - 0.1f);
  updateInfoTextSize();
}

void Page6_Strategy::updateInfoTextSize() {
  int newSize = static_cast<int>(14 * m_infoTextScale);
  m_infoDisplay->textsize(newSize);
  m_infoDisplay->redraw();
}

void Page6_Strategy::cbStrategy(Fl_Widget *w, void *data) {
  ((Page6_Strategy *)data)->handleStrategyClick((CardButton *)w);
}

void Page6_Strategy::cbAction(Fl_Widget *w, void *data) {
  ((Page6_Strategy *)data)->handleActionClick((Fl_Button *)w);
}

void Page6_Strategy::cbZoomIn(Fl_Widget *w, void *data) {
  ((Page6_Strategy *)data)->zoomIn();
}

void Page6_Strategy::cbZoomOut(Fl_Widget *w, void *data) {
  ((Page6_Strategy *)data)->zoomOut();
}

void Page6_Strategy::cbBack(Fl_Widget *w, void *data) {
  auto *page = (Page6_Strategy *)data;
  if (page->m_onBack) {
    page->m_onBack();
  }
}

void Page6_Strategy::cbUndo(Fl_Widget *w, void *data) {
  auto *page = (Page6_Strategy *)data;
  if (page->m_onUndo) {
    page->m_onUndo();
  }
}

void Page6_Strategy::handleStrategyClick(CardButton *btn) {
  std::string hand = btn->label();
  selectHand(hand);
  // Could trigger info update here if needed
}

void Page6_Strategy::handleActionClick(Fl_Button *btn) {
  if (m_onAction) {
    m_onAction(btn->label());
  }
}

void Page6_Strategy::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  // Reposition title elements
  int titleY = Y + 15;
  m_lblStrategy->resize(X, titleY, W, 30);
  m_boardInfo->resize(X, titleY + 40, W, 25);
  m_potInfo->resize(X, titleY + 70, W, 25);

  int headerHeight = 120;

  // Rebuild strategy grid
  rebuildStrategyGrid(W, H);

  // Reposition info panel
  int infoX = X + (W * 2) / 3 + 10;
  int infoY = Y + headerHeight;
  int infoW = (W / 3) - 20;
  int infoH = H - (infoY - Y) - 50;

  m_infoTitle->resize(infoX, infoY, infoW - 80, 30);

  int zoomButtonSize = 30;
  int zoomX = infoX + infoW - 2 * (zoomButtonSize + 5);
  m_zoomOutBtn->resize(zoomX, infoY, zoomButtonSize, zoomButtonSize);
  m_zoomInBtn->resize(zoomX + zoomButtonSize + 5, infoY, zoomButtonSize, zoomButtonSize);

  m_infoDisplay->resize(infoX, infoY + 35, infoW, infoH - 35);

  // Reposition bottom buttons
  m_backBtn->resize(X + 25, Y + H - 55, 120, 40);
  m_undoBtn->resize(X + 155, Y + H - 55, 120, 40);
  m_cardChoice->resize(X + 285, Y + H - 55, 120, 40);
  m_rankChoice->resize(X + 415, Y + H - 55, 80, 40);
  m_suitChoice->resize(X + 505, Y + H - 55, 80, 40);

  // Reposition action buttons
  int btnWidth = 100;
  int btnHeight = 40;
  int spacing = 10;
  int startX = X + 600;
  int startY = Y + H - 55;
  for (size_t i = 0; i < m_actionBtns.size(); ++i) {
    m_actionBtns[i]->resize(startX + i * (btnWidth + spacing), startY, btnWidth, btnHeight);
  }
}
