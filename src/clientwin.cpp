/*
 *   mainwin.cpp - qGoClient's main window (cont. in tables.cpp)
 */

#include <QMainWindow>
#include <QFileDialog>
#include <QWhatsThis>
#include <QMessageBox>
#include <QTimerEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>

#include "clientwin.h"
#include "defines.h"
#include "playertable.h"
#include "gamestable.h"
#include "gamedialog.h"
#include "qgo_interface.h"
#include "komispinbox.h"
#include "igsconnection.h"
#include "mainwindow.h"
#include "newaigamedlg.h"
#include "ui_helpers.h"
#include "msg_handler.h"

#include <qaction.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qevent.h>
#include <qstatusbar.h>
#include <qtooltip.h>
#include <qtoolbutton.h>
#include <qicon.h>

ClientWindow *client_window;

/*
 *   Clientwindow = MainWidget
 */


ClientWindow::ClientWindow(QMainWindow *parent)
	: QMainWindow( parent), seekButtonTimer (0), oneSecondTimer (0)
{
	setupUi(this);

	seekingIcon[0] = QIcon (":/ClientWindowGui/images/clientwindow/seeking0.png");
	seekingIcon[1] = QIcon (":/ClientWindowGui/images/clientwindow/seeking1.png");
	seekingIcon[2] = QIcon (":/ClientWindowGui/images/clientwindow/seeking2.png");
	seekingIcon[3] = QIcon (":/ClientWindowGui/images/clientwindow/seeking3.png");
	NotSeekingIcon = QIcon (":/ClientWindowGui/images/clientwindow/not_seeking.png");

	// init

	setting->cw = this;
	setWindowIcon (QIcon (":/ClientWindowGui/images/clientwindow/qgo.png"));
	myAccount = new Account(this);

	cmd_count = 0;

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

	cb_cmdLine->clear();
	cb_cmdLine->addItem("");

 	escapeFocus = new QAction(this);
	escapeFocus->setShortcut(Qt::Key_Escape);
	connect(escapeFocus, &QAction::triggered, [=] () { setFocus (); });
	cb_cmdLine->addAction (escapeFocus);

	// create instance of telnetConnection
	telnetConnection = new TelnetConnection(this, ListView_players, ListView_games);

	// doubleclick
	connect(ListView_games, SIGNAL(signal_doubleClicked(QTreeWidgetItem*)), this,
		SLOT(slot_click_games(QTreeWidgetItem*)));
	connect(ListView_players, SIGNAL(signal_doubleClicked(QTreeWidgetItem *)), this,
		SLOT(slot_click_players(QTreeWidgetItem*)));

	connect(ListView_players, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(slot_menu_players(const QPoint&)));
	connect(ListView_games, SIGNAL(customContextMenuRequested(const QPoint&)),
		this, SLOT(slot_menu_games(const QPoint&)));

	connect(whoOpenCheck, &QCheckBox::toggled, this, &ClientWindow::slot_whoopen);

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

	i = 0;
	for (;;)
	{
		QString s = setting->readEntry("ENGINE" + QString::number(++i) + "a");
		if (s.isNull ())
			break;
		m_engines.append (new Engine(s, setting->readEntry("ENGINE" + QString::number(i) + "b"),
					     setting->readEntry("ENGINE" + QString::number(i) + "c"),
					     setting->readEntry("ENGINE" + QString::number(i) + "d"),
					     setting->readBoolEntry("ENGINE" + QString::number(i) + "e"),
					     setting->readEntry("ENGINE" + QString::number(i) + "f")));
		bool updated = false;
		for (auto it: m_engines)
			if (it->analysis () && it->boardsize ().isEmpty ()) {
				updated = true;
				it->set_boardsize ("19");
			}
		if (updated)
			QMessageBox::information (this, PACKAGE,
						  tr("Engine configuration updated\n"
						     "Analysis engines now require a board size to be set, assuming 19 for existing entries."));
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
	QString scrkey = screen_key (this);
	s = setting->readEntry("CLIENTWINDOW_" + scrkey);
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
	s = setting->readEntry("CLIENTSPLITTER_" + scrkey);
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

	// create parser and connect signals
	parser = new Parser(this, qgoif);
	connect(parser, &Parser::signal_player, this, &ClientWindow::slot_player);
	connect(parser, &Parser::signal_statsPlayer, this, &ClientWindow::slot_statsPlayer);
	connect(parser, &Parser::signal_game, this, &ClientWindow::slot_game);
	connect(parser, &Parser::signal_message, this, &ClientWindow::slot_message);
	connect(parser, &Parser::signal_svname, this, &ClientWindow::slot_svname);
	connect(parser, &Parser::signal_accname, this, &ClientWindow::slot_accname);
	connect(parser, &Parser::signal_status, this, &ClientWindow::slot_status);
	connect(parser, &Parser::signal_connclosed, this, &ClientWindow::slot_connclosed);
	connect(parser, &Parser::signal_talk, this, &ClientWindow::slot_talk);
	connect(parser, &Parser::signal_checkbox, this, &ClientWindow::slot_checkbox);
	connect(parser, &Parser::signal_addToObservationList, this, &ClientWindow::slot_addToObservationList);
	connect(parser, &Parser::signal_channelinfo, this, &ClientWindow::slot_channelinfo);
	connect(parser, &Parser::signal_matchrequest, this, &ClientWindow::slot_matchrequest);
	connect(parser, &Parser::signal_matchCanceled, this, &ClientWindow::slot_removeMatchDialog);
	connect(parser, &Parser::signal_shout, this, &ClientWindow::slot_shout);
	connect(parser, &Parser::signal_room, this, &ClientWindow::slot_room);
	connect(parser, &Parser::signal_addSeekCondition, this, &ClientWindow::slot_addSeekCondition);
	connect(parser, &Parser::signal_clearSeekCondition, this, &ClientWindow::slot_clearSeekCondition);
	connect(parser, &Parser::signal_cancelSeek, this, &ClientWindow::slot_cancelSeek);
	connect(parser, &Parser::signal_SeekList, this, &ClientWindow::slot_SeekList);
	connect(parser, &Parser::signal_refresh, this, &ClientWindow::slot_refresh);

	connect(parser, &Parser::signal_set_observe, qgoif, &qGoIF::set_observe);
	connect(parser, &Parser::signal_move, qgoif, &qGoIF::slot_move);
	connect(parser, &Parser::signal_gamemove, qgoif, &qGoIF::slot_gamemove);
	connect(parser, &Parser::signal_kibitz, qgoif, &qGoIF::slot_kibitz);
	connect(parser, &Parser::signal_title, qgoif, &qGoIF::slot_title);
	connect(parser, &Parser::signal_komi, qgoif, &qGoIF::slot_komi);
	connect(parser, &Parser::signal_freegame, qgoif, &qGoIF::slot_freegame);
	connect(parser, &Parser::signal_matchcreate, qgoif, &qGoIF::slot_matchcreate);
	connect(parser, &Parser::signal_removestones, qgoif, &qGoIF::slot_removestones);
	connect(parser, &Parser::signal_undo, qgoif, &qGoIF::slot_undo);
	connect(parser, &Parser::signal_result, qgoif, &qGoIF::slot_result);
	connect(parser, &Parser::signal_requestDialog, qgoif, &qGoIF::slot_requestDialog);
	connect(this, &ClientWindow::signal_move, qgoif, &qGoIF::slot_gamemove);
	connect(qgoif, &qGoIF::signal_sendcommand, this, &ClientWindow::slot_sendcommand);
	connect(qgoif, &qGoIF::signal_addToObservationList, this, &ClientWindow::slot_addToObservationList);
	connect(qgo, &qGo::signal_updateFont, this, &ClientWindow::slot_updateFont);
	connect(parser, &Parser::signal_timeAdded, qgoif, &qGoIF::slot_timeAdded);
	//connect(parser, &Parser::signal_undoRequest, qgoif, &qGoIF::slot_undoRequest);

#if 0
	//gamestable
//	connect(ListView_players, SIGNAL(contentsMoving(int, int)), this, SLOT(slot_playerContentsMoving(int, int)));
	connect(ListView_games, SIGNAL(contentsMoving(int, int)), this, SLOT(slot_gamesContentsMoving(int, int)));
#endif

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
	statusMessage->setAlignment(Qt::AlignCenter);
	statusMessage->setText("");
	statusBar()->addPermanentWidget(statusMessage, 0);
/*
	// What's this
	statusWhatsThis = new QLabel(statusBar);
	statusWhatsThis->setAlignment(Qt::AlignCenter);
	statusWhatsThis->setText("WHATSTHIS");
	statusBar->addPermanentWidget(statusWhatsThis, 0);
	QWhatsThis::whatsThisButton(statusWhatsThis);
*/
	// The users widget
	statusUsers = new QLabel(statusBar());
	statusUsers->setAlignment(Qt::AlignCenter);
	statusUsers->setText(" P: 0 / 0 ");
	statusBar()->addPermanentWidget(statusUsers, 0);
	statusUsers->setToolTip (tr("Current online players / watched players"));
	statusUsers->setWhatsThis (tr("Displays the number of current online players\nand the number of online players you are watching.\nA player you are watching has an entry in the 'watch player:' field."));

	// The games widget
	statusGames = new QLabel(statusBar());
	statusGames->setAlignment(Qt::AlignCenter);
	statusGames->setText(" G: 0 / 0 ");
	statusBar()->addPermanentWidget(statusGames, 0);
	statusGames->setToolTip (tr("Current online games / observed games + matches"));
	statusGames->setWhatsThis (tr("Displays the number of games currently played on this server and the number of games you are observing or playing"));

	// The server widget
	statusServer = new QLabel(statusBar());
	statusServer->setAlignment(Qt::AlignCenter);
	statusServer->setText(" OFFLINE ");
	statusBar()->addPermanentWidget(statusServer, 0);
	statusServer->setToolTip (tr("Current server"));
	statusServer->setWhatsThis (tr("Displays the current server's name or OFFLINE if you are not connected to the internet."));

	// The channel widget
	statusChannel = new QLabel(statusBar());
	statusChannel->setAlignment(Qt::AlignCenter);
	statusChannel->setText("");
	statusBar()->addPermanentWidget(statusChannel, 0);
	statusChannel->setToolTip (tr("Current channels and users"));
	statusChannel->setWhatsThis (tr("Displays the current channels you are in and the number of users inthere.\nThe tooltip text contains the channels' title and users' names"));

	// Online Time
	statusOnlineTime = new QLabel(statusBar());
	statusOnlineTime->setAlignment(Qt::AlignCenter);
	statusOnlineTime->setText(" 00:00 ");
	statusBar()->addPermanentWidget(statusOnlineTime, 0);
	statusOnlineTime->setToolTip (tr("Online Time"));
	statusOnlineTime->setWhatsThis (tr("Displays the current online time.\n(A) -> auto answer\n(Hold) -> hold the line"));
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
		toolSeek->setIcon(seekingIcon[imagecounter]);
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
	statusServer->setToolTip (tr("Current server") + "\n" +
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
	toolConnect->setToolTip (tr("Connect with") + " " + cb_connect->currentText());
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

	i = 0;
	for (auto h: m_engines)
	{
		i++;
		setting->writeEntry("ENGINE" + QString::number(i) + "a", h->title());
		setting->writeEntry("ENGINE" + QString::number(i) + "b", h->path());
		setting->writeEntry("ENGINE" + QString::number(i) + "c", h->args());
		setting->writeEntry("ENGINE" + QString::number(i) + "d", h->komi());
		setting->writeBoolEntry("ENGINE" + QString::number(i) + "e", h->analysis());
		setting->writeEntry("ENGINE" + QString::number(i) + "f", h->boardsize());
	}

	// save current connection if at least one host exists
	if (!hostlist.isEmpty())
		setting->writeEntry("ACTIVEHOST",cb_connect->currentText());

	QString scrkey = screen_key (this);
	setting->writeEntry("CLIENTWINDOW_" + scrkey,
			    QString::number(pos().x()) + DELIMITER +
			    QString::number(pos().y()) + DELIMITER +
			    QString::number(size().width()) + DELIMITER +
			    QString::number(size().height()));
	setting->writeEntry("CLIENTSPLITTER_" + scrkey,
			    QString::number(s1->sizes().first()) + DELIMITER +
			    QString::number(s1->sizes().last()) + DELIMITER +
			    QString::number(s2->sizes().first()) + DELIMITER +
			    QString::number(s2->sizes().last()) + DELIMITER +
			    QString::number(s3->sizes().first()) + DELIMITER +
			    QString::number(s3->sizes().last()));

	if (debug_dialog->isVisible ())
		setting->writeEntry("DEBUGWINDOW",
			QString::number(debug_dialog->pos().x()) + DELIMITER +
			QString::number(debug_dialog->pos().y()) + DELIMITER +
			QString::number(debug_dialog->size().width()) + DELIMITER +
			QString::number(debug_dialog->size().height()));

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
void ClientWindow::quit(bool)
{
	saveSettings();
	qApp->quit();
}

// Signal from QApplication
void ClientWindow::slot_last_window_closed ()
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
			if (!tn_wait_for_tn_ready && !tn_ready)
			{
				QTimer::singleShot(200, this, SLOT(set_tn_ready()));
				tn_wait_for_tn_ready = true;
			}
			sendTextFromApp (nullptr);
		case WS:
			// ready or white space -> return
			return;

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
			} while (sendTextFromApp (nullptr) != 0);

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
					//if (myAccount->get_status() == Status::guest)
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
					//if (myAccount->get_status() == Status::guest)
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
			toolConnect->setToolTip (tr("Disconnect from") + " " + cb_connect->currentText());

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
	qDebug() << txt;
}

// used for singleShot actions
void ClientWindow::set_tn_ready()
{
	tn_ready = true;
	tn_wait_for_tn_ready = false;
	sendTextFromApp (nullptr);
}

// send text via telnet session; skipping empty string!
int ClientWindow::sendTextFromApp(const QString &txt, bool localecho)
{
	// implements a simple buffer
	int valid = txt.length();

	// some statistics
	if (valid)
		setBytesOut(valid+2);

	if (myAccount->get_status() == Status::offline)
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
		if (!sendBuffer.isEmpty ())
		{
			sendBuf *s = sendBuffer.takeFirst();

			// send buffer cmd first; then put current cmd to buffer
			telnetConnection->sendTextFromApp(s->get_txt());
//qDebug("SENDBUFFER send: " + s->get_txt());

			// hold the line if cmd is sent; 'ayt' is autosend cmd
			if (s->get_txt().indexOf("ayt") != -1)
				resetCounter();
			if (s->get_localecho())
				sendTextToApp(CONSOLECMDPREFIX + QString(" ") + s->get_txt());
			tn_ready = false;

			if (valid)
			{
				// append current command to send as soon as possible
				sendBuffer.append(new sendBuf(txt, localecho));
//qDebug("SENDBUFFER added: " + txt);
			}
			delete s;
		}
		else if (valid)
		{
			// buffer empty -> send direct
			telnetConnection->sendTextFromApp(txt);

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
				      "#+dbg\t\t#-dbg\n");
			return;
		}

		// detect internal commands
		if (testcmd.contains("+dbg"))
		{
			debug_dialog->show();
			this->activateWindow();
		} else if (testcmd.contains("-dbg"))
			debug_dialog->hide();

		sendTextToApp(testcmd);
		return;
	}

	// echo
	if (1 || localecho)
	{
		// add to Messages, anyway
		// Scroll at bottom of text, set cursor to end of line
		qDebug() << "CMD: " << cmd;
		colored_message (cmd, Qt::blue);
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
		cb_cmdLine->insertItem(0, "");
		cb_cmdLine->clearEditText();

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
	Host *h = nullptr;
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
			cb_connect->addItem (h_->title());
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

	if (!h && hostlist.length () > 0) {
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
		toolConnect->setToolTip (tr("Connect with") + " " + cb_connect->currentText());
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
void ClientWindow::slot_cblooking(bool)
{
	bool val = setLookingMode->isChecked();
//	setLookingMode->setChecked(val);
	set_sessionparameter("looking", val);
	if (val)
		// if looking then set open
		set_sessionparameter("open", true);
}


// checkbox open clicked
void ClientWindow::slot_cbopen(bool)
{
	bool val = setOpenMode->isChecked();
//	setOpenMode->setChecked(val);
	set_sessionparameter("open", val);
	if (!val)
		// if not open then set close
		set_sessionparameter("looking", false);
}

// checkbox quiet clicked
void ClientWindow::slot_cbquiet(bool)
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

	// comment fields
	QTextCursor c = MultiLineEdit2->textCursor ();
	MultiLineEdit2->selectAll();
	MultiLineEdit2->setCurrentFont(setting->fontConsole);
	MultiLineEdit2->setTextCursor (c);
	MultiLineEdit2->setCurrentFont(setting->fontConsole);

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
void ClientWindow::slot_pbrefreshgames(bool)
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
void ClientWindow::slot_pbrefreshplayers(bool)
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
	static QMenu *puw = nullptr;

	// create popup window
	if (!puw)
	{
		puw = new QMenu(0, 0);
		puw->addAction(tr("observe"), this, [=] () { slot_gamesPopup(1); });
		puw->addAction(tr("stats W"), this, [=] () { slot_gamesPopup(2); });
		puw->addAction(tr("stats B"), this, [=] () { slot_gamesPopup(3); });
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
	for (auto it: matchlist)
		if (it->playerOpponentEdit->text() == opponent) {
			matchlist.removeOne (it);
			delete it;
			break;
		}
}


void ClientWindow::slot_matchrequest(const QString &line, bool myrequest)
{
	// set up match dialog
	// match_dialog()
	GameDialog *dlg = nullptr;
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
	for (auto it: matchlist)
		if (it->playerOpponentEdit->text() == opponent) {
			dlg = it;
			break;
		}

	if (!dlg)
	{
		dlg = new GameDialog(0, tr("New Game"));
		matchlist.append (dlg);

		if (myAccount->get_gsname() == NNGS || myAccount->get_gsname() == LGS)
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
			for (QTreeWidgetItem *lvi; (lvi = *lv);)
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
	static QMenu *puw = nullptr;
	static QAction *puw11 = nullptr;
	lv_popupPlayer = static_cast<PlayerTableItem*>(lv);
	// create popup window
	if (!puw)
	{
		puw = new QMenu(0, 0);
		puw->addAction(tr("match"), this, [=] () { slot_playerPopup(1); });
		puw11 = puw->addAction(tr("match within his prefs"), this, [=] () { slot_playerPopup(11); });
		puw->addAction(tr("talk"), this, [=] () { slot_playerPopup(2); });
		// puw->insertSeparator();
		puw->addAction(tr("stats"), this, [=] () { slot_playerPopup(3); });
		puw->addAction(tr("stored games"), this, [=] () { slot_playerPopup(4); });
		puw->addAction(tr("results"), this, [=] () { slot_playerPopup(5); });
		puw->addAction(tr("rating"), this, [=] () { slot_playerPopup(8); });
		puw->addAction(tr("observe game"), this, [=] () { slot_playerPopup(9); });
		puw->addAction(tr("trail"), this, [=] () { slot_playerPopup(12); });
		// puw->insertSeparator();
		puw->addAction(tr("toggle watch list"), this, [=] () { slot_playerPopup(6); });
		puw->addAction(tr("toggle exclude list"), this, [=] () { slot_playerPopup(7); });

	}

	puw11->setEnabled(lv_popupPlayer->get_player ().nmatch);
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
	for (auto dlg: talklist) {
		if (dlg->get_name().indexOf('*') == -1)
		{
			TabWidget_mini_2->removeTab (TabWidget_mini_2->indexOf (dlg->get_tabWidget ()));
			dlg->pageActive = false;
		}
	}
}

// release Talk Tabs
void ClientWindow::slot_pbRelOneTab(QWidget *w)
{
	// seek dialog
	for (auto dlg: talklist) {
		if (dlg->get_tabWidget() == w)
		{
			TabWidget_mini_2->removeTab (TabWidget_mini_2->indexOf (w));
			dlg->pageActive = false;
			return;
		}
	}

}

 // handle chat boxes in a list
void ClientWindow::slot_talk(const QString &name, const QString &text, bool isplayer)
{
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

	Talk *dlg = nullptr;
	for (auto it: talklist)
		if (it->get_name () == name) {
			dlg = it;
			break;
		}

	if (!dlg && !name.isEmpty () && name != tr ("msg*")) {
		// not found -> create new dialog
		dlg = new Talk(name, 0, isplayer);
		talklist.append (dlg);

		connect(dlg, SIGNAL(signal_talkto(QString&, QString&)), this, SLOT(slot_talkto(QString&, QString&)));
		connect(dlg, SIGNAL(signal_matchrequest(const QString&,bool)), this, SLOT(slot_matchrequest(const QString&,bool)));
		connect(dlg->get_le(), SIGNAL(returnPressed()), dlg, SLOT(slot_returnPressed()));
		connect(dlg, SIGNAL(signal_pbRelOneTab(QWidget*)), this, SLOT(slot_pbRelOneTab(QWidget*)));

		dlg->pageActive = false;
		if (!name.isEmpty() && isplayer)
			slot_sendcommand("stats " + name, false);    // automatically request stats

		// play sound on new created dialog
		bonus = true;

	}
	if (dlg) {
		dlg->write(txt);
		QWidget *w = dlg->get_tabWidget ();
		if (!dlg->pageActive)
		{
			TabWidget_mini_2->addTab(w, dlg->get_name());
			dlg->pageActive = true;
			if (name != tr("Shouts*"))
				TabWidget_mini_2->setCurrentIndex(TabWidget_mini_2->indexOf (w));
		}
	}
	// check if it was a channel message
	autoAnswer &= (isplayer && autoAwayMessage && !name.contains('*') && (!text.isEmpty () && text[0] == '>'));
	if (autoAnswer)
	{
		// send when qGo is NOT the active application - TO DO
		sendcommand("tell " + name + " [I'm away right now]");
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

// open a local board
void ClientWindow::slot_preferences(bool)
{
	dlgSetPreferences ();
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
	connect(fileNewBoard, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::none); });
	connect(fileNewVariant, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::variant); });
	connect(fileNew, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::normal); });
	connect(fileOpen, &QAction::triggered, this, &ClientWindow::slotFileOpen);
	connect(fileBatchAnalysis, &QAction::triggered, this, [] (bool) { show_batch_analysis (); });
	connect(computerPlay, &QAction::triggered, this, &ClientWindow::slotComputerPlay);
	connect(fileQuit, &QAction::triggered, this, &ClientWindow::quit);

	/*
	* Menu Connexion
	*/
	connect(Connect, &QAction::triggered, this, &ClientWindow::slotMenuConnect);
	connect(Disconnect, &QAction::triggered, this, &ClientWindow::slotMenuDisconnect);
	Disconnect->setEnabled(false);
	connect(editServers, &QAction::triggered, this, &ClientWindow::slotMenuEditServers);

	/*
	* Menu Settings
	*/
	connect(setPreferences, &QAction::triggered, this, &ClientWindow::slot_preferences);

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
	connect(helpManual, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("index.html")); });
	connect(helpReadme, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("readme.html")); });
	/* There isn't actually a manual.  Well, there is, but it's outdated and we don't ship it.  */
	helpManual->setVisible (false);
	helpManual->setEnabled (false);
	connect(helpAboutApp, &QAction::triggered, [=] (bool) { help_about (); });
	connect(helpAboutQt, &QAction::triggered, [=] (bool) { QMessageBox::aboutQt (this); });
	connect(helpNewVersion, &QAction::triggered, [=] (bool) { help_new_version (); });
}

