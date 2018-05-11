#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "misctools.h"
#include "mainwidget_gui.h"
#include "interfacehandler.h"
#include "setting.h"
#include "defines.h"

//class InterfaceHandler;

class MainWindow;

class MainWidget : public QWidget, public Ui::MainWidgetGui
{ 
	Q_OBJECT

public:
	MainWidget(MainWindow *, QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
	~MainWidget();
	
	void toggleSlider(bool);
	bool getSlider() { return showSlider; }
	void toggleSliderSignal(bool b) { sliderSignalToggle = b; }
	virtual void setFont(const QFont &font);
	void setToolsTabWidget(enum tabType=tabNormalScore, enum tabState=tabSet);
	void doRealScore(bool);
	GameMode toggleMode();
	void setGameMode (GameMode);

	InterfaceHandler *interfaceHandler;

public slots:
	void on_colorButton_clicked(bool);
	void slot_toolsTabChanged(QWidget*);
	virtual void setMarkType(int);
	virtual void doPass();
	virtual void doUndo();// { interfaceHandler->board->doUndo(); }
	virtual void doAdjourn();// { interfaceHandler->board->doAdjourn(); }
	virtual void doResign();// { interfaceHandler->board->doResign(); }
	virtual void doRefresh();// { interfaceHandler->board->doRefresh(); }
	virtual void doScore(bool);
	virtual void sliderChanged(int);

private:
	bool showSlider, sliderSignalToggle;
	MainWindow *m_mainwin;
};

#endif
