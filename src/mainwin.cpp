/*
 *   mainwin.cpp - qGoClient's main window (cont. in tables.cpp)
 */

#include <QMainWindow>
#include <QFileDialog>
#include <QWhatsThis>

#include "mainwin.h"
#include "defines.h"
#include "playertable.h"
#include "gamestable.h"
#include "gamedialog.h"
#include "qgo_interface.h"
//Added by qt3to4:
#include <QTimerEvent>
#include <QKeyEvent>
#include "komispinbox.h"
#include "icons.h"
#include "igsconnection.h"
#include "mainwindow.h"
#include "qnewgamedlg.h"
#include "./pics/clientpics.h"
#include <qaction.h>
#include <qdir.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qevent.h>
#include <qstatusbar.h>
#include <qtooltip.h>
#include <q3popupmenu.h>
#include <q3textstream.h>
#include <qpalette.h>
#include <qtoolbutton.h>
#include <qicon.h>
#include <qpixmap.h>
#include <q3buttongroup.h> 
#include <qobject.h> 
#include <q3listbox.h> 
#include <qmovie.h> 
#include <qradiobutton.h>

#define QGO_NOSTYLES
#ifndef QGO_NOSTYLES
#include <qplatinumstyle.h>
#include <qmotifstyle.h>
#include <qmotifplusstyle.h>
#include <qcdestyle.h>
#include <qsgistyle.h>
#endif

ClientWindow *client_window;

/*
 *   Clientwindow = MainWidget
 */


