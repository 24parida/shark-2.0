#include "ComboStrategyDisplay.hh"
#include "../utils/Colors.hh"
#include <algorithm>

ComboStrategyDisplay::ComboStrategyDisplay(int X, int Y, int W, int H)
    : Fl_Widget(X, Y, W, H) {
  box(FL_DOWN_BOX);
}

void ComboStrategyDisplay::setHandName(const std::string& name) {
  m_handName = name;
  m_scrollOffset = 0;
  // Don't redraw here - setCombos will be called right after and will redraw
}

void ComboStrategyDisplay::setCombos(const std::vector<ComboStrategy>& combos) {
  m_combos = combos;
  m_scrollOffset = 0;
  buildLegend();
  redraw();  // Single redraw for both setHandName + setCombos
}

void ComboStrategyDisplay::buildLegend() {
  m_legend.clear();
  if (m_combos.empty()) return;

  // Get unique actions from first combo (all combos have same actions)
  for (const auto& action : m_combos[0].actions) {
    // Check if already in legend
    bool found = false;
    for (const auto& item : m_legend) {
      if (item.name == action.name) {
        found = true;
        break;
      }
    }
    if (!found) {
      m_legend.push_back({action.name, action.color});
    }
  }
}

void ComboStrategyDisplay::clear() {
  m_handName.clear();
  m_combos.clear();
  m_legend.clear();
  m_scrollOffset = 0;
  redraw();
}

int ComboStrategyDisplay::handle(int event) {
  if (event == FL_MOUSEWHEEL) {
    int totalHeight = static_cast<int>(m_combos.size()) * m_rowHeight + 50;
    int visibleHeight = h() - 10;
    int maxScroll = std::max(0, totalHeight - visibleHeight);

    m_scrollOffset -= Fl::event_dy() * 20;
    m_scrollOffset = std::max(0, std::min(m_scrollOffset, maxScroll));
    redraw();
    return 1;
  }
  return Fl_Widget::handle(event);
}

void ComboStrategyDisplay::draw() {
  // Draw background (oceanic theme)
  fl_color(Colors::InputBg());
  fl_rectf(x(), y(), w(), h());

  // Draw border
  fl_color(Colors::SecondaryBg());
  fl_rect(x(), y(), w(), h());

  if (m_handName.empty() && m_combos.empty()) {
    // Draw placeholder text
    fl_color(Colors::SecondaryText());
    fl_font(FL_HELVETICA, 14);
    fl_draw("Click a hand to see combo details", x() + 10, y() + h() / 2);
    return;
  }

  // Set clip region
  fl_push_clip(x() + 2, y() + 2, w() - 4, h() - 4);

  int px = x() + 8;
  int py = y() + 8 - m_scrollOffset;

  // Draw header with hand name
  if (!m_handName.empty()) {
    fl_color(Colors::PrimaryText());
    fl_font(FL_HELVETICA_BOLD, 16);
    fl_draw(m_handName.c_str(), px, py + 16);
    py += 28;
  }

  // Draw legend
  if (!m_legend.empty()) {
    int legendX = px;
    int legendY = py;
    int swatchSize = 14;
    int itemGap = 8;

    fl_font(FL_HELVETICA, 11);
    for (const auto& item : m_legend) {
      // Draw color swatch
      fl_color(item.color);
      fl_rectf(legendX, legendY, swatchSize, swatchSize);

      // Draw label
      fl_color(Colors::PrimaryText());
      int textW = static_cast<int>(fl_width(item.name.c_str()));
      fl_draw(item.name.c_str(), legendX + swatchSize + 4, legendY + 11);

      legendX += swatchSize + textW + itemGap + 8;

      // Wrap to next line if needed
      if (legendX + 80 > x() + w() - 10) {
        legendX = px;
        legendY += m_legendHeight;
        py += m_legendHeight;
      }
    }
    py += m_legendHeight + 5;

    // Draw separator line
    fl_color(Colors::SecondaryBg());
    fl_line(px, py, px + w() - 20, py);
    py += 8;
  }

  // Draw each combo
  int barWidth = w() - m_comboWidth - 30;

  for (const auto& combo : m_combos) {
    // Skip if completely outside visible area
    if (py + m_rowHeight < y() || py > y() + h()) {
      py += m_rowHeight;
      continue;
    }

    // Draw combo name
    fl_color(Colors::PrimaryText());
    fl_font(FL_HELVETICA_BOLD, 13);
    fl_draw(combo.combo.c_str(), px, py + m_barHeight - 3);

    // Draw action bar
    int barX = px + m_comboWidth;
    int barY = py + 2;

    // Background for bar
    fl_color(Colors::SecondaryBg());
    fl_rectf(barX, barY, barWidth, m_barHeight);

    // Draw colored segments
    int currentX = barX;
    for (const auto& action : combo.actions) {
      if (action.prob < 0.001f) continue;

      int segWidth = static_cast<int>(barWidth * action.prob);
      if (segWidth < 1) segWidth = 1;

      fl_color(action.color);
      fl_rectf(currentX, barY, segWidth, m_barHeight);

      // Draw percentage text if segment is wide enough
      if (segWidth > 25) {
        fl_color(FL_WHITE);
        fl_font(FL_HELVETICA_BOLD, 11);
        char pctText[16];
        snprintf(pctText, sizeof(pctText), "%.0f%%", action.prob * 100);

        // Center text in segment
        int textW = static_cast<int>(fl_width(pctText));
        int textX = currentX + (segWidth - textW) / 2;
        fl_draw(pctText, textX, barY + 13);
      }

      currentX += segWidth;
    }

    // Draw bar border
    fl_color(Colors::PanelBg());
    fl_rect(barX, barY, barWidth, m_barHeight);

    py += m_rowHeight;
  }

  fl_pop_clip();

  // Draw scroll indicator if needed
  int totalHeight = static_cast<int>(m_combos.size()) * m_rowHeight + 50;
  if (totalHeight > h() - 10) {
    int scrollbarH = h() - 10;
    int thumbH = std::max(20, scrollbarH * scrollbarH / totalHeight);
    int thumbY = y() + 5 + (m_scrollOffset * (scrollbarH - thumbH)) / (totalHeight - scrollbarH);

    fl_color(Colors::SecondaryText());
    fl_rectf(x() + w() - 8, thumbY, 5, thumbH);
  }
}
