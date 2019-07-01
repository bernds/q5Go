/*
 *   qgo_interface.cpp - qGoClient's interface to qGo
 */

#include "qgo.h"
//Added by qt3to4:
#include <QTimerEvent>
#include "mainwindow.h"
#include "qgo_interface.h"
#include "tables.h"
#include "defines.h"
#include "gs_globals.h"
#include "misc.h"
#include "config.h"
#include "clientwin.h"
#include "ui_helpers.h"

#include <qstring.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qregexp.h>

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

class MainWindow_IGS : public MainWindow
{
	qGoBoard *m_connector;
public:
	MainWindow_IGS (QWidget *parent, std::shared_ptr<game_record> gr, QString screen_key,
			qGoBoard *connector, bool playing_w, bool playing_b, GameMode mode);
	~MainWindow_IGS ();

	virtual game_state *player_move (int x, int y) override
	{
		game_state *st = MainWindow::player_move (x, y);
		if (st == nullptr)
			return st;
		m_connector->move_played (x, y);
		return st;
	}
	virtual void player_toggle_dead (int x, int y) override
	{
		m_connector->player_toggle_dead (x, y);
	}
	virtual void doPass () override
	{
		if (gfx_board->player_to_move_p ())
			m_connector->slot_doPass ();
	}
	virtual void doResign () override
	{
		m_connector->slot_doResign ();
	}
	void playPassSound ()
	{
		if (local_stone_sound)
			qgo->playPassSound ();
	}
	void playClick ()
	{
		if (local_stone_sound)
			qgo->playStoneSound ();
	}
};

MainWindow_IGS::MainWindow_IGS (QWidget *parent, std::shared_ptr<game_record> gr, QString screen_key,
				qGoBoard *brd, bool playing_w, bool playing_b, GameMode mode)
	: MainWindow (parent, gr, screen_key, mode), m_connector (brd)
{
	gfx_board->set_player_colors (playing_w, playing_b);
	/* Update clock tooltips after recording the player colors.  */
	setGameMode (mode);
}

MainWindow_IGS::~MainWindow_IGS ()
{
}

/*
 *	Playing or Observing
 */

qGoIF::qGoIF(QWidget *p) : QObject()
{
	qgo = new qGo();

	ASSERT(qgo);

	parent = p;
	qgobrd = 0;
	gsName = GS_UNKNOWN;
	localBoardCounter = 10000;
}

qGoIF::~qGoIF()
{
	delete qgo;
}

qGoBoard *qGoIF::find_game_id (int id)
{
	for (auto qb: boardlist)
		if (qb->get_id() == id) {
			return qb;
		}
	return nullptr;
}

qGoBoard *qGoIF::find_game_players (const QString &pl1, const QString &pl2)
{
	for (auto qb: boardlist) {
		if (!qb->get_havegd ())
			continue;
		const QString qb1 = qb->get_wplayer ();
		const QString qb2 = qb->get_bplayer ();
		if (qb1 == pl1 && (pl2.isEmpty () || qb2 == pl2))
			return qb;
		if (qb2 == pl1 && (pl2.isEmpty () || qb1 == pl2))
			return qb;
	}
	return nullptr;
}

void qGoIF::game_end (qGoBoard *qb, const QString &txt)
{
	if (qb == nullptr)
		return;

	// stopped game do not need a timer
	qb->set_stopTimer();

	if (txt == "-")
		return;

	qb->send_kibitz(txt + "\n");

	// set correct result entry
	QString rs = QString::null;
	QString extended_rs = txt;

	if (txt.contains("White forfeits"))
		rs = "B+T";
	else if (txt.contains("Black forfeits"))
		rs = "W+T";
	else if (txt.contains("has run out of time"))
		rs = ((( txt.contains(myName) && qb->get_myColorIsBlack() ) ||
		       ( !txt.contains(myName) && !qb->get_myColorIsBlack() )) ?
		      "W+T" : "B+T");

	else if (txt.contains("W ",  Qt::CaseSensitive)
		 && txt.contains("B ",  Qt::CaseSensitive))
	{
		// NNGS: White resigns. W 59.5 B 66.0
		// IGS: W 62.5 B 93.0
		// calculate result first
		int posw, posb;
		float re1, re2;
		posw = txt.indexOf("W ");
		posb = txt.indexOf("B ");
		bool wfirst = posw < posb;

		if (!wfirst)
		{
			int h = posw;
			posw = posb;
			posb = h;
		}

		QString t1 = txt.mid(posw+1, posb-posw-2);
		QString t2 = txt.right(txt.length()-posb-1);
		re1 = t1.toFloat();
		re2 = t2.toFloat();

		re1 = re2-re1;
		if (re1 < 0)
		{
			re1 = -re1;
			wfirst = !wfirst;
		}

		if (re1 < 0.2)
		{
			rs = "Jigo";
			extended_rs = "        Jigo         ";
		}
		else if (wfirst)
		{
			rs = "B+" + QString::number(re1);
			extended_rs = "Black won by " + QString::number(re1) + " points";
		}
		else
		{
			rs = "W+" + QString::number(re1);
			extended_rs = "White won by " + QString::number(re1) + " points";
		}
	}
	else if (txt.contains("White resigns"))
		rs = "B+R";
	else if (txt.contains("Black resigns"))
		rs = "W+R";
	else if (txt.contains("has resigned the game"))
		rs = ((( txt.contains(myName) && qb->get_myColorIsBlack() ) ||
		       ( !txt.contains(myName) && !qb->get_myColorIsBlack() )) ?
		      "W+R" : "B+R");

	if (!rs.isNull())
	{
		qb->game_result (rs, extended_rs);
	}

	if (txt.contains("adjourned"))
	{
		if (qb->get_havegd ()) {
			QString wplayer = qb->get_wplayer();
			QString bplayer = qb->get_bplayer();
			if (bplayer == myName || wplayer == myName)
				// can only reload own games
				qb->get_win()->refreshButton->setText(tr("LOAD"));
			qDebug() << "game adjourned... #" << QString::number(qb->get_id());
			qb->set_adj(true);
		}
		qb->disconnected (true);
	}

	n_observed--;
	client_window->update_observed_games (n_observed);
}

void qGoIF::game_end (const QString &id, const QString &txt)
{
	game_end (find_game_id (id.toInt ()), txt);
}

void qGoIF::game_end (const QString &pl1, const QString &pl2, const QString &txt)
{
	game_end (find_game_players (pl1, pl2), txt);
}

