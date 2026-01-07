#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include "CardButton.hh"
#include <vector>
#include <string>
#include <functional>

class Page4_VillainRange : public Fl_Group {
  Fl_Box *m_lblTitle;
  Fl_Grid *m_rangeGrid;  // 13×13 grid of hands
  std::vector<CardButton *> m_rangeBtns;
  Fl_Button *m_btnBack, *m_btnNext;

  std::vector<std::string> m_selectedRange;
  std::function<void(const std::vector<std::string>&)> m_onRangeChange;

public:
  Page4_VillainRange(int X, int Y, int W, int H);

  void setBackCallback(Fl_Callback *cb, void *data);
  void setNextCallback(Fl_Callback *cb, void *data);
  void setRangeChangeCallback(std::function<void(const std::vector<std::string>&)> cb);

  std::vector<std::string> getSelectedRange() const { return m_selectedRange; }
  void setSelectedRange(const std::vector<std::string>& range);
  void clearSelection();

protected:
  void resize(int X, int Y, int W, int H) override;

private:
  static void cbRange(Fl_Widget *w, void *data);
  void handleRangeClick(CardButton *btn);
};
