/*
 *   mainwin.cpp - qGoClient's main window (cont. in tables.cpp)
 */

#include <QMainWindow>
#include <QFileDialog>
#include <QWhatsThis>
#include <QMessageBox>
#include <QTimerEvent>
#include <QMenu>
#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QStatusBar>
#include <QToolButton>
#include <QIcon>

#include "config.h"
#include "clientwin.h"
#include "defines.h"
#include "playertable.h"
#include "gamestable.h"
#include "gamedialog.h"
#include "qgo_interface.h"
#include "komispinbox.h"
#include "igsconnection.h"
#include "mainwindow.h"
#include "ui_helpers.h"
#include "msg_handler.h"
#include "dbdialog.h"
#include "analyzedlg.h"

ClientWindow *client_window;

static int get_who_setting (QString str)
{
	QString s = setting->readEntry (str + "_NEW");
	int idx;
	bool ok;
	if (s.isNull ()) {
		s = setting->readEntry (str);
		if (s.isNull ())
			return -1;
		idx = s.toInt (&ok);
		if (!ok)
			return -1;
		/* Compatibility code to adjust for ranks that were removed from the combo box.
		   These were groups of four ranks each: 11-14k, 16-19k, 21-24k and 26-29k.
		   10k is index 20.  */
		switch (idx) {
		case 36: case 37: case 38: case 39: case 40:
			idx = 24; break;
		case 31: case 32: case 33: case 34: case 35:
			idx = 23; break;
		case 26: case 27: case 28: case 29: case 30:
			idx = 22; break;
		case 22: case 23: case 24: case 25:
			idx = 21; break;
		}
		return idx;
	} else {
		idx = s.toInt (&ok);
		if (!ok)
			return -1;
		return idx;
	}
}

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

	setWindowIcon (QIcon (":/ClientWindowGui/images/clientwindow/qgo.png"));

	m_standard_caption = QString (PACKAGE1) + " V" + VERSION;
	clear_server_data ();
	update_caption ();

	cmd_count = 0;

	resetCounter();
	cmd_valid = false;
	tn_ready = false;
	tn_wait_for_tn_ready = false;
	youhavemsg = false;
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
	connect (escapeFocus, &QAction::triggered, [=] () { setFocus (); });
	cb_cmdLine->addAction (escapeFocus);

	// create instance of telnetConnection
	telnetConnection = new TelnetConnection (this, ListView_players, ListView_games);

	m_games_model.set_filter ([this] (const Game &g)->bool { return filter_game (g); });
	m_player_model.set_filter ([this] (const Player &p)->bool { return filter_player (p); });
	ListView_games->set_model (&m_games_model);
	ListView_games->resize_columns ();
	ListView_players->set_model (&m_player_model);
	ListView_players->resize_columns ();
	ListView_games->sortByColumn (2, Qt::AscendingOrder);
	ListView_players->sortByColumn (2, Qt::AscendingOrder);

	// doubleclick
	connect (ListView_games, &GamesTable::signal_doubleClicked, this, &ClientWindow::slot_doubleclick_games);
	connect (ListView_players, &PlayerTable::signal_doubleClicked, this, &ClientWindow::slot_doubleclick_players);

	connect (ListView_players, &PlayerTable::customContextMenuRequested, this, &ClientWindow::slot_menu_players);
	connect (ListView_games, &GamesTable::customContextMenuRequested, this, &ClientWindow::slot_menu_games);

	connect (whoOpenCheck, &QCheckBox::toggled, this, &ClientWindow::slot_whoopen);
	void (QComboBox::*cic) (int) = &QComboBox::currentIndexChanged;
	connect (whoBox1, cic, [this] (int idx) { update_who_rank (whoBox1, idx); });
	connect (whoBox2, cic, [this] (int idx) { update_who_rank (whoBox2, idx); });
	connect (toolObserveMode, &QToolButton::toggled, [] (bool on) { setting->writeBoolEntry ("OBSERVE_SINGLE", on); });

	initStatusBar(this);
	initActions();

	// Set menus
	//initmenus(this);
	// Sets toolbar
	initToolBar();                              //end add eb 5

	populate_cbconnect (setting->readEntry("ACTIVEHOST"));

	//restore players list filters
	int who1 = get_who_setting ("WHO_1");
	int who2 = get_who_setting ("WHO_2");
	if (who1 == -1)
		who1 = whoBox1->currentIndex ();
	if (who2 == -1)
		who2 = whoBox2->currentIndex ();
	whoBox1->setCurrentIndex (who1);
	whoBox2->setCurrentIndex (who2);
	update_who_rank (whoBox1, who1);
	update_who_rank (whoBox2, who2);
	whoOpenCheck->setChecked (setting->readIntEntry("WHO_CB"));

	toolObserveMode->setChecked (setting->readBoolEntry ("OBSERVE_SINGLE"));

	QString s;

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

	// create qGo Interface for board handling
	qgoif = new qGoIF(this);
	// qGo Interface

	// create parser and connect signals
	parser = new Parser(this, qgoif);
	connect(parser, &Parser::signal_message, this, &ClientWindow::slot_message);
	connect(parser, &Parser::signal_svname, this, &ClientWindow::slot_svname);
	connect(parser, &Parser::signal_accname, this, &ClientWindow::slot_accname);
	connect(parser, &Parser::signal_status, this, &ClientWindow::slot_status);
	connect(parser, &Parser::signal_connclosed, this, &ClientWindow::slot_connclosed);
	connect(parser, &Parser::signal_talk, this, &ClientWindow::slot_talk);
	connect(parser, &Parser::signal_channelinfo, this, &ClientWindow::slot_channelinfo);
	connect(parser, &Parser::signal_matchrequest, [this] (const QString &line) { handle_matchrequest (line, false, false); });
	connect(parser, &Parser::signal_matchCanceled, this, &ClientWindow::slot_removeMatchDialog);
	connect(parser, &Parser::signal_shout, this, &ClientWindow::slot_shout);
	connect(parser, &Parser::signal_room, this, &ClientWindow::slot_room);
	connect(parser, &Parser::signal_addSeekCondition, this, &ClientWindow::slot_addSeekCondition);
	connect(parser, &Parser::signal_clearSeekCondition, this, &ClientWindow::slot_clearSeekCondition);
	connect(parser, &Parser::signal_cancelSeek, this, &ClientWindow::slot_cancelSeek);
	connect(parser, &Parser::signal_SeekList, this, &ClientWindow::slot_SeekList);

	connect(parser, &Parser::signal_set_observe, qgoif, &qGoIF::set_observe);
	connect(parser, &Parser::signal_move, qgoif, &qGoIF::slot_move);
	connect(parser, &Parser::signal_kibitz, qgoif, &qGoIF::slot_kibitz);
	connect(parser, &Parser::signal_komi, qgoif, &qGoIF::slot_komi);
	connect(parser, &Parser::signal_freegame, qgoif, &qGoIF::slot_freegame);
	connect(parser, &Parser::signal_removestones, qgoif, &qGoIF::slot_removestones);
	connect(parser, &Parser::signal_undo, qgoif, &qGoIF::slot_undo);
	connect(parser, &Parser::signal_result, qgoif, &qGoIF::slot_result);
	connect(parser, &Parser::signal_requestDialog, qgoif, &qGoIF::slot_requestDialog);

	connect(parser, &Parser::signal_timeAdded, qgoif, &qGoIF::slot_timeAdded);
	//connect(parser, &Parser::signal_undoRequest, qgoif, &qGoIF::slot_undoRequest);

	update_font ();

	// install an event filter
	qApp->installEventFilter(this);

	enable_connection_buttons (false);
}