// handle move info and distribute to different boards
bool qGoIF::parse_move(int src, GameInfo* gi, Game* g, QString txt)
{
#ifdef SHOW_MOVES_DLG
	static Talk *dlg;
#endif

	int         game_id = 0;
	stone_color sc = none;
	QString     pt;
	QString     mv_nr;
	GameMode    mode = modeObserve;
	bool is_own = false;

	switch (src)
	{
		// regular move info (parser)
		case 0:
 			game_id = gi->nr.toInt();
			if (gi->mv_col == QString("B"))
				sc = black;
			else if (gi->mv_col == QString("W"))
				sc = white;
			else
				sc = none;
			mv_nr = gi->mv_nr;
			pt = gi->mv_pt;
			break;

		// adjourn/end/resume of a game (parser)
		case 1:
			// check if g->nr is a number
			if (g->nr == QString("@"))
			{
				qGoBoard *qb = find_game_players (myName, QString ());
				if (qb == nullptr)
				{
					qWarning("*** You are not playing a game to be adjourned!");
					return false;
				}
				game_id = qb->get_id ();
			}
			else
				game_id = g->nr.toInt();

			if (g->Sz == QString("@") || g->Sz == QString("@@"))
			{
				// look for adjourned games
				for (auto qb: boardlist) {
					if (qb->get_adj() && qb->get_id() == 10000 &&
					    qb->get_bplayer() == g->bname &&
					    qb->get_wplayer() == g->wname)
					{
						qDebug("ok, adjourned game found ...");
						if (g->bname != myName && g->wname != myName)
							// observe if I'm not playing
							client_window->sendcommand ("observe " + g->nr, false);
						else
							// ensure that my game is correct stated
							client_window->sendcommand ("games " + g->nr, false);
						qb->set_sentmovescmd(true);
						qb->set_id(g->nr.toInt());
#if 0
						qb->setGameData();
						qb->setMode();
#endif

						qb->set_adj(false);
//						qb->get_win()->toggleMode();
						qb->set_runTimer();
						qb->set_sentmovescmd(true);
						client_window->sendcommand ("moves " + g->nr, false);
						client_window->sendcommand ("all " + g->nr, false);

						qb->send_kibitz(tr("Game continued as Game number %1").arg(g->nr));
						// show new game number;
						qb->get_win()->update_game_record ();

						// renew refresh button
						qb->get_win()->refreshButton->setText(QObject::tr("Refresh", "button label"));

						n_observed++;
						client_window->update_observed_games (n_observed);

						return true;
					}
				}
			}

			// special case: your own game
			if (g->Sz == QString("@@"))// || g->bname == myName || g->wname == myName)
			{
				is_own = true;
				// set up new game
				if (g->bname == g->wname)
					// teaching game
					src = 4, mode = modeTeach;
				else
					// match
					src = 3, mode = modeMatch;
			}
			break;

		// game to observe
		case 2:
			game_id = txt.toInt();
			mode = modeObserve;
			break;

		// match
		case 3:
			game_id = txt.toInt();
			mode = modeMatch;
			break;

		// teaching game
		case 4: game_id = txt.toInt();
			mode = modeTeach;
			break;

		default:
			qWarning("*** qGoIF::parse_move(): unknown case !!! ***");
			return false;
	}

#ifdef SHOW_MOVES_DLG
	// dialog recently used?
	if (!dlg)
	{
		dlg = new Talk("SPIEL", parent, true);
		connect(dlg->get_dialog(), SIGNAL(signal_talkto(QString&, QString&)), parent, SLOT(slot_talkto(QString&, QString&)));

		CHECK_PTR(dlg);
		dlg->write("New Dialog created\n");
	}
#endif

	// board recently used?
	if (!qgobrd || qgobrd->get_id() != game_id)
	{
		// seek dialog
		qgobrd = nullptr;
		for (auto b: boardlist) {
			if (b->get_id () == game_id) {
				qgobrd = b;
				break;
			}
		}
		// not found -> create new dialog
		if (!qgobrd)
		{
			// setup only with mouseclick or automatic in case of matching !!!!
			// added case 0: 'observe' cmd
			if (src < 2) //(src == 1) //
			{
				return false;
			}

			qgobrd = new qGoBoard (this, game_id);
			boardlist.append(qgobrd);

//			qgobrd->get_win()->setOnlineMenu(true);

			CHECK_PTR(qgobrd);

			if (mode == modeObserve) // (src == 2)
			{
				qgobrd->set_sentmovescmd(true);
				client_window->sendcommand ("games " + txt, false);
				client_window->sendcommand ("moves " + txt, false);
				client_window->sendcommand ("all " + txt, false);
			}

			// for own games send "games" cmd to get full info
			if (mode == modeMatch || mode == modeTeach) // (src == 3 || src == 4 || src == 13 || src == 14 )//|| src == 0)
			{
				if (is_own)//|| src== 0)
				{
					// src changed from 1 to 3/4
					qgobrd->set_sentmovescmd(true);
					client_window->sendcommand ("games " + g->nr, false);
					client_window->sendcommand ("moves " + g->nr, false);
					client_window->sendcommand ("all " + g->nr, false);
				}
				else
					client_window->sendcommand ("games " + txt, false);

				if (mode == modeTeach)
				{
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = true;
					qgobrd->havePupil = false;
					qgobrd->mark_set = false;
				}
			}

			// set correct mode in qGo
			qgobrd->set_gsName (gsName);
			qgobrd->set_myName (myName);
			if (src==0)
				qgobrd->set_Mode_real (modeObserve); // special case when not triggered by the 'observe' command (trail, for instance)
			else
				qgobrd->set_Mode_real (mode);

			n_observed++;
			client_window->update_observed_games (n_observed);

			return true;
		}
	}

	switch (src)
	{
		// regular move info (parser)
		case 0:
#ifdef SHOW_MOVES_DLG
			dlg->write("Spiel " + gi->nr + gi->type + "\n");
#endif

			if (gi->mv_col == "T")
			{
#ifdef SHOW_MOVES_DLG
				dlg->write("W: " + gi->wname + " " + gi->wprisoners + " " + gi->wtime + " " + gi->wstones + "\n");
				dlg->write("B: " + gi->bname + " " + gi->bprisoners + " " + gi->btime + " " + gi->bstones + "\n");
#endif

				// set times
				qgobrd->setTimerInfo(gi->btime, gi->bstones, gi->wtime, gi->wstones);
			}
			else if (gi->mv_col == "B" || gi->mv_col == "W")
			{
#ifdef SHOW_MOVES_DLG
				dlg->write(gi->mv_col + " " + gi->mv_nr + " " + gi->mv_pt + "\n");
#endif

				// set move if game is initialized
				if (qgobrd->get_havegd())
				{
					// get all moves till now one times if kibitzing
					if (qgobrd->get_Mode() == modeObserve && !qgobrd->get_sentmovescmd())
					{
						qgobrd->set_sentmovescmd(true);
						client_window->sendcommand ("moves " + gi->nr, false);
						client_window->sendcommand ("all " + gi->nr, false);
					}
					else
					{
						// do normal move
  						qgobrd->set_move(sc, pt, mv_nr);
					}
				}
			}

			break;

		// adjourn/end/resume of a game (parser)
		case 1:
#ifdef SHOW_MOVES_DLG
			dlg->write("Spiel " + g->nr + (g->running ? " running\n" : " STOPPED\n"));
			dlg->write(g->Sz + "\n");
#endif
			if (!qgobrd->get_havegd())
			{
				stone_color own_color = none;
				GameMode mode = modeObserve;
				// check if it's my own game
				if (g->bname == myName || g->wname == myName)
				{
					own_color = g->bname == myName ? black : white;
					// case: trying to observe own game: this is the way to get back to match if
					// board has been closed by mistake
					if (g->bname == g->wname)
						mode = modeTeach;
					else
						mode = modeMatch;
				}
				else if (g->bname == g->wname && !g->bname.contains('*'))
				{
					// case: observing a teaching game
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = false;
					qgobrd->IamPupil = false;
					qgobrd->havePupil = false;
				}
				qgobrd->set_game (g, mode, own_color);
			}
			break;

#ifdef SHOW_MOVES_DLG
		// game to observe (mouse click)
		case 2:
			dlg->write("Observe: " + txt + "\n");
			break;

		// game to play (mouse click and match prompt)
		case 3:
			dlg->write("Play(MATCH): " + txt + "\n");
			break;

		// game to play (mouse click and match prompt)
		case 4:
			dlg->write("Teach: " + txt + "\n");
			break;

#endif
		default:
			//qDebug("qGoInterface::parse_move() - illegal parameter");
			break;
	}

	// assume no new window
	return false;
}