ClientWindow::ClientWindow(QMainWindow *parent, const char* name, Qt::WFlags fl)
	: QMainWindow( parent, name, fl ), seekButtonTimer (0), oneSecondTimer (0)
{
	setupUi(this);

	prefsIcon= QPixmap(qembed_findImage("package_settings"));//QPixmap(ICON_PREFS);
//	infoIcon= QPixmap(ICON_GAMEINFO);
	exitIcon =  QPixmap(qembed_findImage("exit"));//QPixmap(ICON_EXIT);
	fileNewboardIcon = QPixmap(qembed_findImage("newboard"));//QPixmap(ICON_FILENEWBOARD);
	fileNewIcon = QPixmap(qembed_findImage("filenew"));//QPixmap(ICON_FILENEW);
	fileOpenIcon = QPixmap(qembed_findImage("fileopen"));//QPixmap(ICON_FILEOPEN);
//	fileSaveIcon = QPixmap(ICON_FILESAVE);
//	fileSaveAsIcon = QPixmap(ICON_FILESAVEAS);
//	transformIcon = QPixmap(ICON_TRANSFORM);
//	charIcon = QPixmap(ICON_CHARSET);
	manualIcon = QPixmap(qembed_findImage("help"));//QPixmap(ICON_MANUAL);
//	autoplayIcon = QPixmap(ICON_AUTOPLAY);
	connectedIcon = QPixmap(qembed_findImage("connected"));//QPixmap(ICON_CONNECTED);             
	disconnectedIcon = QPixmap(qembed_findImage("connect_no"));//QPixmap(ICON_DISCONNECTED);       
	OpenIcon = QPixmap(qembed_findImage("open"));//QPixmap(ICON_OPEN);              
	LookingIcon = QPixmap(qembed_findImage("looking"));//QPixmap(ICON_LOOKING);
	QuietIcon= QPixmap(qembed_findImage("quiet"));//QPixmap(ICON_QUIET);
	NotOpenIcon = QPixmap(qembed_findImage("not_open"));//QPixmap(ICON_NOT_OPEN);
	NotLookingIcon = QPixmap(qembed_findImage("not_looking"));//QPixmap(ICON_NOT_LOOKING);
	NotQuietIcon= QPixmap(qembed_findImage("not_quiet"));//QPixmap(ICON_NOT_QUIET);       
	RefreshPlayersIcon = QPixmap(qembed_findImage("refresh_players"));//QPixmap(ICON_REFRESH_PLAYERS);    
	RefreshGamesIcon = QPixmap(qembed_findImage("refresh_games"));//QPixmap(ICON_REFRESH_GAMES);        
	ComputerPlayIcon = QPixmap(qembed_findImage("computerplay"));//QPixmap(ICON_COMPUTER_PLAY);
	qgoIcon = QPixmap(qembed_findImage("Bowl"));//QPixmap(ICON_COMPUTER_PLAY);  
	NotSeekingIcon = QPixmap(qembed_findImage("not_seeking"));
	seekingIcon[0] = QPixmap(qembed_findImage("seeking1"));
	seekingIcon[1] = QPixmap(qembed_findImage("seeking2"));
	seekingIcon[2] = QPixmap(qembed_findImage("seeking3"));
	seekingIcon[3] = QPixmap(qembed_findImage("seeking4"));

	// init

	DODEBUG = false;
	DD = 0;
	setting->cw = this;
	setIcon(setting->image0);
	myAccount = new Account(this);

	defaultStyle = qApp->style()->name() ;
	// this is very dirty : we do this because there seem to be no clean way to backtrack from MainWindow to the defaultStyle :-(
	setting->writeEntry("DEFAULT_STYLE",defaultStyle ) ;

	cmd_count = 0;

	sendBuffer.setAutoDelete(true);
	currentCommand = new sendBuf("",0);

	resetCounter();
	cmd_valid = false;
	tn_ready = false;
	tn_wait_for_tn_ready = false;
//	tn_active = false;
	extUserInfo = false;
	youhavemsg = false;
	playerListEmpty = true;
	gamesListSteadyUpdate = false;
	playerListSteadyUpdate = false;
	autoAwayMessage = false;
	watch = ";" + setting->readEntry("WATCH") + ";";
	exclude = ";" + setting->readEntry("EXCLUDE") + ";";
	// statistics
	bytesIn = 0;
	bytesOut = 0;
	seekButtonTimer = 0;
//	toolConnect = 0;

	// set focus, clear entry field
	cb_cmdLine->setFocus();
	cb_cmdLine->clear();
	cb_cmdLine->insertItem("");

	// create instance of telnetConnection
	telnetConnection = new TelnetConnection(this);

	// create parser and connect signals
	parser = new Parser();
	connect(parser, SIGNAL(signal_player(Player*, bool)), SLOT(slot_player(Player*, bool)));
  	connect(parser, SIGNAL(signal_statsPlayer(Player*)), SLOT(slot_statsPlayer(Player*)));
	connect(parser, SIGNAL(signal_game(Game*)), SLOT(slot_game(Game*)));
	connect(parser, SIGNAL(signal_message(QString)), SLOT(slot_message(QString)));
	connect(parser, SIGNAL(signal_svname(GSName&)), SLOT(slot_svname(GSName&)));
	connect(parser, SIGNAL(signal_accname(QString&)), SLOT(slot_accname(QString&)));
	connect(parser, SIGNAL(signal_status(Status)), SLOT(slot_status(Status)));
	connect(parser, SIGNAL(signal_connclosed()), SLOT(slot_connclosed()));
	connect(parser, SIGNAL(signal_talk(const QString&, const QString&, bool)), SLOT(slot_talk(const QString&, const QString&, bool)));
	connect(parser, SIGNAL(signal_checkbox(int, bool)), SLOT(slot_checkbox(int, bool)));
	connect(parser, SIGNAL(signal_addToObservationList(int)), SLOT(slot_addToObservationList(int)));
	connect(parser, SIGNAL(signal_channelinfo(int, const QString&)), SLOT(slot_channelinfo(int, const QString&)));
	connect(parser, SIGNAL(signal_matchrequest(const QString&, bool)), this, SLOT(slot_matchrequest(const QString&, bool)));
  	connect(parser, SIGNAL(signal_matchCanceled(const QString&)), this, SLOT(slot_removeMatchDialog(const QString&)));
	connect(parser, SIGNAL(signal_shout(const QString&, const QString&)), SLOT(slot_shout(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_room(const QString&, bool)),SLOT(slot_room(const QString&, bool)));
	connect(parser, SIGNAL(signal_addSeekCondition(const QString&,const QString&, const QString&, const QString&, const QString&)),this,
			SLOT(slot_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_clearSeekCondition()),this,SLOT(slot_clearSeekCondition()));
	connect(parser, SIGNAL(signal_cancelSeek()),this,SLOT(slot_cancelSeek()));
	connect(parser, SIGNAL(signal_SeekList(const QString&, const QString&)),this,SLOT(slot_SeekList(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_refresh(int)),this, SLOT(slot_refresh(int)));

	// doubleclick
	connect(ListView_games, SIGNAL(signal_doubleClicked(QTreeWidgetItem*)), this,
		SLOT(slot_click_games(QTreeWidgetItem*)));
	connect(ListView_players, SIGNAL(signal_doubleClicked(QTreeWidgetItem *)), this,
		SLOT(slot_click_players(QTreeWidgetItem*)));

	connect(ListView_players, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(slot_menu_players(const QPoint&)));
	connect(ListView_games, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(slot_menu_games(const QPoint&)));

	connect(whoOpenCheck, SIGNAL(toggled(bool)), this, SLOT(slot_whoopen(bool)));

	// restore last setting
	if (!setting)
	{
		setting = new Setting();
		setting->loadSettings();
	}

	initStatusBar(this);
	initActions();

	// Set menus
	//initmenus(this);
	// Sets toolbar
	initToolBar();                              //end add eb 5

	int i;
	QString s;
// ----
// BEGIN find outdated keys:
// ----

	if (setting->readIntEntry("VERSION") < SETTING_VERSION)
	{
		// version settings are not actual
		int s_v = setting->readIntEntry("VERSION");

		switch (s_v)
		{
			case 0:
				for (i = 1; i < 5; i++)
				{
					s = setting->readEntry("USER" + QString::number(i));
					if (s.length() > 0)
					{
						QString i_ = QString::number(i);
						setting->writeEntry("USER" + i_ + "_1", s.section(DELIMITER, 0, 0));
						setting->writeEntry("USER" + i_ + "_2", s.section(DELIMITER, 1, 1));
						setting->writeEntry("USER" + i_ + "_3", s.section(DELIMITER, 2, 2));
						// delete old label
						setting->writeEntry("USER" + i_, 0);
					}
				}
				setting->writeEntry("HOSTWINDOW", 0);

				i = 0;
				for (;;)
				{
					QString s = setting->readEntry("HOST" + QString::number(++i));
					if (s.isNull())
						break;

					// check if 4 delimiters in s
					if (s.contains(DELIMITER) == 4)
					{
						setting->writeEntry("HOST" + QString::number(i) + "a", s.section(DELIMITER, 0, 0));
						setting->writeEntry("HOST" + QString::number(i) + "b", s.section(DELIMITER, 1, 1));
						setting->writeIntEntry("HOST" + QString::number(i) + "c", s.section(DELIMITER, 2, 2).toInt());
						setting->writeEntry("HOST" + QString::number(i) + "d", s.section(DELIMITER, 3, 3));
						setting->writeEntry("HOST" + QString::number(i) + "e", s.section(DELIMITER, 4, 4));
					}
					// delete old hosts
					setting->writeEntry("HOST" + QString::number(i), QString());
				}
				break;

			default:
				break;
		}
	}
// ----
// END find
// ----

	// restore: hostlist
	i = 0;
	for (;;)
	{
		QString s = setting->readEntry("HOST" + QString::number(++i) + "a");
		if (s.isNull ())
			break;
		hostlist.append (new Host(setting->readEntry("HOST" + QString::number(i) + "a"),
					  setting->readEntry("HOST" + QString::number(i) + "b"),
					  setting->readIntEntry("HOST" + QString::number(i) + "c"),
					  setting->readEntry("HOST" + QString::number(i) + "d"),
					  setting->readEntry("HOST" + QString::number(i) + "e"),
					  setting->readEntry("HOST" + QString::number(i) + "f")));
	}
	std::sort (hostlist.begin (), hostlist.end (), [] (Host *a, Host *b) { return *a < *b; });

	// run slot command to initialize combobox and set host
	QString w = setting->readEntry("ACTIVEHOST");
	// int cb_connect
	slot_cbconnect(QString());
	if (!w.isNull ())
		// set host if available
		slot_cbconnect(w);

	//restore players list filters
	whoBox1->setCurrentIndex(setting->readIntEntry("WHO_1"));
	whoBox2->setCurrentIndex(setting->readIntEntry("WHO_2"));
	whoOpenCheck->setChecked(setting->readIntEntry("WHO_CB"));

	// restore size of client window
	s = setting->readEntry("CLIENTWINDOW");
	if (s.length() > 5)
	{
		QPoint p;
		p.setX(s.section(DELIMITER, 0, 0).toInt());
		p.setY(s.section(DELIMITER, 1, 1).toInt());
		QSize sz;
		sz.setWidth(s.section(DELIMITER, 2, 2).toInt());
		sz.setHeight(s.section(DELIMITER, 3, 3).toInt());
		resize(sz);
		move(p);
	}

	// restore splitter in client window
	s = setting->readEntry("CLIENTSPLITTER");
	if (s.length() > 5)
	{
		QList<int> w1, h1, w2;
		w1 << s.section(DELIMITER, 0, 0).toInt() << s.section(DELIMITER, 1, 1).toInt();
		h1 << s.section(DELIMITER, 2, 2).toInt() << s.section(DELIMITER, 3, 3).toInt();
		w2 << s.section(DELIMITER, 4, 4).toInt() << s.section(DELIMITER, 5, 5).toInt();
		//mainTable->s1->setSizes(w1);
    		s1->setSizes(w1);
		s2->setSizes(h1);
		s3->setSizes(w2);
	}

	// restore size of debug window
	s = setting->readEntry("DEBUGWINDOW");
	if (s.length() > 5)
	{
		view_p.setX(s.section(DELIMITER, 0, 0).toInt());
		view_p.setY(s.section(DELIMITER, 1, 1).toInt());
		view_s.setWidth(s.section(DELIMITER, 2, 2).toInt());
		view_s.setHeight(s.section(DELIMITER, 3, 3).toInt());
	}
	else
	{
		view_p = QPoint();
		view_s = QSize();
	}

	// restore size of menu window
	s = setting->readEntry("MENUWINDOW");
	if (s.length() > 5)
	{
		menu_p.setX(s.section(DELIMITER, 0, 0).toInt());
		menu_p.setY(s.section(DELIMITER, 1, 1).toInt());
		menu_s.setWidth(s.section(DELIMITER, 2, 2).toInt());
		menu_s.setHeight(s.section(DELIMITER, 3, 3).toInt());
	}
	else
	{
		menu_p = QPoint();
		menu_s = QSize();
	}

	// restore size of preferences window
	s = setting->readEntry("PREFWINDOW");
	if (s.length() > 5)
	{
		pref_p.setX(s.section(DELIMITER, 0, 0).toInt());
		pref_p.setY(s.section(DELIMITER, 1, 1).toInt());
		pref_s.setWidth(s.section(DELIMITER, 2, 2).toInt());
		pref_s.setHeight(s.section(DELIMITER, 3, 3).toInt());
	}
	else
	{
		pref_p = QPoint();
		pref_s = QSize();
	}

	// 'user' cmd instead of 'who'?
	setColumnsForExtUserInfo();

	// create qGo Interface for board handling
	qgoif = new qGoIF(this);
	// qGo Interface

	connect(parser, SIGNAL(signal_set_observe( const QString&)), qgoif, SLOT(set_observe(const QString&)));
	connect(parser, SIGNAL(signal_move(GameInfo*)), qgoif, SLOT(slot_move(GameInfo*)));
	connect(parser, SIGNAL(signal_move(Game*)), qgoif, SLOT(slot_move(Game*)));
	connect(parser, SIGNAL(signal_kibitz(int, const QString&, const QString&)), qgoif, SLOT(slot_kibitz(int, const QString&, const QString&)));
	connect(parser, SIGNAL(signal_title(const GameInfo *, const QString&)), qgoif, SLOT(slot_title(const GameInfo *, const QString&)));
	connect(parser, SIGNAL(signal_komi(const QString&, const QString&, bool)), qgoif, SLOT(slot_komi(const QString&, const QString&, bool)));
	connect(parser, SIGNAL(signal_freegame(bool)), qgoif, SLOT(slot_freegame(bool)));
	connect(parser, SIGNAL(signal_matchcreate(const QString&, const QString&)), qgoif, SLOT(slot_matchcreate(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_removestones(const QString&, const QString&)), qgoif, SLOT(slot_removestones(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_undo(const QString&, const QString&)), qgoif, SLOT(slot_undo(const QString&, const QString&)));
	connect(parser, SIGNAL(signal_result(const QString&, const QString&, bool, const QString&)), qgoif, SLOT(slot_result(const QString&, const QString&, bool, const QString&)));
	connect(parser, SIGNAL(signal_requestDialog(const QString&, const QString&, const QString&, const QString&)), qgoif, SLOT(slot_requestDialog(const QString&, const QString&, const QString&, const QString&)));
	connect(this, SIGNAL(signal_move(Game*)), qgoif, SLOT(slot_move(Game*)));
 	connect(this, SIGNAL(signal_computer_game(QNewGameDlg*)), qgoif, SLOT(slot_computer_game(QNewGameDlg*)));//SL added eb 12
	connect(qgoif, SIGNAL(signal_sendcommand(const QString&, bool)), this, SLOT(slot_sendcommand(const QString&, bool)));
	connect(qgoif, SIGNAL(signal_addToObservationList(int)), this, SLOT(slot_addToObservationList(int)));
	connect(qgo, SIGNAL(signal_updateFont()), this, SLOT(slot_updateFont()));
	connect(parser, SIGNAL(signal_timeAdded(int, bool)), qgoif, SLOT(slot_timeAdded(int, bool)));
	//connect(parser, SIGNAL(signal_undoRequest(const QString&)), qgoif, SLOT(slot_undoRequest(const QString&)));

	//gamestable
//	connect(ListView_players, SIGNAL(contentsMoving(int, int)), this, SLOT(slot_playerContentsMoving(int, int)));
	connect(ListView_games, SIGNAL(contentsMoving(int, int)), this, SLOT(slot_gamesContentsMoving(int, int)));

	// add menu bar and status bar
//	MainAppWidgetLayout->addWidget(menuBar, 0, 0);



/*
#ifndef QGO_NOSTYLES
	// set style - same as set up a board with qgoif
	int style = setting->readEntry("STYLE").toInt();
	if (style < 0 || style > 6)
	{
		setting->writeEntry("STYLE", "0");
		style = 0;
	}

 
  	
	switch (style)
	{
    case 0:
      //qApp->setStyle(NULL);
		  break;
    
		case 1:
			qApp->setStyle(new QWindowsStyle);
			break;

		case 2:
			qApp->setStyle(new QPlatinumStyle);
			break;

		case 3:
			qApp->setStyle(new QMotifStyle);
			break;

		case 4:
			qApp->setStyle(new QMotifPlusStyle);
			break;

#ifdef Q_WS_WIN
// there seems to be a problem within X11...
		case 5:
			qApp->setStyle(new QCDEStyle);
			break;

		case 6:
			qApp->setStyle(new QSGIStyle);
			break;
#endif

		case 6:
#if (QT_VERSION > 0x030102)
			qApp->setStyle(new QWindowsXPStyle);
#else
			qApp->setStyle(new QWindowsStyle);
#endif
			break;


		default:
			qWarning("Unrecognized style!");
	}
#endif
*/
	slot_updateFont();

	// install an event filter
	qApp->installEventFilter(this);
}

ClientWindow::~ClientWindow()
{
/*	delete qgoif;
	delete setting;
	delete telnetConnection;
	delete parser;
	delete statusUsers;
	delete statusBar;
*/
}

void ClientWindow::initStatusBar(QWidget* /*parent*/)
{
//	statusBar = new QStatusBar(parent);
//	statusBar->resize(-1, 20);
	statusBar()->show();
	statusBar()->setSizeGripEnabled(false);
	statusBar()->showMessage(tr("Ready."));  // Normal indicator

	// Standard Text instead of "message" cause WhatsThisButten overlaps
	statusMessage = new QLabel(statusBar());
	statusMessage->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusMessage->setText("");
	statusBar()->addWidget(statusMessage, 0, true);  // Permanent indicator
/*
	// What's this
	statusWhatsThis = new QLabel(statusBar);
	statusWhatsThis->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusWhatsThis->setText("WHATSTHIS");
	statusBar->addWidget(statusWhatsThis, 0, true);  // Permanent indicator
	QWhatsThis::whatsThisButton(statusWhatsThis);
*/
	// The users widget
	statusUsers = new QLabel(statusBar());
	statusUsers->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusUsers->setText(" P: 0 / 0 ");
	statusBar()->addWidget(statusUsers, 0, true);  // Permanent indicator
	QToolTip::add(statusUsers, tr("Current online players / watched players"));
	QWhatsThis::add(statusUsers, tr("Displays the number of current online players\nand the number of online players you are watching.\nA player you are watching has an entry in the 'watch player:' field."));

	// The games widget
	statusGames = new QLabel(statusBar());
	statusGames->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusGames->setText(" G: 0 / 0 ");
	statusBar()->addWidget(statusGames, 0, true);  // Permanent indicator
	QToolTip::add(statusGames, tr("Current online games / observed games + matches"));
	QWhatsThis::add(statusGames, tr("Displays the number of games currently played on this server and the number of games you are observing or playing"));

	// The server widget
	statusServer = new QLabel(statusBar());
	statusServer->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusServer->setText(" OFFLINE ");
	statusBar()->addWidget(statusServer, 0, true);  // Permanent indicator
	QToolTip::add(statusServer, tr("Current server"));
	QWhatsThis::add(statusServer, tr("Displays the current server's name or OFFLINE if you are not connected to the internet."));

	// The channel widget
	statusChannel = new QLabel(statusBar());
	statusChannel->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusChannel->setText("");
	statusBar()->addWidget(statusChannel, 0, true);  // Permanent indicator
	QToolTip::add(statusChannel, tr("Current channels and users"));
	QWhatsThis::add(statusChannel, tr("Displays the current channels you are in and the number of users inthere.\nThe tooltip text contains the channels' title and users' names"));

	// Online Time
	statusOnlineTime = new QLabel(statusBar());
	statusOnlineTime->setAlignment(Qt::AlignCenter | Qt::SingleLine);
	statusOnlineTime->setText(" 00:00 ");
	statusBar()->addWidget(statusOnlineTime, 0, true);  // Permanent indicator
	QToolTip::add(statusOnlineTime, tr("Online Time"));
	QWhatsThis::add(statusOnlineTime, tr("Displays the current online time.\n(A) -> auto answer\n(Hold) -> hold the line"));
}

void ClientWindow::timerEvent(QTimerEvent* e)
{
	// some variables for forcing send buffer and keep line established
//	static int counter = 3599;
	static int holdTheLine = true;
	static int tnwait = 0;
//	static int statusCnt = 0;
	static QString statusTxt = QString();
	static int imagecounter = 0;
	
	//qDebug( "timer event, id %d", e->timerId() );

	if (e->timerId() == seekButtonTimer)
	{	
		imagecounter = (imagecounter+1) % 4;
		toolSeek->setIconSet(QIcon(seekingIcon[imagecounter]));
		return;
	}

	if (tn_ready)
	{
		// case: ready to send
		tnwait = 0;
	}
	else if (tnwait < 2)
	{
		//qDebug(QString("%1: HoldTheLine SET: something has been sent").arg(statusOnlineTime->text()));
		// case: not ready to send, but maybe waiting for READY message
		tnwait++;
		// something was sent, so reset counter
		resetCounter();
		holdTheLine = true;
		autoAwayMessage = false;
	}
/*	else if (!tn_active)
	{
		qDebug("---> sending forced!!!");
		// case: not ready to send, waiting too long -> force sending
		tnwait = 0;
		set_tn_ready();
	}
	else
		qDebug("---> not forced because of tn_active");
*/
	// show status bar text for 5 seconds
/*	if (statusTxt != statusMessage->text())
	{
		statusTxt = statusMessage->text();
		statusCnt = 5;
	}
	if (statusCnt-- == 0)
		statusMessage->setText("");
*/
	if (counter % 300 == 0)
	{
		// 5 mins away -> set auto answering
		autoAwayMessage = true;
		if (counter == 0)
		{
			// send "ayt" every half hour anyway
			// new: reset timer after one hour of idle state
			//      -> if not observing a game!
			//qDebug(QString("%1 -> HoldTheLine: status = %2").arg(statusOnlineTime->text()).arg(holdTheLine));
			if (holdTheLine)
				sendcommand("ayt", false);

			// 12*5 min
			resetCounter();

			if (myAccount->num_observedgames == 0)
			{
				// nothing observing
				holdTheLine = false;
//qDebug(QString("%1: HoldTheLine END!").arg(statusOnlineTime->text()));
			}
			else
			{
//qDebug(QString("%1: HoldTheLine LENGTHENED: observing game...").arg(statusOnlineTime->text()));
			}
		}
		else if (myAccount->get_gsname() == IGS && holdTheLine)
		{
			sendcommand("ayt", false);
			qDebug() << statusOnlineTime->text() << " ayt";
		}
	}

	counter--;

	// display online time
	onlineCount++;
	int hr = onlineCount/3600;
	int min = (onlineCount % 3600)/60;
	int sec = onlineCount % 60;
	QString pre = " ";
	QString min_;
	QString sec_;

	if (min < 10)
		min_ = "0" + QString::number(min);
	else
		min_ = QString::number(min);

	if (sec < 10)
		sec_ = "0" + QString::number(sec);
	else
		sec_ = QString::number(sec);

	if (hr)
		pre += QString::number(hr) + "h ";

	statusOnlineTime->setText(pre + min_ + ":" + sec_ + " ");

	// some statistics
	QToolTip::remove(statusServer);
	QToolTip::add(statusServer, tr("Current server") + "\n" +
		tr("Bytes in:") + " " + QString::number(bytesIn) + "\n" +
		tr("Bytes out:") + " " + QString::number(bytesOut));
//	LineEdit_bytesIn->setText(QString::number(bytesIn));
//	LineEdit_bytesOut->setText(QString::number(bytesOut));

// DEBUG ONLY BEGIN ****
	// DEBUG display remaining time
	hr = counter/3600;
	min = (counter % 3600)/60;
	sec = counter % 60;
	if (autoAwayMessage)
		pre = "(A) ";
	else
		pre = " ";

	if (min < 10)
		min_ = "0" + QString::number(min);
	else
		min_ = QString::number(min);

	if (sec < 10)
		sec_ = "0" + QString::number(sec);
	else
		sec_ = QString::number(sec);

	if (hr)
		pre += QString::number(hr) + "h ";

	statusMessage->setText(pre + min_ + ":" + sec_ + (holdTheLine ? " Hold" : " "));
// DEBUG ONLY END ****
}

// slot_connect: emitted when connect button has toggled
void ClientWindow::slot_connect(bool b)
{
qDebug() << "connect " << (int)b;
	if (b)
	{
		// create instance of telnetConnection
		if (!telnetConnection)
			qFatal("No telnetConnection!");

		// connect to selected host
		telnetConnection->slotHostConnect();
	}
	else
	{
		// disconnect
		telnetConnection->slotHostQuit();
	}
}

// connection closed
void ClientWindow::slot_connclosed()
{
	// no Timers in offline mode!
	if (seekButtonTimer) {
		qDebug() << "killing seekButtonTimer " << seekButtonTimer;
		killTimer(seekButtonTimer);
	}
	if (oneSecondTimer) {
		qDebug() << "killing oneSecondTimer " << oneSecondTimer;
		killTimer(oneSecondTimer);
	}
	seekButtonTimer = oneSecondTimer = 0;

	// set to offline:
	myAccount->set_offline();
	//pb_connect->setChecked(false);
	toolConnect->setChecked(false);
	seekMenu->clear();
	slot_cancelSeek();

	// clear channel
	prepare_tables(CHANNELS);

	// remove boards
	qgoif->set_initIF();

	qDebug("slot_connclosed()");
	qDebug() << statusOnlineTime->text() << " -> slot_connclosed()";

	qgo->playConnectSound();

	// show current Server name in status bar
	statusServer->setText(" OFFLINE ");

	// set menu
	Connect->setEnabled(true);
	Disconnect->setEnabled(false);
	toolConnect->setChecked(false);
	toolConnect->setPixmap(disconnectedIcon);
	QToolTip::remove(toolConnect);
	QToolTip::add(toolConnect, tr("Connect with") + " " + cb_connect->currentText());
}

// close application
void ClientWindow::saveSettings()
{
	// save setting
	int i = 0;
	for (auto h: hostlist)
	{
		i++;
		setting->writeEntry("HOST" + QString::number(i) + "a", h->title());
		setting->writeEntry("HOST" + QString::number(i) + "b", h->host());
		setting->writeIntEntry("HOST" + QString::number(i) + "c", h->port());
		setting->writeEntry("HOST" + QString::number(i) + "d", h->loginName());
		setting->writeEntry("HOST" + QString::number(i) + "e", h->password());
		setting->writeEntry("HOST" + QString::number(i) + "f", h->codec());
	}
	for (;;)
	{
		QString s = setting->readEntry("HOST" + QString::number(++i) + "a");
		if (s.isNull ())
			break;
		// delete unused entries
		setting->writeEntry("HOST" + QString::number(i) + "a", QString());
		setting->writeEntry("HOST" + QString::number(i) + "b", QString());
		setting->writeEntry("HOST" + QString::number(i) + "c", QString());
		setting->writeEntry("HOST" + QString::number(i) + "d", QString());
		setting->writeEntry("HOST" + QString::number(i) + "e", QString());
		setting->writeEntry("HOST" + QString::number(i) + "f", QString());
	}

	// save current connection if at least one host exists
	if (!hostlist.isEmpty())
		setting->writeEntry("ACTIVEHOST",cb_connect->currentText());

	setting->writeEntry("CLIENTWINDOW",
		QString::number(pos().x()) + DELIMITER +
		QString::number(pos().y()) + DELIMITER +
		QString::number(size().width()) + DELIMITER +
		QString::number(size().height()));

	setting->writeEntry("CLIENTSPLITTER",
		QString::number(s1->sizes().first()) + DELIMITER +
		QString::number(s1->sizes().last()) + DELIMITER +
		QString::number(s2->sizes().first()) + DELIMITER +
		QString::number(s2->sizes().last()) + DELIMITER +
		QString::number(s3->sizes().first()) + DELIMITER +
		QString::number(s3->sizes().last()));

	if (DD)
		setting->writeEntry("DEBUGWINDOW",
			QString::number(DD->pos().x()) + DELIMITER +
			QString::number(DD->pos().y()) + DELIMITER +
			QString::number(DD->size().width()) + DELIMITER +
			QString::number(DD->size().height()));
//	DD = 0;

	if (menu_s.width() > 0)
		setting->writeEntry("MENUWINDOW",
			QString::number(menu_p.x()) + DELIMITER +
			QString::number(menu_p.y()) + DELIMITER +
			QString::number(menu_s.width()) + DELIMITER +
			QString::number(menu_s.height()));

	if (pref_s.width() > 0)
		setting->writeEntry("PREFWINDOW",
			QString::number(pref_p.x()) + DELIMITER +
			QString::number(pref_p.y()) + DELIMITER +
			QString::number(pref_s.width()) + DELIMITER +
			QString::number(pref_s.height()));

	setting->writeBoolEntry("EXTUSERINFO", extUserInfo);

	setting->writeIntEntry("WHO_1", whoBox1->currentIndex());
	setting->writeIntEntry("WHO_2", whoBox2->currentIndex());
	setting->writeBoolEntry("WHO_CB", whoOpenCheck->isChecked());

	// write settings to file
	setting->saveSettings();
}

// close application
void ClientWindow::quit()
{
	saveSettings();
	qApp->quit();
}

// distribute text from telnet session and local commands to tables
void ClientWindow::sendTextToApp(const QString &txt)
{
	static int store_sort_col = -1;
	static int store_games_sort_col = -1;
	static bool player7active = false;

	// put text to parser
 	InfoType it_ = parser->put_line(txt);

	// some statistics
	setBytesIn(txt.length()+2);

//	if (it_ != READY)
		// a input is being parsed -> wait until tn_active == false before sending new cmd
//		tn_active = true;

	// GAME7_END emulation:
	if (player7active && it_ != GAME7)
	{
		player7active = false;
		ListView_games->setSortingEnabled (true);
		if (store_games_sort_col != -1)
			ListView_games->sortItems (store_games_sort_col, Qt::AscendingOrder);
		store_games_sort_col = -1;
	}

	switch (it_)
	{
		case READY:
			// ok, telnet is ready to receive commands
//			tn_active = false;
      			currentCommand->txt="";
			if (!tn_wait_for_tn_ready && !tn_ready)
			{
				QTimer::singleShot(200, this, SLOT(set_tn_ready()));
				tn_wait_for_tn_ready = true;
			}
			sendTextFromApp(NULL);
		case WS:
			// ready or white space -> return
//      currentCommand->txt="";
			return;
			break;

		// echo entered command
		// echo server enter line
		case CMD:
			slot_message(txt);
			break;

		// set client mode
		case NOCLIENTMODE:
			set_sessionparameter("client", true);
			break;

		case YOUHAVEMSG:
			// normally no connection -> wait until logged in
	        	youhavemsg = true;
			break;

		case SERVERNAME:
			slot_message(txt);
			// clear send buffer
			do
			{
				// enable sending
				set_tn_ready();
			} while (sendTextFromApp(NULL) != 0);

			// check if tables are sorted
#if 0 && (QT_VERSION > 0x030006)
			if (ListView_players->sortColumn() == -1)
				ListView_players->setSorting(2);
			if (ListView_games->sortColumn() == -1)
				ListView_games->setSorting(2);
#endif

			switch(myAccount->get_gsname())
			{
				case IGS:
				{
					// IGS - check if client mode
					bool ok;
					/*int cmd_nr =*/ txt.section(' ', 0, 0).toInt(&ok);
					if (!ok)
						set_sessionparameter("client", true);

					// set quiet true; refresh players, games
					//if (myAccount->get_status() == GUEST)
					set_sessionparameter("quiet", true);
					//else
					// set id - only available if registerd; who knows why...
					sendcommand("id qGo " + QString(VERSION), true);
					sendcommand("toggle newrating");
//					if (setting->readBoolEntry("USE_NMATCH"))
//					{
// we wanted to allow user to disable 'nmatch', but IGS disables 'seek' with nmatch
					set_sessionparameter("nmatch",true);

						//temporaary settings to prevent use of Koryo BY on IGS (as opposed to canadian)
						//sendcommand("nmatchrange BWN 0-9 19-19 60-60 60-3600 25-25 0 0 0-0",false);
					send_nmatch_range_parameters();
//					}
					sendcommand("toggle newundo");
					sendcommand("toggle seek");
					sendcommand("seek config_list ");
					sendcommand("room");
						
					slot_refresh(11);
					slot_refresh(10);
					break;
				}
					
				default:
					set_sessionparameter("client", true);
					// set quiet false; refresh players, games
					//if (myAccount->get_status() == GUEST)
					set_sessionparameter("quiet", false);
					slot_refresh(11);
					if (myAccount->get_gsname() != CWS)
						slot_refresh(10);
					break;
			}

			// set menu
			Connect->setEnabled(false);
			Disconnect->setEnabled(true);
			toolConnect->setChecked(true);
			toolConnect->setPixmap(connectedIcon);
			QToolTip::remove(toolConnect);
			QToolTip::add(toolConnect, tr("Disconnect from") + " " + cb_connect->currentText());

			// quiet mode? if yes do clear table before refresh
			gamesListSteadyUpdate = ! setQuietMode->isChecked();
			playerListSteadyUpdate = ! setQuietMode->isChecked();


			// enable extended user info features
			setColumnsForExtUserInfo();

			// check for messages
			if (youhavemsg)
				sendcommand("message", false);

			// let qgo know which server
			qgoif->set_gsName(myAccount->get_gsname());
			// show current Server name in status bar
			statusServer->setText(" " + myAccount->svname + " ");

			// start timer: event every second
			onlineCount = 0;
			oneSecondTimer = startTimer(1000);
			// init shouts
			slot_talk("Shouts*", QString::null, false);
			
			qgo->playConnectSound();
			break;

		// end of 'who'/'user' cmd
		case PLAYER42_END:
		case PLAYER27_END:
			ListView_players->setSortingEnabled (true);
			if (store_sort_col != -1)
				ListView_players->sortItems (store_sort_col, Qt::AscendingOrder);

			if (myAccount->get_gsname()==IGS)
				ListView_players->showOpen(whoOpenCheck->isChecked());
			playerListEmpty = false;
			break;

		// skip table if initial table is to be loaded
		case PLAYER27_START:
		case PLAYER42_START:
			store_sort_col = ListView_players->sortColumn ();
			ListView_players->setSortingEnabled (false);

			if (playerListEmpty)
				prepare_tables(WHO);
			break;

		case GAME7_START:
			// "emulate" GAME7_END
			player7active = true;
			// disable sorting for fast operation; store sort column index
			// unfortunately there is not GAME7_END cmd, thus, it's emulated
			if (playerListEmpty)
			{
				// only if playerListEmpty, else PLAYERXX_END would not arise
				store_games_sort_col = ListView_games->sortColumn ();
				ListView_games->setSortingEnabled (false);
			}
			break;

		case ACCOUNT:
			// let qgo and parser know which account in case of setting something for own games
			qgoif->set_myName(myAccount->acc_name);
			parser->set_myname(myAccount->acc_name);
			break;


    case STATS:
        // we just received a players name as first line of stats -> create the dialog tab
        currentCommand->txt="stats";

       // if (!talklist.current())
	slot_talk( parser->get_statsPlayer()->name, QString::null,true);
        
        //else if (parser->get_statsPlayer()->name != talklist.current()->get_name())
        //    slot_talk( parser->get_statsPlayer()->name,0,true);

      break;
      
		case BEEP:
//			QApplication::beep();
			break;

		default:
			break;
	}
/*
	// skip player list and game list
	if (!DODEBUG)
		switch (it_)
		{
			case PLAYER42:
				if (!extUserInfo || myAccount->get_gsname() != IGS)
					slot_message(txt);
			case PLAYER:
			case GAME:
//			case WS:
			case READY:
			case GAME7:
			case PLAYER27:
			case MOVE:
			case SHOUT:
			case BEEP:
			case KIBITZ:
				return;
				break;

			default:
				break;
		}

	// show all in messages window, but players and games
	// Scroll at bottom of text, set cursor to end of line
	MultiLineEdit_messages->insertLine(txt);
*/
	qDebug() << txt;
}

// used for singleShot actions
void ClientWindow::set_tn_ready()
{
	tn_ready = true;
	tn_wait_for_tn_ready = false;
	sendTextFromApp(0);
}

// send text via telnet session; skipping empty string!
int ClientWindow::sendTextFromApp(const QString &txt, bool localecho)
{
	// implements a simple buffer
	int valid = txt.length();

	// some statistics
	if (valid)
		setBytesOut(valid+2);

	if (myAccount->get_status() == OFFLINE)
	{
		// skip all commands while not telnet connection
		sendTextToApp("Command skipped - no telnet connection: " + txt);
		// reset buffer
		sendBuffer.clear();
		return 0;
	}

	// check if telnet ready
	if (tn_ready)
	{
		sendBuf *s = sendBuffer.getFirst();

		if (s)
		{
			// send buffer cmd first; then put current cmd to buffer
			telnetConnection->sendTextFromApp(s->get_txt());
//qDebug("SENDBUFFER send: " + s->get_txt());

			// hold the line if cmd is sent; 'ayt' is autosend cmd
			if (s->get_txt().indexOf("ayt") != -1)
				resetCounter();
			if (s->get_localecho())
				sendTextToApp(CONSOLECMDPREFIX + QString(" ") + s->get_txt());
			tn_ready = false;

			// delete sent command from buffer 
      //currentCommand->txt = s->get_txt();
      sendBuffer.removeFirst();

      
			if (valid)
			{
				// append current command to send as soon as possible
				sendBuffer.append(new sendBuf(txt, localecho));
//qDebug("SENDBUFFER added: " + txt);
			}
		}
		else if (valid)
		{
			// buffer empty -> send direct
			telnetConnection->sendTextFromApp(txt);
      //currentCommand->txt = txt;
      
			if (!txt.contains("ayt"))
				resetCounter();
			if (localecho)
				sendTextToApp(CONSOLECMDPREFIX + QString(" ") + txt);
			tn_ready = false;

//qDebug("SENDBUFFER send direct: " + txt);
		}
	}
	else if (valid)
	{
//qDebug("SENDBUFFER added: " + txt);
		sendBuffer.append(new sendBuf(txt, localecho));
	}

	return sendBuffer.count();
}

// show command, send it, and tell parser
void ClientWindow::sendcommand(const QString &cmd, bool localecho)
 {
	QString testcmd = cmd;

	// for testing
	if (cmd.indexOf("#") == 0)
	{
		qDebug("detected TEST (#) command");
		testcmd = testcmd.remove(0, 1).trimmed();

		// help
		if (testcmd.length() <= 1)
		{
			sendTextToApp("local cmds available:\n"
				"#+dbgwin\t#+dbg\t\t#-dbg\n");
			return;
		}

		// detect internal commands
		if (testcmd.contains("+dbgwin"))
		{
			// show debug window
			DD->show();
			this->activateWindow();
		}
		else if (testcmd.contains("+dbg"))
		{
			// show debug window and activate debug mode
			qDebug("*** set Debug on ***");
			DD->show();
			DODEBUG = true;
			this->activateWindow();
		}
		else if (testcmd.contains("-dbg"))
		{
			// hide debug window and deactivate debug mode
			qDebug("*** set Debug off ***");
			DODEBUG = false;
			DD->hide();
		}

		sendTextToApp(testcmd);
		return;
	}

	// echo
	if (1 || localecho)
	{
		// add to Messages, anyway
		// Scroll at bottom of text, set cursor to end of line
		qDebug() << "CMD: " << cmd;
		slot_message(cmd,Qt::blue);
	}

	// send to Host
	sendTextFromApp(cmd, localecho);
}

// howver, sendcommand is no slot...
void ClientWindow::slot_sendcommand(const QString &cmd, bool localecho)
{
	sendcommand(cmd, localecho);
}

void ClientWindow::slot_toolbaractivated(const QString &cmd)
{
	// do some cmd lind checks for toolbar too
	bool valid_marker = cmd_valid;
	cmd_valid = true;
	slot_cmdactivated(cmd);
	cmd_valid = valid_marker;
}

// return pressed in edit line -> command to send
void ClientWindow::slot_cmdactivated(const QString &cmd)
{
	if (cmd.isNull ())
		return;

qDebug("cmd_valid: %i", (int)cmd_valid);
	// check if valid cmd -> cmd number risen
	if (cmd_valid)
	{
		// clear field, and restore the blank line at top
		cb_cmdLine->removeItem(1);
		cb_cmdLine->insertItem("",0);
		cb_cmdLine->clearEdit();

		// echo & send
		QString cmdLine = cmd;
		sendcommand(cmdLine.trimmed(),true);
		cmd_valid = false;

		// check for known commands
		if (cmdLine.mid(0,2).contains("ob"))
		{
			QString testcmd;

			qDebug("found cmd: observe");
			testcmd = cmdLine.section(' ', 1, 1);
			if (!testcmd.isEmpty())
			{
//				qgoif->set_observe(testcmd);
				sendcommand("games " + testcmd);
			}
		}
		if (cmdLine.mid(0,3).contains("who"))
		{
			// clear table if manually entered
			prepare_tables(WHO);
			playerListSteadyUpdate = false;
		}
		if (cmdLine.mid(0,5).contains("; \\-1") && myAccount->get_gsname() == IGS)
		{
			// exit all channels
			prepare_tables(CHANNELS);
		}
	}
}

// number of activated cmd
void ClientWindow::slot_cmdactivated_int(int)
{
	if (cb_cmdLine->count() > cmd_count)
	{
		cmd_count = cb_cmdLine->count();
		cmd_valid = true;
	}
	else
		cmd_valid = false;
}

// set session parameter
void ClientWindow::set_sessionparameter(QString par, bool val)
{
	QString value;
	if (val)
		value = " true";
	else
		value = " false";

	switch(myAccount->get_gsname())
	{
		// only toggling...
		case IGS:
			sendcommand("toggle " + par + value);
			break;
			
		default:
			sendcommand("set " + par + value);
			break;
	}
}

// select connection or create new one - combobox connection
void ClientWindow::slot_cbconnect(const QString &txt)
{
	QString text = txt;
	Host *h = NULL;
	int i=1;

	if (text.isNull())
	{
		// txt empty: update combobox - server table has been edited...
		// keep current host
		text = cb_connect->currentText();

		// refill combobox
		cb_connect->clear();
		for (auto h_: hostlist)
		{
			cb_connect->insertItem (h_->title());
			if (h_->title() == text)
			{
				i = cb_connect->count() - 1;
				h = h_;
			}
		}
	}
	else
	{
		for (auto h_: hostlist)
			if (h_->title() == text) {
				h = h_;
				i = hostlist.indexOf (h_);
				break;
			}
	}

	if (!h) {
		h = hostlist.first ();
		i = 0;
	}
	if (!h)
		return;

	// view selected host
	cb_connect->setCurrentIndex(i);

	// inform telnet about selected host
	QString lg = h->loginName();
	QString pw = h->password();
	telnetConnection->setHost(h->host(), lg, pw, h->port(), h->codec());
	if (toolConnect)
	{
		QToolTip::remove(toolConnect);
		QToolTip::add(toolConnect, tr("Connect with") + " " + cb_connect->currentText());
	}
}

// set checkbox status because of server info or because menu / checkbok toggled
void ClientWindow::slot_checkbox(int nr, bool val)
{
	// set checkbox (nr) to val
	switch (nr)
	{
		// open
		case 0:
			//toolOpen->setChecked(val);
			setOpenMode->setChecked(val);
			break;

		// looking
		case 1:
			//toolLooking->setChecked(val);
			setLookingMode->setChecked(val);
			break;

		// quiet
		case 2:
			//toolQuiet->setChecked(val);
			setQuietMode->setChecked(val);
			break;

		default:
			qWarning("checkbox doesn't exist");
			break;
	}
}

// checkbox looking cklicked
void ClientWindow::slot_cblooking()
{
	bool val = setLookingMode->isChecked();
//	setLookingMode->setChecked(val);
	set_sessionparameter("looking", val);
	if (val)
		// if looking then set open
		set_sessionparameter("open", true);
}


// checkbox open clicked
void ClientWindow::slot_cbopen()
{
	bool val = setOpenMode->isChecked();
//	setOpenMode->setChecked(val);
	set_sessionparameter("open", val);
	if (!val)
		// if not open then set close
		set_sessionparameter("looking", false);
}

// checkbox quiet clicked
void ClientWindow::slot_cbquiet()
{
	bool val = setQuietMode->isChecked();
	//setQuietMode->setChecked(val);
  //qDebug("bouton %b",toolQuiet->isChecked());
	set_sessionparameter("quiet", val);

	if (val)
	{
		// if 'quiet' button is once set to true the list is not reliable any longer
		gamesListSteadyUpdate = false;
		playerListSteadyUpdate = false;
	}
}


// switch between 'who' and 'user' cmds
void ClientWindow::setColumnsForExtUserInfo()
{
#if 0 /* @@@ Disabled for now.  Realistically, no one will use anything but IGS.  */
	if (!extUserInfo || (myAccount->get_gsname() != IGS) )
	{
		// set player table's columns to 'who' mode
		ListView_players->removeColumn(11);
		ListView_players->removeColumn(10);
		ListView_players->removeColumn(9);
		ListView_players->removeColumn(8);
		ListView_players->removeColumn(7);
	}
	else if ( ListView_players->columns()  < 9 )
	{
		// set player table's columns to 'user' mode
		// first: remove invisible column
		ListView_players->removeColumn(7);
		// second: add new columns
		ListView_players->addColumn(tr("Info"));
		ListView_players->addColumn(tr("Won"));
		ListView_players->addColumn(tr("Lost"));
		ListView_players->addColumn(tr("Country"));
		ListView_players->addColumn(tr("Match prefs"));
		ListView_players->setColumnAlignment(7, Qt::AlignRight);
		ListView_players->setColumnAlignment(8, Qt::AlignRight);
		ListView_players->setColumnAlignment(9, Qt::AlignRight);
	}
#endif
}

// switch between 'who' and 'user' cmds
void ClientWindow::slot_cbExtUserInfo()
{
	extUserInfo = setting->readBoolEntry("EXTUSERINFO");
	setColumnsForExtUserInfo();
}

void ClientWindow::slot_updateFont()
{
	// lists
	ListView_players->setFont(setting->fontLists);
	ListView_games->setFont(setting->fontLists);
	cb_connect->setFont(setting->fontLists);

	// comment fields
	MultiLineEdit2->selectAll(true);
	MultiLineEdit2->setCurrentFont(setting->fontConsole);
	MultiLineEdit2->selectAll(false);
	MultiLineEdit3->setCurrentFont(setting->fontComments); 
	cb_cmdLine->setFont(setting->fontComments);

	// standard
	setFont(setting->fontStandard);

	// init menu
	viewToolBar->setChecked(setting->readBoolEntry("MAINTOOLBAR"));
	if (setting->readBoolEntry("MAINMENUBAR"))
	{
		viewMenuBar->setChecked(false);
		viewMenuBar->setChecked(true);
	}
	viewStatusBar->setChecked(setting->readBoolEntry("MAINSTATUSBAR"));

	extUserInfo = setting->readBoolEntry("EXTUSERINFO");
	setColumnsForExtUserInfo();
//	slot_userDefinedKeysTextChanged();
}

// refresh button clicked
void ClientWindow::slot_refresh(int i)
{

  QString wparam = "" ;
	// refresh depends on selected page
	switch (i)
	{
		case 10:
			prepare_tables(WHO);
		case 0:
			// send "WHO" command
      			//set the params of "who command"
			if ((whoBox1->currentIndex() >1)  || (whoBox2->currentIndex() >1))
        		{
				wparam.append(whoBox1->currentIndex()==1 ? "9p" : whoBox1->currentText());
				if ((whoBox1->currentIndex())  && (whoBox2->currentIndex()))
					wparam.append("-");

				wparam.append(whoBox2->currentIndex()==1 ? "9p" : whoBox2->currentText());
         		}
			else if ((whoBox1->currentIndex())  || (whoBox2->currentIndex()))
        			wparam.append("1p-9p");
			else
				wparam.append(((myAccount->get_gsname() == IGS) ? "9p-BC" : " "));

			if (whoOpenCheck->isChecked())
				wparam.append(((myAccount->get_gsname() == WING) ? "O" : "o"));//wparam.append(" o");

			if (myAccount->get_gsname() == IGS )//&& extUserInfo)
				sendcommand(wparam.prepend("userlist "));
			else
				sendcommand(wparam.prepend("who "));

      			prepare_tables(WHO);
			break;

		case 11:
			prepare_tables(GAMES);
		case 1:
			// send "GAMES" command
			sendcommand("games");
//			prepare_tables(GAMES);
			// which games are watched right now
			// don't work correct at IGS !!!!
//			sendcommand("watching");
			break;

		default:
			break;
	}

}

void ClientWindow::slot_whoopen (bool checked)
{
	if (myAccount->get_gsname() == IGS)
		ListView_players->showOpen(checked);
}

// refresh games
void ClientWindow::slot_pbrefreshgames()
{
	if (gamesListSteadyUpdate)
		slot_refresh(1);
	else
	{
		// clear table in case of quiet mode
		slot_refresh(11);
		gamesListSteadyUpdate = !setQuietMode->isChecked(); //!CheckBox_quiet->isChecked();
	}
}

// refresh players
void ClientWindow::slot_pbrefreshplayers()
{
	if (playerListSteadyUpdate)
		slot_refresh(0);
	else
	{
		// clear table in case of quiet mode
		slot_refresh(10);
		playerListSteadyUpdate = !setQuietMode->isChecked(); //!CheckBox_quiet->isChecked();
	}
}

void ClientWindow::slot_gamesPopup(int i)
{
	QString t1 = lv_popupGames->text(1);
	QString t3 = lv_popupGames->text(3);
	QString player_nameW = t1.right(1) == "*" ? t1.left (t1.length() - 1) : t1;
	QString player_nameB = t3.right(1) == "*" ? t3.left( t3.length() -1 ) : t3;

	switch (i) {
	case 1:
		// observe
		if (lv_popupGames)
		{
			// set up game for observing
			//if (qgoif->set_observe(lv_popupGames->text(0)))
			{
				QString gameID = lv_popupGames->text(0);
				// if game is set up new -> get moves
				//   set game to observe
				sendcommand("observe " + gameID);

				// check if enough data here
				if (lv_popupGames->text(7).length() == 0)
				{
					// LGS sends the needed information, anyway
					if (myAccount->get_gsname() != LGS)
						sendcommand("games " + gameID);
				}
				else
				{
					//   complete game info
					Game g = lv_popupGames->get_game ();
					g.running = true;
					g.oneColorGo = false;

					emit signal_move(&g);
				}
			}
		}
		break;

	case 2:
		// stats
		slot_sendcommand("stats " + player_nameW, false);
		break;

	case 3:
		// stats
		slot_sendcommand("stats " + player_nameB, false);
		break;

	default:
		break;
	}
}

// doubleclick actions...
void ClientWindow::slot_click_games(QTreeWidgetItem *lv)
{
	// do actions if button clicked on item
	slot_mouse_games(3, lv);
	qDebug("games list double clicked");
}

void ClientWindow::slot_menu_games(const QPoint &pt)
{
	QTreeWidgetItem *item = ListView_games->itemAt (pt);
	// emulate right button
	if (item)
		slot_mouse_games(2, item);
	qDebug("games list double clicked");
}

// mouse click on ListView_games
void ClientWindow::slot_mouse_games(int button, QTreeWidgetItem *lv)
{
	static QMenu *puw = 0;

	// create popup window
	if (!puw)
	{
		puw = new Q3PopupMenu(0, 0);
		puw->insertItem(tr("observe"), this, SLOT(slot_gamesPopup(int)), 0, 1);
		puw->insertItem(tr("stats W"), this, SLOT(slot_gamesPopup(int)), 0, 2);
		puw->insertItem(tr("stats B"), this, SLOT(slot_gamesPopup(int)), 0, 3);
	}
	//puw->hide();

	// do actions if button clicked on item
	switch (button)
	{
		// left
		case 1:
			break;

		// right
		case 2:
			if (lv)
			{
				//puw->move(pt);
				//puw->show();
        			puw->popup( QCursor::pos() );
				// store selected lv
				lv_popupGames = static_cast<GamesTableItem*>(lv);
			}
			break;

		// first menue item if doubleclick
		case 3:
			if (lv)
			{
				// store selected lv
				lv_popupGames = static_cast<GamesTableItem*>(lv);

				slot_gamesPopup(1);
			}
			break;

		default:
			break;
	}
}


void ClientWindow::slot_removeMatchDialog(const QString &opponent)
{
	// set up match dialog
	// match_dialog()
	GameDialog *dlg = NULL;


	// seek dialog
		// look for same opponent
	dlg = matchlist.first();
	while (dlg && dlg->playerOpponentEdit->text() != opponent ) //&& dlg->playerBlackEdit->text() != opponent)
		dlg = matchlist.next();

	if (dlg)
	{
    		matchlist.remove();
    		delete dlg;
    	}
}   


void ClientWindow::slot_matchrequest(const QString &line, bool myrequest)
{
	// set up match dialog
	// match_dialog()
	GameDialog *dlg = NULL;
	QString opponent;

	// seek dialog
	if (!myrequest)
	{
		// match xxxx B 19 1 10
		opponent = line.section(' ', 1, 1);

		// play sound
		qgo->playMatchSound();
	}
	else
	{
		// xxxxx 4k*
		opponent = line.section(' ', 0, 0);
	}

	// look for same opponent
	dlg = matchlist.first();
	while (dlg && dlg->playerOpponentEdit->text() != opponent)// && dlg->playerBlackEdit->text() != opponent)
		dlg = matchlist.next();

	if (!dlg)
	{
		matchlist.insert(0, new GameDialog(0, tr("New Game")));
		dlg = matchlist.current();

		if (myAccount->get_gsname() == NNGS ||
			myAccount->get_gsname() == LGS)
		{
			// now connect suggest signal
			connect(parser,
				SIGNAL(signal_suggest(const QString&, const QString&, const QString&, const QString&, int)),
				dlg,
				SLOT(slot_suggest(const QString&, const QString&, const QString&, const QString&, int)));
		}

		connect(dlg,SIGNAL(signal_removeDialog(dlg)), this, SLOT(slot_removeDialog(dlg)));

		connect(dlg,
			SIGNAL(signal_sendcommand(const QString&, bool)),
			this,
			SLOT(slot_sendcommand(const QString&, bool)));

		connect(dlg,
			SIGNAL(signal_removeDialog(const QString&)),
			this,
			SLOT(slot_removeMatchDialog(const QString&)));


		connect(parser,
			SIGNAL(signal_matchcreate(const QString&, const QString&)),
			dlg,
			SLOT(slot_matchcreate(const QString&, const QString&)));
		connect(parser,
			SIGNAL(signal_notopen(const QString&)),
			dlg,
			SLOT(slot_notopen(const QString&)));
		connect(parser,
			SIGNAL(signal_komirequest(const QString&, int, float, bool)),
			dlg,
			SLOT(slot_komirequest(const QString&, int, float, bool)));
		connect(parser,
			SIGNAL(signal_opponentopen(const QString&)),
			dlg,
			SLOT(slot_opponentopen(const QString&)));
		connect(parser,
			SIGNAL(signal_dispute(const QString&, const QString&)),
			dlg,
			SLOT(slot_dispute(const QString&, const QString&)));

		connect(dlg,
			SIGNAL(signal_matchsettings(const QString&, const QString&, const QString&, assessType)),
			qgoif,
			SLOT(slot_matchsettings(const QString&, const QString&, const QString&, assessType)));
	}

	if (myrequest)
	{
		QString rk = line.section(' ', 1, 1);

		// set values
		/*dlg->playerWhiteEdit->setText(myAccount->acc_name);
		dlg->playerWhiteEdit->setReadOnly(true);
		dlg->playerBlackEdit->setText(opponent);
		dlg->playerBlackEdit->setReadOnly(false);
		*/
		dlg->playerOpponentEdit->setText(opponent);		
		dlg->playerOpponentEdit->setReadOnly(true);
		dlg->set_myName( myAccount->acc_name);

		// set my and opponent's rank for suggestion
		dlg->set_oppRk(rk);
		dlg->playerOpponentRkEdit->setText(rk);
		rk = myAccount->get_rank();
		dlg->set_myRk(rk);
		//dlg->playerWhiteRkEdit->setText(rk);
		dlg->set_gsName(myAccount->get_gsname());
		dlg->handicapSpin->setEnabled(false);

		dlg->buttonDecline->setDisabled(true);

		// my request: free/rated game is also requested
		//dlg->cb_free->setChecked(true);

		// teaching game:
		if (dlg->playerOpponentEdit->text() == myAccount->acc_name)
			dlg->buttonOffer->setText(tr("Teaching"));

		//nmatch settings from opponent 
		bool is_nmatch = false;

		// we want to make sure the player is selected, because the match request may come from an other command (match button on the tab dialog)
		QString lv_popup_name;

		Player p;
		if (lv_popupPlayer)
		{
			p = lv_popupPlayer->get_player ();
			const QString txt1 = lv_popupPlayer->text(1);
			lv_popup_name = (txt1.right(1) == "*" ? txt1.left(txt1.length() -1 ) : txt1);

			is_nmatch = p.nmatch && lv_popup_name == opponent;// && setting->readBoolEntry("USE_NMATCH");
		}

		dlg->set_is_nmatch(is_nmatch);

		if (is_nmatch && p.has_nmatch_settings ())
		{
			dlg->timeSpin->setRange((int)(p.nmatch_timeMin/60), (int)(p.nmatch_timeMax/60));
			dlg->byoTimeSpin->setRange((int)(p.nmatch_BYMin/60), (int)(p.nmatch_BYMax/60));
			dlg->handicapSpin->setRange(p.nmatch_handicapMin, p.nmatch_handicapMax);
		}
		else
		{
			dlg->timeSpin->setRange(0,60);
			dlg->byoTimeSpin->setRange(0,60);
			dlg->handicapSpin->setRange(0,9);
		}
		//no handicap with usual game requests
		dlg->handicapSpin->setEnabled(is_nmatch);
		dlg->play_nigiri_button->setEnabled(is_nmatch);

		//default settings
		dlg->boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
		dlg->timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
		dlg->byoTimeSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
		dlg->komiSpin->setValue(setting->readIntEntry("DEFAULT_KOMI")+0.5);

		dlg->slot_pbsuggest();
	}
	else
	{
		// match xxxx B 19 1 10 - using this line means: I am black!
		bool opp_plays_white = (line.section(' ', 2, 2) == "B");//QString(tr("B")));
		bool opp_plays_nigiri = (line.section(' ', 2, 2) == "N");

		QString handicap, size, time,byotime, byostones ;

		if (line.contains("nmatch"))
		{
			//specific behavior here : IGS nmatch not totally supported
			// disputes are hardly supported
			dlg->set_is_nmatch(true);
			handicap = line.section(' ', 3, 3);
			size = line.section(' ', 4, 4);
			time = line.section(' ', 5, 5);
			byotime = line.section(' ', 6, 6);
			byostones = line.section(' ', 7, 7);
			dlg->timeSpin->setRange(0,100);
			dlg->timeSpin->setValue(time.toInt()/60);
			dlg->byoTimeSpin->setRange(0,100);
			dlg->byoTimeSpin->setValue(byotime.toInt()/60);
			dlg->BY_label->setText(tr(" Byoyomi Time : (")+byostones+ tr(" stones)"));
			dlg->handicapSpin->setRange(0,9);
			dlg->handicapSpin->setValue(handicap.toInt());
			dlg->boardSizeSpin->setRange(1,19);
			dlg->boardSizeSpin->setValue(size.toInt());
		}
		else
		{
			dlg->set_is_nmatch(false);
			size = line.section(' ', 3, 3);
			time = line.section(' ', 4, 4);
			byotime = line.section(' ', 5, 5);
			dlg->timeSpin->setRange(0,1000);
			dlg->timeSpin->setValue(time.toInt());
			dlg->byoTimeSpin->setRange(0,100);
			dlg->byoTimeSpin->setValue(byotime.toInt());
			dlg->handicapSpin->setEnabled(false);
			dlg->play_nigiri_button->setEnabled(false);
			dlg->boardSizeSpin->setRange(1,19);
			dlg->boardSizeSpin->setValue(size.toInt());
		}

		QString rk = getPlayerRk(opponent);
		dlg->set_oppRk(rk);
		QString myrk = myAccount->get_rank();
		dlg->set_myRk(myrk);

		dlg->playerOpponentEdit->setText(opponent);
		dlg->playerOpponentEdit->setReadOnly(true);
		dlg->playerOpponentRkEdit->setText(rk);
		dlg->set_myName( myAccount->acc_name);

		if (opp_plays_white)
		{
/*			dlg->playerBlackEdit->setText(myAccount->acc_name);
			dlg->playerBlackEdit->setReadOnly(true);
			dlg->playerBlackRkEdit->setText(myAccount->get_rank());
			dlg->playerWhiteEdit->setText(opponent);
			dlg->playerWhiteEdit->setReadOnly(false);
			dlg->playerWhiteRkEdit->setText(rk);
*/
			dlg->play_black_button->setChecked(true);


		}
		else if (opp_plays_nigiri)
		{
/*			dlg->playerWhiteEdit->setText(myAccount->acc_name);
			dlg->playerWhiteEdit->setReadOnly(true);
			dlg->playerWhiteRkEdit->setText(myAccount->get_rank());
			dlg->playerBlackEdit->setText(opponent);
			dlg->playerBlackEdit->setReadOnly(false);
			dlg->playerBlackRkEdit->setText(rk);
*/
			dlg->play_nigiri_button->setChecked(true);
		}
		else
			dlg->play_white_button->setChecked(true);

		dlg->buttonDecline->setEnabled(true);
		dlg->buttonOffer->setText(tr("Accept"));
		dlg->buttonCancel->setDisabled(true);

	}

	dlg->slot_changed();
	dlg->show();
	dlg->activateWindow();
	dlg->raise();
}

int ClientWindow::toggle_player_state (const char *list, const QString &symbol)
{
	int change = 0;
	// toggle watch list
	QString cpy = setting->readEntry(list).simplified() + ";";
	QString line;
	QString name;
	bool found = false;
	int cnt = cpy.count(';');
	Player p = lv_popupPlayer->get_player ();

	for (int i = 0; i < cnt; i++)
	{
		name = cpy.section(';', i, i);
		if (!name.isEmpty())
		{
			if (name == p.name)
				// skip player if found
				found = true;
			else
				line += name + ";";
		}
	}
	if (!found)
	{
		// not found -> add to list
		line += lv_popupPlayer->text(1);
		// update player list
		if (p.mark != "M")
		{
			change = 1;
			p.mark = symbol;
		}
	}
	else if (line.length() > 0)
	{
		// skip one ";"
		line.truncate(line.length() - 1);
	}

	if (found)
	{
		if (p.mark != "M")
		{
			change = -1;
			p.mark = "";
		}
	}

	setting->writeEntry(list, line);

	lv_popupPlayer->update_player (p);
	return change;
}

// result of player popup
void ClientWindow::slot_playerPopup(int i)
{
	if (!lv_popupPlayer)
	{
		qWarning("*** programming error - no item selected");
		return;
	}

	// some invited players on IGS get a * after their name
	QString txt1 = lv_popupPlayer->text(1);
	QString player_name = (txt1.right(1) == "*" ? txt1.left( txt1.length() -1 ) : txt1);

	switch (i)
	{
		case 1 :
		case 11 :
			// match
			slot_matchrequest(player_name + " " + lv_popupPlayer->text(2), true);
			break;


		case 2:
    		case 3:
			// talk and stats at the same time
			slot_talk(player_name, QString::null, true);
      			//slot_sendcommand("stats " + lv_popupPlayer->text(1), false);
			break;

		//case 3:
			// stats
			//slot_sendcommand("stats " + lv_popupPlayer->text(1), false);
			//break;

		case 4:
			// stored games
			slot_sendcommand("stored " + player_name, false);
			break;

		case 5:
			// results
			slot_sendcommand("result " + player_name, false);
			break;

		case 12:
			// trail
			slot_sendcommand("trail " + player_name, false);
			break;

		case 6:
			myAccount->num_watchedplayers += toggle_player_state ("WATCH", "W");
			statusUsers->setText(" P: " + QString::number(myAccount->num_players) + " / " + QString::number(myAccount->num_watchedplayers) + " ");
			break;

		case 7:
			toggle_player_state ("EXCLUDE", "X");
			break;

		case 8:
			// rating
			if (myAccount->get_gsname() == IGS)
				slot_sendcommand("prob " + player_name, false);
			else
				slot_sendcommand("rating " + player_name,false);
			break;

		case 9:
		{
			// observe game

			bool found = false;

			// emulate mouse click
			QTreeWidgetItemIterator lv(ListView_games);
			for (QTreeWidgetItem *lvi; lvi = *lv;)
			{
				// compare game ids
				if (lv_popupPlayer->text(3) == lvi->text(0))
				{
					// emulate mouse button - doubleclick on games
					slot_mouse_games(3, lvi);
					found = true;
					break;
				}
				lv++;
			}

			if (!found)
			{
				// if not found -> load new data
				Game g;
				g.nr = lv_popupPlayer->text(3);
//				g.running = true;
//				slot_game(&g);

				// observe
//				if (qgoif->set_observe(g.nr))
					sendcommand("observe " + g.nr, false);
//				sendcommand("games " + g.nr, false);
			}

			break;
		}

		default:
			break;
	}
}

// doubleclick...
void ClientWindow::slot_click_players(QTreeWidgetItem *lv)
{
	// emulate right button
	slot_mouse_players(3, lv);
}
// move over ListView
/*void ClientWindow::slot_moveOver_players()
{
	qDebug("move over player list...");
} */
// mouse menus
void ClientWindow::slot_menu_players(const QPoint& pt)
{
	QTreeWidgetItem *item = ListView_players->itemAt (pt);
	// emulate right button
	if (item)
		slot_mouse_players(2, item);
}
// mouse click on ListView_players
void ClientWindow::slot_mouse_players(int button, QTreeWidgetItem *lv)
{
	static Q3PopupMenu *puw = 0;
	lv_popupPlayer = static_cast<PlayerTableItem*>(lv);
	// create popup window
	if (!puw)
	{
		puw = new Q3PopupMenu(0, 0);
		puw->insertItem(tr("match"), this, SLOT(slot_playerPopup(int)), 0, 1);
		puw->insertItem(tr("match within his prefs"), this, SLOT(slot_playerPopup(int)), 0, 11);
		puw->insertItem(tr("talk"), this, SLOT(slot_playerPopup(int)), 0, 2);
		puw->insertSeparator();
		puw->insertItem(tr("stats"), this, SLOT(slot_playerPopup(int)), 0, 3);
		puw->insertItem(tr("stored games"), this, SLOT(slot_playerPopup(int)), 0, 4);
		puw->insertItem(tr("results"), this, SLOT(slot_playerPopup(int)), 0, 5);
		puw->insertItem(tr("rating"), this, SLOT(slot_playerPopup(int)), 0, 8);
		puw->insertItem(tr("observe game"), this, SLOT(slot_playerPopup(int)), 0, 9);
		puw->insertItem(tr("trail"), this, SLOT(slot_playerPopup(int)), 0, 12);
		puw->insertSeparator();
		puw->insertItem(tr("toggle watch list"), this, SLOT(slot_playerPopup(int)), 0, 6);
		puw->insertItem(tr("toggle exclude list"), this, SLOT(slot_playerPopup(int)), 0, 7);

	}

	puw->setItemEnabled(11,lv_popupPlayer->get_player ().nmatch);
//puw->hide();

	// do actions if button clicked on item
	switch (button)
	{
		// left button
		case 1:
			if (lv)
			{
			}
			break;

		// right button
		case 2:
			if (lv)
			{
				/*QRect r = ListView_players->geometry();
				QPoint p = r.topLeft() + pt;
				puw->move(p);
				puw->show();*/
        			puw->popup( QCursor::pos() );
				// store selected lv
				//lv_popupPlayer = static_cast<PlayerTableItem*>(lv);
			}
			break;

		// first menu item if doubleclick
		case 3:
			if (lv)
			{
				// store selected lv
				//lv_popupPlayer = static_cast<PlayerTableItem*>(lv);

				slot_playerPopup(1);
			}
			break;

		default:
			break;
	}
}

// release Talk Tabs
void ClientWindow::slot_pbRelTabs()
{
	// seek dialog
	Talk *dlg = talklist.first();
	while (dlg)
	{
		if (dlg->get_name().indexOf('*') == -1)
		{
			TabWidget_mini_2->removePage(dlg->get_tabWidget());
			dlg->pageActive = false;
		}
		dlg = talklist.next();
	}
}

// release Talk Tabs
void ClientWindow::slot_pbRelOneTab(QWidget *w)
{
	// seek dialog
	Talk *dlg = talklist.first();
	while (dlg)
	{
		if (dlg->get_tabWidget() == w)
		{
			TabWidget_mini_2->removePage(w);
			dlg->pageActive = false;
			return;
		}
		dlg = talklist.next();
	}

}

 // handle chat boxes in a list
void ClientWindow::slot_talk(const QString &name, const QString &text, bool isplayer)
{
	static Talk *dlg;
	QString txt;
	bool bonus = false;
	bool autoAnswer = true;

	if (!text.isNull () && text != "@@@")
		// text given or player logged in
		txt = text;
	else if (text == "@@@" && isplayer)
	{
		// player logged out -> iplayer == false
		txt = tr("USER NOT LOGGED IN.");
		autoAnswer = false;
	}
	else
	{
		txt = "";
		autoAnswer = false;
	}

	// dialog recently used?
	if (dlg && dlg->get_name() == name)
		dlg->write(txt);
	else if (!name.isEmpty() && name != tr("msg*"))
	{
		// seek dialog
		dlg = talklist.first();
		while (dlg != NULL && dlg->get_name() != name)
			dlg = talklist.next();

		// not found -> create new dialog
		if (!dlg)
		{
			talklist.insert(0, new Talk(name, 0, isplayer));
			dlg = talklist.current();
			connect(dlg, SIGNAL(signal_talkto(QString&, QString&)), this, SLOT(slot_talkto(QString&, QString&)));
			connect(dlg, SIGNAL(signal_matchrequest(const QString&,bool)), this, SLOT(slot_matchrequest(const QString&,bool)));

			// make new multiline field
			TabWidget_mini_2->addTab(dlg->get_tabWidget(), dlg->get_name());

			if (name != tr("Shouts*"))
				TabWidget_mini_2->showPage(dlg->get_tabWidget());

			dlg->pageActive = true;
			connect(dlg->get_le(), SIGNAL(returnPressed()), dlg, SLOT(slot_returnPressed()));
			connect(dlg, SIGNAL(signal_pbRelOneTab(QWidget*)), this, SLOT(slot_pbRelOneTab(QWidget*)));


			//if (!name.isEmpty() && name != tr("Shouts*") && currentCommand->get_txt() !="stats")
			if (!name.isEmpty() && isplayer)
				slot_sendcommand("stats " + name, false);    // automatically request stats
		}

		CHECK_PTR(dlg);
		dlg->write(txt);

		// play sound on new created dialog
		bonus = true;
	}

	// check if it was a channel message
	autoAnswer &= (isplayer && autoAwayMessage && !name.contains('*') && (!text.isEmpty () && text[0] == '>'));
	if (autoAnswer)
	{
		// send when qGo is NOT the active application - TO DO
		sendcommand("tell " + name + " [I'm away right now]");
	}

	if (!dlg->pageActive)
	{
		TabWidget_mini_2->addTab(dlg->get_tabWidget(), dlg->get_name());
		dlg->pageActive = true;
		TabWidget_mini_2->showPage(dlg->get_tabWidget());
		
	}

	// play a sound - not for shouts
	if ((!text.isEmpty() && text[0] == '>' && bonus || !dlg->get_le()->hasFocus()) && !name.contains('*'))
	{
		qgo->playTalkSound();

		// set cursor to last line
		//dlg->get_mle()->setCursorPosition(dlg->get_mle()->lines(), 999); //eb16
		dlg->get_mle()->append("");                                        //eb16
		//dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

		// write time stamp
		MultiLineEdit3->append(statusOnlineTime->text() + " " + name + (autoAnswer ? " (A)" : ""));
	}
	else if (name == tr("msg*"))
	{
		qgo->playTalkSound();

		// set cursor to last line
//		dlg->get_mle()->setCursorPosition(dlg->get_mle()->numLines(), 999); //eb16
		dlg->get_mle()->append(""); //eb16
//		dlg->get_mle()->removeParagraph(dlg->get_mle()->paragraphs()-2);   //eb16

		// write time stamp
		MultiLineEdit3->append(tr("Message") + ": " + text);
	}
}

// talk dialog -> return pressed
void ClientWindow::slot_talkto(QString &receiver, QString &txt)
{
	// echo
	if (txt.length())
	{
		switch (myAccount->get_gsname())
		{
			case IGS:
			{
				bool ok;
				// test if it's a number -> channel
				/*int nr =*/ receiver.toInt(&ok);
				if (ok)
					// yes, channel talk
					sendcommand("yell " + txt, false);
				else if (receiver.contains('*'))
					sendcommand("shout " + txt, false);
				else
					sendcommand("tell " + receiver + " " + txt, false);
			}
				break;

			default:
				// send tell command w/o echo
				if (receiver.contains('*'))
					sendcommand("shout " + txt, false);
				else
					sendcommand("tell " + receiver + " " + txt, false);
				break;
		}

		// lokal echo in talk window
		slot_talk(receiver, "-> " + txt, true);
	}
}

// set players to be watched
void ClientWindow::slot_watchplayer(const QString &txt)
{
	if (txt.length() > 0)
	{
		watch = ";" + txt + ";";
	}
	else
		watch = QString();
}

// set players to be excluded
void ClientWindow::slot_excludeplayer(const QString &txt)
{
	if (txt.length() > 0)
	{
		exclude = ";" + txt + ";";
	}
	else
		exclude = QString();
}

// open a local board       ??? Do we need this ?
void ClientWindow::slot_localBoard()
{
	openLocalBoard();
}

// open a local board, size 19x19, skip questions
void ClientWindow::slot_local19()
{
	openLocalBoard("/19/");
}

// open a local board
void ClientWindow::slot_preferences()
{
	dlgSetPreferences ();
}

void ClientWindow::reStoreWindowSize(QString strKey, bool store)
{
	if (store)
	{
		// ALT-<num>
		setting->writeEntry("CLIENTWINDOW_" + strKey,
			QString::number(pos().x()) + DELIMITER +
			QString::number(pos().y()) + DELIMITER +
			QString::number(size().width()) + DELIMITER +
			QString::number(size().height()));

		setting->writeEntry("CLIENTSPLITTER_" + strKey,
			QString::number(s1->sizes().first()) + DELIMITER +
			QString::number(s1->sizes().last()) + DELIMITER +
			QString::number(s2->sizes().first()) + DELIMITER +
			QString::number(s2->sizes().last()) + DELIMITER +
			QString::number(s3->sizes().first()) + DELIMITER +
			QString::number(s3->sizes().last()));
      //QString::number(mainTable->s3->sizes().last()));
      
		statusBar()->showMessage(tr("Window size saved.") + " (" + strKey + ")");
	}
	else
	{
		// restore size of client window
		QString s = setting->readEntry("CLIENTWINDOW_" + strKey);
		if (s.length() > 5)
		{
			QPoint p;
			p.setX(s.section(DELIMITER, 0, 0).toInt());
			p.setY(s.section(DELIMITER, 1, 1).toInt());
			QSize sz;
			sz.setWidth(s.section(DELIMITER, 2, 2).toInt());
			sz.setHeight(s.section(DELIMITER, 3, 3).toInt());
			resize(sz);
			move(p);
		}

		// restore splitter in client window
		s = setting->readEntry("CLIENTSPLITTER_" + strKey);
		if (s.length() > 5)
		{
			QList<int> w1, h1, w2;
			w1 << s.section(DELIMITER, 0, 0).toInt() << s.section(DELIMITER, 1, 1).toInt();
			h1 << s.section(DELIMITER, 2, 2).toInt() << s.section(DELIMITER, 3, 3).toInt();
			w2 << s.section(DELIMITER, 4, 4).toInt() << s.section(DELIMITER, 5, 5).toInt();
			s1->setSizes(w1);
			s2->setSizes(h1);
			s3->setSizes(w2);
		}

		statusBar()->showMessage(tr("Window size restored.") + " (" + strKey + ")");
	}
}
/*
// set Cursor to last position
void ClientWindow::slot_tabWidgetMainChanged(QWidget *w)
{
	if (w->child("MultiLineEdit_messages"))
	{
		MultiLineEdit_messages->setCursorPosition(MultiLineEdit_messages->numLines(), 999);
		MultiLineEdit_messages->insertLine("");
		MultiLineEdit_messages->removeLine(MultiLineEdit_messages->numLines()-2);
		return;
	}
	if (w->child("MultiLineEdit2"))
	{
		MultiLineEdit2->setCursorPosition(MultiLineEdit2->numLines(), 999);
		MultiLineEdit2->insertLine("");
		MultiLineEdit2->removeLine(MultiLineEdit2->numLines()-2);
	}
}
*/
bool ClientWindow::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() == QEvent::KeyPress)
	{
//qDebug(QString("eventFilter: %1").arg(ev->type()));
//qDebug(QString("eventFilter: obj = %1, class = %2, parent = %3").arg(obj->name()).arg(obj->className()).arg(obj->parent() ? obj->parent()->name() : "0"));
	    QKeyEvent *keyEvent = (QKeyEvent *) ev;
	    int key = keyEvent->key();

//qDebug(QString("eventFilter: keyPress %1").arg(key));

	    if (key >= Qt::Key_0 && key <= Qt::Key_9)
	    {
//qDebug("eventFilter: keyPress -> 0..9");
			QString strKey = QString::number(key - Qt::Key_0);

			if (obj == cb_cmdLine || obj->parent() && obj->parent() == cb_cmdLine || obj == this)
			{
				if (keyEvent->state() & Qt::AltButton)
				{
qDebug("eventFilter: keyPress -> Alt + 0..9");
					// store sizes
					reStoreWindowSize(strKey, true);
					return true;
				}
				else if (keyEvent->state() & Qt::ControlButton)
				{
qDebug("eventFilter: keyPress -> Control + 0..9");
					// restore sizes
					reStoreWindowSize(strKey, false);
					return true;
				}
			}
		}
		else if (key == Qt::Key_F1)
		{
			// help
			qgo->openManual();
		}
	}

	return false;
}

/*
void ClientWindow::keyReleaseEvent(QKeyEvent *e)
{
	if (!(e->state() & AltButton || e->state() & ControlButton))
	{
		// release Keyboard
		releaseKeyboard();
	}
}
*/

// key pressed
void ClientWindow::keyPressEvent(QKeyEvent *e)
{
/*
	if (e->state() & AltButton && e->state() & ControlButton)
	{
		// AltGr Key
//		releaseKeyboard();
	}
	else if (e->state() & AltButton || e->state() & ControlButton)
	{
		// get all keystrokes until both alt and control button release
//		grabKeyboard();
	}

	// check for window resize command = number button
	if (e->key() >= Key_0 && e->key() <= Key_9)
	{
		QString strKey = QString::number(e->key() - Key_0);

		if (e->state() & AltButton)
		{
			// store sizes
			reStoreWindowSize(strKey, true);
		}
		else if (e->state() & ControlButton)
		{
			// restore sizes
			reStoreWindowSize(strKey, false);
		}

		// cut last digit from cb_cmdLine
		if (e->state() & ControlButton || e->state() & AltButton)
		{
			QString tmpTxt = cb_cmdLine->currentText();
			tmpTxt.truncate(tmpTxt.length() - 1);
			cb_cmdLine->changeItem(tmpTxt, cb_cmdLine->currentIndex());
		}

		e->accept();
		return;
	}
*/
	switch (e->key())
	{
		case Qt::Key_Up:  // Scroll up in history
			qDebug("UP");
/*			if (historyCounter > 0)
				historyCounter --;
			if (historyCounter > historyList->count())
				historyCounter = historyList->count();
			LineEdit1->setText(historyList->operator[](historyCounter));
*/			break;

		case Qt::Key_Down:  // Scroll down in history
			qDebug("DOWN");
/*			historyCounter ++;
			if (historyCounter > historyList->count())
				historyCounter = historyList->count();
			LineEdit1->setText(historyList->operator[](historyCounter));
*/			break;

		case Qt::Key_PageUp:
			qDebug("PAGE UP");
			// TODO: pageUp/pageDown are protected.  :(
			// MultiLineEdit1->pageUp(); 
			break;

		case Qt::Key_PageDown:
			qDebug("PAGE DOWN");
			// MultiLineEdit1->pageDown();
			break;

		case Qt::Key_F1:
			// help
			qgo->openManual();
			break;

		case Qt::Key_Escape:
			cb_cmdLine->setFocus();
			cb_cmdLine->setCurrentIndex(0);
			break;

		default:
			e->ignore();
			break;
	}
	e->accept();
} 

/*
void ClientWindow::initmenus(QWidget *parent)                              
{ 
}
*/

void ClientWindow::initActions()
{

// signals and slots connections
	connect( cb_cmdLine, SIGNAL( activated(int) ), this, SLOT( slot_cmdactivated_int(int) ) );
	connect( cb_cmdLine, SIGNAL( activated(const QString&) ), this, SLOT( slot_cmdactivated(const QString&) ) );

	ListView_games->setWhatsThis (tr("Table of games\n\n"
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

	ListView_players->setWhatsThis (tr("Table of players\n\n"
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


	/*
	* Menu File
	*/
	connect(fileNewBoard, SIGNAL(activated()), this, SLOT(slotFileNewBoard()));
	connect(fileNew, SIGNAL(activated()), this, SLOT(slotFileNewGame()));
	connect(fileOpen, SIGNAL(activated()), this, SLOT(slotFileOpen()));
	connect(computerPlay, SIGNAL(activated()), this, SLOT(slotComputerPlay()));
	connect(fileQuit, SIGNAL(activated()), this, SLOT(quit()));

	/*
	* Menu Connexion
	*/
	connect(Connect, SIGNAL(activated()), this, SLOT(slotMenuConnect()));
	connect(Disconnect, SIGNAL(activated()), this, SLOT(slotMenuDisconnect()));
	Disconnect->setEnabled(false);
	connect(editServers, SIGNAL(activated()), this, SLOT(slotMenuEditServers()));
  
	/*
	* Menu Settings
	*/
	connect(setPreferences, SIGNAL(activated()), this, SLOT(slot_preferences()));

	/*
	* Menu View
	*/
	connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));
	connect(viewMenuBar, SIGNAL(toggled(bool)), this, SLOT(slotViewMenuBar(bool)));
	viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar."));
	connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));
    	
	/*
	* Menu Help
	*/
	connect(helpManual, SIGNAL(activated()), this, SLOT(slotHelpManual()));
	connect(helpSoundInfo, SIGNAL(activated()), this, SLOT(slotHelpSoundInfo()));
	connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));
	connect(helpAboutQt, SIGNAL(activated()), this, SLOT(slotHelpAboutQt()));
	connect(helpNewVersion, SIGNAL(activated()), this, SLOT(slotNewVersion()));
}
  
