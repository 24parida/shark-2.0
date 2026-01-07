#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Progress.H>
#include <string>

class Page5_Progress : public Fl_Group {
  Fl_Box *m_lblStatus;
  Fl_Box *m_lblMemory;
  Fl_Progress *m_progressBar;
  Fl_Box *m_lblIteration;
  Fl_Box *m_lblExploitability;

  size_t m_treeMemory = 0;
  size_t m_availableMemory = 0;
  bool m_memoryOk = false;

public:
  Page5_Progress(int X, int Y, int W, int H);

  // Memory estimation
  void setMemoryEstimate(size_t treeBytes, size_t availableBytes);
  bool isMemoryOk() const { return m_memoryOk; }

  // Progress updates
  void setStatus(const std::string& status);
  void setProgress(int current, int total);
  void setIteration(int current, int total);
  void setExploitability(float value);
  void reset();

protected:
  void resize(int X, int Y, int W, int H) override;
};