// regular move info (parser)
void qGoIF::slot_move(GameInfo *gi)
{
	parse_move(0, gi);
}

// start/adjourn/end/resume of a game (parser)
void qGoIF::slot_gamemove(Game *g)
{
	parse_move(1, 0, g);
}

// game to observe (mouse click)
void qGoIF::set_observe(const QString& gameno)
{
	parse_move(2, 0, 0, gameno);
}

// remove all boards
void qGoIF::set_initIF()
{
	for (auto b: boardlist) {
		b->disconnected (false);
	}
	boardlist.clear ();

	n_observed = 0;
	client_window->update_observed_games (n_observed);
}

// a match is created
void qGoIF::slot_matchcreate(const QString &gameno, const QString &opponent)
{
	if (opponent == myName)
		// teaching game
		parse_move(4, 0, 0, gameno);
	else
		parse_move(3, 0, 0, gameno);
}

void qGoIF::slot_matchsettings(const QString &id, const QString &handicap, const QString &komi, assessType kt)
{
	// seek board
	for (auto b: boardlist)
		if (b->get_id() == id.toInt()) {
			b->set_requests(handicap, komi, kt);
			qDebug() << QString("qGoIF::slot_matchsettings: h=%1, k=%2, kt=%3").arg(handicap).arg(komi).arg(kt);
			return;
		}

	qWarning("BOARD CORRESPONDING TO GAME SETTINGS NOT FOUND !!!");
}

void qGoIF::slot_title(const GameInfo *gi, const QString &title)
{
	qGoBoard *qb = find_game_id (gi->nr.toInt ());
	if (qb)
		qb->set_title (title);
}

// komi set or requested
void qGoIF::slot_komi(const QString &nr, const QString &komi, bool isrequest)
{
	qGoBoard *qb;
	static int move_number_memo = -1;
	static QString komi_memo = QString::null;

	// correctness:
	if (komi.isEmpty())
		return;

	if (isrequest)
	{
		// check if opponent is me
		if (nr.isEmpty() || nr == myName)
			return;

		// 'nr' is opponent (IGS/NNGS)
		qb = find_game_players (nr, QString ());

		if (qb)
		{
			// check if same opponent twice (IGS problem...)
			if (move_number_memo == qb->get_mv_counter() && !komi_memo.isNull() && komi_memo == komi)
			{
				qDebug() << QString("...request skipped: opponent %1 wants komi %2").arg(nr).arg(komi);
				return;
			}

			// remember of actual values
			move_number_memo = qb->get_mv_counter();
			komi_memo = komi;

			if (qb->get_reqKomi() == komi  && setting->readBoolEntry("DEFAULT_AUTONEGO"))
			{
				if (qb->get_currentKomi() != komi)
				{
					qDebug("GoIF::slot_komi() : emit komi");
					client_window->sendcommand ("komi " + komi, true);
				}
			}
			else
			{
				slot_requestDialog(tr("komi ") + komi, tr("decline"), QString::number(qb->get_id()), nr);
			}
		}

		return;
	}
	// own game if nr == NULL
	else if (nr.isEmpty())
	{
		if (myName.isEmpty())
		{
			// own name not set -> should never happen!
			qWarning("*** Wrong komi because don't know which online name I have ***");
			return;
		}

		for (auto b: boardlist)
			if (b->get_havegd () && (b->get_wplayer () == myName || b->get_bplayer () == myName)) {
				b->set_komi (komi);
				break;
			}
	}
	else
	{
		// title message follows to move message
		for (auto b: boardlist)
			if (b->get_id () == nr.toInt ()) {
				b->set_komi (komi);
				break;
			}
	}

}

// game free/unfree message received
void qGoIF::slot_freegame(bool freegame)
{
	// what if 2 games (IGS) and mv_counter < 3 in each game? -> problem
	for (auto qb: boardlist)
		if (qb->get_havegd ()
		    && (qb->get_wplayer() == myName || qb->get_bplayer() == myName)
		    && qb->get_mvcount() < 3)
		{
			qb->set_freegame(freegame);
			return;
		}
}

void qGoIF::remove_board (qGoBoard *qb)
{
	boardlist.removeOne (qb);
}

// board window closed...
void qGoIF::window_closing (qGoBoard *qb)
{
	int id = qb->get_id();

	// erase actual pointer to prevent error
	qgobrd = 0;
	qDebug() << "qGoIF::slot_closeevent() -> game " << id << "\n";

	// destroy timers
	qb->set_stopTimer();

	if (id < 0 && id > -10000)
	{
		switch (qb->get_Mode())
		{
			case modeObserve:
				// unobserve
				if (gsName == IGS)
					client_window->sendcommand ("unobserve " + QString::number(-id), false);
				else
					client_window->sendcommand ("observe " + QString::number(-id), false);

				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			case modeMatch:
				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			case modeTeach:
				client_window->sendcommand ("adjourn", false);

				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			default:
				break;
		}
	}

	delete qb;
//	delete win;
}

// kibitz strings
void qGoIF::slot_kibitz(int num, const QString& who, const QString& msg)
{
	qGoBoard *qb = nullptr;
	QString name;

	// own game if num == NULL
	if (num) {
		qb = find_game_id (num);
		name = who;
	} else {
		if (myName.isEmpty())
		{
			// own name not set -> should never happen!
			qWarning("*** qGoIF::slot_kibitz(): Don't know my online name ***");
			return;
		}
		qb = find_game_players (myName, QString ());
		if (qb != nullptr) {
			if (qb->get_wplayer() == myName)
				name = qb->get_bplayer ();
			else if (qb->get_bplayer() == myName)
				name = qb->get_wplayer ();

			// sound for "say" command
			qgo->playSaySound();
		}
	}

	if (!qb)
		qDebug("Board to send kibitz string not in list...");
	else if (!num && !who.isEmpty())
	{
		// special case: opponent has resigned - interesting in quiet mode
		qb->send_kibitz(msg);
		qDebug () << "strange opponent resignation\n";
	}
	else
		qb->send_kibitz(name + ": " + msg + "\n");
}

