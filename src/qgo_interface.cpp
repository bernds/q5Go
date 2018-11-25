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
#include "globals.h"
#include "gs_globals.h"
#include "misc.h"
#include "config.h"
#include "interfacehandler.h"
#include "move.h"
#include "qnewgamedlg.h"
#include <qlabel.h>
//#include <qmultilineedit.h>
#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qtooltip.h>
#include <qcombobox.h>
#include <qslider.h>
#include <q3toolbar.h>
#include <q3ptrlist.h>
#include <qregexp.h>

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

/*
 *	Playing or Observing
 */

qGoIF::qGoIF(QWidget *p) : QObject()
{
	qgo = new qGo();

	ASSERT(qgo);

	setting->qgo = qgo;
	parent = p;
	qgobrd = 0;
	gsName = GS_UNKNOWN;
	boardlist = new Q3PtrList<qGoBoard>;
	boardlist->setAutoDelete(false);
	localBoardCounter = 10000;
}

qGoIF::~qGoIF()
{
	delete boardlist;
	delete qgo;
}


// handle move info and distribute to different boards
bool qGoIF::parse_move(int src, GameInfo* gi, Game* g, QString txt)
{
#ifdef SHOW_MOVES_DLG
	static Talk *dlg;
#endif

	int         game_id = 0;
 	StoneColor  sc = stoneNone;
	QString     pt;
	QString     mv_nr;

	switch (src)
	{
		// regular move info (parser)
		case 0:
 			game_id = gi->nr.toInt();
			if (gi->mv_col == QString("B"))
				sc = stoneBlack;
			else if (gi->mv_col == QString("W"))
				sc = stoneWhite;
			else
				sc = stoneNone;
			mv_nr = gi->mv_nr;
			pt = gi->mv_pt;
			break;

		// adjourn/end/resume of a game (parser)
		case 1:
			// check if g->nr is a number
			if (g->nr == QString("@"))
			{
				qGoBoard *qb = boardlist->first();
				while (qb != NULL && qb->get_bplayer() != myName && qb->get_wplayer() != myName)
				{
					qb = boardlist->next();
				}

				if (qb)
					game_id = qb->get_id();
				else
				{
					qWarning("*** You are not playing a game to be adjourned!");
					return false;
				}
			}
			else
				game_id = g->nr.toInt();

			if (g->Sz == QString("@") || g->Sz == QString("@@"))
			{
				// look for adjourned games
				qGoBoard *qb = boardlist->first();
				while (qb != NULL)
				{
					if (qb->get_adj() && qb->get_id() == 10000 &&
					    qb->get_bplayer() == g->bname &&
					    qb->get_wplayer() == g->wname)
					{
						qDebug("ok, adjourned game found ...");
						if (g->bname != myName && g->wname != myName)
							// observe if I'm not playing
							emit signal_sendcommand("observe " + g->nr, false);
						else
							// ensure that my game is correct stated
							emit signal_sendcommand("games " + g->nr, false);
						qb->set_sentmovescmd(true);
						qb->set_id(g->nr.toInt());
						qb->setGameData();
						qb->setMode();
						qb->set_adj(false);
//						qb->get_win()->toggleMode();
						qb->set_runTimer();
						qb->set_sentmovescmd(true);
						emit signal_sendcommand("moves " + g->nr, false);
						emit signal_sendcommand("all " + g->nr, false);

						qb->send_kibitz(tr("Game continued as Game number %1").arg(g->nr));
						// show new game number;
						qb->get_win()->getBoard()->updateCaption();

						// renew refresh button
						qb->get_win()->getInterfaceHandler()->refreshButton->setText(QObject::tr("Refresh", "button label"));

						// increase number of observed games
						emit signal_addToObservationList(-2);

						return true;
					}

					qb = boardlist->next();
				}
			}

			// special case: your own game
			if (g->Sz == QString("@@"))// || g->bname == myName || g->wname == myName)
			{
				// set up new game
				if (g->bname == g->wname)
					// teaching game
					src = 14;
				else
					// match
					src = 13;
			}
			break;

		// game to observe
		case 2:
			game_id = txt.toInt();
			break;

		// match
		case 3:
			game_id = txt.toInt();
			break;

		// teaching game
		case 4: game_id = txt.toInt();
			break;

		// computer game
		case 6: game_id = ++localBoardCounter; //txt.toInt();
			qDebug() << QString("computer game no. %1").arg(game_id);
			break;

		// remove all boards! -> if connection is closed
		// but: set options for local actions
		case -1:
			qgobrd = boardlist->first();
			while (qgobrd)
			{
				// modeNormal;
				qgobrd->set_stopTimer();
				// enable Menus
//				qgobrd->get_win()->setOnlineMenu(false);

				// set board editable...
				qgobrd->set_Mode_real (modeNormal);

				boardlist->remove();
				qgobrd = boardlist->current();
			}

			// set number of observed games to 0
			emit signal_addToObservationList(0);
			return false;
			break;

		default:
			qWarning("*** qGoIF::parse_move(): unknown case !!! ***");
			return false;
			break;
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
		qgobrd = boardlist->first();
		while (qgobrd != NULL && qgobrd->get_id() != game_id)
			qgobrd = boardlist->next();
		
		// not found -> create new dialog
		if (!qgobrd)
		{
			// setup only with mouseclick or automatic in case of matching !!!!
			// added case 0: 'observe' cmd
			if (src < 2) //(src == 1) //
			{
				return false;
			}

			qgobrd = new qGoBoard();
			boardlist->append(qgobrd);

//			qgobrd->get_win()->setOnlineMenu(true);

			CHECK_PTR(qgobrd);

			// connect with board (MainWindow::CloseEvent())
			connect(qgobrd->get_win(),
				SIGNAL(signal_closeevent()),
				qgobrd,
				SLOT(slot_closeevent()));
			// connect with board (qGoBoard)
			//  board -> to kibitz or say
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_sendcomment(const QString&)),
				qgobrd,
				SLOT(slot_sendcomment(const QString&)));
			// board -> stones
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_addStone(enum StoneColor, int, int)),
				qgobrd,
				SLOT(slot_addStone(enum StoneColor, int, int)));
			// board-> stones (computer game)
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_Stone_Computer(enum StoneColor, int, int)),
				qgobrd,
				SLOT(slot_stoneComputer(enum StoneColor, int, int)));

			connect(qgobrd,
				SIGNAL(signal_2passes(const QString&,const QString& )),
				this,
				SLOT(slot_removestones(const QString&, const QString&)));

			connect(qgobrd,
				SIGNAL(signal_sendcommand(const QString&, bool)),
				this,
				SLOT(slot_sendcommand(const QString&, bool)));
			// board -> commands
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_pass()),
				qgobrd,
				SLOT(slot_doPass()));
#if 0
			connect(qgobrd->get_win()->get_fileClose(),
				SIGNAL(activated()),
				qgobrd,
				SLOT(slot_doAdjourn()));
			connect(qgobrd->get_win()->get_fileQuit(),
				SIGNAL(activated()),
				qgobrd,
				SLOT(slot_doAdjourn()));
