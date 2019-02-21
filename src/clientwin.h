/*
 * mainwindow.h
 */

#ifndef MAINWIN_H
#define MAINWIN_H

#include "ui_clientwindow_gui.h"
#include "miscdialogs.h"
#include "setting.h"
#include "tables.h"
#include "config.h"
#include "telnet.h"
#include "igsinterface.h"
#include "parser.h"
#include "gs_globals.h"
#include "gamestable.h"
#include "playertable.h"
#include "mainwindow.h"

class GamesTable;
class GameDialog;

class sendBuf
{
public:
	sendBuf(QString text, bool echo=true) { txt = text; localecho = echo; }
	~sendBuf() {}
	QString get_txt() { return txt; }
	bool get_localecho() { return localecho; }
	QString txt;

private:
	bool localecho;
};

class ClientWindow : public QMainWindow, public Ui::ClientWindowGui
{
	Q_OBJECT

public:
	ClientWindow(QMainWindow* parent);
	~ClientWindow();

	int sendTextFromApp(const QString&, bool localecho=true);
	void sendcommand(const QString&, bool localecho=false);
	void prepare_tables(InfoType);
	void set_sessionparameter(QString, bool);
	void send_nmatch_range_parameters();

	QString getPlayerRk(QString);
	QString getPlayerExcludeListEntry(QString);
	void saveMenuFrame(QPoint p, QSize s) { menu_p = p; menu_s = s; }
	void savePrefFrame(QPoint p, QSize s) { pref_p = p; pref_s = s; }
	QPoint getMenuPos() { return menu_p; }
	QSize getMenuSize() { return menu_s; }
	QPoint getViewPos() { return view_p; }
	QSize getViewSize() { return view_s; }
	QPoint getPrefPos() { return pref_p; }
	QSize getPrefSize() { return pref_s; }
	void setColumnsForExtUserInfo();
	void setBytesIn(int i) { if (i == -1) bytesIn = 0; else bytesIn += i; }
	void setBytesOut(int i) { if (i == -1) bytesOut = 0; else bytesOut += i; }
	void saveSettings();

	bool preferencesAccept();
	void dlgSetPreferences(int tab=-1);

	Engine *analysis_engine (int boardsize);
	QList<Engine> analysis_engines (int boardsize);

	HostList hostlist;
	QList<Engine *> m_engines;

signals:
	void signal_cmdsent(const QString&);
	void signal_move(Game*);
	void signal_computer_game(QNewGameDlg*);       //SL added eb 12

public slots:
	void slot_setBytesIn(int i) { setBytesIn(i); }
	void slot_setBytesOut(int i) { setBytesOut(i); }

	void slot_updateFont();
	void slot_refresh(int);
	void slot_playerPopup(int);
	void slot_gamesPopup(int);
//	void slot_channelPopup(int);
	void slot_pbRelTabs();
	void slot_pbRelOneTab(QWidget*);
	void slot_statsPlayer(Player*);

	void slot_whoopen (bool);
	// QWidget
//	virtual void resizeEvent (QResizeEvent *);

	// gui_BaseTable:
	virtual void slot_cbExtUserInfo();
	virtual void slot_cblooking(bool);
	virtual void slot_cbopen(bool);
	virtual void slot_cbquiet(bool);
	virtual void quit(bool);
	virtual void slot_last_window_closed();
	virtual void slot_cbconnect(const QString&);
	virtual void slot_connect(bool);
	virtual void slot_pbrefreshgames(bool);
	virtual void slot_pbrefreshplayers(bool);
	virtual void slot_toolbaractivated(const QString&);
	virtual void slot_cmdactivated(const QString&);
	virtual void slot_cmdactivated_int(int);
	virtual void slot_watchplayer(const QString&);
	virtual void slot_excludeplayer(const QString&);
	virtual void slot_preferences(bool = false);
//	virtual void slot_userDefinedKeysTextChanged();
	virtual void slot_pbuser1();
	virtual void slot_pbuser2();
	virtual void slot_pbuser3();
	virtual void slot_pbuser4();