void qGoIF::slot_requestDialog(const QString &yes, const QString &no, const QString & /*id*/, const QString &opponent)
{
	QString opp = opponent;
	if (opp.isEmpty())
		opp = tr("Opponent");

	if (no.isEmpty())
	{
		QMessageBox mb(tr("Request of Opponent"),
			tr("%1 wants to %2\nYES = %3\nCANCEL = %4").arg(opp).arg(yes).arg(yes).arg(tr("ignore request")),
			QMessageBox::NoIcon,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::Cancel | QMessageBox::Escape,
			Qt::NoButton);
		mb.activateWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			client_window->sendcommand (yes, false);
		}
	}
	else
	{
		QMessageBox mb(tr("Request of Opponent"),
			//tr("%1 wants to %2\nYES = %3\nCANCEL = %4").arg(opp).arg(yes).arg(yes).arg(no),
			tr("%1 wants to %2\n\nDo you accept ? \n").arg(opp).arg(yes),
			QMessageBox::Question,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No | QMessageBox::Escape,
			Qt::NoButton);
		mb.activateWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			client_window->sendcommand (yes, false);
		}
		else
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(no);
			client_window->sendcommand (no, false);
		}
	}
}

void qGoIF::slot_removestones(const QString &pt, const QString &game_id)
{
	qGoBoard *qb = nullptr;

	if (pt.isEmpty() && game_id.isEmpty())
	{
		qDebug("slot_removestones(): !pt !game_id");
		// search game
		for (auto b: boardlist)
			if (b->get_Mode() == modeMatch) {
				qb = b;
				break;
			}

		if (!qb)
		{
			qWarning("*** No Match found !!! ***");
			return;
		}

		switch (gsName)
		{
			case IGS:
				/* Can be a re-enter if already scoring.  */
				qb->enter_scoring_mode (true);
				break;

			default:
				if (qb->get_Mode() == modeMatch)
					qb->enter_scoring_mode (false);
				else
					qb->leave_scoring_mode ();
				break;
		}

		return;
	}

	if (!pt.isEmpty() && game_id.isEmpty())
	{
		qDebug("slot_removestones(): pt !game_id");
		// stone coords but no game number:
		// single match mode, e.g. NNGS
		for (auto b: boardlist)
			if (b->get_Mode () == modeMatch || b->get_Mode () == modeTeach) {
				qb = b;
				break;
			}
	}
	else
	{
qDebug("slot_removestones(): game_id");
		// multi match mode, e.g. IGS
		for (auto b: boardlist)
			if (b->get_id () == game_id.toInt ()) {
				qb = b;
				qb->enter_scoring_mode (false);
				break;
			}
	}

	if (!qb)
	{
		qWarning("*** No Match found !!! ***");
		return;
	}

	int i = pt[0].toLatin1() - 'A' + 1;
	// skip j
	if (i > 8)
		i--;

	int j;
	if (pt.length () > 2 && pt[2].toLatin1() >= '0' && pt[2].toLatin1() <= '9')
		j = qb->get_boardsize() + 1 - pt.mid(1,2).toInt();
	else
		j = qb->get_boardsize() + 1 - pt[1].digitValue();

	// mark dead stone
	qb->mark_dead_stone (i - 1, j - 1);
	qb->send_kibitz("removing @ " + pt);
}

// game status received
void qGoIF::slot_result(const QString &txt, const QString &line, bool isplayer, const QString &komi)
{
	static qGoBoard *qb;
	static int column;

	if (isplayer)
	{
		column = 0;
		qb = find_game_players (txt, QString ());
		if (qb)
			qb->receive_score_begin ();
	}
	else if (qb)
	{
		qb->receive_score_line (column, line);
		column++;
		if (txt.toInt() == (int) line.length() - 1)
			qb->receive_score_end ();
	}
}

// undo a move
void qGoIF::slot_undo(const QString &player, const QString &move)
{
	qDebug() << "qGoIF::slot_undo()" << player << move;

	// check if game number given
	bool ok;
	int nr = player.toInt(&ok);
	qGoBoard *qb = ok ? find_game_id (nr) : find_game_players (player, QString ());
	if (qb != nullptr) {
		qb->remote_undo (move);
		return;
	}
	qWarning("*** board for undo not found!");
}

void qGoBoard::remote_undo (const QString &)
{
	if (!m_scoring)
	{
		dec_mv_counter();
		win->delete_last_move ();
		return;
	}

	// back to matchMode
	leave_scoring_mode ();
	win->setGameMode (modeMatch);
	win->getBoard()->previous_move();
	dec_mv_counter();
	send_kibitz(tr("GAME MODE: place stones..."));
}

void qGoBoard::enter_scoring_mode (bool may_reenter)
{
	if (m_scoring) {
		if (may_reenter) {
			leave_scoring_mode ();
		}
	}
	if (m_scoring)
		return;
	send_kibitz (tr ("SCORE MODE: click on a stone to mark as dead..."));
	m_scoring = true;
	win->setGameMode (modeScoreRemote);
}

void qGoBoard::leave_scoring_mode ()
{
	win->setGameMode (gameMode);
	m_scoring = false;
	send_kibitz (tr ("GAME MODE: click to play stones..."));
}

void qGoBoard::mark_dead_stone (int x, int y)
{
	win->mark_dead_external (x, y);
}

// time has been added
void qGoIF::slot_timeAdded(int time, bool toMe)
{
	for (auto qb: boardlist) {
		/* @@@ Not a great test.  */
		if (qb->get_Mode () == modeMatch) {

			// use time == -1 to pause the game
			if (time == -1)
				qb->set_gamePaused(toMe);
			else if ((toMe && qb->get_myColorIsBlack()) || (!toMe && !qb->get_myColorIsBlack()))
				qb->addtime_b(time);
			else
				qb->addtime_w(time);
			return;
		}
	}

	qWarning("*** board for undo not found!");
}

void qGoIF::observer_list_start (int id)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_start ();
}

void qGoIF::observer_list_entry (int id, const QString &n, const QString &r)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_entry (n, r);
}

void qGoIF::observer_list_end (int id)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_end ();
}

/*
 *	qGo Board
 */



// add new board window
qGoBoard::qGoBoard(qGoIF *qif, int gameid) : m_qgoif (qif), id (gameid)
{
	qDebug("::qGoBoard()");
	sent_movescmd = false;
	adjourned = false;
	mv_counter = -1;
	requests_set = false;
	m_game = nullptr;
	win = nullptr;

	ExtendedTeachingGame = false;
	IamTeacher = false;
	IamPupil = false;
	havePupil = false;
	haveControls = false;
	mark_set = false;

	// set timer to 1 second
	timer_id = startTimer(1000);
	game_paused = false;
	req_handicap = QString::null;
	req_komi = -1;
	bt_i = -1;
	wt_i = -1;
	stated_mv_count =0 ;
  BY_timer = setting->readIntEntry("BY_TIMER");

#ifdef SHOW_INTERNAL_TIME
	chk_b = -2;
	chk_w = -2;
#endif

	m_observers.setColumnCount (2);
	m_observers.setHorizontalHeaderItem (0, new QStandardItem ("Name"));
	m_observers.setHorizontalHeaderItem (1, new QStandardItem ("Rk"));
	m_observers.setSortRole (Qt::UserRole + 1);
}

qGoBoard::~qGoBoard()
{
	qDebug("~qGoBoard()");
	if (m_connected)
		m_qgoif->remove_board (this);
	delete win;
	delete m_title;
	delete m_scoring_board;
}

