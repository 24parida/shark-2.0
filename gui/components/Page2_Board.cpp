#include "Page2_Board.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <FL/Fl_Grid.H>
#include <algorithm>
#include <random>
#include <sstream>

Page2_Board::Page2_Board(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  // Main container grid: 4 rows (title, cards, input, nav)
  auto *mainGrid = new Fl_Grid(X, Y, W, H);
  mainGrid->layout(4, 1, 5, 5);  // 4 rows, 1 column, 5px margins

  // Row 0: Title
  m_lblTitle = new Fl_Box(0, 0, 0, 0, "Init Board (3-5 Cards)");
  m_lblTitle->labelfont(FL_BOLD);
  m_lblTitle->labelsize(28);
  m_lblTitle->align(FL_ALIGN_CENTER);
  mainGrid->widget(m_lblTitle, 0, 0);
  mainGrid->row_height(0, 60);

  // Row 1: Card grid (13 rows × 4 columns)
  m_cardGrid = new Fl_Grid(0, 0, 0, 0);
  m_cardGrid->layout(13, 4, 5, 5);  // 13 rows, 4 columns, 5px spacing

  // Create 52 cards and add to grid
  for (int r = 0; r < 13; ++r) {
    for (int c = 0; c < 4; ++c) {
      Fl_Color base;
      switch (RangeData::SUITS[c]) {
        case 'h': base = Colors::Hearts(); break;
        case 'd': base = Colors::Diamonds(); break;
        case 'c': base = Colors::Clubs(); break;
        default:  base = Colors::Spades();
      }

      auto *card = new CardButton(0, 0, 0, 0, base);  // Fl_Grid will size it
      std::string lbl = RangeData::RANKS[r] + std::string(1, RangeData::SUITS[c]);
      card->copy_label(lbl.c_str());
      card->labelsize(16);
      card->callback(cbCard, this);

      m_cardGrid->widget(card, r, c);  // Add to grid at (row, col)
      m_cards.push_back(card);
    }
  }

  // Set all rows/cols to equal weight so they resize proportionally
  for (int i = 0; i < 13; ++i) {
    m_cardGrid->row_weight(i, 1);
  }
  for (int i = 0; i < 4; ++i) {
    m_cardGrid->col_weight(i, 1);
  }

  m_cardGrid->end();
  mainGrid->widget(m_cardGrid, 1, 0);
  // Set a reasonable fixed height for card grid (will be adjusted in resize)
  mainGrid->row_height(1, H - 170);  // Leave space for title, input, nav

  // Row 2: Input + Random button
  auto *inputRow = new Fl_Group(0, 0, 0, 0);
  inputRow->begin();
  m_selDisplay = new Fl_Input(0, 0, 0, 0);
  m_selDisplay->textsize(18);
  m_selDisplay->readonly(1);

  m_btnRand = new Fl_Button(0, 0, 0, 0, "Random Flop");
  m_btnRand->labelsize(18);
  m_btnRand->labelfont(FL_HELVETICA_BOLD);
  m_btnRand->color(Colors::ThemeButtonBg());
  m_btnRand->labelcolor(FL_WHITE);
  m_btnRand->callback(cbRand, this);
  inputRow->end();

  mainGrid->widget(inputRow, 2, 0);
  mainGrid->row_height(2, 50);

  // Row 3: Navigation
  auto *navRow = new Fl_Group(0, 0, 0, 0);
  navRow->begin();
  m_btnBack = new Fl_Button(0, 0, 0, 0, "Back");
  m_btnBack->labelsize(18);
  m_btnBack->labelfont(FL_HELVETICA_BOLD);
  m_btnBack->color(Colors::ThemeButtonBg());
  m_btnBack->labelcolor(FL_WHITE);

  m_btnNext = new Fl_Button(0, 0, 0, 0, "Next");
  m_btnNext->labelsize(18);
  m_btnNext->labelfont(FL_HELVETICA_BOLD);
  m_btnNext->color(Colors::ThemeButtonBg());
  m_btnNext->labelcolor(FL_WHITE);
  navRow->end();

  mainGrid->widget(navRow, 3, 0);
  mainGrid->row_height(3, 60);

  mainGrid->end();
  end();

  // Force initial layout
  resize(X, Y, W, H);
}