#endif
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_adjourn()),
				qgobrd,
				SLOT(slot_doAdjourn()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_undo()),
				qgobrd,
				SLOT(slot_doUndo()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_resign()),
				qgobrd,
				SLOT(slot_doResign()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_done()),
				qgobrd,
				SLOT(slot_doDone()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_refresh()),
				qgobrd,
				SLOT(slot_doRefresh()));
			connect(qgobrd->get_win()->getBoard(),
				SIGNAL(signal_editBoardInNewWindow()),
				qgobrd->get_win(),
				SLOT(slot_editBoardInNewWindow()));
			// teach tools
			connect(qgobrd->get_win()->getMainWidget()->cb_opponent,
				SIGNAL(activated(const QString&)),
				qgobrd,
				SLOT(slot_ttOpponentSelected(const QString&)));
			connect(qgobrd->get_win()->getMainWidget()->pb_controls,
				SIGNAL(toggled(bool)),
				qgobrd,
				SLOT(slot_ttControls(bool)));
			connect(qgobrd->get_win()->getMainWidget()->pb_mark,
				SIGNAL(toggled(bool)),
				qgobrd,
				SLOT(slot_ttMark(bool)));

			if (src == 2)
			{
				qgobrd->set_sentmovescmd(true);
				emit signal_sendcommand("games " + txt, false);
				emit signal_sendcommand("moves " + txt, false);
				emit signal_sendcommand("all " + txt, false);
			}

			// for own games send "games" cmd to get full info
			if (src == 3 || src == 4 || src == 13 || src == 14 )//|| src == 0)
			{
				connect(qgobrd->get_win()->getInterfaceHandler()->normalTools->pb_timeBlack,
					SIGNAL(clicked()),
					qgobrd,
					SLOT(slot_addtimePauseB()));
				connect(qgobrd->get_win()->getInterfaceHandler()->normalTools->pb_timeWhite,
					SIGNAL(clicked()),
					qgobrd,
					SLOT(slot_addtimePauseW()));

				if (src == 13 || src == 14 )//|| src== 0)
				{
					// src changed from 1 to 3/4
					qgobrd->set_sentmovescmd(true);
					emit signal_sendcommand("games " + g->nr, false);
					emit signal_sendcommand("moves " + g->nr, false);
					emit signal_sendcommand("all " + g->nr, false);

					if (src == 13)
						src = 3;
					else
						src = 4;
				}
				else
					emit signal_sendcommand("games " + txt, false);

				if (src == 4)
				{
					// make teaching features visible
					qgobrd->get_win()->getMainWidget()->cb_opponent->setEnabled(true);
					qgobrd->get_win()->getMainWidget()->pb_controls->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_mark->setEnabled(true);
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = true;
					qgobrd->havePupil = false;
					qgobrd->mark_set = false;
				}
			}

			// set correct mode in qGo
			qgobrd->set_id(game_id);
			qgobrd->set_gsName(gsName);
			qgobrd->set_myName(myName);
			if ((game_id == 0) && (src != 6)) //SL added eb 12
				// set local board
				qgobrd->set_Mode_real (modeNormal); // ??? was mode 5, identical to mode 1 ??
			else if (src==0)
				qgobrd->set_Mode_real (modeObserve); // special case when not triggered by the 'observe' command (trail, for instance)
			else
				qgobrd->set_Mode(src);

			// increase number of observed games
			emit signal_addToObservationList(-2);

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
						emit signal_sendcommand("moves " + gi->nr, false);
						emit signal_sendcommand("all " + gi->nr, false);
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

			if (!g->running)
			{
				// stopped game do not need a timer
				qgobrd->set_stopTimer();

				ASSERT(g->Sz);

				if (g->Sz == "-")
				{
					return false;
				}

#if 0
				if (g->Sz.contains("adjourned"))
					qgobrd->send_kibitz(tr("Game has adjourned"));
				else if (g->Sz.contains("White forfeits"))
					qgobrd->send_kibitz(tr("White forfeits on time"));
				else if (g->Sz.contains("Black forfeits"))
					qgobrd->send_kibitz(tr("Black forfeits on time"));
				else if (g->Sz.contains("White resigns"))
					qgobrd->send_kibitz(tr("White resigns"));
				else if (g->Sz.contains("Black resigns"))
					qgobrd->send_kibitz(tr("Black resigns"));
				else
#endif
					qgobrd->send_kibitz(g->Sz);

				// set correct result entry
				QString rs = QString::null;
				QString extended_rs = g->Sz;

				if (g->Sz.contains("White forfeits"))
					rs = "B+T";
				else if (g->Sz.contains("Black forfeits"))
					rs = "W+T";
				else if (g->Sz.contains("has run out of time"))
					 rs = ((( g->Sz.contains(myName) && qgobrd->get_myColorIsBlack() ) ||
						( !g->Sz.contains(myName) && !qgobrd->get_myColorIsBlack() )) ?
						"W+T" : "B+T");

				else if (g->Sz.contains("W ", true) && g->Sz.contains("B ", true))
				{
					// NNGS: White resigns. W 59.5 B 66.0
					// IGS: W 62.5 B 93.0
					// calculate result first
					int posw, posb;
					float re1, re2;
					posw = g->Sz.find("W ");
					posb = g->Sz.find("B ");
					bool wfirst = posw < posb;

					if (!wfirst)
					{
						int h = posw;
						posw = posb;
						posb = h;
					}

					QString t1 = g->Sz.mid(posw+1, posb-posw-2);
					QString t2 = g->Sz.right(g->Sz.length()-posb-1);
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
				else if (g->Sz.contains("White resigns"))
					rs = "B+R";
				else if (g->Sz.contains("Black resigns"))
					rs = "W+R";
				else if (g->Sz.contains("has resigned the game"))
					 rs = ((( g->Sz.contains(myName) && qgobrd->get_myColorIsBlack() ) ||
						( !g->Sz.contains(myName) && !qgobrd->get_myColorIsBlack() )) ?
						"W+R" : "B+R");
				if (!rs.isNull())
				{
					qgobrd->get_win()->getBoard()->getGameData()->result = rs;
					qgobrd->send_kibitz(rs);
					qDebug() << "Result: " << rs;
				}

				QString wplayer = qgobrd->get_wplayer();
				QString bplayer = qgobrd->get_bplayer();
				if (g->Sz.contains("adjourned") && !bplayer.isEmpty() && !wplayer.isEmpty())
				{
					if (bplayer == myName || wplayer == myName)
						// can only reload own games
						qgobrd->get_win()->getInterfaceHandler()->refreshButton->setText(tr("LOAD"));
					qDebug() << "game adjourned... #" << QString::number(qgobrd->get_id());
					qgobrd->set_adj(true);
					qgobrd->set_id(10000);
					qgobrd->clearObserverList();
					qgo->playGameEndSound();
				}
				else
				{
					// case: error -> game has not correctly been removed from list
					GameMode owngame = qgobrd->get_Mode();
					// game ended - if new game started with same number -> prohibit access
					qgobrd->set_id(-qgobrd->get_id());

					// enable Menus
//					qgobrd->get_win()->setOnlineMenu(false);

					// set board editable...
					qgobrd->set_Mode_real (modeNormal);

#if 0
					//autosave ?
					if ((setting->readBoolEntry("AUTOSAVE")) && (owngame == modeObserve)) 
					{
						qgobrd->get_win()->doSave(qgobrd->get_win()->getBoard()->getCandidateFileName(),true);
						qDebug("Game saved");
					}
#endif
					// but allow to use "say" cmd if it was an own game
					//if (owngame == modeMatch)
					bool doSave = ((setting->readBoolEntry("AUTOSAVE")) && (owngame == modeObserve)) ||
							((setting->readBoolEntry("AUTOSAVE_PLAYED")) && (owngame == modeMatch)); 
					wrapupMatchGame(qgobrd, doSave);
					if (owngame != modeObserve)
						QMessageBox::information(qgobrd->get_win(),
									 tr("Game n° ") + QString::number(-qgobrd->get_id()), extended_rs);
					//else
					//	qgo->playGameEndSound();

#if 0
					{
							// allow 'say' command
						qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setReadOnly(false);
						qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setDisabled(false);
					}
#endif
				}

				// decrease number of observed games
				emit signal_addToObservationList(-1);
			}
			else if (!qgobrd->get_havegd())
			{
				// check if it's my own game
				if (g->bname == myName || g->wname == myName)
				{
					// case: trying to observe own game: this is the way to get back to match if
					// board has been closed by mistake
					if (g->bname == g->wname)
						// teach
						qgobrd->set_Mode_real (modeTeach);
					else
						// match
						qgobrd->set_Mode (modeMatch);
				}
				else if (g->bname == g->wname && !g->bname.contains('*'))
				{
					// case: observing a teaching game
					// make teaching features visible while observing
					qgobrd->get_win()->getMainWidget()->cb_opponent->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_controls->setDisabled(true);
					qgobrd->get_win()->getMainWidget()->pb_mark->setDisabled(true);
					qgobrd->ExtendedTeachingGame = true;
					qgobrd->IamTeacher = false;
					qgobrd->IamPupil = false;
					qgobrd->havePupil = false;
				}
				qgobrd->set_game(g);

				if (qgobrd->get_Mode() == modeMatch)
				{
					if (g->bname == myName)
					{
						qgobrd->get_win()->getBoard()->set_myColorIsBlack(true);
						qgobrd->set_myColorIsBlack(true);

						// I'm black, so check before the first move if komi, handi, free is correct
						if (qgobrd->get_requests_set())
						{
							qDebug() << "qGoIF::parse_move() : check_requests";
							qgobrd->check_requests();
						}
					}
					else if (g->wname == myName)
					{
						qgobrd->get_win()->getBoard()->set_myColorIsBlack(false);
						qgobrd->set_myColorIsBlack(false);
					}
					else
						qWarning("*** Wanted to set up game without knowing my color ***");
				}
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
void qGoIF::slot_move(Game *g)
{
	parse_move(1, 0, g);
}

void qGoIF::slot_computer_game(QNewGameDlg * dlg)
{
	Game  g;

	//	g.nr = lv_popupGames->text(0);

	g.wname = dlg->getPlayerWhiteName();
	//	g.wrank = lv_popupGames->text(2);
	g.bname = dlg->getPlayerBlackName();
	//	g.brank = lv_popupGames->text(4);
	g.mv = "0";
	g.Sz.setNum(dlg->getSize()); 
	g.H.setNum(dlg->getHandicap());
	g.K.setNum(dlg->getKomi());
	//g.By = lv_popupGames->text(9);
	//g.FR = lv_popupGames->text(10);
	//g.ob = lv_popupGames->text(11);
	g.running = true;
	g.oneColorGo = dlg->getOneColorGo();


	if ((dlg->getPlayerWhiteType()==HUMAN)&&
		(dlg->getPlayerBlackType()==HUMAN))
	{
		QMessageBox::warning(this->get_parent(), PACKAGE, tr("*** Both players are Human ! ***"));
		return ;
	}

	parse_move(6, 0, &g);
	qgobrd->set_game(&g);
	qgobrd->set_stopTimer();
	
	/*
	* This is creepy ...
	* for some reason, the function 2 lines below zeroes the 'qgobrd' pointer
	* This happens when connected to IGS. Hours of trace did not deliver a clue
	* So we make it dirty : we store the pointer
	*/
	
	qGoBoard *qb = qgobrd;

	QString computerPath = setting->readEntry("COMPUTER_PATH");
#if defined(Q_OS_MACX)
	// Default to built-in gnugo on Mac OS X
	if(computerPath.isNull() || computerPath.isEmpty())
	{
	// Default the computer opponent path to built-in gnugo, if available
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	computerPath = (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())		
		+ "/Contents/MacOS/gnugo";
	}
#endif

	if (!qgobrd->get_win()->startComputerPlay(dlg,dlg->fileName, computerPath))
	{
		// computer program failed -> end game
		slot_closeevent();
		return ;
	}

	/*************************************************** 
	*	OK, we reset the pointer
	****************************************************/
	qgobrd=qb;
	
	// if we have loaded a game, we have to overwrite the game settings
	if (!(dlg->fileName.isNull() || dlg->fileName.isEmpty()))
	{
		qgobrd->gd.size = qgobrd->get_win()->getBoard()->getGameData()->size;
		qgobrd->gd.handicap = qgobrd->get_win()->getBoard()->getGameData()->handicap;
		qgobrd->gd.playerWhite =qgobrd->get_win()->getBoard()->getGameData()->playerWhite;
		qgobrd->gd.playerBlack =qgobrd->get_win()->getBoard()->getGameData()->playerBlack;
	}
	//qgobrd->set_game(&g);
	
		
	qgobrd->get_win()->getBoard()->set_myColorIsBlack((qgobrd->get_win()->blackPlayerType==HUMAN));
	qgobrd->set_myColorIsBlack((qgobrd->get_win()->blackPlayerType==HUMAN));

	if (qgobrd->get_win()->getBoard()->get_myColorIsBlack() != qgobrd->get_win()->getBoard()->getBoardHandler()->getBlackTurn())
		qgobrd->playComputer(qgobrd->get_win()->getBoard()->getBoardHandler()->getBlackTurn() ? stoneWhite : stoneBlack);
}

// game to observe (mouse click)
void qGoIF::set_observe(const QString& gameno)
{
	parse_move(2, 0, 0, gameno);
}

// remove all boards
void qGoIF::set_initIF()
{
	parse_move(-1);
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
	qGoBoard *qb = boardlist->first();
	while (qb && qb->get_id() != id.toInt())
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("BOARD CORRESPONDING TO GAME SETTINGS NOT FOUND !!!");
		return;
	}

	qb->set_requests(handicap, komi, kt);
	qDebug() << QString("qGoIF::slot_matchsettings: h=%1, k=%2, kt=%3").arg(handicap).arg(komi).arg(kt);
}

void qGoIF::slot_title(const GameInfo *gi, const QString &title)
{
	// This used to check that only teaching games can have a title.
	// However, it seems that we can reliably identify boards by game
	// ID - see parser.cpp.

	qGoBoard *qb = boardlist->first();
	while (qb)
	{
		if (qb->get_id() == gi->nr.toInt()
		    && (qb->get_Mode() == modeMatch || qb->get_Mode() == modeTeach
			|| qb->get_Mode() == modeObserve))
		{
			qb->set_title(title);
			return;
		}
		qb = boardlist->next();
	}	
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
		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != nr && qb->get_bplayer() != nr)
			qb = boardlist->next();

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
					emit signal_sendcommand("komi " + komi, true);
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

		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName)
			qb = boardlist->next();
	}
	else
	{
		// title message follows to move message
		qb = boardlist->first();
		while (qb != NULL && qb->get_id() != nr.toInt())
			qb = boardlist->next();
	}

	if (qb)
		qb->set_komi(komi);
}

