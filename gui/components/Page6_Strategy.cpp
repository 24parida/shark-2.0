#include "Page6_Strategy.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <FL/Fl.H>

Page6_Strategy::Page6_Strategy(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  box(FL_FLAT_BOX);
  color(Colors::LightBg());

  // Create main 5-row grid
  m_mainGrid = new Fl_Grid(X, Y, W, H);
  m_mainGrid->layout(5, 1, 5, 5);  // 5 rows, 1 column

  // Row 0: Header (fixed 120px)
  auto *headerSection = createHeaderSection();
  m_mainGrid->widget(headerSection, 0, 0);
  m_mainGrid->row_height(0, 120);
  m_mainGrid->row_weight(0, 0);

  // Row 1: Content (expands by default)
  auto *contentSection = createContentSection();
  m_mainGrid->widget(contentSection, 1, 0);
  m_mainGrid->row_weight(1, 1);  // Expands to fill space

  // Row 2: Card selection (hidden by default)
  m_cardSelectionRow = createCardSelectionRow();
  m_cardSelectionRow->hide();
  m_mainGrid->widget(m_cardSelectionRow, 2, 0);
  m_mainGrid->row_height(2, 0);
  m_mainGrid->row_weight(2, 0);

  // Row 3: Action buttons (fixed 60px)
  m_actionButtonsGroup = createActionRow();
  m_mainGrid->widget(m_actionButtonsGroup, 3, 0);
  m_mainGrid->row_height(3, 60);
  m_mainGrid->row_weight(3, 0);

  // Row 4: Navigation (fixed 60px)
  auto *navRow = createNavigationRow();
  m_mainGrid->widget(navRow, 4, 0);
  m_mainGrid->row_height(4, 60);
  m_mainGrid->row_weight(4, 0);

  m_mainGrid->end();
  end();

  // Force initial layout
  resize(X, Y, W, H);
}

Page6_Strategy::~Page6_Strategy() {
  delete m_infoBuffer;
}

std::string Page6_Strategy::getHandLabel(int row, int col) {
  if (row == col) {
    // Pairs (diagonal): AA, KK, QQ, etc.
    return RangeData::RANKS[row] + RangeData::RANKS[col];
  } else if (col > row) {
    // Suited (upper triangle): AKs, AQs, etc.
    return RangeData::RANKS[row] + RangeData::RANKS[col] + "s";
  } else {
    // Offsuit (lower triangle): AKo, AQo, etc.
    return RangeData::RANKS[col] + RangeData::RANKS[row] + "o";
  }
}

Fl_Group* Page6_Strategy::createHeaderSection() {
  auto *headerGrid = new Fl_Grid(0, 0, 0, 0);
  headerGrid->layout(3, 1, 5, 5);  // 3 rows: title, board, pot

  m_lblStrategy = new Fl_Box(0, 0, 0, 0);
  m_lblStrategy->labelsize(28);
  m_lblStrategy->labelfont(FL_HELVETICA_BOLD);
  m_lblStrategy->align(FL_ALIGN_CENTER);
  headerGrid->widget(m_lblStrategy, 0, 0);
  headerGrid->row_height(0, 40);

  m_boardInfo = new Fl_Box(0, 0, 0, 0);
  m_boardInfo->labelsize(16);
  m_boardInfo->align(FL_ALIGN_CENTER);
  headerGrid->widget(m_boardInfo, 1, 0);
  headerGrid->row_height(1, 30);

  m_potInfo = new Fl_Box(0, 0, 0, 0);
  m_potInfo->labelsize(14);
  m_potInfo->align(FL_ALIGN_CENTER);
  headerGrid->widget(m_potInfo, 2, 0);
  headerGrid->row_height(2, 30);

  headerGrid->end();
  return headerGrid;
}

