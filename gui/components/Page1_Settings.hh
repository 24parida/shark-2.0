#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

class Page1_Settings : public Fl_Group {
  Fl_Grid *m_grid;
  Fl_Input *m_inpStack, *m_inpPot, *m_inpMinBet, *m_inpIters;
  Fl_Float_Input *m_inpAllIn, *m_inpMinExploit, *m_inpThreads;
  Fl_Choice *m_choPotType, *m_choYourPos, *m_choTheirPos;
  Fl_Check_Button *m_chkAutoImport;
  Fl_Button *m_btnNext;

public:
  Page1_Settings(int X, int Y, int W, int H);

  void setNextCallback(Fl_Callback *cb, void *data);

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

protected:
  void resize(int X, int Y, int W, int H) override;
};
