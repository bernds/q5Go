/*
* helpviewer.h
*/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <QMainWindow>
#include <QToolBar>
#include <QTextBrowser>
#include <QAction>

class HelpViewer : public QMainWindow
{
	Q_OBJECT

public:
	HelpViewer(QWidget* parent = 0);
	~HelpViewer();

protected:
	void initToolBar();

private:
	QTextBrowser *browser;
	QToolBar *toolBar;
	QAction *buttonHome, *buttonClose, *buttonBack, *buttonForward;
};

#endif
