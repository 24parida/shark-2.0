#include "CardButton.hh"
#include "../utils/Colors.hh"

const Fl_Color CardButton::HIGHLIGHT = fl_rgb_color(255, 200, 0);
const Fl_Color CardButton::UNCOLORED_BG = fl_rgb_color(40, 75, 120);  // Sea blue (matches theme)

CardButton::CardButton(int X, int Y, int W, int H, Fl_Color baseColor)
    : Fl_Button(X, Y, W, H, ""), m_base(baseColor) {
  box(FL_FLAT_BOX);
  labelcolor(FL_WHITE);
  labelsize(18);
  color(m_base);
  visible_focus(false);
}

void CardButton::toggle() {
  m_sel = !m_sel;
  color(m_sel ? HIGHLIGHT : m_base);
  redraw();
}

void CardButton::select(bool s) {
  if (s != m_sel)
    toggle();
}

void CardButton::setStrategySelected(bool sel) {
  if (m_strategy_sel != sel) {
    m_strategy_sel = sel;
    redraw();
  }
}

void CardButton::setStrategyColors(const std::vector<std::pair<Fl_Color, float>> &colors) {
  m_strategy_colors = colors;
  m_strategy_sel = false;
  m_strategy_mode = true;  // Mark that we're in strategy display mode
  // Don't call redraw() here - let parent batch the redraw
}

void CardButton::clearStrategyMode() {
  m_strategy_colors.clear();
  m_strategy_sel = false;
  m_strategy_mode = false;
}

void CardButton::draw() {
  int x = this->x();
  int y = this->y();
  int w = this->w();
  int h = this->h();

  if (!m_strategy_colors.empty()) {
    // Draw strategy color bars (stacked vertically)
    int current_y = y;
    for (const auto &[color, percentage] : m_strategy_colors) {
      if (percentage > 0.001f) {
        int bar_height = static_cast<int>(h * percentage);
        fl_color(color);
        fl_rectf(x, current_y, w, bar_height);
        current_y += bar_height;
      }
    }
  } else {
    // No strategy - draw solid background color (or highlight if selected)
    fl_color(m_sel ? HIGHLIGHT : m_base);
    fl_rectf(x, y, w, h);
  }

  // Draw subtle border for card definition
  fl_color(fl_rgb_color(20, 35, 60));  // Dark border
  fl_rect(x, y, w, h);

  // Draw label on top with bold font
  // Use dimmed text color for cards not in range (only on strategy page)
  if (m_strategy_mode && m_strategy_colors.empty()) {
    fl_color(fl_rgb_color(100, 130, 170));  // Dimmed blue-gray for out-of-range
  } else {
    fl_color(labelcolor());
  }
  fl_font(FL_HELVETICA_BOLD, labelsize());
  fl_draw(label(), x, y, w, h, FL_ALIGN_CENTER);

  // Draw border if this hand is selected (strategy page)
  if (m_strategy_sel) {
    fl_color(FL_WHITE);  // White border for visibility on dark theme
    int thick = 3;  // Line thickness
    for (int t = 0; t < thick; ++t) {
      fl_rect(x + t, y + t, w - 2 * t, h - 2 * t);
    }
  }
}

int CardButton::handle(int event) {
  if (event == FL_FOCUS || event == FL_UNFOCUS)
    return 0;
  return Fl_Button::handle(event);
}