ClientWindow::~ClientWindow()
{
	ListView_games->setModel (nullptr);
	ListView_players->setModel (nullptr);

	delete telnetConnection;
	delete qgoif;
	delete parser;
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

	update_tables ();

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

			if (num_observedgames == 0)
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
		else if (m_online_server == IGS && holdTheLine)
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

void ClientWindow::enable_connection_buttons (bool on)
{
	cb_connect->setEnabled (!on);
	toolSeek->setEnabled (on);
	setLookingMode->setEnabled (on);
	setQuietMode->setEnabled (on);
	setOpenMode->setEnabled (on);
	refreshPlayers->setEnabled (on);
	refreshGames->setEnabled (on);
}

void ClientWindow::update_caption ()
{
	QString override = setting->readEntry ("WTITLE_CLIENT");
	if (!override.isEmpty ()) {
		setWindowTitle (override);
		return;
	}
	Status st = m_online_status;
	if (st == Status::offline)
		setWindowTitle (m_standard_caption);
	else {
		QString t = m_online_server_name + " - " + m_online_acc_name;
		if (st == Status::guest)
			t += " (guest)";
		setWindowTitle (t);
	}
}

// slot_connect: emitted when connect button has toggled
void ClientWindow::slot_connect (bool b)
{
	if (b) {
		int idx = cb_connect->currentIndex ();
		if (idx == -1)
			return;
		m_active_host = setting->m_hosts[idx];

		// connect to selected host
		telnetConnection->connect_host (m_active_host);
	} else {
		// disconnect
		telnetConnection->slotHostQuit ();
	}
	enable_connection_buttons (b);
}

void ClientWindow::clear_server_data ()
{
	m_online_server = GS_UNKNOWN;
	m_online_server_name = tr ("Unknown server");
	m_online_acc_name = tr ("Unset account name");
	m_online_status = Status::offline;
}

// connection closed
void ClientWindow::slot_connclosed ()
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

	clear_server_data ();
	update_caption ();
	//pb_connect->setChecked(false);
	toolConnect->setChecked(false);
	seekMenu->clear();
	slot_cancelSeek();

	// clear channel
	prepare_channels ();

	// remove boards
	qgoif->set_initIF();

	qDebug("slot_connclosed()");
	qDebug() << statusOnlineTime->text() << " -> slot_connclosed()";

	qgo->playConnectSound();

	auto saved_list = matchlist;
	matchlist.clear ();
	for (auto it: saved_list)
		delete it;

	// show current Server name in status bar
	statusServer->setText(" OFFLINE ");

	// set menu
	Connect->setEnabled(true);
	Disconnect->setEnabled(false);
	toolConnect->setChecked(false);
	toolConnect->setToolTip (tr("Connect with") + " " + cb_connect->currentText());

	m_player_table.clear ();
	m_games_table.clear ();
	m_send_buffer.clear ();

	m_playerlist_active = false;
	m_player7_active = false;

	m_player_init_complete = false;
	m_games_init_complete = false;
}

