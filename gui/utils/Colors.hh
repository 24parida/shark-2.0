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

  // Strategy colors - cohesive palette
  // Cool tones for passive actions, warm progression for aggressive actions
  inline Fl_Color FoldColor()  { return fl_rgb_color(91, 141, 238); }   // Clear blue - passive
  inline Fl_Color CheckColor() { return fl_rgb_color(94, 186, 125); }   // Fresh green - safe/neutral
  inline Fl_Color CallColor()  { return fl_rgb_color(94, 186, 125); }   // Same green as check

  // Bet/raise colors - warm progression (0=smallest, 2=all-in)
  inline Fl_Color BetColor(int betIndex) {
    switch (betIndex) {
      case 0: return fl_rgb_color(245, 166, 35);   // Golden amber - mild aggression
      case 1: return fl_rgb_color(224, 124, 84);   // Terracotta coral - moderate
      case 2: return fl_rgb_color(196, 69, 105);   // Deep rose - maximum aggression
      default: return fl_rgb_color(196, 69, 105);  // Deep rose for any additional
    }
  }
}