void qGoBoard::observer_list_start ()
{
	if (win) {
		win->cb_opponent->clear();
		win->TextLabel_opponent->setText(tr("opponent:"));
		win->cb_opponent->addItem(tr("-- none --"));
		if (havePupil && ttOpponent != tr("-- none --"))
		{
			win->cb_opponent->addItem(ttOpponent, 1);
			win->cb_opponent->setCurrentIndex(1);
		}
	}

	m_observers.setRowCount (0);
}

void qGoBoard::observer_list_entry (const QString &n, const QString &r)
{
	QList<QStandardItem *> list;
	QStandardItem *nm_item = new QStandardItem (n);
	nm_item->setData (n, Qt::UserRole + 1);
	list.append (nm_item);
	QStandardItem *rk_item = new QStandardItem (r);
	rk_item->setData (rkToKey (r, false), Qt::UserRole + 1);
	list.append (rk_item);
	m_observers.appendRow (list);
	if (win) {
		win->cb_opponent->addItem (n);
		int cnt = win->cb_opponent->count();
		win->TextLabel_opponent->setText(tr("opponent:") + " (" + QString::number(cnt-1) + ")");
	}
}

void qGoBoard::observer_list_end ()
{
}

void qGoBoard::receive_score_begin ()
{
	m_scoring_board = new go_board (m_state->get_board ());
}

void qGoBoard::receive_score_end ()
{
	game_state *st = m_state;
	m_scoring_board->territory_from_markers ();
	game_state *st_new = st->add_child_edit (*m_scoring_board, m_state->to_move (), true);
	st->transfer_observers (st_new);
	delete m_scoring_board;
	m_scoring_board = nullptr;
}

void qGoBoard::receive_score_line (int n, const QString &line)
{
	// ok, now mark territory points only
	// 0 .. black
	// 1 .. white
	// 2 .. free
	// 3 .. neutral
	// 4 .. white territory
	// 5 .. black territory

	for (int y = 0; y < line.length (); y++)
		switch (line[y].digitValue ()) {
		case 4:
			m_scoring_board->set_mark (n, y, mark::terr, 0);
			break;
		case 5:
			m_scoring_board->set_mark (n, y, mark::terr, 1);
			break;
		default:
			break;
		}
}

/* Called when we have game info and all the moves up to this point.  */
void qGoBoard::game_startup ()
{
	bool am_black = m_own_color == black;
	bool am_white = m_own_color == white;
	win = new MainWindow_IGS (0, m_game, screen_key (client_window), this, am_white, am_black, gameMode);
	win->show ();
	win->set_observer_model (&m_observers);

	game_state *root = m_game->get_root ();
	if (root != m_state)
		root->transfer_observers (m_state);

	// disable some Menu items
//	win->setOnlineMenu(true);

	// connect with board (MainWindow::CloseEvent())
	connect(win, SIGNAL(signal_closeevent()), this, SLOT(slot_closeevent()));
	// connect with board (qGoBoard)
	//  board -> to kibitz or say
	connect(win, SIGNAL(signal_sendcomment(const QString&)), this, SLOT(slot_sendcomment(const QString&)));

	// board -> commands
	connect(win, SIGNAL(signal_adjourn()), this, SLOT(slot_doAdjourn()));
	connect(win, SIGNAL(signal_undo()), this, SLOT(slot_doUndo()));
	connect(win, SIGNAL(signal_done()), this, SLOT(slot_doDone()));

	// teach tools
	connect(win->cb_opponent, SIGNAL(activated(const QString&)), this, SLOT(slot_ttOpponentSelected(const QString&)));
	connect(win->pb_controls, SIGNAL(toggled(bool)), this, SLOT(slot_ttControls(bool)));
	connect(win->pb_mark, SIGNAL(toggled(bool)), this, SLOT(slot_ttMark(bool)));

	if (ExtendedTeachingGame) {
		// make teaching features visible while observing
		win->cb_opponent->setDisabled(IamTeacher);
		win->pb_controls->setDisabled(IamTeacher);
		win->pb_mark->setDisabled(IamTeacher);
	}

	if (gameMode == modeMatch || gameMode == modeTeach) {
		connect (win->normalTools->btimeView,
			 &ClockView::clicked, this, &qGoBoard::slot_addtimePauseB);
		connect (win->normalTools->wtimeView,
			 &ClockView::clicked, this, &qGoBoard::slot_addtimePauseW);
	}

	if (m_comments.length () > 0)
		win->append_comment (m_comments);

}

// set game info to one board
void qGoBoard::set_game(Game *g, GameMode mode, stone_color own_color)
{
	go_board startpos (g->Sz.toInt ());
	int handi = g->H.toInt ();
	if (handi >= 2)
		startpos = new_handicap_board (g->Sz.toInt(), handi);
	enum ranked rt = ranked::ranked;
	if (g->FR.contains("F"))
		rt = ranked::free;
	else if (g->FR.contains("T"))
		rt = ranked::teaching;

	int timelimit = 0;
	std::string overtime = "";
	if (rt != ranked::teaching)
	{
		int byoTime = g->By.toInt() * 60;
		if (wt_i > 0)
			timelimit = (((wt_i > bt_i ? wt_i : bt_i) / 60) + 1) * 60;
		else
			timelimit = 60;

		if (byoTime != 0)
			overtime = "25/" + std::to_string (byoTime) + " Canadian";
	}

	std::string place = "";
	switch (gsName)
	{
	case IGS:
		place = "IGS";
		break;
	case LGS:
		place = "LGS";
		break;
	case WING:
		place = "WING";
		break;
	case CWS:
		place = "CWS";
		break;
	default:
		break;
	}

	game_info info (m_title ? m_title->toStdString () : "",
			g->wname.toStdString (), g->bname.toStdString (),
			g->wrank.toStdString (), g->brank.toStdString (),
			"", g->K.toFloat(), handi, rt, "",
			QDate::currentDate().toString("yyyy-MM-dd").toStdString (),
			place, "", "", "",
			std::to_string (timelimit), overtime, -1);

	m_game = std::make_shared<game_record> (startpos, handi >= 2 ? white : black, info);
	start_observing (m_game->get_root ());
	if (g->FR.contains("F"))
		m_freegame = FREE;
	else if (g->FR.contains("T"))
		m_freegame = TEACHING;
	else
		m_freegame = RATED;

	// needed for correct sound
	stated_mv_count = g->mv.toInt();
	set_Mode_real (mode);
	m_own_color = own_color;

	if (stated_mv_count == 0)
		game_startup ();
}

void qGoBoard::set_title(const QString &t)
{
	if (m_title)
		delete m_title;
	m_title = new QString (t);

	if (m_game == nullptr)
		return;

	m_game->set_title (t.toStdString ());
	if (win)
		win->update_game_record ();
}

void qGoBoard::set_komi(const QString &k)
{
	QString k_;
	// check whether the string contains two commas
	if (k.count('.') > 1)
		k_ = k.left(k.length()-1);
	else
		k_ = k;

	qDebug() << "set komi to " << k_;

#if 0
	gd.komi = k_.toFloat();
	win->getBoard()->setGameData(&gd);
	win->normalTools->komi->setText(k_);
#endif
}

void qGoBoard::set_freegame(bool f)
{
	m_game->set_ranked_type (f ? ranked::free : ranked::ranked);
	win->update_game_record ();
}