void Page2_Board::setBackCallback(Fl_Callback *cb, void *data) {
  m_btnBack->callback(cb, data);
}

void Page2_Board::setNextCallback(Fl_Callback *cb, void *data) {
  m_btnNext->callback(cb, data);
}

void Page2_Board::setBoardChangeCallback(std::function<void(const std::vector<std::string>&)> cb) {
  m_onBoardChange = cb;
}

void Page2_Board::clearSelection() {
  m_selectedCards.clear();
  for (auto *card : m_cards) {
    card->select(false);
  }
  updateDisplay();
}

void Page2_Board::randomFlop() {
  clearSelection();

  std::vector<std::string> allCards;
  for (const auto& rank : RangeData::RANKS) {
    for (char suit : RangeData::SUITS) {
      allCards.push_back(rank + std::string(1, suit));
    }
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(allCards.begin(), allCards.end(), g);

  for (int i = 0; i < 3 && i < (int)allCards.size(); ++i) {
    const auto& card = allCards[i];
    m_selectedCards.push_back(card);

    for (auto *btn : m_cards) {
      if (std::string(btn->label()) == card) {
        btn->select(true);
        break;
      }
    }
  }

  updateDisplay();
  if (m_onBoardChange) {
    m_onBoardChange(m_selectedCards);
  }
}

void Page2_Board::cbCard(Fl_Widget *w, void *data) {
  ((Page2_Board *)data)->handleCardClick((CardButton *)w);
}

void Page2_Board::cbRand(Fl_Widget *w, void *data) {
  ((Page2_Board *)data)->handleRandom();
}

void Page2_Board::handleCardClick(CardButton *btn) {
  std::string card = btn->label();

  auto it = std::find(m_selectedCards.begin(), m_selectedCards.end(), card);
  if (it != m_selectedCards.end()) {
    m_selectedCards.erase(it);
    btn->select(false);
  } else if (m_selectedCards.size() < 5) {
    m_selectedCards.push_back(card);
    btn->select(true);
  }

  updateDisplay();
  if (m_onBoardChange) {
    m_onBoardChange(m_selectedCards);
  }
}

void Page2_Board::handleRandom() {
  randomFlop();
}

void Page2_Board::updateDisplay() {
  std::ostringstream oss;
  for (size_t i = 0; i < m_selectedCards.size(); ++i) {
    if (i > 0) oss << " ";
    oss << m_selectedCards[i];
  }
  m_selDisplay->value(oss.str().c_str());
}

void Page2_Board::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  // Update card grid height based on available space
  if (children() > 0) {
    auto *mainGrid = dynamic_cast<Fl_Grid*>(child(0));
    if (!mainGrid || mainGrid->children() < 4) return;

    // Adjust row 1 (card grid) height based on window size
    int cardGridHeight = H - 180;  // Leave space for title (60) + input (50) + nav (60) + margins (10)
    mainGrid->row_height(1, cardGridHeight);
    mainGrid->resize(X, Y, W, H);

    // Row 2: Input row - position input and button
    auto *inputRow = mainGrid->child(2);
    int rowW = inputRow->w();
    int rowX = inputRow->x();
    int rowY = inputRow->y();

    int inputWidth = static_cast<int>(rowW * 0.6);
    int btnWidth = static_cast<int>(rowW * 0.3);
    int spacing = 20;
    int totalW = inputWidth + spacing + btnWidth;
    int startX = rowX + (rowW - totalW) / 2;

    m_selDisplay->resize(startX, rowY + 5, inputWidth, 40);
    m_btnRand->resize(startX + inputWidth + spacing, rowY + 5, btnWidth, 40);

    // Row 3: Nav row - position back and next buttons
    auto *navRow = mainGrid->child(3);
    int navX = navRow->x();
    int navY = navRow->y();
    int navW = navRow->w();

    m_btnBack->resize(navX + 25, navY + 2, 120, 40);
    m_btnNext->resize(navX + navW - 145, navY + 2, 120, 40);
  }
}
