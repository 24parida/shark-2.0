#include "Page1_Settings.hh"
#include "utils/Colors.hh"
#include "utils/EmbeddedLogo.hh"
#include <FL/Fl.H>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <algorithm>
#include <string>

Page1_Settings::Page1_Settings(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  m_grid = new Fl_Grid(X, Y, W, H);
  m_grid->layout(11, 5, 10, 10);  // 11 rows, 5 columns (left spacer + gap), 10px margins

  // Row 0: Stack Size
  auto *lblStack = new Fl_Box(0, 0, 0, 0, "Stack Size:");
  lblStack->labelsize(24);
  lblStack->labelfont(FL_HELVETICA_BOLD);
  lblStack->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblStack, 0, 1);

  m_inpStack = new Fl_Input(0, 0, 0, 0);
  m_inpStack->textsize(24);
  m_inpStack->value("100");
  m_grid->widget(m_inpStack, 0, 2);

  // Row 1: Starting Pot
  auto *lblPot = new Fl_Box(0, 0, 0, 0, "Starting Pot:");
  lblPot->labelsize(24);
  lblPot->labelfont(FL_HELVETICA_BOLD);
  lblPot->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblPot, 1, 1);

  m_inpPot = new Fl_Input(0, 0, 0, 0);
  m_inpPot->textsize(24);
  m_inpPot->value("10");
  m_grid->widget(m_inpPot, 1, 2);

  // Row 2: Min Bet
  auto *lblMinBet = new Fl_Box(0, 0, 0, 0, "Initial Min Bet:");
  lblMinBet->labelsize(24);
  lblMinBet->labelfont(FL_HELVETICA_BOLD);
  lblMinBet->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblMinBet, 2, 1);

  m_inpMinBet = new Fl_Input(0, 0, 0, 0);
  m_inpMinBet->textsize(24);
  m_inpMinBet->value("2");
  m_grid->widget(m_inpMinBet, 2, 2);

  // Row 3: All-In Threshold
  auto *lblAllIn = new Fl_Box(0, 0, 0, 0, "All-In Thresh:");
  lblAllIn->labelsize(24);
  lblAllIn->labelfont(FL_HELVETICA_BOLD);
  lblAllIn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblAllIn, 3, 1);

  m_inpAllIn = new Fl_Float_Input(0, 0, 0, 0);
  m_inpAllIn->textsize(24);
  m_inpAllIn->value("0.67");
  m_grid->widget(m_inpAllIn, 3, 2);

  // Row 4: Pot Type
  auto *lblPotType = new Fl_Box(0, 0, 0, 0, "Type of pot:");
  lblPotType->labelsize(24);
  lblPotType->labelfont(FL_HELVETICA_BOLD);
  lblPotType->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblPotType, 4, 1);

  m_choPotType = new Fl_Choice(0, 0, 0, 0);
  m_choPotType->textsize(24);
  m_choPotType->add("Single Raise|3-bet|4-bet");
  m_choPotType->value(0);
  m_grid->widget(m_choPotType, 4, 2);

  // Row 5: Your Position
  auto *lblYourPos = new Fl_Box(0, 0, 0, 0, "Your pos:");
  lblYourPos->labelsize(24);
  lblYourPos->labelfont(FL_HELVETICA_BOLD);
  lblYourPos->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblYourPos, 5, 1);

  m_choYourPos = new Fl_Choice(0, 0, 0, 0);
  m_choYourPos->textsize(24);
  m_choYourPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
  m_choYourPos->value(0);
  m_grid->widget(m_choYourPos, 5, 2);

  // Row 6: Their Position
  auto *lblTheirPos = new Fl_Box(0, 0, 0, 0, "Their pos:");
  lblTheirPos->labelsize(24);
  lblTheirPos->labelfont(FL_HELVETICA_BOLD);
  lblTheirPos->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblTheirPos, 6, 1);

  m_choTheirPos = new Fl_Choice(0, 0, 0, 0);
  m_choTheirPos->textsize(24);
  m_choTheirPos->add("SB|BB|UTG|UTG+1|MP|LJ|HJ|CO|BTN");
  m_choTheirPos->value(1);
  m_grid->widget(m_choTheirPos, 6, 2);

  // Row 7: Iterations
  auto *lblIters = new Fl_Box(0, 0, 0, 0, "Iterations:");
  lblIters->labelsize(24);
  lblIters->labelfont(FL_HELVETICA_BOLD);
  lblIters->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblIters, 7, 1);

  m_inpIters = new Fl_Input(0, 0, 0, 0);
  m_inpIters->textsize(24);
  m_inpIters->value("100");
  m_grid->widget(m_inpIters, 7, 2);

  // Row 8: Min Exploitability
  auto *lblMinExploit = new Fl_Box(0, 0, 0, 0, "Min Exploit %:");
  lblMinExploit->labelsize(24);
  lblMinExploit->labelfont(FL_HELVETICA_BOLD);
  lblMinExploit->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblMinExploit, 8, 1);

  m_inpMinExploit = new Fl_Float_Input(0, 0, 0, 0);
  m_inpMinExploit->textsize(24);
  m_inpMinExploit->value("0.1");
  m_grid->widget(m_inpMinExploit, 8, 2);

  // Row 9: Thread Count
  auto *lblThreads = new Fl_Box(0, 0, 0, 0, "Thread Count:");
  lblThreads->labelsize(24);
  lblThreads->labelfont(FL_HELVETICA_BOLD);
  lblThreads->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  m_grid->widget(lblThreads, 9, 1);

  m_inpThreads = new Fl_Float_Input(0, 0, 0, 0);
  m_inpThreads->textsize(24);
  // Default to (num_cores - 1), minimum 1
  unsigned int numCores = std::thread::hardware_concurrency();
  int defaultThreads = std::max(1, static_cast<int>(numCores) - 1);
  m_inpThreads->value(std::to_string(defaultThreads).c_str());
  m_grid->widget(m_inpThreads, 9, 2);

  // Column 4: Options on the right side (column 3 is gap)
  // Auto-import checkbox (row 0, aligned with Stack Size)
  m_chkAutoImport = new Fl_Check_Button(0, 0, 0, 0, "Auto-import ranges");
  m_chkAutoImport->labelsize(24);
  m_chkAutoImport->labelfont(FL_HELVETICA_BOLD);
  m_chkAutoImport->value(1);
  m_grid->widget(m_chkAutoImport, 0, 4);  // Row 0, column 4

  // Force Donk Check checkbox (row 1, right below Auto-import)
  m_chkForceDonkCheck = new Fl_Check_Button(0, 0, 0, 0, "Force Donk Check");
  m_chkForceDonkCheck->labelsize(24);
  m_chkForceDonkCheck->labelfont(FL_HELVETICA_BOLD);
  m_chkForceDonkCheck->value(1);  // Default enabled
  m_grid->widget(m_chkForceDonkCheck, 1, 4);  // Row 1, column 4

  // Subtext for Force Donk Check (row 2, right below checkbox)
  auto *lblDonkSubtext = new Fl_Box(0, 0, 0, 0, "applied to flop only, recommended for memory savings");
  lblDonkSubtext->labelsize(12);
  lblDonkSubtext->labelfont(FL_HELVETICA_ITALIC);
  lblDonkSubtext->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP);
  m_grid->widget(lblDonkSubtext, 2, 4);  // Row 2, column 4


  // Set column weights for resizing
  m_grid->col_weight(0, 5);   // Left spacer
  m_grid->col_weight(1, 20);  // Labels column
  m_grid->col_weight(2, 30);  // Inputs column
  m_grid->col_weight(3, 5);   // Gap column
  m_grid->col_weight(4, 40);  // Options column

  // Set row weights (all equal)
  for (int i = 0; i < 11; ++i) {
    m_grid->row_weight(i, 1);
  }

  m_grid->end();

  // Bet sizes panel (outside grid, manually positioned)
  // Background panel
  m_panelBetSizes = new Fl_Box(0, 0, 0, 0);
  m_panelBetSizes->box(FL_ROUNDED_BOX);
  m_panelBetSizes->color(fl_rgb_color(30, 50, 70));  // Slightly lighter than bg

  // Title (bold, larger)
  m_panelBetSizesTitle = new Fl_Box(0, 0, 0, 0, "Default Bet Sizes");
  m_panelBetSizesTitle->labelsize(20);
  m_panelBetSizesTitle->labelfont(FL_HELVETICA_BOLD);
  m_panelBetSizesTitle->labelcolor(FL_WHITE);
  m_panelBetSizesTitle->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

  // Content (formatted with alignment - pipes aligned)
  m_panelBetSizesContent = new Fl_Box(0, 0, 0, 0,
    "Flop :  50%, 100%       |  Raise: 100%\n"
    "Turn :  33%, 66%, 100%  |  Raise: 50%, 100%\n"
    "River:  33%, 66%, 100%  |  Raise: 50%, 100%");
  m_panelBetSizesContent->labelsize(13);  // Smaller to fit in box
  m_panelBetSizesContent->labelfont(FL_COURIER);  // Monospace for alignment
  m_panelBetSizesContent->labelcolor(FL_WHITE);
  m_panelBetSizesContent->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

  // Logo (positioned below bet sizes panel) - loaded from embedded data
  m_logoImage = new Fl_PNG_Image("embedded_logo", embedded_logo_png, embedded_logo_png_len);
  m_logoBox = new Fl_Box(0, 0, 0, 0);
  m_logoBox->image(m_logoImage);
  m_logoBox->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

  // Developer credit (positioned below logo)
  m_lblCredit = new Fl_Box(0, 0, 0, 0, "Developed by Anubhav Parida");
  m_lblCredit->labelsize(16);
  m_lblCredit->labelfont(FL_TIMES_ITALIC);
  m_lblCredit->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

  // Next button (outside grid, fixed at bottom center)
  m_btnNext = new Fl_Button((W - 225) / 2, H - 70, 225, 52, "Next");
  m_btnNext->labelsize(18);
  m_btnNext->labelfont(FL_HELVETICA_BOLD);
  m_btnNext->color(Colors::ThemeButtonBg());
  m_btnNext->labelcolor(FL_WHITE);

  end();

  // Force initial layout
  resize(X, Y, W, H);

  // Start logo bounce animation
  Fl::add_timeout(0.016, logoAnimCallback, this);  // ~60fps
}

