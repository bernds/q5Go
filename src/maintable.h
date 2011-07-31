/*
*   maintable.h
*/

#ifndef MAINTABLE_H
#define MAINTABLE_H

#include "gamestable.h"
#include "playertable.h"
#include <qvariant.h>
#include <qwidget.h>
#include <qsplitter.h>
#include <qpushbutton.h>
#include <qlayout.h>
//#include <qmultilineedit.h>
#include <qtextedit.h> //eb16
#include <qtabwidget.h>
#include <qmainwindow.h>
#include <qcombobox.h>

class MainTable;

class MainAppWidget : public QMainWindow
{
	Q_OBJECT

public:
	MainAppWidget( QWidget* parent = 0, const char* name = 0, WFlags fl = 
WType_TopLevel );
	~MainAppWidget();

	MainTable* mainTable;
	QComboBox* cb_cmdLine;
	QMainWindow* MyCustomWidget1;

public slots:
	virtual void slot_cmdactivated(const QString&);
	virtual void slot_cmdactivated_int(int);

protected:
	QGridLayout* MainAppWidgetLayout;

protected slots:
	virtual void languageChange();
};


class MainTable : public QWidget
{ 
	Q_OBJECT
		
public:
	MainTable( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~MainTable();
	
	QTextEdit* MultiLineEdit2;
	PlayerTable* ListView_players;
	QTabWidget* TabWidget_mini;
	QTabWidget* TabWidget_players;
	QWidget* games;
	QWidget* players;
	GamesTable* ListView_games;
	QTabWidget* TabWidget_mini_2;
	QWidget* shout;
	QTextEdit* MultiLineEdit3;
	QPushButton* pb_releaseTalkTabs;
	QSplitter *s1, *s2, *s3;
	
protected:
	QGridLayout* mainTableLayout;
	QGridLayout* gamesLayout;
	QGridLayout* playersLayout;
	QGridLayout* shoutLayout;
	bool event( QEvent* );
};

#endif // MAINTABLE_H 
