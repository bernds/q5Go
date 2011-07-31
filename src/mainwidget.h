#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "mainwidget_gui.h"
#include "interfacehandler.h"
#include "setting.h"
#include "defines.h"

//class InterfaceHandler;
class NormalTools;
class ScoreTools;

class MainWidget : public MainWidgetGui
{ 
	Q_OBJECT
		
public:
	MainWidget(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~MainWidget();
	
	void toggleSlider(bool);
	bool getSlider() { return showSlider; }
	void toggleSliderSignal(bool b) { sliderSignalToggle = b; }
	virtual void setFont(const QFont &font);
	void setToolsTabWidget(enum tabType=tabNormalScore, enum tabState=tabSet);
	void doRealScore(bool);

	InterfaceHandler *interfaceHandler;
	ScoreTools *scoreTools;

public slots:
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
};

#endif
