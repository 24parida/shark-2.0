#include "Page2_Board.hh"
#include "../utils/RangeData.hh"
#include "../utils/Colors.hh"
#include <algorithm>
#include <random>
#include <sstream>

Page2_Board::Page2_Board(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  // Title
  m_lblTitle = new Fl_Box(X, Y + 15, W, 50, "Init Board (3-5 Cards)");
  m_lblTitle->labelfont(FL_BOLD);
  m_lblTitle->labelsize(21);
  m_lblTitle->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);

  // Card grid (will be created in rebuildCardGrid)
  rebuildCardGrid(W, H);

  // Selection display
  int inputWidth = static_cast<int>(W * 0.6);
  int randButtonWidth = 200;
  int inputSpacing = 20;
  int totalWidth = inputWidth + randButtonWidth + inputSpacing;
  int startX = X + (W - totalWidth) / 2;
  int inputY = Y + H - 110;

  m_selDisplay = new Fl_Input(startX, inputY, inputWidth, 40);
  m_selDisplay->textsize(18);
  m_selDisplay->readonly(1);

  m_btnRand = new Fl_Button(m_selDisplay->x() + m_selDisplay->w() + inputSpacing,
                            inputY, randButtonWidth, 40, "Generate Random Flop");
  m_btnRand->labelsize(18);
  m_btnRand->callback(cbRand, this);

  // Navigation buttons
  int navY = Y + H - 60;
  m_btnBack = new Fl_Button(X + 25, navY, 150, 40, "Back");
  m_btnBack->labelsize(18);

  m_btnNext = new Fl_Button(X + W - 175, navY, 150, 40, "Next");
  m_btnNext->labelsize(18);

  end();
}

void Page2_Board::rebuildCardGrid(int W, int H) {
  // Clear existing cards
  for (auto *card : m_cards) {
    remove(card);
    delete card;
  }
  m_cards.clear();

  int cols = 4, rows = 13;
  int topMargin = 80;
  int bottomReservedSpace = 150;
  int availableHeight = H - topMargin - bottomReservedSpace;
  int availableWidth = W - 100;
  int spacing = 8;

  int maxButtonWidth = (availableWidth - (cols - 1) * spacing) / cols;
  int maxButtonHeight = (availableHeight - (rows - 1) * spacing) / rows;
  int buttonSize = std::min(maxButtonWidth, maxButtonHeight);
  buttonSize = std::min(65, std::max(45, buttonSize));

  int totalGridWidth = cols * buttonSize + (cols - 1) * spacing;
  int totalGridHeight = rows * buttonSize + (rows - 1) * spacing;
  int GX = x() + (W - totalGridWidth) / 2;
  int GY = y() + topMargin + (availableHeight - totalGridHeight) / 2;

  // Create card grid
  for (int r = 0; r < rows; ++r) {
    for (int s = 0; s < cols; ++s) {
      int cx = GX + s * (buttonSize + spacing);
      int cy = GY + r * (buttonSize + spacing);

      Fl_Color base;
      switch (RangeData::SUITS[s]) {
        case 'h': base = Colors::Hearts(); break;
        case 'd': base = Colors::Diamonds(); break;
        case 'c': base = Colors::Clubs(); break;
        default:  base = Colors::Spades();
      }

      auto *cb = new CardButton(cx, cy, buttonSize, buttonSize, base);
      std::string lbl = RangeData::RANKS[r] + std::string(1, RangeData::SUITS[s]);
      cb->copy_label(lbl.c_str());
      cb->labelsize(16);
      cb->callback(cbCard, this);
      m_cards.push_back(cb);
    }
  }
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

    // Find and select the card button
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
    // Deselect
    m_selectedCards.erase(it);
    btn->select(false);
  } else if (m_selectedCards.size() < 5) {
    // Select (max 5 cards)
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

  m_lblTitle->resize(X, Y + 15, W, 50);

  // Rebuild card grid with new dimensions
  rebuildCardGrid(W, H);

  // Reposition bottom controls
  int inputWidth = static_cast<int>(W * 0.6);
  int randButtonWidth = 200;
  int inputSpacing = 20;
  int totalWidth = inputWidth + randButtonWidth + inputSpacing;
  int startX = X + (W - totalWidth) / 2;
  int inputY = Y + H - 110;

  m_selDisplay->resize(startX, inputY, inputWidth, 40);
  m_btnRand->resize(m_selDisplay->x() + m_selDisplay->w() + inputSpacing, inputY, randButtonWidth, 40);

  int navY = Y + H - 60;
  m_btnBack->resize(X + 25, navY, 150, 40);
  m_btnNext->resize(X + W - 175, navY, 150, 40);
}