Fl_Group* Page6_Strategy::createContentSection() {
  auto *contentGrid = new Fl_Grid(0, 0, 0, 0);
  contentGrid->layout(1, 2, 10, 10);  // 1 row, 2 cols (strategy + analysis)

  // LEFT: Strategy grid (13×13)
  m_strategyGrid = new Fl_Grid(0, 0, 0, 0);
  m_strategyGrid->layout(13, 13, 1, 1);  // 13×13, minimal 1px spacing

  // Create 169 buttons ONCE
  for (int r = 0; r < 13; ++r) {
    for (int c = 0; c < 13; ++c) {
      std::string hand = getHandLabel(r, c);
      auto *btn = new CardButton(0, 0, 0, 0, Colors::UncoloredCard());
      btn->box(FL_FLAT_BOX);
      btn->copy_label(hand.c_str());
      btn->labelsize(14);
      btn->labelfont(FL_HELVETICA_BOLD);
      btn->callback(cbStrategy, this);
      btn->clear_visible_focus();

      m_strategyGrid->widget(btn, r, c);
      m_strategyBtns.push_back(btn);
    }
  }

  // Equal weights for proportional resizing
  for (int i = 0; i < 13; ++i) {
    m_strategyGrid->row_weight(i, 1);
    m_strategyGrid->col_weight(i, 1);
  }
  m_strategyGrid->end();

  // RIGHT: Analysis panel
  m_analysisPanel = createAnalysisPanel();

  contentGrid->widget(m_strategyGrid, 0, 0);
  contentGrid->widget(m_analysisPanel, 0, 1);
  contentGrid->col_weight(0, 66);  // Strategy: 2/3 width
  contentGrid->col_weight(1, 34);  // Analysis: 1/3 width
  contentGrid->row_weight(0, 1);   // Allow row to expand vertically

  contentGrid->end();
  return contentGrid;
}

Fl_Group* Page6_Strategy::createAnalysisPanel() {
  // Use simple Fl_Group with manual positioning (not nested Fl_Grid)
  auto *panel = new Fl_Group(0, 0, 0, 0);
  panel->box(FL_DOWN_BOX);
  panel->color(Colors::PanelBg());
  panel->begin();

  // Header: Title centered
  m_infoTitle = new Fl_Box(0, 0, 100, 35, "Hand Analysis");
  m_infoTitle->labelsize(16);
  m_infoTitle->labelfont(FL_HELVETICA_BOLD);
  m_infoTitle->labelcolor(Colors::PrimaryText());
  m_infoTitle->align(FL_ALIGN_CENTER);

  // Keep pointers null (removed zoom buttons)
  m_zoomOutBtn = nullptr;
  m_zoomInBtn = nullptr;

  // Text display (kept for backwards compatibility, hidden by default)
  m_infoBuffer = new Fl_Text_Buffer();
  m_infoDisplay = new Fl_Text_Display(0, 0, 100, 100);
  m_infoDisplay->buffer(m_infoBuffer);
  m_infoDisplay->textsize(14);
  m_infoDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
  m_infoDisplay->color(Colors::InputBg());
  m_infoDisplay->box(FL_DOWN_BOX);
  m_infoDisplay->textcolor(Colors::PrimaryText());
  m_infoDisplay->hide();

  // Visual combo strategy display
  m_comboDisplay = new ComboStrategyDisplay(0, 0, 100, 100);

  panel->end();
  return panel;
}