// game free/unfree message received
void qGoIF::slot_freegame(bool freegame)
{
	qGoBoard *qb;

	qb = boardlist->first();
	// what if 2 games (IGS) and mv_counter < 3 in each game? -> problem
	while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName && qb->get_mvcount() < 3)
		qb = boardlist->next();

	if (qb)
		qb->set_freegame(freegame);
}

// board window closed...
void qGoIF::slot_closeevent()
{
	int id = 0;
	qGoBoard *qb;

	// erase actual pointer to prevent error
	qgobrd = 0;

	// seek board
	qb = boardlist->first();
	while (qb && (id = qb->get_id()) > 0)
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("CLOSED BOARD NOT FOUND IN LIST !!!");
		return;
	}

	qDebug() << QString("qGoIF::slot_closeevent() -> game %1").arg(qb->get_id());

	// destroy timers
	qb->set_stopTimer();

	if (id < 0 && id > -10000)
	{
		switch (qb->get_Mode())
		{
			case modeObserve:
				// unobserve
				emit signal_sendcommand("observe " + QString::number(-id), false);

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			case modeMatch:

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			case modeTeach:
				emit signal_sendcommand("adjourn", false);

				// decrease number of observed games
				emit signal_addToObservationList(-1);
				break;

			default:
				break;
		}
	}

//	MainWindow *win = qb->get_win();
//	qgo->removeBoardWindow(win);
	boardlist->remove();

	delete qb;
//	delete win;
}

// kibitz strings
void qGoIF::slot_kibitz(int num, const QString& who, const QString& msg)
{
	qGoBoard *qb;
	QString name;

	// own game if num == NULL
	if (!num)
	{
		if (myName.isEmpty())
		{
			// own name not set -> should never happen!
			qWarning("*** qGoIF::slot_kibitz(): Don't know my online name ***");
			return;
		}

		qb = boardlist->first();
		while (qb != NULL && qb->get_wplayer() != myName && qb->get_bplayer() != myName)
			qb = boardlist->next();

		if (qb && qb->get_wplayer() == myName)
			name = qb->get_bplayer();
		else if (qb)
			name = qb->get_wplayer();

		if (qb)
			// sound for "say" command
			qgo->playSaySound();
	}
	else
	{
		// seek board to send kibitz
		qb = boardlist->first();
		while (qb && qb->get_id() != num)
			qb = boardlist->next();

		name = who;
	}

	if (!qb)
		qDebug("Board to send kibitz string not in list...");
	else if (!num && !who.isEmpty())
	{
		// special case: opponent has resigned - interesting in quiet mode
		qb->send_kibitz(msg);
		//qgo->playGameEndSound();
		wrapupMatchGame(qb, true);
	}
	else
		qb->send_kibitz(name + ": " + msg);
}