// close application
void ClientWindow::saveSettings()
{
	// save current connection if at least one host exists
	if (setting->m_hosts.size () > 0)
		setting->writeEntry ("ACTIVEHOST", cb_connect->currentText ());

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

	if (pref_s.width() > 0)
		setting->writeEntry("PREFWINDOW",
			QString::number(pref_p.x()) + DELIMITER +
			QString::number(pref_p.y()) + DELIMITER +
			QString::number(pref_s.width()) + DELIMITER +
			QString::number(pref_s.height()));

	setting->writeIntEntry ("WHO_1_NEW", whoBox1->currentIndex ());
	setting->writeIntEntry ("WHO_2_NEW", whoBox2->currentIndex ());
	setting->writeBoolEntry ("WHO_CB", whoOpenCheck->isChecked ());

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
void ClientWindow::sendTextToApp (const QString &txt)
{
	// put text to parser
 	InfoType it_ = parser->put_line (txt);

	// some statistics
	bytesIn += txt.length () + 2;

	// GAME7_END emulation:
	if (m_player7_active && it_ != GAME7)
	{
		finish_game_list ();
		m_player7_active = false;
	}

	switch (it_)
	{
	case READY:
		if (!tn_wait_for_tn_ready && !tn_ready)
		{
			QTimer::singleShot (200, this, &ClientWindow::set_tn_ready);
			tn_wait_for_tn_ready = true;
		}

	case WS:
		// ready or white space -> return
		return;

		// echo entered command
		// echo server enter line
	case CMD:
		slot_message (txt);
		break;

		// set client mode
	case NOCLIENTMODE:
		set_sessionparameter ("client", true);
		break;

	case YOUHAVEMSG:
		// normally no connection -> wait until logged in
		youhavemsg = true;
		break;

	case SERVERNAME:
		slot_message (txt);
		// clear send buffer
		do
		{
			// enable sending
			set_tn_ready ();
		} while (!m_send_buffer.empty ());

		switch (m_online_server)
		{
		case IGS:
		{
			// IGS - check if client mode
			set_sessionparameter ("client", true);

			// set quiet true; refresh players, games
			//if (myAccount->get_status () == Status::guest)
			if (m_active_host.quiet == -1)
				set_sessionparameter ("quiet", true);
			else
				set_sessionparameter ("quiet", m_active_host.quiet != 0);
			sendcommand ("id " PACKAGE " " VERSION, true);
			sendcommand ("toggle newrating");

			// we wanted to allow user to disable 'nmatch', but IGS disables 'seek' with nmatch
			if (1 || setting->readBoolEntry ("USE_NMATCH"))
			{
				set_sessionparameter ("nmatch",true);
				// temporaary settings to prevent use of Koryo BY on IGS (as opposed to canadian)
				// sendcommand("nmatchrange BWN 0-9 19-19 60-60 60-3600 25-25 0 0 0-0",false);
				send_nmatch_range_parameters ();
			}

			sendcommand ("toggle newundo");
			sendcommand ("toggle seek");
			sendcommand ("seek config_list ");
			sendcommand ("room");

			break;
		}

		default:
			set_sessionparameter ("client", true);
			// set quiet false; refresh players, games
			//if (myAccount->get_status() == Status::guest)
			if (m_active_host.quiet == -1)
				set_sessionparameter ("quiet", false);
			else
				set_sessionparameter ("quiet", m_active_host.quiet != 0);
			break;
		}

		refresh_players ();
		refresh_games ();

		// set menu
		Connect->setEnabled (false);
		Disconnect->setEnabled (true);
		toolConnect->setChecked (true);
		toolConnect->setToolTip (tr ("Disconnect from") + " " + cb_connect->currentText ());

		// check for messages
		if (youhavemsg)
			sendcommand ("message", false);

		// let qgo know which server
		qgoif->set_gsName (m_online_server);
		// show current Server name in status bar
		statusServer->setText (" " + m_online_server_name + " ");

		// start timer: event every second
		onlineCount = 0;
		oneSecondTimer = startTimer (1000);
		// init shouts
		slot_talk ("Shouts*", QString (), false, false);

		qgo->playConnectSound ();
		break;

		// end of 'who'/'user' cmd
	case PLAYER42_END:
	case PLAYER27_END:
		m_playerlist_active = false;
		finish_player_list ();
		break;

	case PLAYER27_START:
	case PLAYER42_START:
		m_playerlist_active = true;
		break;

	case GAME7_START:
		// "emulate" GAME7_END
		m_player7_active = true;
		break;

	case ACCOUNT:
		// let qgo and parser know which account in case of setting something for own games
		qgoif->set_myName (m_online_acc_name);
		parser->set_myname (m_online_acc_name);
		break;

	case STATS:
		// we just received a players name as first line of stats -> create the dialog tab
		slot_talk (parser->get_statsPlayer ()->name, QString (), true, false);

		break;

	case BEEP:
#if 0
		QApplication::beep ();
#endif
		break;

	default:
		break;
	}
	qDebug () << txt;
}

// used for singleShot actions
void ClientWindow::set_tn_ready()
{
	tn_ready = true;
	tn_wait_for_tn_ready = false;
	try_send ();
}

void ClientWindow::try_send ()
{
	if (!tn_ready || m_send_buffer.empty ())
		return;

	const sendBuf &s = m_send_buffer.front ();

	telnetConnection->sendTextFromApp (s.txt);
	tn_ready = false;

	// hold the line if cmd is sent; 'ayt' is autosend cmd
	if (s.txt.indexOf("ayt") != -1)
		resetCounter ();
	if (s.localecho)
		sendTextToApp (CONSOLECMDPREFIX + QString(" ") + s.txt);

	m_send_buffer.pop_front ();
}

void ClientWindow::enqueue_command (const QString &cmd, bool localecho)
{
	if (m_online_status == Status::offline) {
		// skip all commands while not telnet connection
		sendTextToApp ("Command skipped - no telnet connection: " + cmd);
		// reset buffer
		m_send_buffer.clear ();
		return;
	}

	m_send_buffer.emplace_back (cmd, localecho);
	try_send ();
}

// show command, send it, and tell parser
void ClientWindow::sendcommand (const QString &cmd, bool localecho)
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

	enqueue_command (cmd, localecho);
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
			// Manually entered who command, prepare table
			prepare_player_list ();
		}
		if (cmdLine.mid(0,5).contains("; \\-1") && m_online_server == IGS)
		{
			// exit all channels
			prepare_channels ();
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

	switch(m_online_server)
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

/* Update the connection combobox when the server table has changed.  */
void ClientWindow::populate_cbconnect (const QString &prev_text)
{
	// refill combobox
	cb_connect->clear ();
	for (auto &h: setting->m_hosts) {
		cb_connect->addItem (h.title);
		if (h.title == prev_text) {
			cb_connect->setCurrentIndex (cb_connect->count() - 1);
			slot_cbconnect (prev_text);
		}
	}
	if (setting->m_hosts.size () > 0 && cb_connect->currentIndex () == -1) {
		cb_connect->setCurrentIndex (0);
		slot_cbconnect (cb_connect->currentText ());
	}
}

// Select connection
void ClientWindow::slot_cbconnect (const QString &txt)
{
	if (toolConnect)
		toolConnect->setToolTip (tr("Connect with") + " " + txt);
}

void ClientWindow::set_open_mode (bool val)
{
	setOpenMode->setChecked (val);
}

void ClientWindow::set_looking_mode (bool val)
{
	setLookingMode->setChecked (val);
}

void ClientWindow::set_quiet_mode (bool val)
{
	setQuietMode->setChecked (val);
}

// checkbox looking cklicked
void ClientWindow::slot_cblooking (bool)
{
	bool val = setLookingMode->isChecked();
//	setLookingMode->setChecked(val);
	set_sessionparameter("looking", val);
	if (val)
		// if looking then set open
		set_sessionparameter("open", true);
}


// checkbox open clicked
void ClientWindow::slot_cbopen (bool)
{
	bool val = setOpenMode->isChecked();
	set_sessionparameter("open", val);
	if (!val)
		// if not open then set close
		set_sessionparameter("looking", false);
}

// checkbox quiet clicked
void ClientWindow::slot_cbquiet (bool)
{
	bool val = setQuietMode->isChecked ();
	set_sessionparameter ("quiet", val);
	for (auto &h: setting->m_hosts) {
		if (h.host == m_active_host.host && h.login_name == m_active_host.login_name
		    && h.title == m_active_host.title)
		{
			h.quiet = val;
			break;
		}
	}
}

void ClientWindow::update_font ()
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
}

