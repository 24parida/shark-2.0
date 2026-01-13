#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_PNG_Image.H>
#include <string>

class Page1_Settings : public Fl_Group {
  Fl_Grid *m_grid;
  Fl_Input *m_inpStack, *m_inpPot, *m_inpMinBet, *m_inpIters;
  Fl_Float_Input *m_inpAllIn, *m_inpMinExploit, *m_inpThreads;
  Fl_Choice *m_choPotType, *m_choYourPos, *m_choTheirPos;
  Fl_Check_Button *m_chkAutoImport, *m_chkForceDonkCheck;
  Fl_Button *m_btnNext;

  // Bet sizes panel (outside grid, manually positioned)
  Fl_Box *m_panelBetSizes;
  Fl_Box *m_panelBetSizesTitle;
  Fl_Box *m_panelBetSizesContent;

  // Logo with bounce animation
  Fl_Box *m_logoBox;
  Fl_PNG_Image *m_logoImage;
  Fl_Image *m_logoScaled{nullptr};  // Scaled copy (managed)
  int m_logoBaseX{0}, m_logoBaseY{0};  // Base position for animation
  int m_logoW{0}, m_logoH{0};  // Logo dimensions
  double m_logoAnimTime{0.0};  // Animation time accumulator

  static void logoAnimCallback(void *data);
  void updateLogoAnimation();

  // Developer credit (static text)
  Fl_Box *m_lblCredit;

public:
  Page1_Settings(int X, int Y, int W, int H);

  void setNextCallback(Fl_Callback *cb, void *data);
  void stopAnimation();  // Call when page becomes hidden

  // Validation - returns empty string if valid, error message if invalid
  std::string validateInputs() const;

  // Getters
  int getStackSize() const;
  int getStartingPot() const;
  int getMinBet() const;
  int getIterations() const;
  int getThreadCount() const;
  float getAllInThreshold() const;
  float getMinExploitability() const;
  const char* getPotType() const;
  const char* getYourPosition() const;
  const char* getTheirPosition() const;
  bool getAutoImport() const;
  bool getForceDonkCheck() const;

protected:
  void resize(int X, int Y, int W, int H) override;
};