Fl_Group* Page6_Strategy::createCardSelectionRow() {
  // Card selection: 3 rows - label, dropdowns, spacer (absorbs extra space)
  auto *grid = new Fl_Grid(0, 0, 0, 0);
  grid->layout(3, 3, 20, 15);  // 3 rows, 3 cols
  grid->box(FL_FLAT_BOX);
  grid->color(Colors::SecondaryBg());

  // Row 0: Label centered
  auto *spacer1 = new Fl_Box(0, 0, 0, 0);
  grid->widget(spacer1, 0, 0);
  grid->col_weight(0, 1);

  m_cardSelLabel = new Fl_Box(0, 0, 0, 0, "Choose a card:");
  m_cardSelLabel->labelsize(18);
  m_cardSelLabel->labelfont(FL_HELVETICA_BOLD);
  m_cardSelLabel->align(FL_ALIGN_CENTER);
  grid->widget(m_cardSelLabel, 0, 1);
  grid->col_width(1, 400);  // Wide enough for dropdowns + gap
  grid->col_weight(1, 0);

  auto *spacer2 = new Fl_Box(0, 0, 0, 0);
  grid->widget(spacer2, 0, 2);
  grid->col_weight(2, 1);

  // Row 1: Dropdowns side by side, centered
  auto *spacer3 = new Fl_Box(0, 0, 0, 0);
  grid->widget(spacer3, 1, 0);

  // Inner grid for the two dropdowns - fixed height
  auto *dropdownGrid = new Fl_Grid(0, 0, 0, 0);
  dropdownGrid->layout(1, 2, 10, 40);  // 40px gap between dropdowns

  m_rankChoice = new Fl_Choice(0, 0, 0, 0, "Rank:");
  m_rankChoice->labelsize(14);
  m_rankChoice->textsize(16);
  m_rankChoice->callback(cbCardSelected, this);
  dropdownGrid->widget(m_rankChoice, 0, 0);
  dropdownGrid->col_width(0, 170);
  dropdownGrid->col_weight(0, 0);

  m_suitChoice = new Fl_Choice(0, 0, 0, 0, "Suit:");
  m_suitChoice->labelsize(14);
  m_suitChoice->textsize(16);
  m_suitChoice->callback(cbCardSelected, this);
  dropdownGrid->widget(m_suitChoice, 0, 1);
  dropdownGrid->col_width(1, 170);
  dropdownGrid->col_weight(1, 0);

  // Fix dropdown row height so it doesn't expand
  dropdownGrid->row_height(0, 35);
  dropdownGrid->row_weight(0, 0);

  dropdownGrid->end();
  grid->widget(dropdownGrid, 1, 1);

  auto *spacer4 = new Fl_Box(0, 0, 0, 0);
  grid->widget(spacer4, 1, 2);

  // Row 2: Spacer row to absorb extra vertical space
  auto *bottomSpacer1 = new Fl_Box(0, 0, 0, 0);
  grid->widget(bottomSpacer1, 2, 0);
  auto *bottomSpacer2 = new Fl_Box(0, 0, 0, 0);
  grid->widget(bottomSpacer2, 2, 1);
  auto *bottomSpacer3 = new Fl_Box(0, 0, 0, 0);
  grid->widget(bottomSpacer3, 2, 2);

  // Row heights: label fixed, dropdowns fixed, spacer expands
  grid->row_height(0, 40);
  grid->row_weight(0, 0);
  grid->row_height(1, 50);
  grid->row_weight(1, 0);
  grid->row_weight(2, 1);  // Spacer absorbs extra space

  grid->end();
  return grid;
}

Fl_Group* Page6_Strategy::createActionRow() {
  auto *row = new Fl_Group(0, 0, 0, 0);
  row->begin();
  // Action buttons will be added dynamically via setActions()
  row->end();
  return row;
}

