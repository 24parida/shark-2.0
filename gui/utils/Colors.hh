#pragma once
#include <FL/Fl.H>
#include <FL/fl_draw.H>

namespace Colors {
  // Card suit colors
  inline Fl_Color Hearts()   { return fl_rgb_color(180, 30, 30); }
  inline Fl_Color Diamonds() { return fl_rgb_color(30, 30, 180); }
  inline Fl_Color Clubs()    { return fl_rgb_color(30, 180, 30); }
  inline Fl_Color Spades()   { return fl_rgb_color(20, 20, 20); }

  // UI element colors
  inline Fl_Color Highlight()    { return fl_rgb_color(255, 200, 0); }
  inline Fl_Color UncoloredCard() { return fl_rgb_color(80, 80, 80); }
  inline Fl_Color ButtonBg()     { return fl_rgb_color(220, 220, 220); }
  inline Fl_Color LightBg()      { return fl_rgb_color(240, 240, 240); }
  inline Fl_Color InfoBg()       { return fl_rgb_color(230, 230, 230); }
  inline Fl_Color InfoSelBg()    { return fl_rgb_color(200, 200, 200); }

  // Range grid colors
  inline Fl_Color PairSelected()    { return fl_rgb_color(100, 200, 100); }
  inline Fl_Color SuitedSelected()  { return fl_rgb_color(100, 100, 200); }
  inline Fl_Color DefaultCell()     { return fl_rgb_color(80, 80, 80); }

  // Strategy colors
  inline Fl_Color FoldColor()  { return fl_rgb_color(255, 50, 50); }   // Red
  inline Fl_Color CallColor()  { return fl_rgb_color(50, 255, 50); }   // Green

  // Generate bet color based on action index
  inline Fl_Color BetColor(int actionIndex, int totalBetActions) {
    if (totalBetActions <= 1) {
      return fl_rgb_color(50, 50, 255);  // Blue
    }
    float ratio = static_cast<float>(actionIndex) / (totalBetActions - 1);
    int r = static_cast<int>(50 + ratio * 205);
    int g = 50;
    int b = static_cast<int>(255 - ratio * 205);
    return fl_rgb_color(r, g, b);
  }
}