void ClientWindow::initToolBar()
{

	QToolButton *tb;


	//connect( cb_connect, SIGNAL( activated(const QString&) ), this, SLOT( slot_cbconnect(const QString&) ) );

	//  toolConnect->setText(tr("Connect with") + " " + cb_connect->currentText()); 
	//connect( toolConnect, SIGNAL( toggled(bool) ), this, SLOT( slot_connect(bool) ) );  //end add eb 5
  
    
	QIcon  OIC,OIC2, OIC3 ;//= new QIconSet;

	OIC.setPixmap ( NotOpenIcon, QIcon::Automatic, QIcon::Normal, QIcon::Off);
	OIC.setPixmap ( OpenIcon, QIcon::Automatic, QIcon::Normal, QIcon::On );

	setOpenMode->setIconSet(OIC);

	OIC2.setPixmap (NotLookingIcon, QIcon::Automatic, QIcon::Normal, QIcon::Off);
	OIC2.setPixmap (LookingIcon, QIcon::Automatic, QIcon::Normal, QIcon::On );
	setLookingMode->setIconSet(OIC2);

	OIC.setPixmap ( NotQuietIcon, QIcon::Automatic, QIcon::Normal, QIcon::Off);
	OIC.setPixmap ( QuietIcon, QIcon::Automatic, QIcon::Normal, QIcon::On);
	setQuietMode->setIconSet(OIC);
  
	OIC3.setPixmap ( NotSeekingIcon, QIcon::Automatic, QIcon::Normal, QIcon::Off);
	OIC3.setPixmap ( seekingIcon[0], QIcon::Automatic, QIcon::Normal, QIcon::On);

	toolSeek->setIconSet(OIC3);
	seekMenu = new Q3PopupMenu();
	toolSeek->setPopup(seekMenu);
	toolSeek->setPopupDelay(1);

	whatsThis = QWhatsThis::createAction (this);
	whatsThis->addTo (Toolbar);

	//added the icons
	refreshPlayers->setIconSet(QIcon(RefreshPlayersIcon));
	refreshGames->setIconSet(QIcon(RefreshGamesIcon));
	fileNew->setIconSet(QIcon(fileNewIcon));
	fileNewBoard->setIconSet(QIcon(fileNewboardIcon));
	fileOpen->setIconSet(QIcon(fileOpenIcon));
	fileQuit->setIconSet(QIcon(exitIcon));
	computerPlay->setIconSet(QIcon(ComputerPlayIcon));
	Connect->setIconSet(QIcon(connectedIcon));
	Disconnect->setIconSet(QIcon(disconnectedIcon));
	helpManual->setIconSet(QIcon(manualIcon));
	setPreferences->setIconSet(QIcon(prefsIcon));
	setIcon(qgoIcon);
}
// SLOTS