void qGoIF::slot_requestDialog(const QString &yes, const QString &no, const QString & /*id*/, const QString &opponent)
{
	QString opp = opponent;
	if (opp.isEmpty())
		opp = tr("Opponent");

	if (no.isEmpty())
	{
		QMessageBox mb(tr("Request of Opponent"),
			QString(tr("%1 wants to %2\nYES = %3\nCANCEL = %4")).arg(opp).arg(yes).arg(yes).arg(tr("ignore request")),
			QMessageBox::NoIcon,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::Cancel | QMessageBox::Escape,
			Qt::NoButton);
		mb.setActiveWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			emit signal_sendcommand(yes, false);
		}
	}
	else
	{
		QMessageBox mb(tr("Request of Opponent"),
			//QString(tr("%1 wants to %2\nYES = %3\nCANCEL = %4")).arg(opp).arg(yes).arg(yes).arg(no),
			QString(tr("%1 wants to %2\n\nDo you accept ? \n")).arg(opp).arg(yes),
			QMessageBox::Question,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No | QMessageBox::Escape,
			Qt::NoButton);
		mb.setActiveWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			emit signal_sendcommand(yes, false);
		}
		else
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(no);
			emit signal_sendcommand(no, false);
		}
	}
}

void qGoIF::slot_sendcommand(const QString &text, bool show)
{
	emit signal_sendcommand(text, show);
}

void qGoIF::slot_removestones(const QString &pt, const QString &game_id)
{
	qGoBoard *qb = boardlist->first();

	if (pt.isEmpty() && game_id.isEmpty())
	{
qDebug("slot_removestones(): !pt !game_id");
		// search game
		qb = boardlist->first();
		while (qb && (qb->get_Mode() != modeMatch) && (qb->get_Mode() != modeComputer))
			qb = boardlist->next();
		
		if (!qb)
		{
			qWarning("*** No Match found !!! ***");
			return;
		}

		switch (gsName)
		{
			case IGS:
				// no parameter -> restore counting
				if (qb->get_win()->getInterfaceHandler()->passButton->text() == QString(tr("Done")))
				{
qDebug("slot_removestones(): IGS restore counting");
					// undo has done -> reset counting
					qb->get_win()->doRealScore(false);
					qb->get_win()->getBoard()->setMode(modeMatch);
					qb->get_win()->getBoard()->previousMove();
					qb->dec_mv_counter();
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: RESET - click on a stone to mark as dead..."));
				}
				else if (qb->get_Mode() == modeMatch)
				{
qDebug("slot_removestones(): IGS score mode");
					// set to count mode
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
				}
				break;

			default:
				if ((qb->get_Mode() == modeMatch) || (qb->get_Mode() == modeComputer))
				{
qDebug("slot_removestones(): NON IGS counting");
					// set to count mode
					qb->get_win()->doRealScore(true);
					qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
				}
				else
				{
qWarning("slot_removestones(): NON IGS no match");
					// back to matchMode
					qb->get_win()->doRealScore(false);
					qb->get_win()->getBoard()->setMode(modeMatch);
					qb->get_win()->getBoard()->previousMove();
					qb->dec_mv_counter();
					qb->send_kibitz(tr("GAME MODE: place stones..."));
				}
				break;
		}

		return;
	}

	if (!pt.isEmpty() && game_id.isEmpty())
	{
qDebug("slot_removestones(): pt !game_id");
		// stone coords but no game number:
		// single match mode, e.g. NNGS
		while (qb && qb->get_Mode() != modeMatch && qb->get_Mode() != modeTeach)
			qb = boardlist->next();
	}
	else
	{
qDebug("slot_removestones(): game_id");
		// multi match mode, e.g. IGS
		while (qb && qb->get_id() != game_id.toInt())
			qb = boardlist->next();

		if (qb && qb->get_win()->getInterfaceHandler()->passButton->text() != QString(tr("Done")))
		{
			// set to count mode
			qb->get_win()->doRealScore(true);
			qb->send_kibitz(tr("SCORE MODE: click on a stone to mark as dead..."));
		}
	}
		
	if (!qb)
	{
		qWarning("*** No Match found !!! ***");
		return;
	}

	int i = pt[0].toAscii() - 'A' + 1;
	// skip j
	if (i > 8)
		i--;
	
	int j;
	if (pt[2].toAscii() >= '0' && pt[2].toAscii() <= '9')
		j = qb->get_boardsize() + 1 - pt.mid(1,2).toInt();
	else
		j = qb->get_boardsize() + 1 - pt[1].digitValue();

	// mark dead stone
	qb->get_win()->getBoard()->getBoardHandler()->markDeadStone(i, j);
	qb->send_kibitz("removing @ " + pt);
}

// game status received
void qGoIF::slot_result(const QString &txt, const QString &line, bool isplayer, 
const QString &komi)
{
	static qGoBoard *qb;
	static float wcount, bcount;
	static int column;

	if (isplayer)
	{
		qb = boardlist->first();
		// find right board - slow, but just once a game
		while (qb != NULL
		       && ((qb->get_wplayer() != txt && qb->get_bplayer() != txt
			    && qb->get_Mode() != modeEdit)))
			qb = boardlist->next();

		if (qb && qb->get_wplayer() == txt)
		{
			// prisoners white + komi
			wcount = line.toFloat() + komi.toFloat();
		}
		else if (qb && qb->get_bplayer() == txt)
		{
			// prisoners black
			bcount = line.toFloat();
			// ok, start counting column
			column = 0;
		}
	}
	else if (qb)
	{
		// ok, now mark territory points only
		// 0 .. black
		// 1 .. white
		// 2 .. free
		// 3 .. neutral
		// 4 .. white territory
		// 5 .. black territory

		// one line at a time
		column++;
		int i;
		for (i = 0; i < (int) line.length(); i++)
		{
			switch(line[i].digitValue())
			{
				case 4:
					// add white territory
					wcount++;
					qb->get_win()->getBoard()->setMark(column, i+1, markTerrWhite, true);
					break;

				case 5:
					// add black territory
					bcount++;
					qb->get_win()->getBoard()->setMark(column, i+1, markTerrBlack, true);
					break;

				default:
					break;
			}
		}
		// send kibitz string after counting
		if (txt.toInt() == (int) line.length() - 1)
			/*send_kibitz*/ qDebug() << tr("Game Status: W:") << QString::number(wcount) << " " << tr("B:") << QString::number(bcount);

		// show territory
		qb->get_win()->getBoard()->updateCanvas();
	}
}

// undo a move
void qGoIF::slot_undo(const QString &player, const QString &move)
{
	qDebug("qGoIF::slot_undo()");
	qGoBoard *qb = boardlist->first();

	// check if game number given
	bool ok;
	int nr = player.toInt(&ok);
	if (ok)
		while (qb != NULL && qb->get_id() != nr)
			qb = boardlist->next();
	else
		while (qb != NULL && qb->get_wplayer() != player && qb->get_bplayer() != player)
			qb = boardlist->next();

	if (!qb)
	{
		qWarning("*** board for undo not found!");
		return;
	}

	if (move != "Pass")
	{
		// only the last '0' is necessary
		qb->set_move(stoneNone, 0, 0);
		return;
	}

	// back to matchMode
	qb->get_win()->doRealScore(false);
	qb->get_win()->getBoard()->setMode(modeMatch);
	qb->get_win()->getBoard()->previousMove();
	qb->dec_mv_counter();
	qb->send_kibitz(tr("GAME MODE: place stones..."));
}



