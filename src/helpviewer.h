/*
* helpviewer.h
*/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <q3mainwindow.h>
#include <q3textbrowser.h>
#include <qtoolbutton.h>

class HelpViewer : public Q3MainWindow
{
	Q_OBJECT
		
public:
	HelpViewer(QWidget* parent = 0, const char* name = 0, Qt::WFlags f = Qt::WType_TopLevel);
	~HelpViewer();
	
protected:
	void initToolBar();
	
private:
	Q3TextBrowser *browser;
	Q3ToolBar *toolBar;
	QToolButton *buttonHome, *buttonClose;
};

#endif