// convert seconds to time string
QString qGoBoard::secToTime(int seconds)
{
	bool neg = seconds < 0;
	if (neg)
		seconds = -seconds;

	int h = seconds / 3600;
	seconds -= h*3600;
	int m = seconds / 60;
	int s = seconds - m*60;

	QString sec;

	// prevailling 0 for seconds
	if ((h || m) && s < 10)
		sec = "0" + QString::number(s);
	else
		sec = QString::number(s);

	if (h)
	{
		QString min;

		// prevailling 0 for minutes
		if (h && m < 10)
			min = "0" + QString::number(m);
		else
			min = QString::number(m);

		return (neg ? "-" : "") + QString::number(h) + ":" + min + ":" + sec;
	}
	else
		return (neg ? "-" : "") + QString::number(m) + ":" + sec;
}

// send regular time Info
void qGoBoard::timerEvent(QTimerEvent*)
{
	// wait until first move
	if (win == nullptr || mv_counter < 0 || id < 0 || game_paused)
		return;

	if (mv_counter % 2)
	{
		// B's turn
		bt_i--;

		bool warn = bt_i > - 1 && bt_i <= BY_timer;

#ifdef SHOW_INTERNAL_TIME
		chk_b--;
		if (chk_b < bt_i + 20)
			chk_b = bt_i;
		win->setTimes("(" + secToTime(chk_b) + ") " + secToTime(bt_i), b_stones,
						"(" + secToTime(chk_w) + ") " + wt, w_stones,
						warn, false, bt_i);
#else
		win->setTimes(secToTime(bt_i), b_stones, wt, w_stones, warn, false, bt_i);
#endif
	}
	else
	{
		// W's turn
		wt_i--;

		bool warn = wt_i > - 1 && wt_i <= BY_timer;

#ifdef SHOW_INTERNAL_TIME
		chk_w--;
		if (chk_w < bt_i + 20)
			chk_w = wt_i;
		win->setTimes("(" + secToTime(chk_b) + ") " + bt, b_stones,
					  "(" + secToTime(chk_w) + ") " + secToTime(wt_i), w_stones,
						false, warn, wt_i);
#else
		win->setTimes(bt, b_stones, secToTime(wt_i), w_stones, false, warn, wt_i);
#endif
	}
}

// stop timer
void qGoBoard::set_stopTimer()
{
	if (timer_id)
		killTimer(timer_id);

	timer_id = 0;
}

// stop timer
void qGoBoard::set_runTimer()
{
	if (!timer_id)
		timer_id = startTimer(1000);
}

// send regular time info
void qGoBoard::setTimerInfo(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	bt_i = btime.toInt();
	wt_i = wtime.toInt();
	b_stones = bstones;
	w_stones = wstones;

#ifdef SHOW_INTERNAL_TIME
	if (chk_b < 0)
	{
		chk_b = bt_i;
		chk_w = wt_i;
	}
#endif

	// set string in any case
	bt = secToTime(bt_i);
	wt = secToTime(wt_i);

	if (m_game != nullptr)
		update_time_info (m_state);
#if 0
	// set initial timer until game is initialized
	if (!have_gameData)
		win->setTimes(bt, bstones, wt, wstones,
					  bt_i <= BY_timer && bt_i > -1 && mv_counter % 2 && bt_i %2,
					  wt_i <= BY_timer && wt_i > -1 && (mv_counter % 2 && bt_i %2) == 0);
#endif
}

// addtime function
void qGoBoard::addtime_b(int m)
{
	bt_i += m*60;
	bt = secToTime(bt_i);
	win->setTimes(secToTime(bt_i), b_stones, wt, w_stones, false, false, 0);
}

void qGoBoard::addtime_w(int m)
{
	wt_i += m*60;
	wt = secToTime(wt_i);
	win->setTimes(bt, b_stones, secToTime(wt_i), w_stones, false, false, 0);
}

void qGoBoard::set_Mode_real(GameMode mode)
{
	gameMode = mode;
}

void qGoBoard::update_time_info (game_state *st)
{
	if (!st->was_move_p ())
		return;
	stone_color sc = st->get_move_color ();
	qDebug () << "updating time for " << (sc == black ? "black" : "white");
	st->set_time_left (sc, std::to_string (sc == white ? wt_i : bt_i));
	QString &whichstones = sc == white ? w_stones : b_stones;
	if (whichstones != "-1")
		st->set_stones_left (sc, whichstones.toStdString ());
}

void qGoBoard::set_move(stone_color sc, QString pt, QString mv_nr)
{
	int mv_nr_int;

	// IGS: case undo with 'mark': no following command
	// -> from qgoIF::slot_undo(): set_move(stoneNone, 0, 0)
	if (mv_nr.isEmpty())
		// undo one move
		mv_nr_int = mv_counter - 1;
	else
		mv_nr_int = mv_nr.toInt();

	if (mv_nr_int > mv_counter)
	{
		if (mv_nr_int != mv_counter + 1 && mv_counter != -1)
			// case: move has done but "moves" cmd not executed yet
			qWarning("**** LOST MOVE !!!! ****");
		else if (mv_counter == -1 && mv_nr_int != 0)
		{
			qDebug("move skipped");
			// skip moves until "moves" cmd executed
			return;
		}
		else
			mv_counter++;
	}
	else if (mv_nr_int + 1 == mv_counter)
	{
		// scoring mode? (NNGS)
		if (gameMode == modeScore)
		{
			leave_scoring_mode ();
		}

		// special case: undo handicap
		if (mv_counter <= 0 && m_game->handicap () > 0)
		{
			go_board new_root = new_handicap_board (m_game->boardsize (), 0);
			m_game->replace_root (new_root, black);
			m_game->set_handicap (0);
			qDebug("set Handicap to 0");
		}

		if (pt.isEmpty())
		{
			// case: undo
			qDebug() << "set_move(): UNDO in game " << QString::number(id);
			qDebug() << "...mv_nr = " << mv_nr;
			remote_undo ("");

			mv_counter--;
		}

		return;
	}
	else
		// move already done...
		return;

	if (pt.contains("Handicap"))
	{
		QString handi = pt.simplified();
		int h = handi.section(' ', 1, 1).toInt();

		// check if handicap is set with initGame() - game data from server do not
		// contain correct handicap in early stage, because handicap is first move!
		if (m_game->handicap () != h)
		{
			m_game->set_handicap (h);
			go_board new_root = new_handicap_board (m_game->boardsize (), h);
			m_game->replace_root (new_root, h > 1 ? white : black);
			qDebug("corrected Handicap");
		}
	}
	else if (pt.contains("Pass", Qt::CaseInsensitive))
	{
		qDebug () << "pass found\n";
		game_state *st = m_state;
		game_state *new_st = m_state->add_child_pass ();
		st->transfer_observers (new_st);
		if (win != nullptr)
			win->playPassSound();
	}
	else
	{
		if (gameMode == modeMatch && mv_counter < 2 && m_own_color == white)
		{
			// if black has not already done - maybe too late here???
			if (requests_set)
			{
				qDebug() << "qGoBoard::set_move() : check_requests at move " << mv_counter;
				check_requests();
			}
		}

		int i = pt[0].toLatin1() - 'A' + 1;
		// skip j
		if (i > 8)
			i--;

		int j;

		if (pt.length () > 2 && pt[2].toLatin1() >= '0' && pt[2].toLatin1() <= '9')
			j = m_game->boardsize () + 1 - pt.mid(1,2).toInt();
		else
			j = m_game->boardsize () + 1 - pt[1].digitValue();

		game_state *st = m_state;
		game_state *st_new = st->add_child_move (i - 1, j - 1, sc, game_state::add_mode::set_main);
		if (st_new != nullptr) {
			st->transfer_observers (st_new);
			if (win != nullptr)
				win->playClick ();
		} else {
			/* @@@ do something sensible.  */
		}

		if (st_new != nullptr && stated_mv_count <= mv_counter && wt_i >= 0 && bt_i >= 0)
			update_time_info (st_new);
	}
	qDebug () << "found move " << mv_counter << " of " << stated_mv_count << "\n";
	if (mv_counter + 1 == stated_mv_count && win == nullptr)
		game_startup ();
}

