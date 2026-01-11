#pragma once
#include <FL/Fl_Widget.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <vector>
#include <string>
#include <map>

// Displays combo strategies as visual colored bars
class ComboStrategyDisplay : public Fl_Widget {
public:
  struct ActionProb {
    std::string name;
    float prob;
    Fl_Color color;
  };

  struct ComboStrategy {
    std::string combo;  // e.g., "As4s"
    std::vector<ActionProb> actions;
  };

  ComboStrategyDisplay(int X, int Y, int W, int H);

  void setHandName(const std::string& name);
  void setCombos(const std::vector<ComboStrategy>& combos);
  void clear();

  int handle(int event) override;

protected:
  void draw() override;

private:
  struct LegendItem {
    std::string name;
    Fl_Color color;
  };

  std::string m_handName;
  std::vector<ComboStrategy> m_combos;
  std::vector<LegendItem> m_legend;
  int m_scrollOffset = 0;
  int m_rowHeight = 28;
  int m_comboWidth = 120;  // Wider to fit "Range Average"
  int m_barHeight = 18;
  int m_legendHeight = 24;

  void buildLegend();
};
