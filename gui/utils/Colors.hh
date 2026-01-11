#pragma once
#include <FL/Fl.H>
#include <FL/fl_draw.H>

namespace Colors {
  // ========== Oceanic Blue Theme ==========
  inline Fl_Color PrimaryBg()    { return fl_rgb_color(20, 40, 80); }     // Deep ocean blue
  inline Fl_Color SecondaryBg()  { return fl_rgb_color(30, 60, 100); }    // Lighter ocean
  inline Fl_Color PanelBg()      { return fl_rgb_color(40, 75, 120); }    // Sea blue
  inline Fl_Color InputBg()      { return fl_rgb_color(25, 50, 90); }     // Dark blue-gray
  inline Fl_Color PrimaryText()  { return FL_WHITE; }                     // White text
  inline Fl_Color SecondaryText() { return fl_rgb_color(180, 220, 255); } // Light cyan
  inline Fl_Color ThemeButtonBg() { return fl_rgb_color(0, 150, 180); }   // Teal accent

  // Card suit colors (adjusted for dark theme visibility)
  inline Fl_Color Hearts()   { return fl_rgb_color(220, 70, 70); }    // Vibrant red
  inline Fl_Color Diamonds() { return fl_rgb_color(100, 130, 220); }  // Softer blue
  inline Fl_Color Clubs()    { return fl_rgb_color(70, 180, 70); }    // Fresh green
  inline Fl_Color Spades()   { return fl_rgb_color(90, 100, 120); }   // Steel gray-blue

  // UI element colors (updated for oceanic theme)
  inline Fl_Color Highlight()    { return fl_rgb_color(255, 200, 0); }   // Golden - keep
  inline Fl_Color UncoloredCard() { return fl_rgb_color(40, 75, 120); }  // Sea blue
  inline Fl_Color ButtonBg()     { return fl_rgb_color(0, 150, 180); }   // Teal accent
  inline Fl_Color LightBg()      { return fl_rgb_color(40, 75, 120); }   // Sea blue
  inline Fl_Color InfoBg()       { return fl_rgb_color(30, 60, 100); }   // Lighter ocean
  inline Fl_Color InfoSelBg()    { return fl_rgb_color(50, 90, 140); }   // Selected blue

  // Range grid colors
  inline Fl_Color PairSelected()    { return fl_rgb_color(85, 170, 85); }   // Rich green for pairs
  inline Fl_Color SuitedSelected()  { return fl_rgb_color(85, 115, 185); }  // Nice blue for suited
  inline Fl_Color DefaultCell()     { return fl_rgb_color(65, 85, 115); }   // Slate blue-gray for offsuit

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
