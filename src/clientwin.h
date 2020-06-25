/*
 * mainwindow.h
 */

#ifndef MAINWIN_H
#define MAINWIN_H

#include <list>
#include <unordered_map>

#include "ui_clientwindow_gui.h"
#include "miscdialogs.h"
#include "setting.h"
#include "tables.h"
#include "config.h"
#include "telnet.h"
#include "parser.h"
#include "gs_globals.h"
#include "gamestable.h"
#include "playertable.h"
#include "mainwindow.h"

class GamesTable;
class GameDialog;

struct sendBuf
{
	QString txt;
	bool localecho;

	sendBuf (QString text, bool echo=true) { txt = text; localecho = echo; }
};

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#include <QHash>

namespace std
{
    template<> struct hash<QString>
    {
        std::size_t operator() (const QString& s) const noexcept
        {
            return (size_t)qHash (s);
        }
    };
}
#endif

class ClientWindow : public QMainWindow, public Ui::ClientWindowGui
{
	Q_OBJECT

	TelnetConnection *telnetConnection;
	Parser *parser;
	qGoIF *qgoif;

	QString m_standard_caption;

	bool m_player7_active = false;
	bool m_playerlist_active = false;
	bool m_player_init_complete = false;
	bool m_games_init_complete = false;

	// online time
	int onlineCount;
	bool youhavemsg;
	bool playerListEmpty;
	bool playerListSteadyUpdate;
	bool gamesListSteadyUpdate;
	bool autoAwayMessage;

	bool m_whoopen = false;
	/* Initialize both to prevent the code that keeps them in order
	   to cause problems when they are initialized with the real values.  */
	QString m_who1_rk = "zz";
	QString m_who2_rk = "a";

	// cmd_xx get current cmd number, and, if higher than last, cmd is valid,
	//    else cmd is from history list
	int cmd_count;
	bool cmd_valid;

	// telnet ready
	bool tn_ready;
	bool tn_wait_for_tn_ready;

	ChannelList channellist;
	QList<Talk *> talklist;
	QList<GameDialog *> matchlist;
	std::list<sendBuf> m_send_buffer;
	std::unordered_map<QString, Player> m_player_table;
	std::unordered_map<QString, Game> m_games_table;
	table_model<Game> m_games_model;
	table_model<Player> m_player_model;

	QLabel *statusUsers, *statusGames, *statusServer, *statusChannel;
	QLabel *statusOnlineTime, *statusMessage;

	QString watch;
	QString exclude;

	// frame sizes
	QPoint view_p;
	QSize view_s;
	QPoint pref_p;
	QSize pref_s;

	// Entries corresponding to the currently shown context menu
	Player m_menu_player;
	Game m_menu_game;

	// statistics
	int bytesIn, bytesOut;

	// event filter
	int seekButtonTimer;
	int oneSecondTimer;

	// menus
	QIcon seekingIcon[4], NotSeekingIcon;
	QMenu *seekMenu;

	QAction *whatsThis, *escapeFocus;

	// timing aids
	int counter;

	// Statistics
	int num_watchedplayers = 0;
	int num_observedgames = 0;

	// Online status
	Status m_online_status;
	GSName m_online_server;
	QString m_online_server_name;
	QString m_online_acc_name;
	QString m_online_rank;

	// reset internal counter (quarter hour) for timing functions of class ClientWindow
	void resetCounter() { counter = 899; }
	void timerEvent (QTimerEvent*);

	void clear_server_data ();

	void initStatusBar (QWidget*);
	void initToolBar ();
	void initActions ();
	void update_font ();
	void update_caption ();

	void set_tn_ready ();
	void try_send ();
	void enqueue_command (const QString &, bool localecho = true);
	int sendTextFromApp (const QString&, bool localecho=true);
	void set_sessionparameter (QString, bool);
	void send_nmatch_range_parameters ();

	int toggle_player_state (const char *list, const QString &symbol);
	QString getPlayerRk (QString);

	void colored_message (QString, QColor);