void ClientWindow::server_player_entered (const QString &)
{
	/* We have no information beyond the name.  Refreshing the entire player list
	   is the historical behaviour the program has always had.  @@@ Rethink.  */
	refresh_players ();
}

void ClientWindow::refresh_players ()
{
	prepare_player_list ();

	if (m_online_server == IGS)
		sendcommand ("userlist");
	else
		sendcommand ("who");
}

void ClientWindow::refresh_games ()
{
	prepare_game_list ();
	sendcommand ("games");
}

void ClientWindow::slot_whoopen (bool checked)
{
	m_whoopen = checked;
	update_tables ();
}

// refresh games
void ClientWindow::slot_pbrefreshgames(bool)
{
	refresh_games ();
}

// refresh players
void ClientWindow::slot_pbrefreshplayers(bool)
{
	refresh_players ();
}

// doubleclick actions...
void ClientWindow::slot_doubleclick_games (const Game &g)
{
	sendcommand ("observe " + g.nr);
}

void ClientWindow::slot_menu_games (const QPoint &pt)
{
	QModelIndex idx = ListView_games->indexAt (pt);
	if (!idx.isValid ())
		return;
	const Game *g = m_games_model.find (idx);
	if (g != nullptr)
		slot_mouse_games (*g);
}

// mouse click on ListView_games
void ClientWindow::slot_mouse_games (const Game &g)
{
	static QMenu *puw = nullptr;

	if (!puw)
	{
		puw = new QMenu (0, 0);
		puw->addAction (tr ("observe"), [=] () { sendcommand ("observe " + m_menu_game.nr); });
		puw->addAction (tr ("stats W"),
				[=] () {
					const QString &t1 = m_menu_game.wname;
					QString player_name = t1.right (1) == "*" ? t1.left (t1.length () - 1) : t1;
					sendcommand ("stats " + player_name, false);
				});
		puw->addAction (tr ("stats B"),
				[=] () {
					const QString &t1 = m_menu_game.bname;
					QString player_name = t1.right (1) == "*" ? t1.left (t1.length () - 1) : t1;
					sendcommand ("stats " + player_name, false);
				});
	}

	m_menu_game = g;
	puw->popup (QCursor::pos ());
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

void ClientWindow::handle_matchrequest (const QString &line, bool myrequest, bool from_talk_tab)
{
	// set up match dialog
	// match_dialog()
	GameDialog *dlg = nullptr;
	QString opponent;
	QString opp_rk;

	// seek dialog
	if (!myrequest)
	{
		// match xxxx B 19 1 10
		opponent = line.section (' ', 1, 1);
		opp_rk = getPlayerRk (opponent);
		// play sound
		qgo->playMatchSound ();
	}
	else
	{
		// xxxxx 4k*
		opponent = line.section (' ', 0, 0);
		opp_rk = line.section (' ', 1, 1);
	}
	QString myrk = m_online_rank;

	// look for same opponent
	for (auto it: matchlist)
		if (it->playerOpponentEdit->text() == opponent) {
			dlg = it;
			break;
		}

	GSName gs = m_online_server;
	if (!dlg)
	{
		dlg = new GameDialog (0, gs, m_online_acc_name, myrk, opponent, opp_rk);
		matchlist.append (dlg);

		if (gs == NNGS || gs == LGS)
			connect (parser, &Parser::signal_suggest, dlg, &GameDialog::slot_suggest);

		connect (dlg, &GameDialog::signal_removeDialog, this, &ClientWindow::slot_removeMatchDialog);

		connect (parser, &Parser::signal_matchcreate, dlg, &GameDialog::slot_matchcreate);
		connect (parser, &Parser::signal_notopen, dlg, &GameDialog::slot_notopen);
		connect (parser, &Parser::signal_komirequest, dlg, &GameDialog::slot_komirequest);
		connect (parser, &Parser::signal_opponentopen, dlg, &GameDialog::slot_opponentopen);
		connect (parser, &Parser::signal_dispute, dlg, &GameDialog::slot_dispute);

		connect (dlg, &GameDialog::signal_matchsettings, qgoif, &qGoIF::slot_matchsettings);
	}

	if (myrequest)
	{
		dlg->buttonDecline->setDisabled(true);

		// my request: free/rated game is also requested
		//dlg->cb_free->setChecked(true);

		// teaching game:
		if (opponent == m_online_acc_name)
			dlg->buttonOffer->setText(tr("Teaching"));

		// If we came here through a context menu, we can check the player's nmatch setting.
		bool is_nmatch = !from_talk_tab && m_menu_player.nmatch;

		qDebug () << (is_nmatch ? "dlg is nmatch" : "dlg is not nmatch") << " with " << opponent;
		dlg->set_is_nmatch(is_nmatch);

		if (is_nmatch && m_menu_player.has_nmatch_settings ())
		{
			const auto &p = m_menu_player;
			int min_time = p.nmatch_timeMin / 60;
			int max_time = p.nmatch_timeMax / 60;
			if (max_time == 0 || max_time < min_time)
				min_time = 0, max_time = 60;
			dlg->timeSpin->setRange (min_time, max_time);
			int bmin_time = p.nmatch_BYMin / 60;
			int bmax_time = p.nmatch_BYMax / 60;
			if (bmax_time == 0 || bmax_time < bmin_time)
				bmin_time = 0, bmax_time = 60;
			dlg->byoTimeSpin->setRange (bmin_time, bmax_time);
			dlg->handicapSpin->setRange (p.nmatch_handicapMin, p.nmatch_handicapMax);
		}
		else
		{
			dlg->timeSpin->setRange (0, 60);
			dlg->byoTimeSpin->setRange (0, 60);
			dlg->handicapSpin->setRange (0, 9);
		}

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
		bool opp_plays_white = line.section(' ', 2, 2) == "B";
		bool opp_plays_nigiri = line.section(' ', 2, 2) == "N";

		QString handicap, size, time,byotime, byostones;

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
			dlg->boardSizeSpin->setRange(1,19);
			dlg->boardSizeSpin->setValue(size.toInt());
		}

		if (opp_plays_white)
			dlg->play_black_button->setChecked (true);
		else if (opp_plays_nigiri)
			dlg->play_nigiri_button->setChecked (true);
		else
			dlg->play_white_button->setChecked (true);

		dlg->buttonDecline->setEnabled(true);
		dlg->buttonOffer->setText(tr("Accept"));
		dlg->buttonCancel->setDisabled(true);
	}

	dlg->setting_changed();
	dlg->show();
	dlg->activateWindow();
	dlg->raise();
}