// board window closed
void qGoBoard::slot_closeevent()
{
	/* Don't delete it later.  */
	win = 0;
	if (id > 0)
	{
		id = -id;
		m_qgoif->window_closing (this);
//		emit signal_closeevent(id);
	}
	else
		qWarning("id < 0 ******************");
}

void qGoBoard::disconnected (bool remove_from_list)
{
	if (remove_from_list && m_connected)
		m_qgoif->remove_board (this);
	m_connected = false;
	set_stopTimer();

	qgo->playGameEndSound ();
	m_observers.clear ();

	// set board editable...
	set_Mode_real (modeNormal);
	if (win) {
		win->setGameMode (modeNormal);
		win->getBoard()->set_player_colors (true, true);
	}
	/* @@@ Sometimes we get a game result without moves, if we started observing just
	   as the game ended.  We should arrange for some way to delete this qGoBoard.  */
}

// write kibitz strings to comment window
void qGoBoard::send_kibitz(const QString &msg)
{
	// if observing a teaching game
	if (ExtendedTeachingGame && !IamTeacher)
	{
		// get msgs from teacher
		if (msg.indexOf("#OP:") != -1)
		{
			// opponent has been selected
			QString opp;
			opp = msg.section(':', 2, -1).remove(QRegExp("\\(.*$")).trimmed();

			if (opp == "0")
				slot_ttOpponentSelected(tr("-- none --"));
			else
				slot_ttOpponentSelected(opp);
			return;
		}
		else if (IamPupil)
		{
			if (msg.indexOf("#TC:ON") != -1)
			{
				// teacher gives controls to pupil
				haveControls = true;
				win->pb_controls->setEnabled(true);
				win->pb_controls->setChecked(true);
				return;
			}
			else if (msg.indexOf("#TC:OFF") != -1)
			{
				// teacher takes controls back
				haveControls = false;
				win->pb_controls->setDisabled(true);
				win->pb_controls->setChecked(false);
				return;
			}
		}
	}
	else if (ExtendedTeachingGame && IamTeacher)
	{
		// get msgs from pupil
		if (msg.indexOf("#OC:OFF") != -1)
		{
			// pupil gives controls back
			haveControls = true;
			win->pb_controls->setChecked(false);
			return;
		}
		else if (havePupil
			 && msg.section(' ', 0, 0) == ttOpponent.section(' ', 0, 0)
			 && ((myColorIsBlack && (mv_counter % 2))
			     || (!myColorIsBlack && ((mv_counter % 2 + 1)
						     || mv_counter == -1))
			     || !haveControls))
		{
			// move from opponent - it's his turn (his color or he has controls)!
			// xxx [rk]: B1 (7)
			//   it's ensured that yyy [rk] didn't send forged message
			//   e.g.: yyy [rk]: xxx[rk]: S1 (3)
			QString s;
			s = msg.section(':', 1, -1).remove(QRegExp("\\(.*$")).trimmed().toUpper();

			// check whether it's a position
			// e.g. B1, A17, NOT: ok, yes
			if (s.length() < 4 && s[0] >= 'A' && s[0] < 'U' && s[1] > '0' && s[1] <= '9')
			{
				// ok, could be a real pt
				// now teacher has to send it: pupil -> teacher -> server
				if (gsName == IGS)
					client_window->sendcommand (s + " " + QString::number(id), false);
				else
					client_window->sendcommand (s, false);
				return;
			}
		}
	}

	// skip my own messages
//	qDebug() << "msg.indexOf(myName) = " << QString::number(msg.indexOf(myName));
//	if (msg.indexOf(myName) == 0)
//		return;

	// normal stuff...
	QString to_add = msg;
	if (!to_add.contains(QString::number(mv_counter + 1)))
		to_add = "(" + QString::number(mv_counter + 1) + ") " + to_add;
	if (m_state != nullptr) {
		const std::string old_comment = m_state->comment ();
		m_state->set_comment (old_comment + to_add.toStdString ());
	}
	if (win)
		win->append_comment (to_add);
	else
		m_comments += to_add;
}

void qGoBoard::slot_sendcomment(const QString &comment)
{
	// # has to be at the first position
	if (comment.indexOf("#") == 0)
	{
		qDebug("detected (#) -> send command");
		QString testcmd = comment;
		testcmd = testcmd.remove(0, 1).trimmed();
		client_window->sendcommand (testcmd, true);
		return;
	}

	if (gameMode == modeObserve || gameMode == modeTeach || gameMode == modeMatch && ExtendedTeachingGame)
	{
		// kibitz
		client_window->sendcommand ("kibitz " + QString::number(id) + " " + comment, false);
		send_kibitz("-> " + comment + "\n");
	}
	else
	{
		// say
		client_window->sendcommand ("say " + comment, false);
		send_kibitz("-> " + comment + "\n");
	}
}

void qGoBoard::send_coords (int x, int y)
{
	if (id < 0)
		return;

	if (x > 7)
		x++;
	QChar c1 = x + 'A';
	int c2 = m_game->boardsize () - y;

	if (ExtendedTeachingGame && IamPupil)
		client_window->sendcommand ("kibitz " + QString::number(id) + " " + QString(c1) + QString::number(c2), false);
	else
		client_window->sendcommand (QString(c1) + QString::number(c2) + " " + QString::number(id), false);
}

void qGoBoard::player_toggle_dead (int x, int y)
{
	send_coords (x, y);
}

void qGoBoard::move_played (int x, int y)
{
	update_time_info (m_state);
	send_coords (x, y);
}

void qGoBoard::game_result (const QString &rs, const QString &extended_rs)
{
	m_game->set_result (rs.toStdString ());
	send_kibitz(rs);
	bool autosave = setting->readBoolEntry (gameMode == modeObserve ? "AUTOSAVE" : "AUTOSAVE_PLAYED");

	/* Note that win can be null - observing a game just as it ends may cause the
	   server to send a result without moves (or maybe before, but that is still unclear).  */
	if (autosave && win)
	{
		win->doSave("", true);
		qDebug("Game saved");
	}

	GameMode prev_mode = gameMode;
	id = -id;

	disconnected (true);

	if (prev_mode != modeObserve)
		QMessageBox::information(win, tr("Game #") + QString::number(id), extended_rs);

	qDebug() << "Result: " << rs;
}

