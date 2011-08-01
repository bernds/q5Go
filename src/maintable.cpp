/*
 *   maintable.cpp
 */

#include "maintable.h"
#include "setting.h"
#include "gamestable.h"
#include "playertable.h"
#include "icons.h"

//#include <qmultilineedit.h>
#include <q3textedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <q3mainwindow.h>
#include <qcombobox.h>
#include <qimage.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QEvent>

/* 
 *  Constructs a MainAppWidget as a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'.
 *
 */
MainAppWidget::MainAppWidget( QWidget* parent, const char* name, Qt::WFlags fl )
	: Q3MainWindow( parent, name, fl )
{
	(void)statusBar();
	if ( !name )
	setName( "MainAppWidget" );
	setCentralWidget( new QWidget( this, "qt_central_widget" ) );
	MainAppWidgetLayout = new Q3GridLayout( centralWidget(), 1, 1, 2, 2, "MainAppWidgetLayout"); 

	mainTable = new MainTable( centralWidget(), "mainTable" );

	MainAppWidgetLayout->addMultiCellWidget( mainTable, 0, 1, 0, 1 );

	cb_cmdLine = new QComboBox( FALSE, centralWidget(), "cb_cmdLine" );
	cb_cmdLine->setProperty( "sizePolicy", QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)0, cb_cmdLine->sizePolicy().hasHeightForWidth() ) );
	cb_cmdLine->setProperty( "editable", QVariant( TRUE, 0 ) );

	MainAppWidgetLayout->addMultiCellWidget( cb_cmdLine, 2, 2, 0, 1 );

	languageChange();
	resize( QSize(483, 574).expandedTo(minimumSizeHint()) );
	setAttribute( Qt::WA_WState_Polished, false );

	// signals and slots connections
	connect( cb_cmdLine, SIGNAL( activated(int) ), this, SLOT( slot_cmdactivated_int(int) ) );
	connect( cb_cmdLine, SIGNAL( activated(const QString&) ), this, SLOT( slot_cmdactivated(const QString&) ) );

	// tab order
}

/*
 *  Destroys the object and frees any allocated resources
 */
MainAppWidget::~MainAppWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void MainAppWidget::languageChange()
{
	setProperty( "caption", tr( "MainAppWidget" ) );
	Q3WhatsThis::add( cb_cmdLine, tr( "Command line\n"
		"\n"
		"Type <command>+<ENTER> to send to Go server. If not online use 'connect' button first.\n"
		"\n"
		"Starting with '#' is a internal command as if the server sent the line (without '#').\n"
		"\n"
		"In order to open a window use the mouse click instead of entering the 'observe' command." ) );
}

void MainAppWidget::slot_cmdactivated(const QString&)
{
	qWarning( "MainAppWidget::slot_cmdactivated(const QString&): Not implemented yet" );
}

void MainAppWidget::slot_cmdactivated_int(int)
{
	qWarning( "MainAppWidget::slot_cmdactivated_int(int): Not implemented yet" );
}