int ClientWindow::toggle_player_state (const char *list, const QString &symbol)
{
	int change = 0;
	// toggle watch list
	QString cpy = setting->readEntry (list).simplified () + ";";
	QString line;
	QString name;
	bool found = false;
	int cnt = cpy.count(';');

	for (int i = 0; i < cnt; i++)
	{
		name = cpy.section(';', i, i);
		if (!name.isEmpty())
		{
			if (name == m_menu_player.name)
				// skip player if found
				found = true;
			else
				line += name + ";";
		}
	}
	if (!found)
	{
		// not found -> add to list
		line += m_menu_player.name;
		// update player list
		if (m_menu_player.mark != "M")
		{
			change = 1;
			m_menu_player.mark = symbol;
		}
	}
	else if (line.length() > 0)
	{
		// skip one ";"
		line.truncate(line.length() - 1);
	}

	if (found)
	{
		if (m_menu_player.mark != "M")
		{
			change = -1;
			m_menu_player.mark = "";
		}
	}

	setting->writeEntry (list, line);

	auto it = m_player_table.find (m_menu_player.name);
	if (it != std::end (m_player_table))
		it->second.mark = m_menu_player.mark;

	return change;
}

void ClientWindow::slot_doubleclick_players (const Player &p)
{
	m_menu_player = p;
	handle_matchrequest (menu_player_name () + " " + m_menu_player.rank, true, false);
}