void Page1_Settings::setNextCallback(Fl_Callback *cb, void *data) {
  m_btnNext->callback(cb, data);
}

// Helper to check if a string is a valid integer
static bool isValidInt(const char* str) {
  if (!str || *str == '\0') return false;
  char* end;
  long val = std::strtol(str, &end, 10);
  return *end == '\0' && val >= 0;
}

// Helper to check if a string is a valid float
static bool isValidFloat(const char* str) {
  if (!str || *str == '\0') return false;
  char* end;
  std::strtod(str, &end);
  return *end == '\0';
}

std::string Page1_Settings::validateInputs() const {
  const char* stackStr = m_inpStack->value();
  const char* potStr = m_inpPot->value();
  const char* minBetStr = m_inpMinBet->value();
  const char* itersStr = m_inpIters->value();
  const char* allInStr = m_inpAllIn->value();
  const char* minExploitStr = m_inpMinExploit->value();
  const char* threadsStr = m_inpThreads->value();

  // Check integer fields
  if (!isValidInt(stackStr))
    return "Stack Size must be a valid integer";
  if (!isValidInt(potStr))
    return "Starting Pot must be a valid integer";
  if (!isValidInt(minBetStr))
    return "Initial Min Bet must be a valid integer";
  if (!isValidInt(itersStr))
    return "Iterations must be a valid integer";

  // Check float fields
  if (!isValidFloat(allInStr))
    return "All-In Threshold must be a valid number";
  if (!isValidFloat(minExploitStr))
    return "Min Exploitability must be a valid number";
  if (!isValidFloat(threadsStr))
    return "Thread Count must be a valid number";

  // Get actual values
  int stackSize = atoi(stackStr);
  int pot = atoi(potStr);
  int minBet = atoi(minBetStr);
  int iters = atoi(itersStr);
  float allIn = static_cast<float>(atof(allInStr));
  float minExploit = static_cast<float>(atof(minExploitStr));
  int threads = atoi(threadsStr);

  // Value range checks
  if (stackSize <= 0)
    return "Stack Size must be greater than 0";
  if (pot <= 0)
    return "Starting Pot must be greater than 0";
  if (minBet <= 0)
    return "Initial Min Bet must be greater than 0";
  if (stackSize < minBet)
    return "Stack Size must be >= Initial Min Bet";
  if (iters <= 0)
    return "Iterations must be greater than 0";
  if (allIn <= 0 || allIn > 1)
    return "All-In Threshold must be between 0 and 1";
  if (minExploit < 0 || minExploit > 100)
    return "Min Exploitability must be between 0% and 100%";
  if (threads <= 0)
    return "Thread Count must be greater than 0";

  return "";  // Valid
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
  return m_inpMinExploit->value() ? static_cast<float>(atof(m_inpMinExploit->value())) : 0.1f;
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

bool Page1_Settings::getForceDonkCheck() const {
  return m_chkForceDonkCheck->value() != 0;
}

void Page1_Settings::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  // Resize grid to fill most of the space
  m_grid->resize(X, Y, W, H - 80);

  // Position bet sizes panel in the right column area (below Force Donk Check subtext)
  int panelX = X + static_cast<int>(W * 0.60);  // Start at ~60% from left
  int panelY = Y + static_cast<int>((H - 80) * 0.32);  // ~32% down (below subtext)
  int panelW = static_cast<int>(W * 0.35);  // 35% width
  int panelH = static_cast<int>((H - 80) * 0.22);  // 22% height

  // Dynamic font sizing based on panel width
  int titleFontSize = std::max(14, std::min(24, panelW / 15));
  int contentFontSize = std::max(9, std::min(14, panelW / 25));
  int creditFontSize = std::max(12, std::min(18, panelW / 20));

  // Background panel
  m_panelBetSizes->resize(panelX, panelY, panelW, panelH);

  // Title at top of panel (with padding)
  int titleH = titleFontSize + 10;
  int padding = 8;
  m_panelBetSizesTitle->resize(panelX + padding, panelY + padding, panelW - 2*padding, titleH);
  m_panelBetSizesTitle->labelsize(titleFontSize);

  // Content below title
  int contentY = panelY + padding + titleH + 2;
  int contentH = panelH - titleH - 2*padding - 2;
  m_panelBetSizesContent->resize(panelX + padding, contentY, panelW - 2*padding, contentH);
  m_panelBetSizesContent->labelsize(contentFontSize);

  // Logo below bet sizes panel (roughly same size as panel)
  int logoY = panelY + panelH + 10;
  int logoH = static_cast<int>((H - 80) * 0.30);  // 30% height for logo

  // Store base position and dimensions for animation
  m_logoBaseX = panelX;
  m_logoBaseY = logoY;
  m_logoW = panelW;
  m_logoH = logoH;

  // Position will be updated by animation, but set initial
  m_logoBox->resize(panelX, logoY, panelW, logoH);

  // Scale logo to fit the box while maintaining aspect ratio
  if (m_logoImage && m_logoImage->w() > 0 && m_logoImage->h() > 0) {
    float scaleW = static_cast<float>(panelW) / m_logoImage->w();
    float scaleH = static_cast<float>(logoH) / m_logoImage->h();
    float scale = std::min(scaleW, scaleH) * 0.9f;  // 90% to leave some margin
    int newW = static_cast<int>(m_logoImage->w() * scale);
    int newH = static_cast<int>(m_logoImage->h() * scale);

    // Delete old scaled image to prevent memory leak
    if (m_logoScaled) {
      delete m_logoScaled;
    }
    m_logoScaled = m_logoImage->copy(newW, newH);
    m_logoBox->image(m_logoScaled);
  }

  // Credit text below logo, right-aligned with panel's right edge
  int creditY = logoY + logoH + 5;
  int creditH = 25;
  m_lblCredit->resize(panelX, creditY, panelW, creditH);
  m_lblCredit->labelsize(creditFontSize);

  // Keep Next button at bottom center
  m_btnNext->resize((W - 225) / 2, Y + H - 70, 225, 52);
}

