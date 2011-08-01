
/*
 *   qgo_interface.h
 */

#ifndef QGO_INTERFACE_H
#define QGO_INTERFACE_H

#undef SHOW_INTERNAL_TIME
#undef SHOW_MOVES_DLG

#include "tables.h"
#include "defines.h"
#include "globals.h"
#include "gs_globals.h"
#include "setting.h"
#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>
#include <q3ptrlist.h>

// from qGo:
#include "qgo.h"
//Added by qt3to4:
#include <QTimerEvent>
#include "mainwindow.h"
//#include "qgoclient_interface.h"


//-----------

class Game;

//-----------

class GameInfo
{
public:
	GameInfo() {};
	~GameInfo() {};
	// 15 Game 43 I: xxxx (4 223 16) vs yyyy (5 59 13)
	// 15 144(B): B12
	QString nr;
	QString type;
	QString wname;
	QString wprisoners;
	QString wtime;
	QString wstones;
	QString bname;
	QString bprisoners;
	QString btime;
	QString bstones;
	QString mv_col;	// T..Time, B..Black, W..White, I..Init
	QString mv_nr;
	QString mv_pt;
};

//-----------

class qGoBoard : public QObject
{
	Q_OBJECT

public:
	qGoBoard(qGoIF*, qGo*);
	~qGoBoard();
	int get_id() const { return id; }
	void set_id(int i) { id = i; gd.gameNumber = i; }
	GameData get_gameData() { return gd; }
	void set_title(const QString&);
	bool get_haveTitle() { return haveTitle; }
	void set_komi(const QString&);
	void set_freegame(bool);
	bool get_havegd() { return have_gameData; }
	bool get_sentmovescmd() { return sent_movescmd; }
	void set_sentmovescmd(bool m) { sent_movescmd = m; }
	bool get_adj() { return adjourned; }
	QString get_bplayer() { return gd.playerBlack; }
	QString get_wplayer() { return gd.playerWhite; }
	void set_adj(bool a) { adjourned = a; }
	void set_game(Game *g);

	void set_Mode(int);
	GameMode get_Mode() { return gameMode; }
	void set_move(StoneColor, QString, QString);
	void send_kibitz(const QString);
	MainWindow *get_win() { return win; }
	void initGame() { win->getBoard()->initGame(&gd); }
	void setGameData() {win->getBoard()->setGameData(&gd); }
	void setMode() { win->getBoard()->setMode(gameMode); }
	void setTimerInfo(const QString&, const QString&, const QString&, const QString&);
	void timerEvent(QTimerEvent*);
	QString secToTime(int);
	void set_stopTimer();
	void set_runTimer();
	void set_gamePaused(bool p) { game_paused = p; }
	int get_boardsize() { return gd.size; }
	int get_mvcount() { return mv_counter; }
	void set_myColorIsBlack(bool b);
	bool get_myColorIsBlack() { return myColorIsBlack; }
	void set_requests(const QString &handicap, const QString &komi, assessType);
	void check_requests();
	QString get_reqKomi() { return req_komi; }
	QString get_currentKomi() { return QString::number(gd.komi); }
	void dec_mv_counter() { mv_counter--; }
	int get_mv_counter() { return mv_counter; }
	bool get_requests_set() { return requests_set; }
	qGo* get_qgo() { return qgo; }
	void set_gsName(GSName g) { gsName = g; }
	void addtime_b(int m);
	void addtime_w(int m);
	void set_myName(const QString &n) { myName = n; }
	void clearObserverList() { win->getListView_observers()->clear(); }
	void playComputer(StoneColor);

	// teaching features
	bool        ExtendedTeachingGame;
	bool        IamTeacher;
	bool        IamPupil;
	bool        havePupil;
	bool        haveControls;
	QString     ttOpponent;
	bool        mark_set;
	int         mark_counter;
	GameData    gd;

signals:
	// to qGoIF
//	void signal_closeevent(int);
	void signal_sendcommand(const QString&, bool);
	void signal_2passes(const QString&, const QString&);
  
public slots:
	// MainWindow
	void slot_closeevent();
	void slot_sendcomment(const QString&);

	// Board
	void slot_addStone(enum StoneColor, int, int);
	void slot_stoneComputer(enum StoneColor, int, int);    
	void slot_PassComputer(StoneColor c) ;                 
	void slot_UndoComputer(StoneColor c) ;                 
 	void slot_DoneComputer() ;                 
	void slot_doPass();
	void slot_doResign();
	void slot_doUndo();
	void slot_doAdjourn();
	void slot_doDone();
	void slot_doRefresh();

	// normaltools
	void slot_addtimePauseW();
	void slot_addtimePauseB();

	// teachtools
	void slot_ttOpponentSelected(const QString&);
	void slot_ttControls(bool);
	void slot_ttMark(bool);

private:
	int timer_id;
	bool		game_paused;
	bool        have_gameData;
	bool        sent_movescmd;
	bool        adjourned;
	bool        myColorIsBlack;
	bool        haveTitle;
	GameMode    gameMode;
	//GameData    gd;
	int         id;
	MainWindow  *win;
	qGo         *qgo;
	int         mv_counter;
	int	        stated_mv_count ;
	bool		    sound;
	int         bt_i, wt_i;
	QString     bt, wt;
	QString     b_stones, w_stones;
	QString     req_handicap;
	QString     req_komi;
	assessType  req_free;
	bool		    requests_set;
	GSName      gsName;
	QString     myName;
  int         BY_timer;

#ifdef SHOW_INTERNAL_TIME
	int chk_b, chk_w;
#endif
};

//-----------

class qGoIF : public QObject
{
	Q_OBJECT

public:
	qGoIF(QWidget*);
	~qGoIF();
//	bool set_observe(QString);
	void set_initIF();
	void set_myName(const QString &n) { myName = n; }
	qGo  *get_qgo() { return qgo; };
	void set_gsName(GSName n) { gsName = n; }
	void set_localboard(QString file=QString::null);
	void set_localgame();
	void openPreferences(int tab=-1);
	QWidget *get_parent() { return parent; }
	void wrapupMatchGame(qGoBoard *, bool);

signals:
	void signal_sendcommand(const QString&, bool);
	void signal_addToObservationList(int);

public slots:
	// parser/mainwindow
	void slot_move(GameInfo*);
	void slot_move(Game*);
	void slot_computer_game(QNewGameDlg*);
	void slot_kibitz(int, const QString&, const QString&);
	void slot_title(const QString&);
	void slot_komi(const QString&, const QString&, bool);
	void slot_freegame(bool);
	void slot_matchcreate(const QString&, const QString&);
	void slot_removestones(const QString&, const QString&);
	void slot_undo(const QString&, const QString&);
	void slot_result(const QString&, const QString&, bool, const QString&);
	void slot_matchsettings(const QString&, const QString&, const QString&, assessType);
	void slot_requestDialog(const QString&, const QString&, const QString&, const QString&);
	void slot_timeAdded(int, bool);
	//void slot_undoRequest(const QString &);

	// qGoBoard
//	void slot_closeevent(int) {};
	void slot_closeevent();
	void slot_sendcommand(const QString&, bool);
	void set_observe(const QString&);

protected:
	bool parse_move(int src, GameInfo* gi=0, Game* g=0, QString txt=QString::null);

private:
	qGo     *qgo;
	QWidget *parent;
	// actual pointer, for speedup reason
	qGoBoard *qgobrd;
	QString  myName;
	Q3PtrList<qGoBoard> *boardlist;
	GSName   gsName;
	int      localBoardCounter;
//	int      lockObserveCmd;
};

#endif

