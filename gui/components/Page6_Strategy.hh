#pragma once
#include <FL/Fl_Group.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include "CardButton.hh"
#include "ComboStrategyDisplay.hh"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <functional>

class Page6_Strategy : public Fl_Group {
  Fl_Box *m_lblStrategy;
  Fl_Box *m_boardInfo;
  Fl_Box *m_potInfo;
  Fl_Box *m_infoTitle;

  std::vector<CardButton *> m_strategyBtns;
  Fl_Text_Display *m_infoDisplay;
  Fl_Text_Buffer *m_infoBuffer;
  ComboStrategyDisplay *m_comboDisplay;

  std::vector<Fl_Button *> m_actionBtns;
  Fl_Box *m_cardSelLabel;
  Fl_Choice *m_rankChoice, *m_suitChoice;

  Fl_Button *m_zoomInBtn, *m_zoomOutBtn;
  Fl_Button *m_backBtn, *m_undoBtn, *m_copyRangeBtn;

  // Grid structure
  Fl_Grid *m_mainGrid;
  Fl_Group *m_cardSelectionRow;
  Fl_Group *m_actionButtonsGroup;
  Fl_Grid *m_strategyGrid;
  Fl_Group *m_analysisPanel;

  float m_infoTextScale = 1.0f;
  bool m_initialLayoutDone = false;

  std::function<void(const std::string&)> m_onAction;
  std::function<void()> m_onBack;
  std::function<void()> m_onUndo;
  std::function<void(const std::string&)> m_onHandSelect;
  std::function<void(const std::string&)> m_onCardSelected;
  std::function<std::string()> m_onCopyRange;
  std::function<void()> m_onShowOverallStrategy;

  // Store current strategies for range export
  std::map<std::string, std::map<std::string, float>> m_currentStrategies;

public:
  Page6_Strategy(int X, int Y, int W, int H);
  ~Page6_Strategy();

  void setActionCallback(std::function<void(const std::string&)> cb);
  void setBackCallback(std::function<void()> cb);
  void setUndoCallback(std::function<void()> cb);
  void setHandSelectCallback(std::function<void(const std::string&)> cb);
  void setCardSelectedCallback(std::function<void(const std::string&)> cb);
  void setCopyRangeCallback(std::function<std::string()> cb);
  void setShowOverallStrategyCallback(std::function<void()> cb);

  void setTitle(const std::string& title);
  void setBoardInfo(const std::string& board);
  void setPotInfo(const std::string& pot);
  void setInfoText(const std::string& text);
  void setComboStrategies(const std::string& handName, const std::vector<ComboStrategyDisplay::ComboStrategy>& combos);
  void setOverallStrategy(const std::vector<ComboStrategyDisplay::ComboStrategy>& overall);
  void setActions(const std::vector<std::string>& actions);
  void deselectHand();

  void updateStrategyGrid(const std::map<std::string, std::map<std::string, float>>& strategies);
  void selectHand(const std::string& hand);

  void zoomIn();
  void zoomOut();

  // Chance node handling
  void showCardSelection(bool show);
  void showStrategyGrid(bool show);
  void showAnalysisPanel(bool show);
  void populateCardChoices(const std::vector<std::string>& availableCards);

  void show() override;
  void draw() override;

protected:
  void resize(int X, int Y, int W, int H) override;

private:
  static void cbStrategy(Fl_Widget *w, void *data);
  static void cbAction(Fl_Widget *w, void *data);
  static void cbZoomIn(Fl_Widget *w, void *data);
  static void cbZoomOut(Fl_Widget *w, void *data);
  static void cbBack(Fl_Widget *w, void *data);
  static void cbUndo(Fl_Widget *w, void *data);
  static void cbCardSelected(Fl_Widget *w, void *data);
  static void cbCopyRange(Fl_Widget *w, void *data);

  void handleStrategyClick(CardButton *btn);
  void handleActionClick(Fl_Button *btn);
  void updateInfoTextSize();
  void positionManualWidgets();

  // Helper methods for creating sections
  std::string getHandLabel(int row, int col);
  Fl_Group* createHeaderSection();
  Fl_Group* createContentSection();
  Fl_Group* createAnalysisPanel();
  Fl_Group* createCardSelectionRow();
  Fl_Group* createActionRow();
  Fl_Group* createNavigationRow();
};