	// telnet:
	void sendTextToApp(const QString&);
	// parser:
	void slot_player(Player*, bool);
	void slot_game(Game*);
  	void slot_message(QString);
	void slot_svname(GSName&);
	void slot_accname(QString&);
	void slot_status(Status);
	void slot_connclosed();
	void slot_talk(const QString&, const QString&, bool);
	void slot_checkbox(int, bool);
	void slot_addToObservationList(int);
	void slot_channelinfo(int, const QString&);
	void slot_matchrequest(const QString&, bool);
  	void slot_removeMatchDialog(const QString&);
	void slot_shout(const QString&, const QString&);
	void slot_room(const QString&, bool );
	void slot_enterRoom(const QString& );
	void slot_leaveRoom();
	void slot_RoomListClicked(QListWidgetItem*);
	void slot_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&);
	void slot_clearSeekCondition();
	void slot_cancelSeek();
	void slot_seek(int i);
	void slot_seek(bool);
	void slot_SeekList(const QString&, const QString&);

	// gamestable/playertable:
	virtual void slot_mouse_games(int, QTreeWidgetItem*);
	virtual void slot_mouse_players(int, QTreeWidgetItem*);
	virtual void slot_click_games(QTreeWidgetItem*);
	virtual void slot_click_players(QTreeWidgetItem *);
	virtual void slot_menu_games(const QPoint&);
	virtual void slot_menu_players(const QPoint&);
//	void slot_moveOver_players();
//	void slot_moveOver_games();
	void slot_playerContentsMoving(int x, int y);
	void slot_gamesContentsMoving(int x, int y);
	// gui_talkdialog:
	virtual void slot_talkto(QString&, QString&);
	// gamedialog, qgoif
	void slot_sendcommand(const QString&, bool);
	//menus
	void slotFileOpen(bool);
	void slotComputerPlay(bool);

	void slotMenuConnect(bool);
	void slotMenuDisconnect(bool);
	void slotMenuEditServers(bool);

	void slotViewToolBar(bool toggle);
	void slotViewStatusBar(bool toggle);
	void slotViewMenuBar(bool toggle);


protected slots:
	void set_tn_ready();

private:
	TelnetConnection   *telnetConnection;
	Parser             *parser;
	qGoIF              *qgoif;
	Account            *myAccount;
//	QTextCodec         *textCodec;
//	QTextStream        *textStream;
//	QString            buffer;

	// online time
	int                onlineCount;
	bool               youhavemsg;
	bool               playerListEmpty;
	bool               playerListSteadyUpdate;
	bool               gamesListSteadyUpdate;
//	bool               gamesListEmpty;
	bool               autoAwayMessage;

	// cmd_xx get current cmd number, and, if higher than last, cmd is valid,
	//    else cmd is from history list
	int                cmd_count;
	bool               cmd_valid;
	// telnet ready
	bool               tn_ready;
	bool               tn_wait_for_tn_ready;
	//	bool               tn_active;

	ChannelList channellist;
	QList<Talk *> talklist;
	QList<GameDialog *> matchlist;
	QList<sendBuf *> sendBuffer;

	QLabel *statusUsers, *statusGames, *statusServer, *statusChannel;
	QLabel *statusOnlineTime, *statusMessage;

	// timing aids
	void 		timerEvent(QTimerEvent*);
	int		counter;

	// reset internal counter (quarter hour) for timing functions of class ClientWindow
	void		resetCounter() { counter = 899; }

	void initStatusBar(QWidget*);
	//void initmenus(QWidget*);
	void initToolBar();
	void initActions();

	int toggle_player_state (const char *list, const QString &symbol);

	void colored_message(QString, QColor);

	QString		watch;
	QString		exclude;

	// frame sizes
	QPoint		menu_p;
	QSize		menu_s;
	QPoint		view_p;
	QSize		view_s;
	QPoint		pref_p;
	QSize		pref_s;

	// popup window save
	PlayerTableItem	*lv_popupPlayer;
	GamesTableItem	*lv_popupGames;

	// extended user info
	bool		extUserInfo;

	// statistics
	int		bytesIn, bytesOut;

	// event filter
 	int		seekButtonTimer ;
	int oneSecondTimer;

  	// menus
	QIcon 	seekingIcon[4], NotSeekingIcon;

	QMenu *seekMenu;
	//QStringList 	*seekConditionList ;

	QAction *whatsThis, *escapeFocus;
};

/* Constructed in main, potentially hidden but always present.  */
extern ClientWindow *client_window;

#endif