MainTable::MainTable(QWidget* parent,  const char* name, Qt::WFlags fl)
    : QWidget(parent, name, fl)
{
	s1 = new QSplitter(Qt::Horizontal, this);
	s2 = new QSplitter(Qt::Vertical, s1);
	s3 = new QSplitter(Qt::Vertical, s1);

	mainTableLayout = new Q3GridLayout(this); 
	mainTableLayout->setSpacing(6);
	mainTableLayout->setMargin(0);

	TabWidget_mini = new QTabWidget(s3, "TabWidget_mini");
	TabWidget_mini->setProperty("sizePolicy", QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, TabWidget_mini->sizePolicy().hasHeightForWidth()));
	TabWidget_mini->setProperty("focusPolicy", (int)Qt::NoFocus);

	games = new QWidget(TabWidget_mini, "games");
	gamesLayout = new Q3GridLayout(games); 
	gamesLayout->setSpacing(6);
	gamesLayout->setMargin(0);

	ListView_games = new GamesTable(games, "ListView_games");
	ListView_games->setProperty("sizePolicy", QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)5, ListView_games->sizePolicy().hasHeightForWidth()));
	Q3WhatsThis::add(ListView_games, tr("Table of games\n\n"
		"right click to observe\n\n"
		"Symbol explanation: (click on tab to sort by)\n"
		"Id\tgame number\n"
		"White/WR\twhite player's name and rank\n"
		"Black/BR\tblack player's name and rank\n"
		"Mv\tnumber of moves at last refresh\n"
		"Sz\tboard size\n"
		"H\thandicap\n"
		"K\tkomi\n"
		"By\tbyoyomi time\n"
		"FR\tfree (FI), rated (I) or teach (TI) game\n"
		"(Ob)\tnumber of observers at last refresh\n\n"
		"This table can be updated by 'Refresh games'"));

	gamesLayout->addWidget(ListView_games, 0, 0);
	TabWidget_mini->insertTab(games, tr("Games"));

	MultiLineEdit2 = new Q3TextEdit(s3, "MultiLineEdit2");
	s3->setResizeMode(MultiLineEdit2, QSplitter::KeepSize);
	MultiLineEdit2->setCurrentFont(setting->fontComments);
	MultiLineEdit2->setProperty("focusPolicy", (int)Qt::NoFocus);
	MultiLineEdit2->setProperty("readOnly", QVariant(TRUE, 0));
	QToolTip::add(MultiLineEdit2, tr("relevant messages from/to server"));

	TabWidget_players = new QTabWidget(s2, "TabWidget_mini");
	TabWidget_players->setProperty("sizePolicy", QSizePolicy((QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, TabWidget_mini->sizePolicy().hasHeightForWidth()));
	TabWidget_players->setProperty("focusPolicy", (int)Qt::NoFocus);

	players = new QWidget(TabWidget_players, "players");
	playersLayout = new Q3GridLayout(players); 
	playersLayout->setSpacing(6);
	playersLayout->setMargin(0);

	ListView_players = new PlayerTable(players, "ListView_players");
	ListView_players->setProperty("sizePolicy", QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, ListView_players->sizePolicy().hasHeightForWidth()));
	Q3WhatsThis::add(ListView_players, tr("Table of players\n\n"
		"right click for menu\n\n"
		"Symbol explanation: (click on tab to sort by)\n"
		"Stat\tplayer's stats:\n"
		"\tX...close\n"
		"\t!...looking = wants to play a game\n"
		"\tQ...quiet = doesn't receive system messages\n"
		"\tS...shout = can't receive shouts\n"
		"\t??...unknown - player entered after last table update\n"
		"name\tplayer's name\n"
		"Rk\tplayer's rank\n"
		"pl\tplaying game (only one game visible)\n"
		"ob\tobserving game (only one game visible)\n"
		"Idle\tidle time\n"
		"X\tprivate info:\n"
		"\tM..me\n"
		"\tX..excluded from shout\n"
		"\tW..watched (sort: X entries, then Rk)\n"
		"and, if extended player info (Toolbox) is active (IGS only):\n"
		"Info\tplayer's info string\n"
		"Won\tnumber of games won by player\n"
		"Lost\tnumber of games lost by player\n"
		"Country\torigin of player (from e-mail address)\n"
		"Lang\tpreferred language\n\n"
		"This table can be updated by 'Refresh players'\n\n"
		"Menu entries (right click):\n"
		"match\trequest for match (dialog arises)\n"
		"talk\ttalk to player (tab arises)\n"
		"----\n"
		"stats\t\tshow player's stats\n"
		"stored games\tshow stored games\n"
		"results\t\tshow results\n"
		"rating\t\tshow rating (NNGS/IGS only)\n"
		"observe game\tshow game currently played by player\n"
		"----\n"
		"toggle watch list\t\tput/remove player to/from watch list - make 'W' entry at 'X' column; entry/leave sounds are activated\n"
		"toggle exclude list\tsimilar to watch - make 'X' entry; player's shouts are no longer shown"));

	playersLayout->addWidget(ListView_players, 0, 0);
	TabWidget_players->insertTab(players, tr("Players"));

	TabWidget_mini_2 = new QTabWidget(s2, "TabWidget_mini_2");
	s2->setResizeMode(TabWidget_mini_2, QSplitter::KeepSize);
	TabWidget_mini_2->setProperty("sizePolicy", QSizePolicy((QSizePolicy::SizeType)1, (QSizePolicy::SizeType)7, TabWidget_mini_2->sizePolicy().hasHeightForWidth()));
	TabWidget_mini_2->setProperty("focusPolicy", (int)Qt::NoFocus);
	TabWidget_mini_2->setProperty("tabPosition", (int)QTabWidget::Bottom);

	shout = new QWidget(TabWidget_mini_2, "shout");
	shoutLayout = new Q3GridLayout(shout); 
	shoutLayout->setSpacing(6);
	shoutLayout->setMargin(0);

	MultiLineEdit3 = new Q3TextEdit(shout, "MultiLineEdit3");
	MultiLineEdit3->setCurrentFont(setting->fontComments);
	MultiLineEdit3->setProperty("focusPolicy", (int)Qt::NoFocus);
	MultiLineEdit3->setProperty("readOnly", QVariant(TRUE, 0));
	QToolTip::add(MultiLineEdit3, tr("Log online-time and name of arriving message"));

	shoutLayout->addWidget(MultiLineEdit3, 0, 0);

	pb_releaseTalkTabs = new QPushButton(tr("Close all talk tabs"), shout, "HideAllTalkTabs");
	Q3WhatsThis::add(pb_releaseTalkTabs, tr("Close all tabs containing a player's name (without '*'). The messages will not be deleted. "
		"If you want to see it again click with right button on player's name and choose talk (same as '#24 *name*')"));

	shoutLayout->addWidget(pb_releaseTalkTabs, 1, 0);

	TabWidget_mini_2->insertTab(shout, tr("msg*"));

	mainTableLayout->addWidget(s1, 0, 0);
}
 
/*  
 *  Destroys the object and frees any allocated resources
 */

MainTable::~MainTable()
{
    // no need to delete child widgets, Qt does it all for us
}

/*  
 *  Main event handler. Reimplemented to handle application
 *  font changes
 */

bool MainTable::event(QEvent* ev)
{
	bool ret = QWidget::event(ev); 
	if (ev->type() == QEvent::ApplicationFontChange)
	{
/*
		QFont MultiLineEdit2_font( MultiLineEdit2->font());
		MultiLineEdit2_font.setFamily("Courier");
		MultiLineEdit2_font.setPointSize(10);
		MultiLineEdit2->setFont(MultiLineEdit2_font); 
		QFont MultiLineEdit3_font( MultiLineEdit3->font());
		MultiLineEdit3_font.setFamily("Courier");
		MultiLineEdit3_font.setPointSize(10);
		MultiLineEdit3->setFont(MultiLineEdit3_font); 
*/
		MultiLineEdit2->setCurrentFont(setting->fontComments);
		MultiLineEdit3->setCurrentFont(setting->fontComments);
	}
	return ret;
}


