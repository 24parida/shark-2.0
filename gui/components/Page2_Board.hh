#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include "CardButton.hh"
#include <vector>
#include <string>
#include <functional>

class Page2_Board : public Fl_Group {
  Fl_Box *m_lblTitle;
  Fl_Grid *m_cardGrid;  // 13×4 grid of cards
  std::vector<CardButton *> m_cards;
  Fl_Input *m_selDisplay;
  Fl_Button *m_btnRand, *m_btnBack, *m_btnNext;

  std::vector<std::string> m_selectedCards;
  std::function<void(const std::vector<std::string>&)> m_onBoardChange;

public:
  Page2_Board(int X, int Y, int W, int H);

  void setBackCallback(Fl_Callback *cb, void *data);
  void setNextCallback(Fl_Callback *cb, void *data);
  void setBoardChangeCallback(std::function<void(const std::vector<std::string>&)> cb);

  std::vector<std::string> getSelectedCards() const { return m_selectedCards; }
  void clearSelection();
  void randomFlop();

protected:
  void resize(int X, int Y, int W, int H) override;

private:
  static void cbCard(Fl_Widget *w, void *data);
  static void cbRand(Fl_Widget *w, void *data);

  void handleCardClick(CardButton *btn);
  void handleRandom();
  void updateDisplay();
};
