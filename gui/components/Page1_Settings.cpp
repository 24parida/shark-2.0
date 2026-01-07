#include "Page1_Settings.hh"
#include <cstdlib>

Page1_Settings::Page1_Settings(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  m_grid = new Fl_Grid(X, Y, W, H);
  m_grid->layout(11, 2, 10, 10);  // 11 rows, 2 columns, 10px margins

  // Row 0: Stack Size
  auto *lblStack = new Fl_Box(0, 0, 0, 0, "Stack Size:");
  lblStack->labelsize(24);
  lblStack->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblStack, 0, 0);

  m_inpStack = new Fl_Input(0, 0, 0, 0);
  m_inpStack->textsize(24);
  m_grid->widget(m_inpStack, 0, 1);

  // Row 1: Starting Pot
  auto *lblPot = new Fl_Box(0, 0, 0, 0, "Starting Pot:");
  lblPot->labelsize(24);
  lblPot->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblPot, 1, 0);

  m_inpPot = new Fl_Input(0, 0, 0, 0);
  m_inpPot->textsize(24);
  m_grid->widget(m_inpPot, 1, 1);

  // Row 2: Min Bet
  auto *lblMinBet = new Fl_Box(0, 0, 0, 0, "Initial Min Bet:");
  lblMinBet->labelsize(24);
  lblMinBet->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblMinBet, 2, 0);

  m_inpMinBet = new Fl_Input(0, 0, 0, 0);
  m_inpMinBet->textsize(24);
  m_grid->widget(m_inpMinBet, 2, 1);

  // Row 3: All-In Threshold
  auto *lblAllIn = new Fl_Box(0, 0, 0, 0, "All-In Thresh:");
  lblAllIn->labelsize(24);
  lblAllIn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblAllIn, 3, 0);

  m_inpAllIn = new Fl_Float_Input(0, 0, 0, 0);
  m_inpAllIn->textsize(24);
  m_inpAllIn->value("0.67");
  m_grid->widget(m_inpAllIn, 3, 1);

  // Row 4: Pot Type
  auto *lblPotType = new Fl_Box(0, 0, 0, 0, "Type of pot:");
  lblPotType->labelsize(24);
  lblPotType->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblPotType, 4, 0);

  m_choPotType = new Fl_Choice(0, 0, 0, 0);
  m_choPotType->textsize(24);
  m_choPotType->add("Single Raise|3-bet|4-bet");
  m_choPotType->value(0);
  m_grid->widget(m_choPotType, 4, 1);

  // Row 5: Your Position
  auto *lblYourPos = new Fl_Box(0, 0, 0, 0, "Your pos:");
  lblYourPos->labelsize(24);
  lblYourPos->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblYourPos, 5, 0);

  m_choYourPos = new Fl_Choice(0, 0, 0, 0);
  m_choYourPos->textsize(24);
  m_choYourPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
  m_choYourPos->value(0);
  m_grid->widget(m_choYourPos, 5, 1);

  // Row 6: Their Position
  auto *lblTheirPos = new Fl_Box(0, 0, 0, 0, "Their pos:");
  lblTheirPos->labelsize(24);
  lblTheirPos->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblTheirPos, 6, 0);

  m_choTheirPos = new Fl_Choice(0, 0, 0, 0);
  m_choTheirPos->textsize(24);
  m_choTheirPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
  m_choTheirPos->value(1);
  m_grid->widget(m_choTheirPos, 6, 1);

  // Row 7: Iterations
  auto *lblIters = new Fl_Box(0, 0, 0, 0, "Iterations:");
  lblIters->labelsize(24);
  lblIters->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblIters, 7, 0);

  m_inpIters = new Fl_Input(0, 0, 0, 0);
  m_inpIters->textsize(24);
  m_inpIters->value("100");
  m_grid->widget(m_inpIters, 7, 1);

  // Row 8: Min Exploitability
  auto *lblMinExploit = new Fl_Box(0, 0, 0, 0, "Min Exploitability (%):");
  lblMinExploit->labelsize(24);
  lblMinExploit->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblMinExploit, 8, 0);

  m_inpMinExploit = new Fl_Float_Input(0, 0, 0, 0);
  m_inpMinExploit->textsize(24);
  m_inpMinExploit->value("1.0");
  m_grid->widget(m_inpMinExploit, 8, 1);

  // Row 9: Thread Count
  auto *lblThreads = new Fl_Box(0, 0, 0, 0, "Thread Count:");
  lblThreads->labelsize(24);
  lblThreads->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblThreads, 9, 0);

  m_inpThreads = new Fl_Float_Input(0, 0, 0, 0);
  m_inpThreads->textsize(24);
  m_inpThreads->value("0");  // 0 = auto-detect
  m_grid->widget(m_inpThreads, 9, 1);

  // Row 10: Auto-import checkbox
  auto *spacer = new Fl_Box(0, 0, 0, 0);
  m_grid->widget(spacer, 10, 0);

  m_chkAutoImport = new Fl_Check_Button(0, 0, 0, 0, "Auto-import ranges based on positions");
  m_chkAutoImport->labelsize(24);
  m_chkAutoImport->value(1);
  m_grid->widget(m_chkAutoImport, 10, 1);

  // Set column weights for resizing
  m_grid->col_weight(0, 40);  // Labels column
  m_grid->col_weight(1, 60);  // Inputs column

  // Set row weights (all equal)
  for (int i = 0; i < 11; ++i) {
    m_grid->row_weight(i, 1);
  }

  m_grid->end();

  // Next button (outside grid, fixed at bottom center)
  m_btnNext = new Fl_Button((W - 225) / 2, H - 70, 225, 52, "Next");
  m_btnNext->labelsize(18);

  end();
}

void Page1_Settings::setNextCallback(Fl_Callback *cb, void *data) {
  m_btnNext->callback(cb, data);
}

int Page1_Settings::getStackSize() const {
  return m_inpStack->value() ? atoi(m_inpStack->value()) : 0;
}

int Page1_Settings::getStartingPot() const {
  return m_inpPot->value() ? atoi(m_inpPot->value()) : 0;
}

int Page1_Settings::getMinBet() const {
  return m_inpMinBet->value() ? atoi(m_inpMinBet->value()) : 0;
}

int Page1_Settings::getIterations() const {
  return m_inpIters->value() ? atoi(m_inpIters->value()) : 0;
}

int Page1_Settings::getThreadCount() const {
  return m_inpThreads->value() ? atoi(m_inpThreads->value()) : 0;
}

float Page1_Settings::getAllInThreshold() const {
  return m_inpAllIn->value() ? static_cast<float>(atof(m_inpAllIn->value())) : 0.67f;
}

float Page1_Settings::getMinExploitability() const {
  return m_inpMinExploit->value() ? static_cast<float>(atof(m_inpMinExploit->value())) : 1.0f;
}

const char* Page1_Settings::getPotType() const {
  return m_choPotType->text();
}

const char* Page1_Settings::getYourPosition() const {
  return m_choYourPos->text();
}

const char* Page1_Settings::getTheirPosition() const {
  return m_choTheirPos->text();
}

bool Page1_Settings::getAutoImport() const {
  return m_chkAutoImport->value() != 0;
}

void Page1_Settings::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  // Resize grid to fill most of the space
  m_grid->resize(X, Y, W, H - 80);

  // Keep Next button at bottom center
  m_btnNext->resize((W - 225) / 2, Y + H - 70, 225, 52);
}
