#include "Page5_Progress.hh"
#include "../utils/MemoryUtil.hh"
#include <FL/Fl.H>
#include <sstream>
#include <iomanip>

Page5_Progress::Page5_Progress(int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H) {

  int centerY = Y + H / 2 - 150;

  // Status label
  m_lblStatus = new Fl_Box(X, centerY, W, 40, "Preparing...");
  m_lblStatus->labelsize(28);
  m_lblStatus->labelfont(FL_HELVETICA_BOLD);
  m_lblStatus->align(FL_ALIGN_CENTER);

  // Memory info label
  m_lblMemory = new Fl_Box(X, centerY + 50, W, 30);
  m_lblMemory->labelsize(18);
  m_lblMemory->align(FL_ALIGN_CENTER);

  // Progress bar
  m_progressBar = new Fl_Progress(X + W / 4, centerY + 90, W / 2, 30);
  m_progressBar->minimum(0);
  m_progressBar->maximum(100);
  m_progressBar->value(0);
  m_progressBar->labelsize(16);

  // Iteration counter
  m_lblIteration = new Fl_Box(X, centerY + 130, W, 30);
  m_lblIteration->labelsize(20);
  m_lblIteration->align(FL_ALIGN_CENTER);

  // Exploitability display
  m_lblExploitability = new Fl_Box(X, centerY + 170, W, 30);
  m_lblExploitability->labelsize(20);
  m_lblExploitability->align(FL_ALIGN_CENTER);

  end();
}

void Page5_Progress::setMemoryEstimate(size_t treeBytes, size_t availableBytes) {
  m_treeMemory = treeBytes;
  m_availableMemory = availableBytes;
  m_memoryOk = (treeBytes < availableBytes * 0.9);  // 90% safety margin

  std::ostringstream oss;
  oss << "Estimated memory: " << MemoryUtil::formatBytes(treeBytes)
      << " / " << MemoryUtil::formatBytes(availableBytes)
      << " available";

  if (!m_memoryOk) {
    oss << " - WARNING: Insufficient memory!";
    m_lblMemory->labelcolor(FL_RED);
  } else {
    m_lblMemory->labelcolor(FL_FOREGROUND_COLOR);
  }

  m_lblMemory->copy_label(oss.str().c_str());
  redraw();
}

void Page5_Progress::setStatus(const std::string& status) {
  m_lblStatus->copy_label(status.c_str());
  redraw();
  Fl::check();  // Force GUI update
}

void Page5_Progress::setProgress(int current, int total) {
  if (total > 0) {
    float percent = (float)current / total * 100.0f;
    m_progressBar->value(percent);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << percent << "%";
    m_progressBar->copy_label(oss.str().c_str());
  }
  redraw();
  Fl::check();
}

void Page5_Progress::setIteration(int current, int total) {
  std::ostringstream oss;
  oss << "Iteration: " << current << " / " << total;
  m_lblIteration->copy_label(oss.str().c_str());
  redraw();
  Fl::check();
}

void Page5_Progress::setExploitability(float value) {
  std::ostringstream oss;
  oss << "Exploitability: " << std::fixed << std::setprecision(4) << value << "%";
  m_lblExploitability->copy_label(oss.str().c_str());
  redraw();
  Fl::check();
}

void Page5_Progress::reset() {
  m_lblStatus->label("Preparing...");
  m_lblMemory->label("");
  m_lblIteration->label("");
  m_lblExploitability->label("");
  m_progressBar->value(0);
  m_progressBar->label("");
  m_treeMemory = 0;
  m_availableMemory = 0;
  m_memoryOk = false;
  redraw();
}

void Page5_Progress::resize(int X, int Y, int W, int H) {
  Fl_Group::resize(X, Y, W, H);

  int centerY = Y + H / 2 - 150;

  m_lblStatus->resize(X, centerY, W, 40);
  m_lblMemory->resize(X, centerY + 50, W, 30);
  m_progressBar->resize(X + W / 4, centerY + 90, W / 2, 30);
  m_lblIteration->resize(X, centerY + 130, W, 30);
  m_lblExploitability->resize(X, centerY + 170, W, 30);
}