void ClientWindow::initToolBar()
{
	//connect( cb_connect, SIGNAL( activated(const QString&) ), this, SLOT( slot_cbconnect(const QString&) ) );

	//  toolConnect->setText(tr("Connect with") + " " + cb_connect->currentText()); 
	//connect( toolConnect, SIGNAL( toggled(bool) ), this, SLOT( slot_connect(bool) ) );  //end add eb 5

	seekMenu = new QMenu();
	toolSeek->setMenu (seekMenu);
	toolSeek->setPopupMode (QToolButton::InstantPopup);

	whatsThis = QWhatsThis::createAction (this);
	Toolbar->addAction (whatsThis);
	//tb->setProperty( "geometry", QRect(0, 0, 20, 20));
}

// SLOTS

void ClientWindow::slotFileOpen(bool)
{
	std::shared_ptr<game_record> gr = open_file_dialog (this);
	if (gr == nullptr)
		return;

	MainWindow *win = new MainWindow (0, gr);
	win->show ();
}

Engine *ClientWindow::analysis_engine (int boardsize)
{
	for (auto e: m_engines) {
		if (e->analysis () && e->boardsize ().toInt () == boardsize)
			return e;
	}
	return nullptr;
}

QList<Engine> ClientWindow::analysis_engines (int boardsize)
{
	QList<Engine> l;
	for (auto e: m_engines) {
		if (e->analysis () && e->boardsize ().toInt () == boardsize)
			l.append (*e);
	}
	return l;
}