void ClientWindow::slot_menu_players (const QPoint& pt)
{
	QModelIndex idx = ListView_players->indexAt (pt);
	if (!idx.isValid ())
		return;
	const Player *p = m_player_model.find (idx);
	if (p != nullptr)
		slot_mouse_players (*p);
}

QString ClientWindow::menu_player_name ()
{
	// some invited players on IGS get a * after their name
	const QString &txt1 = m_menu_player.name;
	return txt1.right (1) == "*" ? txt1.left (txt1.length () - 1) : txt1;
}

// mouse click on ListView_players
void ClientWindow::slot_mouse_players (const Player &p)
{
	static QMenu *puw = nullptr;

	if (!puw)
	{
		puw = new QMenu (0, 0);
		puw->addAction (tr ("match"),
				[=] () { handle_matchrequest (menu_player_name () + " " + m_menu_player.rank, true, false); });
		puw->addAction (tr ("talk"),
				[=] () { slot_talk (menu_player_name (), QString (), true, false); });
		// puw->insertSeparator();
		puw->addAction (tr ("stats"),
				[=] () { slot_talk (menu_player_name (), QString (), true, false); });
		puw->addAction (tr ("stored games"),
				[=] () { sendcommand ("stored " + menu_player_name (), false); });
		puw->addAction (tr ("results"),
				[=] () { sendcommand ("result " + menu_player_name (), false); });
		puw->addAction (tr ("rating"),
				[=] () {
					// rating
					if (m_online_server == IGS)
						sendcommand ("prob " + menu_player_name (), false);
					else
						sendcommand ("rating " + menu_player_name (), false);
				});
		puw->addAction (tr ("observe game"),
				[=] () {
					QString game_id = m_menu_player.play_str;
					sendcommand ("observe " + game_id, false);
				});
		puw->addAction (tr ("trail"),
				[=] () { sendcommand ("trail " + menu_player_name (), false); });
		// puw->insertSeparator();
		puw->addAction (tr ("toggle watch list"),
				[=] () {
					num_watchedplayers += toggle_player_state ("WATCH", "W");
					watch = ";" + setting->readEntry ("WATCH") + ";";
					update_player_stats ();
				});
		puw->addAction (tr ("toggle exclude list"),
				[=] () {
					toggle_player_state ("EXCLUDE", "X");
					exclude = ";" + setting->readEntry ("EXCLUDE") + ";";
				});
	}

	m_menu_player = p;
	puw->popup (QCursor::pos ());
}