// set independent local board
void qGoIF::set_localboard(QString file)
{
	qGoBoard *qb = new qGoBoard();
	qb->set_id(0);
	// normal mode
	qb->set_Mode_real (modeNormal);
	if (file == "/19/")
	{
		qb->set_komi("5.5");
		// special case - set up 19x19 board without any questions
	}
	else if (file.isNull ())
		qb->get_win()->slotFileNewGame();
	else
		qb->get_win()->doOpen(file, 0, false);
//	qb->get_win()->setOnlineMenu(false);
	qb->get_win()->setGameMode (modeNormal);

	boardlist->append(qb);
	qb->set_id(++localBoardCounter);

	// connect with board (MainWindow::CloseEvent())
	connect(qb->get_win(),
		SIGNAL(signal_closeevent()),
		qb,
		SLOT(slot_closeevent()));
}

// set independent local board + open game
void qGoIF::set_localgame()
{
	qGoBoard *qb = new qGoBoard();
	qb->set_id(0);
	// normal mode
	qb->set_Mode_real (modeNormal);
	qb->get_win()->slotFileOpen();
//	qb->get_win()->setOnlineMenu(false);

	boardlist->append(qb);
	qb->set_id(++localBoardCounter);

	// connect with board (MainWindow::CloseEvent())
	connect(qb->get_win(),
		SIGNAL(signal_closeevent()),
		qb,
		SLOT(slot_closeevent()));
}

// direct access to preferences dialog
void qGoIF::openPreferences(int tab)
{
	MainWindow *win = new MainWindow(0, PACKAGE);

	win->dlgSetPreferences(tab);

	delete win;
}

// time has been added
void qGoIF::slot_timeAdded(int time, bool toMe)
{
	qGoBoard *qb = boardlist->first();
	while (qb != NULL && qb->get_Mode() != modeMatch)
		qb = boardlist->next();

	if (!qb)
	{
		qWarning("*** board for undo not found!");
		return;
	}

	// use time == -1 to pause the game
	if (time == -1)
	{
		qb->set_gamePaused(toMe);
		return;
	}

	if (toMe && qb->get_myColorIsBlack() || !toMe && !qb->get_myColorIsBlack())
		qb->addtime_b(time);
	else
		qb->addtime_w(time);
}


void qGoIF::wrapupMatchGame(qGoBoard * qgobrd, bool doSave)
{
	qgo->playGameEndSound();				
	qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setReadOnly(false);
	qgobrd->get_win()->getInterfaceHandler()->commentEdit2->setDisabled(false);
	
	//autosave ?
	if (doSave) 
	{
		qgobrd->get_win()->doSave(qgobrd->get_win()->getBoard()->getCandidateFileName(),true);
		qDebug("Game saved");
	}


}


/*
 *	qGo Board
 */



// add new board window
qGoBoard::qGoBoard() : QObject()
{
	qDebug("::qGoBoard()");
	have_gameData = false;
	sent_movescmd = false;
	adjourned = false;
	gd = GameData();
	mv_counter = -1;
	requests_set = false;
	haveTitle = false;
	win = new MainWindow(0, PACKAGE);

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
	qgo->addBoardWindow(win);

	// disable some Menu items
//	win->setOnlineMenu(true);
}

qGoBoard::~qGoBoard()
{
qDebug("~qGoBoard()");
	qgo->removeBoardWindow(win);
	// do NOT delete *win, cause MainWindow does it first!
	delete win;
}

// set game info to one board
void qGoBoard::set_game(Game *g)
{
	gd.playerBlack = g->bname;
	gd.playerWhite = g->wname;
	gd.rankBlack = g->brank;
	gd.rankWhite = g->wrank;
//	QString status = g->Sz.simplifyWhiteSpace();
	gd.size = g->Sz.toInt();
	gd.handicap = g->H.toInt();
	gd.komi = g->K.toFloat();
	gd.gameNumber = id;
	if (g->FR.contains("F"))
		gd.freegame = FREE;
	else if (g->FR.contains("T"))
		gd.freegame = TEACHING;
	else
		gd.freegame = RATED;
	if (gd.freegame != TEACHING)
	{
		gd.timeSystem = canadian;
		gd.byoTime = g->By.toInt()*60;
		gd.byoStones = 25;
		if (wt_i > 0)
			gd.timelimit = (((wt_i > bt_i ? wt_i : bt_i) / 60) + 1) * 60;
		else
			gd.timelimit = 60;

		if (gd.byoTime == 0)
			// change to absolute
			gd.timeSystem = absolute;
		else
			gd.overtime = "25/" + QString::number(gd.byoTime) + " Canadian";
	}
	
	else
	{
		gd.timeSystem = time_none;
		gd.byoTime = 0;
		gd.byoStones = 0;
		gd.timelimit = 0;
	}
	gd.style = 1;
	gd.oneColorGo = g->oneColorGo ;
  	
	switch (gsName)
	{
		case IGS :
			gd.place = "IGS";
			break ;
		case LGS :
			gd.place = "LGS";
			break ;
		case WING :
			gd.place = "WING";
			break ;
		case CWS :
			gd.place = "CWS";
			break;
		default :
			;
	}

	gd.date = QDate::currentDate().toString("dd MM yyyy") ;
	
	setMode();
	initGame();
	setMode();
//	win->toggleMode();
	have_gameData = true;

	// needed for correct sound
	stated_mv_count = g->mv.toInt();
	sound = false;
}

void qGoBoard::set_title(const QString &t)
{
	gd = win->getBoard()->getGameData();
	gd.gameName = t;
	win->getBoard()->setGameData(&gd);
	win->getBoard()->updateCaption();

	haveTitle = true;
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

	gd.komi = k_.toFloat();
	win->getBoard()->setGameData(&gd);
	win->getInterfaceHandler()->normalTools->komi->setText(k_);
}

