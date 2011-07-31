/*
* helpviewer.h
*/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <qmainwindow.h>
#include <qtextbrowser.h>
#include <qtoolbutton.h>

class HelpViewer : public QMainWindow
{
	Q_OBJECT
		
public:
	HelpViewer(QWidget* parent = 0, const char* name = 0, WFlags f = WType_TopLevel);
	~HelpViewer();
	
protected:
	void initToolBar();
	
private:
	QTextBrowser *browser;
	QToolBar *toolBar;
	QToolButton *buttonHome, *buttonClose;
};

#endif
