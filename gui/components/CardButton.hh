#pragma once
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <vector>
#include <utility>

class CardButton : public Fl_Button {
  Fl_Color m_base;
  bool m_sel = false;
  bool m_strategy_sel = false;
  static const Fl_Color HIGHLIGHT;
  static const Fl_Color UNCOLORED_BG;
  std::vector<std::pair<Fl_Color, float>> m_strategy_colors;

public:
  CardButton(int X, int Y, int W, int H, Fl_Color baseColor);

  void toggle();
  void select(bool s);
  bool selected() const { return m_sel; }

  void setStrategySelected(bool sel);
  bool strategySelected() const { return m_strategy_sel; }
  void setStrategyColors(const std::vector<std::pair<Fl_Color, float>> &colors);

protected:
  void draw() override;
  int handle(int event) override;
};