void qGoBoard::slot_doPass()
{
	if (id > 0)
		client_window->sendcommand ("pass", false);
}

void qGoBoard::slot_doResign()
{
	if (id > 0)
		client_window->sendcommand ("resign", false);
}

void qGoBoard::slot_doUndo()
{
	if (id > 0) {
		if (gsName ==IGS)
			client_window->sendcommand ("undoplease", false);
		else
			client_window->sendcommand ("undo", false);
	}
}

void qGoBoard::slot_doAdjourn()
{
	qDebug("button: adjourn");
	if (id > 0)
		client_window->sendcommand ("adjourn", false);
}

void qGoBoard::slot_doDone()
{
	if (id > 0)
		client_window->sendcommand ("done", false);
}

void qGoBoard::set_requests(const QString &handicap, const QString &komi, assessType kt)
{
qDebug() << "set req_handicap to " << handicap;
qDebug() << "set req_komi to " << komi;
	req_handicap = handicap;
	req_komi = komi;
	req_free = kt;
	requests_set = true;
}

// compare settings with requested values
void qGoBoard::check_requests()
{
	// check if handicap requested via negotiation (if activated!)
	// do not set any value while reloading a game
	if (req_handicap.isNull() || gameMode == modeTeach || setting->readBoolEntry("DEFAULT_AUTONEGO"))
		return;

	if (m_game->handicap () != req_handicap.toInt())
	{
		client_window->sendcommand ("handicap " + req_handicap, false);
		qDebug() << "Requested handicap: " << req_handicap;
	}
	else
		qDebug("Handicap settings ok...");

	if (m_game->komi () != req_komi.toFloat())
	{
		qDebug("qGoBoard::check_requests() : emit komi");
		client_window->sendcommand ("komi " + req_komi, false);
	}

	// if size != 19 don't care, it's a free game
	if (req_free != noREQ && m_game->boardsize () == 19 && mv_counter < 1)
	{
		// check if requests are equal to settings or if teaching game
		if ((m_freegame == FREE && req_free == FREE) ||
		    (m_freegame != FREE && req_free == RATED) ||
		    req_free == TEACHING)
		    return;

		switch (gsName)
		{
			case NNGS:
				if (m_freegame == FREE)
					client_window->sendcommand ("unfree", false);
				else
					client_window->sendcommand ("free", false);
				break;

			// default: toggle
			default:
				client_window->sendcommand ("free", false);
				break;
		}
	}
else
qDebug("Rated game settings ok...");
}

// click on time field
void qGoBoard::slot_addtimePauseB()
{
	if (m_own_color == black)
	{
		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					client_window->sendcommand ("unpause", false);
				else
					client_window->sendcommand ("pause", false);
				break;
		}
	}
	else
		client_window->sendcommand ("addtime 1", false);
}

// click on time field
void qGoBoard::slot_addtimePauseW()
{
	if (m_own_color == white)
	{
		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					client_window->sendcommand ("unpause", false);
				else
					client_window->sendcommand ("pause", false);
				break;
		}
	}
	else
		client_window->sendcommand ("addtime 1", false);
}

// for several reasons: let me know which is my color
void qGoBoard::set_myColorIsBlack(bool b)
{
	myColorIsBlack = b;
}

// teachtools - teacher has selected an opponent
void qGoBoard::slot_ttOpponentSelected(const QString &opponent)
{
	if (opponent == tr("-- none --"))
	{
		havePupil = false;
	}
	else
	{
		ttOpponent = opponent;
		havePupil = true;
		// teacher has controls when opponent is selected
		haveControls = IamTeacher;
	}
	win->pb_controls->setEnabled(IamTeacher && havePupil);

	if (IamTeacher)
	{
		if (opponent == tr("-- none --"))
			client_window->sendcommand ("kibitz " + QString::number(id) + " #OP:0", false);
		else
			client_window->sendcommand ("kibitz " + QString::number(id) + " #OP:" + opponent, false);
		// set teacher's color -> but: teacher can play both colors anyway
		set_myColorIsBlack(mv_counter % 2 + 1);
	}
	else
	{
		bool found = false;
		// look for correct item
		QComboBox *cb = win->cb_opponent;
		int cnt = cb->count();
		for (int i = 0; i < cnt && !found; i++)
		{
			cb->setCurrentIndex(i);
			if (cb->currentText() == opponent)
			{
				cb->setCurrentIndex(i);
				found = true;
			}
		}

		if (!found)
		{
			cb->insertItem(cnt, opponent);
			cb->setCurrentIndex(cnt);
		}

		// check if it's me
		QString opp = opponent.section(' ', 0, 0);
		if (opp == myName)
		{
			IamPupil = true;
			set_Mode_real(modeMatch);
			win->getBoard()->set_player_colors (mv_counter % 2, !(mv_counter % 2));
			set_myColorIsBlack(mv_counter % 2);
		}
		else
		{
			IamPupil = false;
			set_Mode_real(modeObserve);
		}
	}
}

// teachtools - controls button has toggled
void qGoBoard::slot_ttControls(bool on)
{
	// teacher: on -> give controls to pupil
	//          off -> take controls back
	//		teacher has controls if BUTTON RELEASED (!ON)
	// pupil: only off -> give controls back
	//		pupil has controls if BUTTON PRESSED (ON)
	if (IamTeacher)
	{
		// check whether controls key has been clicked or has been toggled by command
		if ((haveControls && !on) || (!haveControls && on))
		{
			// toggled by command
		}
		else
		{
			if (on)
				client_window->sendcommand ("kibitz " + QString::number(id) + " #TC:ON", false);
			else
				client_window->sendcommand ("kibitz " + QString::number(id) + " #TC:OFF", false);

			haveControls = !on;
		}
	}
	else if (IamPupil)
	{
		// set game mode
		if (on)
		{
			// pupil is able to play both colors
			set_Mode_real (modeTeach);
		}
		else
		{
			// pupil has only one color
			set_Mode_real (modeMatch);
		}

		// check whether controls key has been clicked or has been toggled by command
		if ((!haveControls && !on) || (haveControls && on))
			// toggled by command
			return;

		client_window->sendcommand ("kibitz " + QString::number(id) + " #OC:OFF", false);
		win->pb_controls->setDisabled(true);
		haveControls = false;
	}
}

// teachtools - mark button has toggled
void qGoBoard::slot_ttMark(bool on)
{
	switch (gsName)
	{
		case IGS:
			// simple 'mark' cmd
			if (on)
				client_window->sendcommand ("mark", false);
			else
				client_window->sendcommand ("undo", false);
			break;

		case LGS:
		case CTN:
			// use undo cmd
			if (on)
			{
				mark_set = true;
				mark_counter = mv_counter;
			}
			else
			{
				mark_set = false;
				for (; mark_counter <= mv_counter; mark_counter++);
					client_window->sendcommand ("undo", false);
				mark_counter = 0;
			}
			break;

		default:
			// use undo [n] cmd
			if (on)
			{
				mark_set = true;
				mark_counter = mv_counter;
			}
			else
			{
				mark_set = false;
				client_window->sendcommand ("undo " + QString::number(mv_counter - mark_counter + 1), false);
				mark_counter = 0;
			}
			break;
	}
}

