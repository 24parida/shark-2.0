#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include "CardButton.hh"
#include <vector>
#include <string>
#include <map>
#include <functional>

class Page6_Strategy : public Fl_Group {
  Fl_Box *m_lblStrategy;
  Fl_Box *m_boardInfo;
  Fl_Box *m_potInfo;
  Fl_Box *m_infoTitle;

  std::vector<CardButton *> m_strategyBtns;
  Fl_Text_Display *m_infoDisplay;
  Fl_Text_Buffer *m_infoBuffer;

  std::vector<Fl_Button *> m_actionBtns;
  Fl_Choice *m_cardChoice, *m_rankChoice, *m_suitChoice;

  Fl_Button *m_zoomInBtn, *m_zoomOutBtn;
  Fl_Button *m_backBtn, *m_undoBtn;

  float m_infoTextScale = 1.0f;

  std::function<void(const std::string&)> m_onAction;
  std::function<void()> m_onBack;
  std::function<void()> m_onUndo;

public:
  Page6_Strategy(int X, int Y, int W, int H);
  ~Page6_Strategy();

  void setActionCallback(std::function<void(const std::string&)> cb);
  void setBackCallback(std::function<void()> cb);
  void setUndoCallback(std::function<void()> cb);

  void setTitle(const std::string& title);
  void setBoardInfo(const std::string& board);
  void setPotInfo(const std::string& pot);
  void setInfoText(const std::string& text);
  void setActions(const std::vector<std::string>& actions);

  void updateStrategyGrid(const std::map<std::string, std::map<std::string, float>>& strategies);
  void selectHand(const std::string& hand);

  void zoomIn();
  void zoomOut();

protected:
  void resize(int X, int Y, int W, int H) override;

private:
  static void cbStrategy(Fl_Widget *w, void *data);
  static void cbAction(Fl_Widget *w, void *data);
  static void cbZoomIn(Fl_Widget *w, void *data);
  static void cbZoomOut(Fl_Widget *w, void *data);
  static void cbBack(Fl_Widget *w, void *data);
  static void cbUndo(Fl_Widget *w, void *data);

  void handleStrategyClick(CardButton *btn);
  void handleActionClick(Fl_Button *btn);
  void rebuildStrategyGrid(int W, int H);
  void updateInfoTextSize();
};