Fl_Group* Page6_Strategy::createNavigationRow() {
  auto *grid = new Fl_Grid(0, 0, 0, 0);
  grid->layout(1, 4, 10, 10);  // 1 row, 4 cols: back, undo, spacer, copy range

  m_backBtn = new Fl_Button(0, 0, 0, 0, "Back");
  m_backBtn->labelsize(16);
  m_backBtn->labelfont(FL_HELVETICA_BOLD);
  m_backBtn->color(Colors::ThemeButtonBg());
  m_backBtn->labelcolor(FL_WHITE);
  m_backBtn->callback(cbBack, this);
  grid->widget(m_backBtn, 0, 0);
  grid->col_width(0, 120);
  grid->col_weight(0, 0);

  m_undoBtn = new Fl_Button(0, 0, 0, 0, "Undo");
  m_undoBtn->labelsize(16);
  m_undoBtn->labelfont(FL_HELVETICA_BOLD);
  m_undoBtn->color(Colors::ThemeButtonBg());
  m_undoBtn->labelcolor(FL_WHITE);
  m_undoBtn->callback(cbUndo, this);
  grid->widget(m_undoBtn, 0, 1);
  grid->col_width(1, 120);
  grid->col_weight(1, 0);

  // Spacer takes remaining space
  auto *spacer = new Fl_Box(0, 0, 0, 0);
  grid->widget(spacer, 0, 2);
  grid->col_weight(2, 1);

  // Copy Range button on the right
  m_copyRangeBtn = new Fl_Button(0, 0, 0, 0, "Copy Range");
  m_copyRangeBtn->labelsize(14);
  m_copyRangeBtn->labelfont(FL_HELVETICA_BOLD);
  m_copyRangeBtn->color(Colors::ThemeButtonBg());
  m_copyRangeBtn->labelcolor(FL_WHITE);
  m_copyRangeBtn->callback(cbCopyRange, this);
  grid->widget(m_copyRangeBtn, 0, 3);
  grid->col_width(3, 120);
  grid->col_weight(3, 0);

  // Fixed row height so buttons don't expand vertically
  grid->row_height(0, 40);
  grid->row_weight(0, 0);

  grid->end();
  return grid;
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

void Page6_Strategy::setHandSelectCallback(std::function<void(const std::string&)> cb) {
  m_onHandSelect = cb;
}

void Page6_Strategy::setCardSelectedCallback(std::function<void(const std::string&)> cb) {
  m_onCardSelected = cb;
}

void Page6_Strategy::setCopyRangeCallback(std::function<std::string()> cb) {
  m_onCopyRange = cb;
}

void Page6_Strategy::setShowOverallStrategyCallback(std::function<void()> cb) {
  m_onShowOverallStrategy = cb;
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
  // Show text display, hide combo display
  m_infoDisplay->show();
  m_comboDisplay->hide();
  // Don't call redraw() on entire page - widgets handle their own redraw
}

void Page6_Strategy::setComboStrategies(const std::string& handName, const std::vector<ComboStrategyDisplay::ComboStrategy>& combos) {
  m_comboDisplay->setHandName(handName);
  m_comboDisplay->setCombos(combos);
  // Show combo display, hide text display
  m_comboDisplay->show();
  m_infoDisplay->hide();
  // Don't call redraw() on entire page - combo display handles its own redraw
}

void Page6_Strategy::setOverallStrategy(const std::vector<ComboStrategyDisplay::ComboStrategy>& overall) {
  m_comboDisplay->setHandName("Overall Range Strategy");
  m_comboDisplay->setCombos(overall);
  m_comboDisplay->show();
  m_infoDisplay->hide();
  // Don't call redraw() on entire page - combo display handles its own redraw
}

void Page6_Strategy::deselectHand() {
  // Deselect all strategy buttons
  for (auto *btn : m_strategyBtns) {
    btn->setStrategySelected(false);
  }
  // Show overall strategy
  if (m_onShowOverallStrategy) {
    m_onShowOverallStrategy();
  }
}

void Page6_Strategy::setActions(const std::vector<std::string>& actions) {
  // Clear existing buttons
  for (auto *btn : m_actionBtns) {
    m_actionButtonsGroup->remove(btn);
    delete btn;
  }
  m_actionBtns.clear();

  // Clear existing children
  if (m_actionButtonsGroup->children() > 0) {
    m_actionButtonsGroup->clear();
  }

  if (actions.empty()) return;

  // Create horizontal grid for buttons - use 0,0,0,0 and let resizable handle sizing
  auto *btnGrid = new Fl_Grid(0, 0, 0, 0);
  btnGrid->layout(1, actions.size(), 10, 10);

  for (size_t i = 0; i < actions.size(); ++i) {
    auto *btn = new Fl_Button(0, 0, 0, 0);
    btn->copy_label(actions[i].c_str());
    btn->labelsize(16);
    btn->labelfont(FL_HELVETICA_BOLD);
    btn->color(Colors::ThemeButtonBg());
    btn->labelcolor(FL_WHITE);
    btn->callback(cbAction, this);
    btnGrid->widget(btn, 0, i);
    btnGrid->col_weight(i, 1);  // Equal width
    m_actionBtns.push_back(btn);
  }

  btnGrid->end();
  m_actionButtonsGroup->add(btnGrid);
  m_actionButtonsGroup->resizable(btnGrid);  // Make grid fill the group

  // Get action row dimensions and resize grid to fit
  int rowX = m_actionButtonsGroup->x();
  int rowY = m_actionButtonsGroup->y();
  int rowW = m_actionButtonsGroup->w();
  int rowH = m_actionButtonsGroup->h();

  // Only resize if parent has valid dimensions
  if (rowW > 20 && rowH > 10) {
    btnGrid->resize(rowX + 10, rowY + 5, rowW - 20, rowH - 10);
  }

  m_actionButtonsGroup->redraw();
}

void Page6_Strategy::updateStrategyGrid(const std::map<std::string, std::map<std::string, float>>& strategies) {
  // Update each button with strategy colors
  for (auto *btn : m_strategyBtns) {
    std::string hand = btn->label();
    if (hand.empty()) continue;

    auto it = strategies.find(hand);
    if (it != strategies.end()) {
      // Hand found - set strategy colors
      const auto& actionProbs = it->second;

      // First pass: count bet/raise actions to assign indices
      int betIndex = 0;
      std::vector<std::pair<Fl_Color, float>> colors;

      for (const auto& [action, prob] : actionProbs) {
        // Assign colors based on action type
        Fl_Color color;
        if (action.find("Fold") != std::string::npos || action.find("fold") != std::string::npos) {
          color = Colors::FoldColor();
        } else if (action.find("Check") != std::string::npos || action.find("check") != std::string::npos) {
          color = Colors::CheckColor();
        } else if (action.find("Call") != std::string::npos || action.find("call") != std::string::npos) {
          color = Colors::CallColor();
        } else {
          // Bet or Raise - use indexed color
          color = Colors::BetColor(betIndex);
          betIndex++;
        }
        colors.emplace_back(color, prob);
      }

      btn->setStrategyColors(colors);
    } else {
      // Hand not found - clear strategy colors to show gray/uncolored button
      btn->setStrategyColors({});
    }
  }
  redraw();
}

void Page6_Strategy::selectHand(const std::string& hand) {
  // Only update buttons that need to change - CardButton::setStrategySelected
  // already handles its own redraw when state changes
  for (auto *btn : m_strategyBtns) {
    btn->setStrategySelected(btn->label() == hand);
  }
  // Don't call redraw() here - buttons redraw themselves when selection changes
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

void Page6_Strategy::showCardSelection(bool show) {
  if (m_mainGrid && m_cardSelectionRow) {
    bool currentlyShown = (m_cardSelectionRow->visible() != 0);
    if (show != currentlyShown) {
      int H = h();

      if (show) {
        // Chance node: hide content, card selection expands to fill space
        m_mainGrid->row_height(1, 0);
        m_mainGrid->row_weight(1, 0);  // Content doesn't expand
        m_mainGrid->row_weight(2, 1);  // Card selection EXPANDS to push nav to bottom
        m_cardSelectionRow->show();
      } else {
        // Action node: content expands, hide card selection
        m_mainGrid->row_height(2, 0);
        m_mainGrid->row_weight(2, 0);  // Card selection doesn't expand
        m_mainGrid->row_weight(1, 1);  // Content EXPANDS
        m_cardSelectionRow->hide();
      }

      m_mainGrid->resize(x(), y(), w(), h());
      redraw();
    }
  }
}

void Page6_Strategy::showStrategyGrid(bool show) {
  if (m_strategyGrid) {
    bool currentlyVisible = (m_strategyGrid->visible() != 0);
    if (show != currentlyVisible) {
      if (show) {
        m_strategyGrid->show();
      } else {
        m_strategyGrid->hide();
      }
      // Widget handles its own redraw on show/hide
    }
  }
}

void Page6_Strategy::showAnalysisPanel(bool show) {
  if (m_analysisPanel) {
    bool currentlyVisible = (m_analysisPanel->visible() != 0);
    if (show != currentlyVisible) {
      if (show) {
        m_analysisPanel->show();
      } else {
        m_analysisPanel->hide();
      }
      // Widget handles its own redraw on show/hide
    }
  }
}

void Page6_Strategy::populateCardChoices(const std::vector<std::string>& availableCards) {
  m_rankChoice->clear();
  m_suitChoice->clear();

  std::set<char> ranks, suits;
  for (const auto& card : availableCards) {
    if (card.length() >= 2) {
      ranks.insert(card[0]);
      suits.insert(card[1]);
    }
  }

  for (char r : ranks) {
    m_rankChoice->add(std::string(1, r).c_str());
  }
  for (char s : suits) {
    m_suitChoice->add(std::string(1, s).c_str());
  }
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

void Page6_Strategy::cbCardSelected(Fl_Widget *w, void *data) {
  auto *page = (Page6_Strategy*)data;

  // Check if both rank and suit are selected
  if (page->m_rankChoice->value() < 0 || page->m_suitChoice->value() < 0) {
    return;  // Not both selected yet
  }

  // Get selected rank and suit
  const char* rank = page->m_rankChoice->text();
  const char* suit = page->m_suitChoice->text();

  if (!rank || !suit) return;

  // Construct card string (e.g., "Ah", "Kd")
  std::string cardStr = std::string(rank) + std::string(suit);

  // Notify parent
  if (page->m_onCardSelected) {
    page->m_onCardSelected(cardStr);
  }
}

void Page6_Strategy::cbCopyRange(Fl_Widget *w, void *data) {
  auto *page = (Page6_Strategy *)data;
  if (page->m_onCopyRange) {
    std::string rangeStr = page->m_onCopyRange();
    if (!rangeStr.empty()) {
      // Copy to clipboard
      Fl::copy(rangeStr.c_str(), static_cast<int>(rangeStr.length()), 1);
    }
  }
}

void Page6_Strategy::handleStrategyClick(CardButton *btn) {
  std::string hand = btn->label();

  // Check if this hand is already selected (toggle behavior)
  if (btn->strategySelected()) {
    // Deselect and show overall strategy
    deselectHand();
    return;
  }

  selectHand(hand);

  // Notify parent to update analysis panel with combo details
  if (m_onHandSelect) {
    m_onHandSelect(hand);
  }
}

void Page6_Strategy::handleActionClick(Fl_Button *btn) {
  if (m_onAction) {
    m_onAction(btn->label());
  }
}

void Page6_Strategy::show() {
  Fl_Group::show();
  // Mark that we need to recalculate layout on next draw
  m_initialLayoutDone = false;
}

void Page6_Strategy::draw() {
  // On first draw (or after show), force full layout calculation
  if (!m_initialLayoutDone) {
    m_initialLayoutDone = true;

    // Force Fl_Grid to recalculate by doing actual resize
    if (m_mainGrid) {
      m_mainGrid->resize(x(), y(), w(), h());
    }

    // Now position manually-managed widgets
    positionManualWidgets();
  }

  Fl_Group::draw();
}

void Page6_Strategy::positionManualWidgets() {
  // Position analysis panel widgets
  if (m_analysisPanel && m_infoTitle && m_infoDisplay && m_comboDisplay) {
    int px = m_analysisPanel->x();
    int py = m_analysisPanel->y();
    int pw = m_analysisPanel->w();
    int ph = m_analysisPanel->h();

    if (pw > 50 && ph > 50) {
      int headerH = 40;
      // Title spans full width, centered
      m_infoTitle->resize(px + 5, py + 5, pw - 10, headerH - 10);
      // Both displays share the same position (only one is visible at a time)
      m_infoDisplay->resize(px + 5, py + headerH, pw - 10, ph - headerH - 5);
      m_comboDisplay->resize(px + 5, py + headerH, pw - 10, ph - headerH - 5);
    }
  }

  // Position action buttons grid
  if (m_actionButtonsGroup && m_actionButtonsGroup->children() > 0) {
    int rowX = m_actionButtonsGroup->x();
    int rowY = m_actionButtonsGroup->y();
    int rowW = m_actionButtonsGroup->w();
    int rowH = m_actionButtonsGroup->h();

    if (rowW > 20 && rowH > 10) {
      Fl_Widget* child = m_actionButtonsGroup->child(0);
      if (child) {
        child->resize(rowX + 10, rowY + 5, rowW - 20, rowH - 10);
      }
    }
  }
}

void Page6_Strategy::resize(int X, int Y, int W, int H) {
  bool sizeChanged = (X != x() || Y != y() || W != w() || H != h());

  if (sizeChanged) {
    Fl_Group::resize(X, Y, W, H);
    positionManualWidgets();
  }
}
