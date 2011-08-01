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
#include <q3textedit.h> //eb16
#include <qtabwidget.h>
#include <q3mainwindow.h>
#include <qcombobox.h>
//Added by qt3to4:
#include <QEvent>
#include <Q3GridLayout>

class MainTable;

class MainAppWidget : public Q3MainWindow
{
	Q_OBJECT

public:
	MainAppWidget( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 
Qt::WType_TopLevel );
	~MainAppWidget();

	MainTable* mainTable;
	QComboBox* cb_cmdLine;
	Q3MainWindow* MyCustomWidget1;

public slots:
	virtual void slot_cmdactivated(const QString&);
	virtual void slot_cmdactivated_int(int);

protected:
	Q3GridLayout* MainAppWidgetLayout;

protected slots:
	virtual void languageChange();
};


class MainTable : public QWidget
{ 
	Q_OBJECT
		
public:
	MainTable( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
	~MainTable();
	
	Q3TextEdit* MultiLineEdit2;
	PlayerTable* ListView_players;
	QTabWidget* TabWidget_mini;
	QTabWidget* TabWidget_players;
	QWidget* games;
	QWidget* players;
	GamesTable* ListView_games;
	QTabWidget* TabWidget_mini_2;
	QWidget* shout;
	Q3TextEdit* MultiLineEdit3;
	QPushButton* pb_releaseTalkTabs;
	QSplitter *s1, *s2, *s3;
	
protected:
	Q3GridLayout* mainTableLayout;
	Q3GridLayout* gamesLayout;
	Q3GridLayout* playersLayout;
	Q3GridLayout* shoutLayout;
	bool event( QEvent* );
};

#endif // MAINTABLE_H 