// release Talk Tabs
void ClientWindow::slot_pbRelTabs ()
{
	// seek dialog
	for (auto dlg: talklist) {
		if (dlg->get_name ().indexOf ('*') == -1)
			TabWidget_mini_2->removeTab (TabWidget_mini_2->indexOf (dlg));
	}
}

// release Talk Tabs
void ClientWindow::slot_pbRelOneTab (QWidget *w)
{
	// seek dialog
	for (auto dlg: talklist) {
		if (dlg == w) {
			TabWidget_mini_2->removeTab (TabWidget_mini_2->indexOf (w));
			return;
		}
	}

}

 // handle chat boxes in a list
void ClientWindow::slot_talk(const QString &name, const QString &text, bool isplayer, bool colored)
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
		dlg = new Talk (name, 0, isplayer);
		talklist.append (dlg);

		connect (dlg, &Talk::signal_talkto, this, &ClientWindow::slot_talkto);
		connect (dlg, &Talk::signal_pbRelOneTab, this, &ClientWindow::slot_pbRelOneTab);

		if (!name.isEmpty() && isplayer)
			sendcommand("stats " + name, false);    // automatically request stats

		// play sound on new created dialog
		bonus = true;

	}
	if (dlg) {
		QColor col;
		if (colored)
			col = setting->values.chat_color;
		dlg->write (txt, col);
		if (dlg->parentWidget () != TabWidget_mini_2)
		{
			TabWidget_mini_2->addTab (dlg, dlg->get_name());
			if (name != tr("Shouts*"))
				TabWidget_mini_2->setCurrentIndex (TabWidget_mini_2->indexOf (dlg));
		}
	}
	// check if it was a channel message
	autoAnswer &= isplayer && autoAwayMessage && !name.contains('*') && text.startsWith ('>');
	if (autoAnswer)
	{
		// send when qGo is NOT the active application - TO DO
		sendcommand("tell " + name + " [I'm away right now]");
	}

	// play a sound - not for shouts
	if (((text.startsWith ('>') && bonus) || !dlg->lineedit_has_focus()) && !name.contains('*'))
	{
		qgo->playTalkSound();

		// write time stamp
		MultiLineEdit3->append(statusOnlineTime->text() + " " + name + (autoAnswer ? " (A)" : ""));
	}
	else if (name == tr("msg*"))
	{
		qgo->playTalkSound();

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
		switch (m_online_server)
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
		slot_talk(receiver, "-> " + txt, true, true);
	}
}

// open a local board
void ClientWindow::slot_preferences(bool)
{
	dlgSetPreferences ();
}