	void populate_cbconnect (const QString &);
	void enable_connection_buttons (bool);

	void saveSettings ();

	QString menu_player_name ();

	void prepare_channels ();
	void prepare_game_list ();
	void prepare_player_list ();
	void finish_game_list ();
	void finish_player_list ();

	void update_player_stats ();
	void update_game_stats ();

	bool filter_game (const Game &);
	bool filter_player (const Player &);

	void refresh_players ();
	void refresh_games ();
	void update_tables ();

	void update_olq_state_from_player_info (const Player &);
	void update_who_rank (QComboBox *, int);
public:
	ClientWindow (QMainWindow* parent);
	~ClientWindow ();

	void sendcommand(const QString&, bool localecho=false);
	void savePrefFrame(QPoint p, QSize s) { pref_p = p; pref_s = s; }
	QPoint getViewPos() { return view_p; }
	QSize getViewSize() { return view_s; }
	QPoint getPrefPos() { return pref_p; }
	QSize getPrefSize() { return pref_s; }

	void accept_preferences ();
	void dlgSetPreferences(int tab = 0);

	Engine *analysis_engine (int boardsize);
	QList<Engine> analysis_engines (int boardsize);

	/* Called from parser.cpp.  */
	void server_add_game (const Game &);
	void server_remove_game (const QString &);

	void server_add_player (const Player &, bool);
	void server_player_entered (const QString &);
	void server_remove_player (const QString &);

	void stats_player (const Player &);

	void set_open_mode (bool);
	void set_looking_mode (bool);
	void set_quiet_mode (bool);

	void update_observed_games (int);
	void handle_matchrequest (const QString&, bool, bool);

public slots:
	void slot_pbRelTabs ();
	void slot_pbRelOneTab (QWidget*);

	void slot_whoopen (bool);

	// gui_BaseTable:
	void slot_cblooking (bool);
	void slot_cbopen (bool);
	void slot_cbquiet (bool);
	void quit (bool = false);
	void slot_last_window_closed ();
	void slot_cbconnect (const QString&);
	void slot_connect (bool);
	void slot_pbrefreshgames (bool);
	void slot_pbrefreshplayers (bool);
	void slot_cmdactivated (const QString&);
	void slot_cmdactivated_int (int);
	void slot_preferences (bool = false);

	// telnet:
	void sendTextToApp (const QString&);
	// parser:
  	void slot_message (QString);
	void slot_svname (GSName&);
	void slot_accname (QString&);
	void slot_status (Status);
	void slot_connclosed ();
	void slot_talk (const QString&, const QString&, bool);
	void slot_channelinfo (int, const QString&);
  	void slot_removeMatchDialog (const QString&);
	void slot_shout (const QString&, const QString&);
	void slot_room (const QString&, bool);
	void slot_enterRoom (const QString&);
	void slot_leaveRoom ();
	void slot_RoomListClicked (QListWidgetItem*);
	void slot_addSeekCondition (const QString&, const QString&, const QString&, const QString&, const QString&);
	void slot_clearSeekCondition ();
	void slot_cancelSeek ();
	void slot_seek (int i);
	void slot_seek (bool);
	void slot_SeekList (const QString&, const QString&);

	// gamestable/playertable:
	void slot_mouse_games (const Game &);
	void slot_mouse_players (const Player &);
	void slot_doubleclick_games (const Game &);
	void slot_doubleclick_players (const Player &);
	void slot_menu_games (const QPoint&);
	void slot_menu_players (const QPoint&);

	// gui_talkdialog:
	void slot_talkto (QString&, QString&);

	//menus
	void slotFileOpen (bool);
	void slotFileOpenDB (bool);

	void slotMenuConnect (bool);
	void slotMenuDisconnect (bool);
	void slotMenuEditServers (bool);

	void slotViewToolBar (bool toggle);
	void slotViewStatusBar (bool toggle);
	void slotViewMenuBar (bool toggle);
};

/* Constructed in main, potentially hidden but always present.  */
extern ClientWindow *client_window;

#endif