void ClientWindow::slotHelpManual()
{
	setting->qgo->openManual();
}

void ClientWindow::slotHelpSoundInfo()
{
	// show info
	setting->qgo->testSound(true);
}

void ClientWindow::slotHelpAbout()
{
	setting->qgo->slotHelpAbout();
}

void ClientWindow::slotHelpAboutQt()
{
	QMessageBox::aboutQt(this);
}

void ClientWindow::slotNewVersion()
{
	QMessageBox::warning(this ,PACKAGE,NEWVERSIONWARNING);
}


void ClientWindow::slotFileNewBoard()
{
	setting->qgo->addBoardWindow();
}

void ClientWindow::slotFileNewGame()
{   
  MainWindow *w = setting->qgo->addBoardWindow() ;
	w->slotFileNewGame();
}

void ClientWindow::slotFileOpen()
{
	//if (!checkModified())
	//	return;
	QString fileName(QFileDialog::getOpenFileName(setting->readEntry("LAST_DIR"),
		tr("SGF Files (*.sgf *.SGF);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)"), this));
	if (fileName.isEmpty())
		return;

	MainWindow *w = setting->qgo->addBoardWindow() ; 
	w->doOpen(fileName, w->getFileExtension(fileName));
}

void ClientWindow::slotComputerPlay()
{
#ifndef Q_OS_MACX
	QString s =  setting->readEntry("COMPUTER_PATH");

	if (setting->readEntry("COMPUTER_PATH").isNull() ||
	setting->readEntry("COMPUTER_PATH").isEmpty())
	{
		QMessageBox::warning(this, PACKAGE, tr("You did not set the Computer program path !"));
		dlgSetPreferences(3);
		return;
	}
#endif
	QNewGameDlg * dlg = new QNewGameDlg(this);
	if(!(dlg->exec()== QDialog::Accepted))
		return;

	emit signal_computer_game(dlg);
}


