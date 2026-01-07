#include "CardButton.hh"

const Fl_Color CardButton::HIGHLIGHT = fl_rgb_color(255, 200, 0);
const Fl_Color CardButton::UNCOLORED_BG = fl_rgb_color(80, 80, 80);

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
  redraw();
}

void CardButton::draw() {
  if (!m_strategy_colors.empty()) {
    int x = this->x();
    int y = this->y();
    int w = this->w();
    int h = this->h();

    // Draw base background
    fl_color(FL_BACKGROUND_COLOR);
    fl_rectf(x, y, w, h);

    // Draw strategy color bars
    int current_y = y;
    for (const auto &[color, percentage] : m_strategy_colors) {
      if (percentage > 0.001f) {
        int bar_height = static_cast<int>(h * percentage);
        fl_color(color);
        fl_rectf(x, current_y, w, bar_height);
        current_y += bar_height;
      }
    }

    // Redraw label on top
    fl_color(labelcolor());
    fl_font(labelfont(), labelsize());
    fl_draw(label(), x, y, w, h, FL_ALIGN_CENTER);

    // Draw black border if this hand is selected in strategy display
    if (m_strategy_sel) {
      fl_color(FL_BLACK);
      fl_line_style(FL_SOLID, 4);

      int corner_radius = 3;

      // Draw corners
      fl_arc(x + corner_radius, y + corner_radius, corner_radius * 2, corner_radius * 2, 90, 180);
      fl_arc(x + w - corner_radius * 3, y + corner_radius, corner_radius * 2, corner_radius * 2, 0, 90);
      fl_arc(x + corner_radius, y + h - corner_radius * 3, corner_radius * 2, corner_radius * 2, 180, 270);
      fl_arc(x + w - corner_radius * 3, y + h - corner_radius * 3, corner_radius * 2, corner_radius * 2, 270, 360);

      // Connect corners with lines
      fl_line(x + corner_radius, y, x + w - corner_radius, y);
      fl_line(x + w, y + corner_radius, x + w, y + h - corner_radius);
      fl_line(x + corner_radius, y + h, x + w - corner_radius, y + h);
      fl_line(x, y + corner_radius, x, y + h - corner_radius);

      fl_line_style(0);
    }
  } else {
    // For non-strategy buttons, draw shaded background if uncolored
    if (m_base == FL_BACKGROUND_COLOR) {
      int x = this->x();
      int y = this->y();
      int w = this->w();
      int h = this->h();

      fl_color(UNCOLORED_BG);
      fl_rectf(x, y, w, h);
    }
    Fl_Button::draw();
  }
}

int CardButton::handle(int event) {
  if (event == FL_FOCUS || event == FL_UNFOCUS)
    return 0;
  return Fl_Button::handle(event);
}