void ClientWindow::initActions()
{
	void (QComboBox::*cact1) (int) = &QComboBox::activated;
	void (QComboBox::*cact2) (const QString &) = &QComboBox::activated;
	connect (cb_cmdLine, cact1, this, &ClientWindow::slot_cmdactivated_int);
	connect (cb_cmdLine, cact2, this, &ClientWindow::slot_cmdactivated);

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
	connect(fileNewBoard, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::none, screen_key (this)); });
	connect(fileNewVariant, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::variant, screen_key (this)); });
	connect(fileNew, &QAction::triggered, [=] (bool) { open_local_board (this, game_dialog_type::normal, screen_key (this)); });
	connect(fileOpen, &QAction::triggered, this, &ClientWindow::slotFileOpen);
	connect(fileOpenDB, &QAction::triggered, this, &ClientWindow::slotFileOpenDB);
	connect(fileBatchAnalysis, &QAction::triggered, this, [] (bool) { show_batch_analysis (); });
	connect(computerPlay, &QAction::triggered, [this] (bool) { play_engine (this); });
	connect(twoEnginePlay, &QAction::triggered, [this] (bool) { play_two_engines (this); });
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
	connect(viewToolBar, &QAction::toggled, this, &ClientWindow::slotViewToolBar);
	connect(viewMenuBar, &QAction::toggled, this, &ClientWindow::slotViewMenuBar);
	viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar."));
	connect(viewStatusBar, &QAction::toggled, this, &ClientWindow::slotViewStatusBar);

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

	connect (actionTutorials, &QAction::triggered, [] () { show_tutorials (); });
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

void ClientWindow::slotFileOpen (bool)
{
	std::shared_ptr<game_record> gr = open_file_dialog (this);
	if (gr == nullptr)
		return;

	MainWindow *win = new MainWindow (0, gr);
	win->show ();
}

void ClientWindow::slotFileOpenDB (bool)
{
	std::shared_ptr<game_record> gr = open_db_dialog (this);
	if (gr == nullptr)
		return;

	MainWindow *win = new MainWindow (0, gr);
	win->show ();
}

QList<Engine> ClientWindow::analysis_engines (int boardsize)
{
	QList<Engine> l;
	for (auto &e: setting->m_engines) {
		if (e.analysis && (!e.restrictions || e.boardsize.toInt () == boardsize))
			l.append (e);
	}
	return l;
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

void ClientWindow::stats_player(const Player &p)
{
	if (p.name.isEmpty())
		return;

	for (auto dlg: talklist) {
		if (dlg->get_name () == p.name) {
			dlg->stats_rating->setText (p.rank);
			dlg->stats_info->setText (p.info);
			dlg->stats_default->setText (p.extInfo);
			dlg->stats_wins->setText (p.won);
			dlg->stats_loss->setText (p.lost);
			dlg->stats_country->setText (p.country);
			dlg->stats_playing->setText (p.play_str);
			dlg->stats_rated->setText (p.rated);
			dlg->stats_address->setText (p.address);

			// stored either idle time or last access
			dlg->stats_idle->setText (p.idle);
			dlg->Label_Idle->setText (!p.idle.isEmpty() && p.idle.at(0).isDigit() ? "Idle :": "Last log :");
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

	refresh_players ();
}

void ClientWindow::slot_leaveRoom ()
{
	slot_enterRoom ("0");
}


void ClientWindow::slot_RoomListClicked(QListWidgetItem *qli)
{
	slot_enterRoom(qli->text().section(":",0,0));
}

void ClientWindow::slot_addSeekCondition(const QString& a, const QString& b, const QString& c, const QString& d, const QString& )
{
	QString time_condition;

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

void ClientWindow::slot_seek (int i)
{
	toolSeek->setChecked (true);
	toolSeek->setMenu (nullptr);

	//seek entry 1 19 5 3 0
	QString send_seek = "seek entry " + QString::number (i) + " 19 ";

	//childish, but we want to have an animated icon there
	seekButtonTimer = startTimer (200);

	switch (cb_seek_handicap->currentIndex ())
	{
		case 0:
			send_seek.append ("1 1 0");
			break;

		case 1:
			send_seek.append ("2 2 0");
			break;

		case 2:
			send_seek.append ("5 5 0");
			break;

		case 3:
			send_seek.append ("9 9 0");
			break;

		case 4:
			send_seek.append ("0 9 0");
			break;

		case 5:
			send_seek.append ("9 0 0");
			break;
	}

	sendcommand (send_seek, false);
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
	if (m_online_server != IGS || m_online_status == Status::offline)
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
	PreferencesDialog dlg (tab);

	if (dlg.exec() == QDialog::Accepted)
	{
		saveSettings();
	}
}

void ClientWindow::accept_preferences ()
{
	if (setting->engines_changed) {
		analyze_dialog->update_engines ();
	}

	// Update all boards with settings
	qgoif->update_settings ();
	qgo->updateAllBoardSettings ();
	update_font ();
	update_caption ();

	update_db_prefs ();

	if (setting->nmatch_settings_modified)
	{
		send_nmatch_range_parameters();
		setting->nmatch_settings_modified = false;
	}
	if (setting->hosts_changed) {
		populate_cbconnect (cb_connect->currentText ());
		setting->hosts_changed = false;
	}
	toolObserveMode->setChecked (setting->readBoolEntry ("OBSERVE_SINGLE"));
}