void ClientWindow::slotMenuConnect()
{
	// push button
	toolConnect->toggle();             //SL added eb 5
}    

void ClientWindow::slotMenuDisconnect()
{
	// push button
	toolConnect->toggle();              //SL added eb 5
}

void ClientWindow::slotMenuEditServers()
{
	dlgSetPreferences(4);
} 

void ClientWindow::slotViewStatusBar(bool toggle)
{
	if (!toggle)
		statusBar()->hide();
	else
		statusBar()->show();

	setting->writeBoolEntry("MAINSTATUSBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void ClientWindow::slotViewMenuBar(bool toggle)
{
#if 0
	if (!toggle)
		menuBar->hide();
	else
		menuBar->show();
#endif
	setting->writeBoolEntry("MAINMENUBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void ClientWindow::slotViewToolBar(bool toggle)  
{
	if (!toggle)
		Toolbar->hide();
	else
		Toolbar->show();

	setting->writeBoolEntry("MAINTOOLBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void ClientWindow::slot_statsPlayer(Player *p)
{
	 Talk *dlg;
  
  if (!p->name.isEmpty())
	  {
		// seek dialog
		  dlg = talklist.first();
		  while (dlg != NULL && dlg->get_name() != p->name)
		  	dlg = talklist.next();

	  	//  found 
	  	if (dlg)
        {
         dlg->stats_rating->setText(p->rank);
         dlg->stats_info->setText(p->info);
         dlg->stats_default->setText(p->extInfo);
	 dlg->stats_wins->setText(p->won);
	 dlg->stats_loss->setText(p->lost );
	 dlg->stats_country->setText(p->country);
         dlg->stats_playing->setText(p->play_str);
         dlg->stats_rated->setText(p->rated);
         dlg->stats_address->setText(p->address);
	 
	 // stored either idle time or last access	 
	 dlg->stats_idle->setText(p->idle);
	 dlg->Label_Idle->setText(!p->idle.isEmpty() && p->idle.at(0).isDigit() ? "Idle :": "Last log :");

	 
        }   
    }


  
}  

void ClientWindow::slot_room(const QString& room, bool b)
{
	//do we already have the same room number in list ?
	if (RoomList->findItem(room.left(3), Q3ListView::BeginsWith ))
		return;
	//so far, we just create the room if it is open
	if (!b)
		RoomList->insertItem (room,  -1 );
	
	//RoomList->item(RoomList->count()-1)->setSelectable(!b);		
}	

void ClientWindow::slot_enterRoom(const QString& room)
{
	sendcommand("join " + room);

	if (room == "0")
		statusBar()->showMessage(tr("rooms left"));
	else
		statusBar()->showMessage(tr("Room ")+ room);

	//refresh the players table
	slot_refresh(0);
}

void ClientWindow::slot_leaveRoom()
{
	slot_enterRoom("0");
}	


void ClientWindow::slot_RoomListClicked(Q3ListBoxItem *qli)
{
	slot_enterRoom(qli->text().section(":",0,0));
}	

void ClientWindow::slot_addSeekCondition(const QString& a, const QString& b, const QString& c, const QString& d, const QString& )
{
	QString time_condition ;
	
	time_condition = QString::number(int(b.toInt() / 60)) + " min + " + QString::number(int(c.toInt() / 60)) + " min / " + d + " stones";

	seekMenu->insertItem(time_condition, this, SLOT(slot_seek(int)), 0, a.toInt());
}


void ClientWindow::slot_clearSeekCondition()
{
	seekMenu->clear();
}

void ClientWindow::slot_seek(bool b)
{
//qDebug("seek button pressed : status %i", (int)b);

	//if the button was just pressed on, we have already used the popup menu : nothing to do
	if (b)
		return;

	sendcommand("seek entry_cancel",false);
}

void ClientWindow::slot_cancelSeek()
{

	toolSeek->setChecked(false);
	toolSeek->setPopup(seekMenu);
	toolSeek->setPopupDelay(1);
	toolSeek->setIconSet(QIcon(NotSeekingIcon));
	killTimer(seekButtonTimer);
	seekButtonTimer = 0;
}

void ClientWindow::slot_seek(int i)
{

	toolSeek->setChecked(true);
	toolSeek->setPopup(NULL);

	//seek entry 1 19 5 3 0
	QString send_seek = 	"seek entry " + 
				QString::number(i) + 
				" 19 " ;

	//childish, but we want to have an animated icon there
	seekButtonTimer = startTimer(200);

	switch (cb_seek_handicap->currentIndex())
	{
		case 0 :
			send_seek.append("1 1 0");
			break ;

		case 1 :
			send_seek.append("2 2 0");
			break ;
		
		case 2 :
			send_seek.append("5 5 0");
			break ;

		case 3 :
			send_seek.append("9 9 0");
			break ;

		case 4 :
			send_seek.append("0 9 0");
			break ;	

		case 5 :
			send_seek.append("9 0 0");
			break ;
	}
	
	sendcommand(send_seek,false);

}

void ClientWindow::slot_SeekList(const QString& player, const QString& condition)
{
	QString Rk = getPlayerRk(player);
	QString hop = Rk.right(1);
	if ((Rk.right(1) != "?") && (Rk.right(1) != "+"))
		Rk.append(" ");

	slot_message(player.leftJustified(15,' ',true) + Rk.rightJustified(5) +  " : " + condition);
}

/*
* on IGS, sends the 'nmatch'time, BY, handicap ranges
* command syntax : "nmatchrange 	BWN 	0-9 19-19	 60-60 		60-3600 	25-25 		0 0 0-0"
*				(B/W/ nigiri)	Hcp Sz	   Main time (secs)	BY time (secs)	BY stones	Koryo time
*/
void ClientWindow::send_nmatch_range_parameters()
{
	
	if ((myAccount->get_gsname() != IGS) || (myAccount->get_status() == OFFLINE))
		return ;

	QString c = "nmatchrange ";
	c.append(setting->readBoolEntry("NMATCH_BLACK") ? "B" : "");
	c.append(setting->readBoolEntry("NMATCH_WHITE") ? "W" : "");
	c.append(setting->readBoolEntry("NMATCH_NIGIRI") ? "N" : "");
	c.append(" 0-");
	c.append(QString::number(setting->readIntEntry("NMATCH_HANDICAP")));
	c.append(" ");
	c.append(QString::number(setting->readIntEntry("DEFAULT_SIZE")));
	c.append("-19 ");
	c.append(QString::number(setting->readIntEntry("DEFAULT_TIME")*60));
	c.append("-");
	c.append(QString::number(setting->readIntEntry("NMATCH_MAIN_TIME")*60));
	c.append(" ");
	c.append(QString::number(setting->readIntEntry("DEFAULT_BY")*60));
	c.append("-");
	c.append(QString::number(setting->readIntEntry("NMATCH_BYO_TIME")*60));
	c.append(" 25-25 0 0 0-0");
	
	sendcommand(c, true);
}

void ClientWindow::dlgSetPreferences(int tab)
{
	PreferencesDialog dlg(this, tr("preferences"), true);

	if (tab >= 0)
	{
		// set to default tab - no check for Qt 2.3.x
		if (dlg.tabWidget->count() <= tab+1)
			dlg.tabWidget->setCurrentPage(tab);
	}

	// Interface tab
//	dlg.styleListBox->setCurrentIndex(setting->readIntEntry("STYLE"));
//	dlg.woodListBox->setCurrentIndex(setting->readIntEntry("SKIN"));
	dlg.LineEdit_goban->setText(setting->readEntry("SKIN"));
	dlg.LineEdit_Table->setText(setting->readEntry("SKIN_TABLE"));
	dlg.languageComboBox->insertStringList(setting->getAvailableLanguages());
	dlg.languageComboBox->setCurrentIndex(setting->convertLanguageCodeToNumber());
	dlg.commentComboBox->setCurrentIndex(setting->readIntEntry("VIEW_COMMENT"));
//	dlg.stonesShadowCheckBox->setChecked(setting->readBoolEntry("STONES_SHADOW"));
	dlg.radioButtonStones_2D->setChecked((setting->readIntEntry("STONES_LOOK")==1));
	dlg.radioButtonStones_3D->setChecked((setting->readIntEntry("STONES_LOOK")==2));
	dlg.radioButtonStone_real->setChecked((setting->readIntEntry("STONES_LOOK")==3));

//	dlg.stonesShellsCheckBox->setChecked(setting->readBoolEntry("STONES_SHELLS"));
	dlg.stoneSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_STONE"));
	dlg.autoplaySoundCheckBox->setChecked(setting->readBoolEntry("SOUND_AUTOPLAY"));
	dlg.talkSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_TALK"));
	dlg.matchSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_MATCH"));
	dlg.passSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_PASS"));
	dlg.gameEndSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_GAMEEND"));
	dlg.timeSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_TIME"));
	dlg.saySoundCheckBox->setChecked(setting->readBoolEntry("SOUND_SAY"));
	dlg.enterSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_ENTER"));
	dlg.leaveSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_LEAVE"));
	dlg.disConnectSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_DISCONNECT"));
	dlg.connectSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_CONNECT"));
	dlg.variationComboBox->setCurrentIndex(setting->readIntEntry("VAR_GHOSTS"));
	dlg.coordsCheckBox->setChecked(setting->readBoolEntry("BOARD_COORDS"));
	dlg.sgfCoordsCheckBox->setChecked(setting->readBoolEntry("SGF_BOARD_COORDS"));
	dlg.toolTipCoordsCheckBox->setChecked(setting->readBoolEntry("BOARD_COORDS_TIP"));
	dlg.cursorCheckBox->setChecked(setting->readBoolEntry("CURSOR"));
	dlg.smallerMarksCheckBox->setChecked(setting->readBoolEntry("SMALL_MARKS"));
	dlg.boldMarksCheckBox->setChecked(setting->readBoolEntry("BOLD_MARKS"));
	dlg.adjustFontSizeCheckBox->setChecked(setting->readBoolEntry("ADJ_FONT"));
	//dlg.smallerStonesCheckBox->setChecked(setting->readBoolEntry("SMALL_STONES"));
	dlg.tooltipsCheckBox->setChecked(!(setting->readBoolEntry("TOOLTIPS")));
	dlg.timerComboBox->setCurrentIndex(setting->readIntEntry("TIMER_INTERVAL"));
	dlg.BYTimeSpin->setValue(setting->readIntEntry("BY_TIMER"));
	dlg.sgfTimeTagsCheckBox->setChecked(setting->readBoolEntry("SGF_TIME_TAGS"));
	dlg.sliderCheckBox->setChecked(setting->readBoolEntry("SLIDER"));
	dlg.sidebarCheckBox->setChecked(setting->readBoolEntry("SIDEBAR"));
	dlg.sidebarLeftCheckBox->setChecked(setting->readBoolEntry("SIDEBAR_LEFT"));
//	dlg.rememberFontCheckBox->setChecked(setting->readBoolEntry("REM_FONT"));
	dlg.variableFontCheckBox->setChecked(setting->readBoolEntry("VAR_FONT"));
	dlg.antiClickoCheckBox->setChecked(setting->readBoolEntry("ANTICLICKO"));

	// Import tab
	dlg.blackStoneEdit->setText(QString(QChar(setting->charset->blackStone)));
	dlg.whiteStoneEdit->setText(QString(QChar(setting->charset->whiteStone)));
	dlg.starPointEdit->setText(QString(QChar(setting->charset->starPoint)));
	dlg.emptyPointEdit->setText(QString(QChar(setting->charset->emptyPoint)));
	dlg.hBorderEdit->setText(QString(QChar(setting->charset->hBorder)));
	dlg.vBorderEdit->setText(QString(QChar(setting->charset->vBorder)));
	dlg.addBrotherCheckBox->setChecked(setting->addImportAsBrother);

	// SGF Loading tab
	dlg.rememberDirCheckBox->setChecked(setting->readBoolEntry("REM_DIR"));
	dlg.codecListBox->setCurrentItem(setting->readIntEntry("CODEC"));

	// Client Window tab
	dlg.LineEdit_watch->setText(setting->readEntry("WATCH"));
	dlg.LineEdit_exclude->setText(setting->readEntry("EXCLUDE"));
	dlg.CheckBox_extUserInfo->setChecked(setting->readBoolEntry("EXTUSERINFO"));
//	dlg.CheckBox_useNmatch->setChecked(setting->readBoolEntry("USE_NMATCH"));
	dlg.checkBox_Nmatch_Black->setChecked(setting->readBoolEntry("NMATCH_BLACK"));
	dlg.checkBox_Nmatch_White->setChecked(setting->readBoolEntry("NMATCH_WHITE"));
	dlg.checkBox_Nmatch_Nigiri->setChecked(setting->readBoolEntry("NMATCH_NIGIRI"));
	dlg.HandicapSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_HANDICAP"));
	dlg.timeSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_MAIN_TIME"));
	dlg.BYSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_BYO_TIME"));

	dlg.LineEdit_computer->setText(setting->readEntry("COMPUTER_PATH")); //SL added eb 12
	dlg.computerButtonWhite->setChecked(setting->readBoolEntry("COMPUTER_WHITE"));
	dlg.computerButtonBlack->setChecked(setting->readBoolEntry("COMPUTER_BLACK"));
	dlg.humanButtonWhite->setChecked(!setting->readBoolEntry("COMPUTER_WHITE"));
	dlg.humanButtonBlack->setChecked(!setting->readBoolEntry("COMPUTER_BLACK"));
	dlg.computerSizeSpin->setValue(setting->readIntEntry("COMPUTER_SIZE"));
	dlg.computerHandicapSpin->setValue(setting->readIntEntry("COMPUTER_HANDICAP"));

	// Go Server tab
	dlg.boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
	dlg.timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
	dlg.BYSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
	dlg.komiSpinDefault->setValue(setting->readIntEntry("DEFAULT_KOMI"));
	dlg.automaticNegotiationCheckBox->setChecked(setting->readBoolEntry("DEFAULT_AUTONEGO"));
	dlg.CheckBox_autoSave->setChecked(setting->readBoolEntry("AUTOSAVE"));
	dlg.CheckBox_autoSave_Played->setChecked(setting->readBoolEntry("AUTOSAVE_PLAYED"));
	/*
	// do some size changes
	QSize qs = dlg.pb_add->minimumSizeHint();
	if (dlg.pb_new->minimumSizeHint().width() > qs.width())
		qs = dlg.pb_new->minimumSizeHint();
	if (dlg.pb_delete->minimumSizeHint().width() > qs.width())
		qs = dlg.pb_delete->minimumSizeHint();

	if (qs.width() > 0)
	{
		dlg.pb_add->setMaximumSize(qs);
		dlg.pb_new->setMaximumSize(qs);
		dlg.pb_delete->setMaximumSize(qs);
	}
	dlg.pb_add->resize( QSize().expandedTo(qs));
	dlg.pb_new->resize( QSize().expandedTo(qs));
	dlg.pb_delete->resize( QSize().expandedTo(qs));
*/
	if (dlg.exec() == QDialog::Accepted)
	{
		preferencesSave(&dlg);
		preferencesAccept();
	}
}

bool ClientWindow::preferencesSave(PreferencesDialog *dlg)
{
	ASSERT (dlg);

//	setting->writeIntEntry("STYLE", dlg->styleListBox->currentIndex());
//	setting->writeIntEntry("SKIN", dlg->woodListBox->currentIndex());
	setting->writeEntry("SKIN", dlg->LineEdit_goban->text());
	setting->writeEntry("SKIN_TABLE", dlg->LineEdit_Table->text());
	setting->writeEntry("LANG", setting->convertNumberToLanguage(dlg->languageComboBox->currentIndex()));
	setting->writeIntEntry("VIEW_COMMENT", dlg->commentComboBox->currentIndex());
//	setting->writeBoolEntry("STONES_SHADOW", dlg->stonesShadowCheckBox->isChecked());
//	setting->writeBoolEntry("STONES_SHELLS", dlg->stonesShellsCheckBox->isChecked());
	int i = 3;
	if ( dlg->radioButtonStones_2D->isChecked())
		i=1;
	else if ( dlg->radioButtonStones_3D->isChecked())
		i=2;
	setting->writeIntEntry("STONES_LOOK", i);

	setting->writeBoolEntry("SOUND_STONE", dlg->stoneSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_AUTOPLAY", dlg->autoplaySoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_TALK", dlg->talkSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_MATCH", dlg->matchSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_GAMEEND", dlg->passSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_PASS", dlg->gameEndSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_TIME", dlg->timeSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_SAY", dlg->saySoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_ENTER", dlg->enterSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_LEAVE", dlg->leaveSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_DISCONNECT", dlg->disConnectSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_CONNECT", dlg->connectSoundCheckBox->isChecked());
	setting->writeIntEntry("VAR_GHOSTS", dlg->variationComboBox->currentIndex());
	setting->writeBoolEntry("BOARD_COORDS", dlg->coordsCheckBox->isChecked());
	setting->writeBoolEntry("SGF_BOARD_COORDS", dlg->sgfCoordsCheckBox->isChecked());
	setting->writeBoolEntry("SLIDER", dlg->sliderCheckBox->isChecked());
	setting->writeBoolEntry("SIDEBAR", dlg->sidebarCheckBox->isChecked());
	setting->writeBoolEntry("SIDEBAR_LEFT", dlg->sidebarLeftCheckBox->isChecked());
	setting->writeBoolEntry("BOARD_COORDS_TIP", dlg->toolTipCoordsCheckBox->isChecked());
	setting->writeBoolEntry("CURSOR", dlg->cursorCheckBox->isChecked());
	setting->writeBoolEntry("SMALL_MARKS", dlg->smallerMarksCheckBox->isChecked());
	setting->writeBoolEntry("BOLD_MARKS", dlg->boldMarksCheckBox->isChecked());
	setting->writeBoolEntry("ADJ_FONT", dlg->adjustFontSizeCheckBox->isChecked());
	//setting->writeBoolEntry("SMALL_STONES", dlg->smallerStonesCheckBox->isChecked());
	setting->writeBoolEntry("TOOLTIPS", !(dlg->tooltipsCheckBox->isChecked()));
	setting->writeIntEntry("BY_TIMER", dlg->BYTimeSpin->text().toInt());
	setting->writeIntEntry("TIMER_INTERVAL", dlg->timerComboBox->currentIndex());
	setting->writeBoolEntry("SGF_TIME_TAGS", dlg->sgfTimeTagsCheckBox->isChecked());
	setting->charset->blackStone = dlg->blackStoneEdit->text().at(0).latin1();
	setting->charset->whiteStone = dlg->whiteStoneEdit->text().at(0).latin1();
	setting->charset->starPoint = dlg->starPointEdit->text().at(0).latin1();
	setting->charset->emptyPoint = dlg->emptyPointEdit->text().at(0).latin1();
	setting->charset->hBorder = dlg->hBorderEdit->text().at(0).latin1();
	setting->charset->vBorder = dlg->vBorderEdit->text().at(0).latin1();
	setting->addImportAsBrother = dlg->addBrotherCheckBox->isChecked();
	setting->writeBoolEntry("REM_DIR", dlg->rememberDirCheckBox->isChecked());
	setting->writeBoolEntry("ANTICLICKO", dlg->antiClickoCheckBox->isChecked());
	setting->writeBoolEntry("VAR_FONT", dlg->variableFontCheckBox->isChecked());
	setting->writeIntEntry("CODEC", dlg->codecListBox->currentItem());

	// Client Window Tab
	setting->writeEntry("WATCH", dlg->LineEdit_watch->text());
	setting->writeEntry("EXCLUDE", dlg->LineEdit_exclude->text());
	setting->writeBoolEntry("EXTUSERINFO", dlg->CheckBox_extUserInfo->isChecked());
//	setting->writeBoolEntry("USE_NMATCH", dlg->CheckBox_useNmatch->isChecked());

	//Checks wether the nmatch parameters have been modified, in order to send a new nmatchrange command
	if( 	(setting->readBoolEntry("NMATCH_BLACK") != dlg->checkBox_Nmatch_Black->isChecked()) ||
		(setting->readBoolEntry("NMATCH_WHITE") != dlg->checkBox_Nmatch_White->isChecked()) ||
		(setting->readBoolEntry("NMATCH_NIGIRI") != dlg->checkBox_Nmatch_Nigiri->isChecked()) ||
		(setting->readIntEntry("NMATCH_MAIN_TIME") != dlg->timeSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("NMATCH_BYO_TIME") != dlg->BYSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("NMATCH_HANDICAP") != dlg->HandicapSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_SIZE") != dlg->boardSizeSpin->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_TIME") != dlg->timeSpin->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_BY") != dlg->BYSpin->text().toInt()) )
			setting->nmatch_settings_modified = true;

	setting->writeBoolEntry("NMATCH_BLACK", dlg->checkBox_Nmatch_Black->isChecked());
	setting->writeBoolEntry("NMATCH_WHITE", dlg->checkBox_Nmatch_White->isChecked());
	setting->writeBoolEntry("NMATCH_NIGIRI", dlg->checkBox_Nmatch_Nigiri->isChecked());
	setting->writeIntEntry("NMATCH_MAIN_TIME", dlg->timeSpin_Nmatch->text().toInt());
	setting->writeIntEntry("NMATCH_BYO_TIME", dlg->BYSpin_Nmatch->text().toInt());
	setting->writeIntEntry("NMATCH_HANDICAP", dlg->HandicapSpin_Nmatch->text().toInt());


	setting->writeIntEntry("DEFAULT_SIZE", dlg->boardSizeSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_TIME", dlg->timeSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_BY", dlg->BYSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_KOMI", dlg->komiSpinDefault->text().toFloat());
	setting->writeBoolEntry("DEFAULT_AUTONEGO", dlg->automaticNegotiationCheckBox->isChecked());
	setting->writeBoolEntry("AUTOSAVE", dlg->CheckBox_autoSave->isChecked());
	setting->writeBoolEntry("AUTOSAVE_PLAYED", dlg->CheckBox_autoSave_Played->isChecked());

	// Computer Tab
	setting->writeEntry("COMPUTER_PATH", dlg->LineEdit_computer->text());
	setting->writeBoolEntry("COMPUTER_WHITE", dlg->computerButtonWhite->isChecked());
	setting->writeBoolEntry("COMPUTER_BLACK", dlg->computerButtonBlack->isChecked());
	setting->writeIntEntry("COMPUTER_SIZE", dlg->computerSizeSpin->text().toInt());
	setting->writeIntEntry("COMPUTER_HANDICAP", dlg->computerHandicapSpin->text().toInt());

	return true;
}

bool ClientWindow::preferencesAccept()
{
	// Interface tab
	//setApplicationStyle();

	// Update all boards with settings
	setting->qgo->updateAllBoardSettings();
	setting->qgo->updateFont();

	if (setting->nmatch_settings_modified)
	{
		setting->cw->send_nmatch_range_parameters();
		setting->nmatch_settings_modified = false ;
	}

	return true;//result;
}