void ClientWindow::slotComputerPlay(bool)
{
	if (m_engines.isEmpty ())
	{
		QMessageBox::warning(this, PACKAGE, tr("You did not configure any engines!"));
		dlgSetPreferences (3);
		return;
	}

	NewAIGameDlg dlg (this, m_engines);
	if (dlg.exec() != QDialog::Accepted)
		return;

	int eidx = dlg.engine_index ();
	const Engine *engine = m_engines[eidx];
	int hc = dlg.handicap ();
	game_info info = dlg.create_game_info ();
	go_board starting_pos = new_handicap_board (dlg.board_size (), dlg.handicap ());

	std::shared_ptr<game_record> gr = std::make_shared<game_record> (starting_pos, hc > 1 ? white : black, info);

	if (gr == nullptr)
		return;
	bool computer_white = dlg.computer_white_p ();
	new MainWindow_GTP (0, gr, *engine, !computer_white, computer_white);
}


void ClientWindow::slotMenuConnect(bool)
{
	// push button
	toolConnect->toggle();
}

void ClientWindow::slotMenuDisconnect(bool)
{
	// push button
	toolConnect->toggle();
}

void ClientWindow::slotMenuEditServers(bool)
{
	dlgSetPreferences (4);
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
	if (p->name.isEmpty())
		return;

	for (auto dlg: talklist) {
		if (dlg->get_name() == p->name) {
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
	QList<QListWidgetItem *> items = RoomList->findItems(room.left(3), Qt::MatchStartsWith);
	//do we already have the same room number in list ?
	if (items.length () > 0)
		return;
	//so far, we just create the room if it is open
	if (!b)
		RoomList->insertItem (-1, room);

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


void ClientWindow::slot_RoomListClicked(QListWidgetItem *qli)
{
	slot_enterRoom(qli->text().section(":",0,0));
}

void ClientWindow::slot_addSeekCondition(const QString& a, const QString& b, const QString& c, const QString& d, const QString& )
{
	QString time_condition ;
	
	time_condition = QString::number(int(b.toInt() / 60)) + " min + " + QString::number(int(c.toInt() / 60)) + " min / " + d + " stones";

	int a_int = a.toInt ();
	seekMenu->addAction(time_condition, this, [=] () { slot_seek(a_int); });
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
	toolSeek->setMenu (seekMenu);
	toolSeek->setPopupMode (QToolButton::InstantPopup);
	toolSeek->setIcon(NotSeekingIcon);
	killTimer(seekButtonTimer);
	seekButtonTimer = 0;
}

void ClientWindow::slot_seek(int i)
{
	toolSeek->setChecked (true);
	toolSeek->setMenu (nullptr);

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
	
	if (myAccount->get_gsname() != IGS || myAccount->get_status() == Status::offline)
		return;

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
	PreferencesDialog dlg;

	if (tab >= 0)
	{
		if (dlg.tabWidget->count() <= tab+1)
			dlg.tabWidget->setCurrentIndex (tab);
	}

	if (dlg.exec() == QDialog::Accepted)
	{
		saveSettings();
	}
}

bool ClientWindow::preferencesAccept()
{
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