void qGoBoard::set_freegame(bool f)
{
	if (f)
	{
		win->getBoard()->getInterfaceHandler()->normalTools->TextLabel_free->setText(tr("free"));
		gd.freegame = FREE;
	}
	else
	{
		win->getBoard()->getInterfaceHandler()->normalTools->TextLabel_free->setText(tr("rated"));
		gd.freegame = RATED;
	}
	win->getBoard()->setGameData(&gd);
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
	if (mv_counter < 0 || id < 0 || game_paused)
		return;

	if (mv_counter % 2)
	{
		// B's turn
		bt_i--;

#ifdef SHOW_INTERNAL_TIME
		chk_b--;
		if (chk_b < bt_i + 20)
			chk_b = bt_i;
		win->getInterfaceHandler()->setTimes("(" + secToTime(chk_b) + ") " + secToTime(bt_i), b_stones, "(" + secToTime(chk_w) + ") " + wt, w_stones);
#else
		win->getInterfaceHandler()->setTimes(secToTime(bt_i), b_stones, wt, w_stones);
#endif
	}
	else
	{
		// W's turn
		wt_i--;

#ifdef SHOW_INTERNAL_TIME
		chk_w--;
		if (chk_w < bt_i + 20)
			chk_w = wt_i;
		win->getInterfaceHandler()->setTimes("(" + secToTime(chk_b) + ") " + bt, b_stones, "(" + secToTime(chk_w) + ") " + secToTime(wt_i), w_stones);
#else
		win->getInterfaceHandler()->setTimes(bt, b_stones, secToTime(wt_i), w_stones);
#endif
	}

	// warn if I am within the last 10 seconds
	if (gameMode == modeMatch)
	{
		if (myColorIsBlack && bt_i <= BY_timer && bt_i > -1 && mv_counter % 2) // ||
		{
			// each second we alternate button background color
			if (bt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}		
	
		else if ( !myColorIsBlack && wt_i <= BY_timer && wt_i > -1 && (mv_counter % 2) == 0)
		{
			if (wt_i %2)
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(Qt::red);
			else 
				win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;//setPaletteBackgroundColor(Qt::PaletteBase);
			qgo->playTimeSound();
		}
		
		//we have to reset the color when not anymore in warning period)
		else if (win->getInterfaceHandler()->normalTools->pb_timeBlack->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeBlack->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		
		else if (win->getInterfaceHandler()->normalTools->pb_timeWhite->paletteBackgroundColor() == Qt::red)
			win->getInterfaceHandler()->normalTools->pb_timeWhite->setPaletteBackgroundColor(win->getInterfaceHandler()->normalTools->palette().color(QPalette::Active,QColorGroup::Button)) ;

		

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

	// set initial timer until game is initialized
	if (!have_gameData)
		win->getInterfaceHandler()->setTimes(bt, bstones, wt, wstones);

	// if time info available, sound can be played
	// cause no moves cmd in execution
	sound = true;
}

// addtime function
void qGoBoard::addtime_b(int m)
{
	bt_i += m*60;
	bt = secToTime(bt_i);
	win->getInterfaceHandler()->setTimes(secToTime(bt_i), b_stones, wt, w_stones);
}

void qGoBoard::addtime_w(int m)
{
	wt_i += m*60;
	wt = secToTime(wt_i);
	win->getInterfaceHandler()->setTimes(bt, b_stones, secToTime(wt_i), w_stones);
}

void qGoBoard::set_Mode_real(GameMode mode)
{
	switch (mode)
	{
	case modeNormal:
		win->getBoard()->set_isLocalGame(true);
		break;
	case modeObserve:
		win->getBoard()->set_isLocalGame(false);
		break;
	case modeMatch:
		win->getBoard()->set_isLocalGame(false);
		break;
	case modeTeach:
		win->getBoard()->set_isLocalGame(false);
		break;
	case modeComputer:
		win->getBoard()->set_isLocalGame(true);
		break;
	}

	win->setGameMode(mode);
}

void qGoBoard::set_Mode(int src)
{
  fprintf (stderr, "Set mode: %d\n", src);
	switch (src)
	{
	case 1:
		set_Mode_real (modeNormal);
		break;
	case 2:
		set_Mode_real (modeObserve);
		break;
	case 3:
		set_Mode_real (modeMatch);
		break;

	case 4:
		set_Mode_real (modeTeach);
		win->getBoard()->set_isLocalGame(false);
		break;

	case 5:
		// for game 0; local game
		set_Mode_real (modeNormal);
		return;

	case 6:
		set_Mode_real (modeComputer);
		break;
	default:
		break;
	}
}

void qGoBoard::set_move(StoneColor sc, QString pt, QString mv_nr)
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
			// back to matchMode
			win->doRealScore(false);
			win->getBoard()->setMode(modeMatch);
		}

		// special case: undo handicap
		if (mv_counter <= 0 && gd.handicap)
		{
			gd.handicap = 0;
			win->getBoard()->getBoardHandler()->setHandicap(0);
			qDebug("set Handicap to 0");
		}

		if (pt.isEmpty())
		{
			// case: undo
			qDebug() << "set_move(): UNDO in game " << QString::number(id);
			qDebug() << "...mv_nr = " << mv_nr;

			                                                                       //added eb 9
			Move *m=win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			Move *last=win->getBoard()->getBoardHandler()->lastValidMove ; //win->getBoard()->getBoardHandler()->getTree()->findLastMoveInMainBranch();

			if (m!=last)                          // equivalent to display_incoming_move = false
				win->getBoard()->getBoardHandler()->getTree()->setCurrent(last) ;
			else
				m = m->parent ;                   //we are going to delete the node. We will bactrack to m

			win->getBoard()->deleteNode();
			win->getBoard()->getBoardHandler()->lastValidMove = win->getBoard()->getBoardHandler()->getTree()->getCurrent();
			win->getBoard()->getBoardHandler()->getTree()->getCurrent()->marker = NULL ; //(just in case)
			win->getBoard()->getBoardHandler()->getStoneHandler()->checkAllPositions();
                             
			win->getBoard()->getBoardHandler()->getTree()->setCurrent(m) ;   //we return where we were observing the game
			// this is not very nice, but it ensures things stay clean
			win->getBoard()->getBoardHandler()->updateMove(m);                      // end add eb 9

			mv_counter--;

			// ok for sound - no moves cmd in execution
			sound = true;
		}

		return;
	}
	else
		// move already done...
		return;

	if (pt.contains("Handicap"))
	{
		QString handi = pt.simplifyWhiteSpace();
		int h = handi.section(' ', 1, 1).toInt();

		// check if handicap is set with initGame() - game data from server do not
		// contain correct handicap in early stage, because handicap is first move!
		if (gd.handicap != h)
		{
			gd.handicap = h;
			win->getBoard()->getBoardHandler()->setHandicap(h);
			qDebug("corrected Handicap");
		}
	}
	else if (pt.contains("Pass",false))
	{
		win->getBoard()->doSinglePass();
		if (win->getBoard()->getBoardHandler()->local_stone_sound)
			qgo->playPassSound();
	}
	else
	{
		if ((gameMode == modeMatch) && (mv_counter < 2) && !(myColorIsBlack))
		{
			// if black has not already done - maybe too late here???
			if (requests_set)
			{
				qDebug() << "qGoBoard::set_move() : check_requests at move " << mv_counter;
				check_requests();
			}
		}

		int i = pt[0].toAscii() - 'A' + 1;
		// skip j
		if (i > 8)
			i--;

		int j;

		if (pt[2].toAscii() >= '0' && pt[2].toAscii() <= '9')
			j = gd.size + 1 - pt.mid(1,2).toInt();
		else
			j = gd.size + 1 - pt[1].digitValue();

		// avoid sound problem during execution of "moves" cmd
		if (stated_mv_count > mv_counter)
			sound = false;

		else if (gameMode == modeComputer)
			sound = true;

		win->getBoard()->addStone(sc, i, j, sound);
		Move *m = win->getBoard()->getBoardHandler()->getTree()->getCurrent();
		if (stated_mv_count > mv_counter ||
			//gameMode == modeTeach ||
			//ExtendedTeachingGame ||
			wt_i < 0 || bt_i < 0)
		{
			m->setTimeinfo(false);
		}
		else
		{
			m->setTimeLeft(sc == stoneBlack ? bt_i : wt_i);
			int stones = -1;
			if (sc == stoneBlack)
				stones = b_stones.toInt();
			else
				stones = w_stones.toInt();
			m->setOpenMoves(stones);
			m->setTimeinfo(true);

			// check for a common error -> times and open moves identical
			if (m->parent &&
				m->parent->parent &&
				m->parent->parent->getTimeinfo() &&
				m->parent->parent->getTimeLeft() == m->getTimeLeft() &&
				m->parent->parent->getOpenMoves() == m->getOpenMoves())
			{
				m->parent->parent->setTimeinfo(false);
			}
		}
	}
}

// board window closed
void qGoBoard::slot_closeevent()
{
	if (id > 0)
	{
		id = -id;
//		emit signal_closeevent(id);
	}
	else
		qWarning("id < 0 ******************");
}