void Page1_Settings::stopAnimation() {
  // Stop logo animation
  Fl::remove_timeout(logoAnimCallback, this);
}

// Logo bounce animation - makes the shark look lively
void Page1_Settings::logoAnimCallback(void *data) {
  auto *self = static_cast<Page1_Settings*>(data);
  self->updateLogoAnimation();
}

void Page1_Settings::updateLogoAnimation() {
  // Advance animation time (fixed timestep)
  m_logoAnimTime += 0.016;  // ~60fps timestep

  // Sync with typewriter: 27 chars × 100ms = 2.7s typing time
  // 2 bounces during typing = speed of pi*2/2.7 = 2.33
  double bounce = std::abs(std::sin(m_logoAnimTime * 2.33)) * 12.0;

  // Subtle secondary wobble for liveliness (proportionally faster)
  double wobble = std::sin(m_logoAnimTime * 4.66) * 2.0;

  // Combine - bounce goes 0 to 12, wobble adds variation
  int offsetY = static_cast<int>(bounce + wobble);

  // Calculate the area that needs redrawing (old + new logo position)
  int oldY = m_logoBox->y();
  int newY = m_logoBaseY - offsetY;

  // Apply animated position (vertical only)
  m_logoBox->resize(m_logoBaseX, newY, m_logoW, m_logoH);

  // Only damage the specific region around the logo to avoid full-screen flicker
  int damageY = std::min(oldY, newY) - 2;
  int damageH = m_logoH + std::abs(newY - oldY) + 4;
  damage(FL_DAMAGE_ALL, m_logoBaseX - 2, damageY, m_logoW + 4, damageH);

  // Schedule next frame with add_timeout (doesn't accumulate when window loses focus)
  Fl::add_timeout(0.016, logoAnimCallback, this);
}