// write kibitz strings to comment window
void qGoBoard::send_kibitz(const QString msg)
{
	// observer info
	if (msg[0] == '0')
	{
		if (msg[1] == '0')
		{
			// finish
			win->updateObserverCnt();
		}
		else if (msg.length() < 5)
		{
			//init
			win->getMainWidget()->cb_opponent->clear();
			win->getMainWidget()->TextLabel_opponent->setText(tr("opponent:"));
			win->getMainWidget()->cb_opponent->insertItem(tr("-- none --"));
			if (havePupil && ttOpponent != tr("-- none --"))
			{
				win->getMainWidget()->cb_opponent->insertItem(ttOpponent, 1);
				win->getMainWidget()->cb_opponent->setCurrentItem(1);
			}

			win->clearObserver();
		}
		else
		{
			win->getMainWidget()->cb_opponent->insertItem(msg.right(msg.length() - 3));
//			int cnt = win->getMainWidget()->cb_opponent->count();
//			win->getMainWidget()->TextLabel_opponent->setText(tr("opponent:") + " (" + QString::number(cnt-1) + ")");

			win->addObserver(msg.right(msg.length() - 3));
		}

		return;
	}

	// if observing a teaching game
	if (ExtendedTeachingGame && !IamTeacher)
	{
		// get msgs from teacher
		if (msg.find("#OP:") != -1)
		{
			// opponent has been selected
			QString opp;
			opp = msg.section(':', 2, -1).remove(QRegExp("\\(.*$")).stripWhiteSpace();

			if (opp == "0")
				slot_ttOpponentSelected(tr("-- none --"));
			else
				slot_ttOpponentSelected(opp);
		}
		else if (IamPupil)
		{
			if (msg.find("#TC:ON") != -1)
			{
				// teacher gives controls to pupil
				haveControls = true;
				win->getMainWidget()->pb_controls->setEnabled(true);
				win->getMainWidget()->pb_controls->setOn(true);
			}
			else if (msg.find("#TC:OFF") != -1)
			{
				// teacher takes controls back
				haveControls = false;
				win->getMainWidget()->pb_controls->setDisabled(true);
				win->getMainWidget()->pb_controls->setOn(false);
			}
		}
	}
	else if (ExtendedTeachingGame && IamTeacher)
	{
		// get msgs from pupil
		if (msg.find("#OC:OFF") != -1)
		{
			// pupil gives controls back
			haveControls = true;
			win->getMainWidget()->pb_controls->setOn(false);
		}
		else if (havePupil && msg.section(' ', 0, 0) == ttOpponent.section(' ', 0, 0) &&
		        (myColorIsBlack && (mv_counter % 2) || !myColorIsBlack && ((mv_counter % 2 + 1) || mv_counter == -1) ||
			   !haveControls))
		{
			// move from opponent - it's his turn (his color or he has controls)!
			// xxx [rk]: B1 (7)
			//   it's ensured that yyy [rk] didn't send forged message
			//   e.g.: yyy [rk]: xxx[rk]: S1 (3)
			QString s;
			s = msg.section(':', 1, -1).remove(QRegExp("\\(.*$")).stripWhiteSpace().upper();

			// check whether it's a position
			// e.g. B1, A17, NOT: ok, yes
			if (s.length() < 4 && s[0] >= 'A' && s[0] < 'U' && s[1] > '0' && s[1] <= '9')
			{
				// ok, could be a real pt
				// now teacher has to send it: pupil -> teacher -> server
				if (gsName == IGS)
					emit signal_sendcommand(s + " " + QString::number(id), false);
				else
					emit signal_sendcommand(s, false);
			}
		}
	}

	// skip my own messages
qDebug() << "msg.find(myName) = " << QString::number(msg.find(myName));
//	if (msg.find(myName) == 0)
//		return;

	// normal stuff...
	if (msg.contains(QString::number(mv_counter + 1)))
	{
qDebug("qGoBoard::send_kibitz()");
//		win->getInterfaceHandler()->displayComment(msg);
		// update comment in SGF list
		win->getInterfaceHandler()->board->getBoardHandler()->updateComment(msg);
	}
	else
	{
//		win->getInterfaceHandler()->displayComment("(" + QString::number(mv_counter + 1) + ") " + msg);
		// update comment in SGF list
		win->getInterfaceHandler()->board->getBoardHandler()->updateComment("(" + QString::number(mv_counter + 1) + ") " + msg);
	}
}

void qGoBoard::slot_sendcomment(const QString &comment)
{
	// # has to be at the first position
	if (comment.find("#") == 0)
	{
		qDebug("detected (#) -> send command");
		QString testcmd = comment;
		testcmd = testcmd.remove(0, 1).stripWhiteSpace();
		emit signal_sendcommand(testcmd, true);
		return;
	}

	if (gameMode == modeObserve || gameMode == modeTeach || gameMode == modeMatch && ExtendedTeachingGame)
	{
		// kibitz
		emit signal_sendcommand("kibitz " + QString::number(id) + " " + comment, false);
		send_kibitz("-> " + comment);
	}
	else
	{
		// say
		emit signal_sendcommand("say " + comment, false);
		send_kibitz("-> " + comment);
	}
}

void qGoBoard::slot_addStone(StoneColor /*c*/, int x, int y)
{
	if (id < 0)
		return;

	if (x > 8)
		x++;
	QChar c1 = x - 1 + 'A';
	int c2 = gd.size + 1 - y;

	if (ExtendedTeachingGame && IamPupil)
		emit signal_sendcommand("kibitz " + QString::number(id) + " " + QString(c1) + QString::number(c2), false);
	else if (gsName == IGS)
		emit signal_sendcommand(QString(c1) + QString::number(c2) + " " + QString::number(id), false);
	else if (!win->getBoard()->getBoardHandler()->getGtp())
		emit signal_sendcommand(QString(c1) + QString::number(c2), false);
}

void qGoBoard::slot_stoneComputer(StoneColor c, int x, int y)
{
	if (id < 0)
		return;

	if (x > 8)
		x++;
	char c1 = x - 1 + 'A';
	//int c2 = gd.size + 1 - y;
	int c2 =win->getBoard()->getGameData()->size + 1 - y;
	
	
  mv_counter++;
  
	switch (c)
  {
    case stoneWhite :
      if (win->getBoard()->getBoardHandler()->getGtp()->playwhite(c1 ,c2))
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to play the stone within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }
      if (win->blackPlayerType==COMPUTER && (win->getBoard()->getBoardHandler()->getGtp()->getLastMessage() != "illegal move") )
           playComputer(c);  
        
       break;
        
    case stoneBlack :
      if (win->getBoard()->getBoardHandler()->getGtp()->playblack(c1 ,c2))
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to play the stone within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }

      if (win->whitePlayerType==COMPUTER && (win->getBoard()->getBoardHandler()->getGtp()->getLastMessage() != "illegal move"))
           playComputer(c);   
      break;  
     
    default :
    	;           
   }
}

void qGoBoard::slot_PassComputer(StoneColor c)
{
	if (id < 0)
		return;

  mv_counter++;

  //check if this is the second pass move.
   if ((win->getBoard()->getBoardHandler()->getTree()->getCurrent()->isPassMove())&&(win->getBoard()->getBoardHandler()->getTree()->getCurrent()->parent->isPassMove()))
      {
        emit signal_2passes(0,0);
        return;
      }
  
  // if simple pass, tell computer and move on
	switch (c)
  {
    case stoneWhite :
      if (win->getBoard()->getBoardHandler()->getGtp()->playwhitePass())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to pass within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }
      if (win->blackPlayerType==COMPUTER)
           playComputer(c);

       break;

    case stoneBlack :
      if (win->getBoard()->getBoardHandler()->getGtp()->playblackPass())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to pass within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }

      if (win->whitePlayerType==COMPUTER)
           playComputer(c);
      break;

    default :
    	;   
      
   }

}

void qGoBoard::slot_UndoComputer(StoneColor /*c*/)
{
	if (id < 0)
		return;

  mv_counter++;
  
  if (win->getBoard()->getBoardHandler()->getGtp()->undo())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to undo within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }

  win->getBoard()->deleteNode();

  if (win->getBoard()->getBoardHandler()->getGtp()->undo())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to undo within program \n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }

  win->getBoard()->deleteNode();

}


void qGoBoard::playComputer(StoneColor c)
{

  // have the computer play a stone of color 'c'

  win->getBoard()->setCursor(Qt::WaitCursor);
  get_win()->getInterfaceHandler()->passButton->setEnabled(false);
  get_win()->getInterfaceHandler()->undoButton->setEnabled(false);
  
  switch (c)
  {
    case stoneWhite :

       if (win->getBoard()->getBoardHandler()->getGtp()->genmoveBlack())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to have the program play its stone\n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }
        break;

    case stoneBlack :
       if (win->getBoard()->getBoardHandler()->getGtp()->genmoveWhite())
        {
          QMessageBox::warning(win, PACKAGE, tr("Failed to have the program play its stone\n") + win->getBoard()->getBoardHandler()->getGtp()->getLastMessage());
          return;
        }
        break;

    default :
    	;
   }

   QString computer_answer = win->getBoard()->getBoardHandler()->getGtp()->getLastMessage();

   /* Normal procedure would be
   GameInfo *gi

   gi ....
   emit signal || parse move(0,qi)
   */
   QString mv_nr;
   mv_nr.setNum(mv_counter+1);

   win->getBoard()->unsetCursor();
   get_win()->getInterfaceHandler()->passButton->setEnabled(true);
   get_win()->getInterfaceHandler()->undoButton->setEnabled(true);
   
   if (computer_answer == "resign")
   	{
	win->getInterfaceHandler()->displayComment((myColorIsBlack ? "White resigned" : "Black resigned"));
	win->getBoard()->getBoardHandler()->getGameData()->result = (myColorIsBlack ? "B+R" : "W+R");
	slot_DoneComputer();
	return ;
	}
   
   
   set_move(c==stoneBlack ? stoneWhite : stoneBlack , computer_answer, mv_nr);

   //the computer just played. Are we after 2 passes moves ?
 
   if ((win->getBoard()->getBoardHandler()->getTree()->getCurrent()->isPassMove())&&(win->getBoard()->getBoardHandler()->getTree()->getCurrent()->parent->isPassMove()))
      {
       emit signal_2passes(0,0);
       return;
       }

   // trick : if we have the computer play against himself, we recurse ...
   if (win->blackPlayerType ==   win->whitePlayerType)
      playComputer( c==stoneBlack ? stoneWhite : stoneBlack);
   
}

void qGoBoard::slot_DoneComputer()
{
   win->getInterfaceHandler()->passButton->setEnabled(false);
   win->getInterfaceHandler()->commentEdit->append( win->getBoard()->getBoardHandler()->getGameData()->result); 
   win->getInterfaceHandler()->restoreToolbarButtons();
}


void qGoBoard::slot_doPass()
{
  if (gameMode == modeComputer)
    slot_PassComputer(myColorIsBlack ? stoneBlack : stoneWhite);

  else  if (id > 0)
    emit signal_sendcommand("pass", false);
}

void qGoBoard::slot_doResign()
{
	if (gameMode == modeComputer)
    		{
		win->getInterfaceHandler()->displayComment((myColorIsBlack ? "Black resigned" : "White resigned"));
		win->getBoard()->getBoardHandler()->getGameData()->result = (myColorIsBlack ? "W+R" : "B+R");
		slot_DoneComputer();
		}	
	else if (id > 0)
		emit signal_sendcommand("resign", false);
}

void qGoBoard::slot_doUndo()
{
  	if (gameMode == modeComputer)
    		slot_UndoComputer(myColorIsBlack ? stoneBlack : stoneWhite);
    
  	else if (id > 0)
		if (gsName ==IGS)
			emit signal_sendcommand("undoplease", false);
		else
			emit signal_sendcommand("undo", false); 
}

void qGoBoard::slot_doAdjourn()
{
	qDebug("button: adjourn");
	if (id > 0)
		emit signal_sendcommand("adjourn", false);
}

void qGoBoard::slot_doDone()
{
	if(win->getBoard()->getBoardHandler()->getGtp())   //we are now in score mode !
    		slot_DoneComputer();
    
  	else if (id > 0)
		emit signal_sendcommand("done", false);
}

void qGoBoard::slot_doRefresh()
{
	// no refresh for ended or adjourned games
	if (id > 0 && id != 10000)
	{
		if (!have_gameData)
			emit signal_sendcommand("games " + QString::number(id), false);
		else
		{
			set_sentmovescmd(true);
			emit signal_sendcommand("moves " + QString::number(id), false);
//			emit signal_sendcommand("all " + QString::number(id), false);
		}

//		if (ExtendedTeachingGame)
			// get observers
		emit signal_sendcommand("all " + QString::number(id), false);
	}
	else if (id == 10000 && (gd.playerBlack == myName || gd.playerWhite == myName))
	{
		// load stored game
		emit signal_sendcommand("load " + gd.playerWhite + "-" + gd.playerBlack, false);
	}
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

	if (gd.handicap != req_handicap.toInt())
	{
		emit signal_sendcommand("handicap " + req_handicap, false);
		qDebug() << "Requested handicap: " << req_handicap;
	}
	else
		qDebug("Handicap settings ok...");

	if (gd.komi != req_komi.toFloat())
	{
		qDebug("qGoBoard::check_requests() : emit komi");
		emit signal_sendcommand("komi " + req_komi, false);
	}

	// if size != 19 don't care, it's a free game
	if (req_free != noREQ && gd.size == 19 && mv_counter < 1)
	{
		// check if requests are equal to settings or if teaching game
		if ((gd.freegame == FREE && req_free == FREE) ||
		    (gd.freegame != FREE && req_free == RATED) ||
		    req_free == TEACHING)
		    return;

		switch (gsName)
		{
			case NNGS:
				if (gd.freegame == FREE)
					emit signal_sendcommand("unfree", false);
				else
					emit signal_sendcommand("free", false);
				break;

			// default: toggle
			default:
				emit signal_sendcommand("free", false);
				break;
		}
	}
else
qDebug("Rated game settings ok...");
}

// click on time field
void qGoBoard::slot_addtimePauseB()
{
	if (myColorIsBlack)
	{

		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					emit signal_sendcommand("unpause", false);
				else
					emit signal_sendcommand("pause", false);
				break;
		}
	}
	else
		emit signal_sendcommand("addtime 1", false);
}

// click on time field
void qGoBoard::slot_addtimePauseW()
{
	if (!myColorIsBlack)
	{
		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					emit signal_sendcommand("unpause", false);
				else
					emit signal_sendcommand("pause", false);
				break;
		}
	}
	else
		emit signal_sendcommand("addtime 1", false);
}

// for several reasons: let me know which is my color
void qGoBoard::set_myColorIsBlack(bool b)
{
	myColorIsBlack = b;

	// set correct tooltips
	QToolTip::remove(win->getInterfaceHandler()->normalTools->pb_timeBlack);
	QToolTip::remove(win->getInterfaceHandler()->normalTools->pb_timeWhite);

	if (b)
	{
		switch (gsName)
		{
			case IGS:
				QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeBlack, tr("remaining time / stones"));
				break;

			default:
				QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeBlack, tr("click to pause/unpause the game"));
				break;
		}

		QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeWhite, tr("click to add 1 minute to your opponent's clock"));
	}
	else
	{
		switch (gsName)
		{
			case IGS:
				QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeBlack, tr("remaining time / stones"));
				break;

			default:
				QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeWhite, tr("click to pause/unpause the game"));
				break;
		}

		QToolTip::add(win->getInterfaceHandler()->normalTools->pb_timeBlack, tr("click to add 1 minute to your opponent's clock"));
	}
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
	win->getMainWidget()->pb_controls->setEnabled(IamTeacher && havePupil);

	if (IamTeacher)
	{
		if (opponent == tr("-- none --"))
			emit signal_sendcommand("kibitz " + QString::number(id) + " #OP:0", false);
		else
			emit signal_sendcommand("kibitz " + QString::number(id) + " #OP:" + opponent, false);
		// set teacher's color -> but: teacher can play both colors anyway
		set_myColorIsBlack(mv_counter % 2 + 1);
	}
	else
	{
		bool found = false;
		// look for correct item
		QComboBox *cb = win->getMainWidget()->cb_opponent;
		int cnt = cb->count();
		for (int i = 0; i < cnt && !found; i++)
		{
			cb->setCurrentItem(i);
			if (cb->currentText() == opponent)
			{
				cb->setCurrentItem(i);
				found = true;
			}
		}

		if (!found)
		{
			cb->insertItem(opponent, cnt);
			cb->setCurrentItem(cnt);
		}

		// check if it's me
		QString opp = opponent.section(' ', 0, 0);
		if (opp == myName)
		{
			IamPupil = true;
			set_Mode_real(modeMatch);
			win->getBoard()->set_myColorIsBlack(mv_counter % 2);
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
		if (haveControls && !on || !haveControls && on)
		{
			// toggled by command
		}
		else
		{
			if (on)
				emit signal_sendcommand("kibitz " + QString::number(id) + " #TC:ON", false);
			else
				emit signal_sendcommand("kibitz " + QString::number(id) + " #TC:OFF", false);

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
		if (!haveControls && !on || haveControls && on)
			// toggled by command
			return;

		emit signal_sendcommand("kibitz " + QString::number(id) + " #OC:OFF", false);
		win->getMainWidget()->pb_controls->setDisabled(true);
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
				emit signal_sendcommand("mark", false);
			else
				emit signal_sendcommand("undo", false);
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
					emit signal_sendcommand("undo", false);
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
				emit signal_sendcommand("undo " + QString::number(mv_counter - mark_counter + 1), false);
				mark_counter = 0;
			}
			break;
	}
}

