/*
* boardhandler.cpp
*/

#include "qgo.h"
#include "boardhandler.h"
#include "stonehandler.h"
#include "stone.h"
#include "board.h"
#include "sgfparser.h"
#include "move.h"
#include "interfacehandler.h"
#include "matrix.h"
#include "textview.h"
#include "setting.h"
#include "qnewgamedlg.h"
#include <qapplication.h>
#include <qclipboard.h> 
#include <qlabel.h>
#include <qmessagebox.h>
#include <q3ptrstack.h>

#define MARK_TERRITORY_VISITED     99
#define MARK_TERRITORY_DAME       999
#define MARK_SEKI                  10

class Setting;

BoardHandler::BoardHandler(Board *b)
: board(b)
{
	CHECK_PTR(board);
	
	// Create a StoneHandler instance
	stoneHandler = new StoneHandler(this);
	CHECK_PTR(stoneHandler);
	
	// Create a SGFParser instance
	sgfParser = new SGFParser(this);
	CHECK_PTR(sgfParser);
	
	// Create a variation tree
	tree = new Tree(board->getBoardSize());
	CHECK_PTR(tree);
	
	currentMove = 0;
	lastValidMove = NULL;
	gameMode = modeNormal;
	markType = markNone;
	capturesBlack = capturesWhite = 0;
	markedDead = false;
	
	// Set game data to default
	gameData = new GameData();
	
	clipboardNode = NULL;
	nodeResults = NULL;
	gtp = NULL;

  	// Assume we display events (incoming move when observing)
  	display_incoming_move = true ;

	local_stone_sound = setting->readBoolEntry("SOUND_STONE");
}

BoardHandler::~BoardHandler()
{
	delete stoneHandler;
	delete sgfParser;
	if (clipboardNode != NULL)
		Tree::traverseClear(clipboardNode);
	delete tree;
//	delete gameData;
	if (nodeResults)
		delete nodeResults;
	if (gtp)
		delete gtp;
}

void BoardHandler::clearData()
{
	tree->init(board->getBoardSize());
	lastValidMove = NULL;
	currentMove = 0;
	gameMode = modeNormal;
	markType = markNone;
	stoneHandler->clearData();
	board->getInterfaceHandler()->clearData();
	capturesBlack = capturesWhite = 0;
	markedDead = false;
	if (nodeResults != NULL)
		nodeResults->clear();
}

void BoardHandler::prepareBoard()
{
	// Clear up trash and reset the board to move 0
	stoneHandler->clearData();
	currentMove = 0;
	tree->setToFirstMove();
	if (tree->getCurrent() == NULL)
		qFatal("   *** Oops! Bad things happened reading the sgf file! ***");
	
	gameMode = modeNormal;
	markType = markNone;
	board->hideAllMarks();
	updateMove(tree->getCurrent());
	board->getInterfaceHandler()->setSliderMax(tree->mainBranchSize()-1);
	lastValidMove = NULL;
}

void BoardHandler::prepareComputerBoard() //added eb 12
{
  gameMode = modeComputer;
  tree->setCurrent(tree->findLastMoveInMainBranch()); 
	updateMove(tree->getCurrent());
	board->getInterfaceHandler()->setSliderMax(tree->mainBranchSize()-1);
	lastValidMove = tree->getCurrent();
  board->getInterfaceHandler()->disableToolbarButtons();
  stoneHandler->checkAllPositions();
}                                         //end add eb 12


void BoardHandler::setMode(GameMode mode)
{
	// Leaving score mode
	if (gameMode == modeScore && mode != modeScore)
		leaveScoreMode();
	
	gameMode = mode;
}

void BoardHandler::initGame(GameData *d, bool sgf)
{
	CHECK_PTR(d);
	
//	delete gameData;
	gameData = new GameData(d);
	
	// We have handicap? Then add the necessary stones.
	// Dont do this when reading sgfs, as they add those stones
	// with AB.
	if (d->handicap > 0 && !sgf)
	{
		setHandicap(d->handicap);
		stoneHandler->checkAllPositions();
	}
	
	board->getInterfaceHandler()->normalTools->komi->setText(QString::number(d->komi));
	board->getInterfaceHandler()->normalTools->handicap->setText(QString::number(d->handicap));
	
	if (d->byoTime >= 0 && d->byoTime < 10000)
	{
		if (gameMode == modeNormal)
		{
			if (d->timeSystem == canadian)
				board->getInterfaceHandler()->normalTools->byoyomi->setText(QString::number(d->byoTime/60) + "/" + QString::number(d->byoStones));
			else if (d->timeSystem == byoyomi)
				board->getInterfaceHandler()->normalTools->byoyomi->setText(QString::number(d->byoPeriods) + "x" + QString::number(d->byoTime) + "s");
			else
				board->getInterfaceHandler()->normalTools->byoyomi->setText("0");
		}
		else
			board->getInterfaceHandler()->normalTools->byoyomi->setText(QString::number(d->byoTime));
	}
	else
	{
		board->getInterfaceHandler()->normalTools->byoyomi->setText("-");
		board->getInterfaceHandler()->normalTools->TextLabel_free->setText("-");
		return;
	}
	
	if (d->freegame == FREE)
		board->getInterfaceHandler()->normalTools->TextLabel_free->setText(QApplication::tr("free"));
	else if (d->freegame == RATED)
		board->getInterfaceHandler()->normalTools->TextLabel_free->setText(QApplication::tr("rated"));
	else
		board->getInterfaceHandler()->normalTools->TextLabel_free->setText(QApplication::tr("teach"));
}

int BoardHandler::hasStone(int x, int y)
{
	return stoneHandler->hasStone(x, y);
}

void BoardHandler::addStone(StoneColor c, int x, int y, bool sound)
{
	bool shown = false;
	
	// TODO DEBUG
#ifndef NO_DEBUG
	if (gameMode == modeScore)
		qWarning("addStone in score mode ???? - Something went wrong.");
#endif
	
 	if ((hasStone(x, y) == 1) && display_incoming_move)
	{
		// In edit mode, clicking on a present stone hides it.
		if (gameMode == modeEdit)
		{
			removeStone(x, y, true); // hide the stone
			board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), getNumBrothers(),
				getNumSons(), hasParent(), hasPrevBrother(),
				hasNextBrother());
			board->updateCanvas();
		}
		
		return;
	}

	Move *mr = tree->getCurrent(),          //added eb 8
	*remember = mr;  // Remember current when we are browsing through an observing game
	CHECK_PTR(mr);


	bool do_not_show = false;
	if (tree->getCurrent() != NULL &&  lastValidMove != NULL &&
		tree->getCurrent() != lastValidMove &&  gameMode == modeObserve)
	{
		do_not_show=true;
		tree->setCurrent(lastValidMove);
		currentMove = lastValidMove->getMoveNumber();
	}                                                                      //end add eb 8

	if (display_incoming_move == do_not_show) //SL added eb 9
		qDebug ("Pb : we are observing a game - display_incoming_move and do_not_show disagree");

  
	if ((tree->getCurrent() != NULL && lastValidMove != NULL && tree->getCurrent() != lastValidMove) ||
		(gameMode == modeNormal && tree->getCurrent()->getGameMode() == modeEdit) ||
		lastValidMove == NULL)
	{
		stoneHandler->checkAllPositions();
	}
	
	if (x < 1 || x > board->getBoardSize() || y < 1 || y > board->getBoardSize())
		qWarning("BoardHandler::addStone() - Invalid position: %d/%d", x, y);

	Stone *s = board->addStoneSprite(c, x, y, shown);
	if (s == NULL)
		  return;
	CHECK_PTR(s);

	// qDebug("Game Mode = %s", gameMode == modeNormal ? "NORMAL" : "EDIT");
	
	// Remember captures from move before adding the stone
	capturesBlack = tree->getCurrent()->getCapturesBlack();
	capturesWhite = tree->getCurrent()->getCapturesWhite();
	
	// Normal mode, increase move counter and add the stone as new node
	if (gameMode == modeNormal || gameMode == modeObserve || 
		gameMode == modeMatch || gameMode == modeTeach || gameMode == modeComputer)
	{
		currentMove++;
		if (setting->readIntEntry("VAR_GHOSTS"))
			board->removeGhosts();
		addMove(c, x, y);
		if (sound && local_stone_sound)
			setting->qgo->playClick();

	if (!do_not_show)                       
		board->updateLastMove(c,x,y);       
	}
	// Edit mode...
	else if (gameMode == modeEdit)
	{
		// ...we are currently in a normal mode node, so add the edited version as variation,
		// but dont remove the marks.
		// If this is our root move in an -empty- tree, dont add a node, then we edit root.
		if (tree->getCurrent()->getGameMode() == modeNormal &&      // Its normal mode?
			!(tree->getCurrent() == tree->getRoot() &&              // Its root?
			tree->count() == 1))			            // Its an empty tree?
			addMove(c,x,y, false);             //SL add eb 8
		// ...we are currently already in a node that was created in edit mode, so continue
		// editing this node and dont add a new variation.
		// If its root in an empty tree, and if its the first editing move, change move data. Else do nothing.
		else if (currentMove == 0 && tree->getCurrent()->getGameMode() != modeEdit)
		{
			tree->getCurrent()->setGameMode(modeEdit);
			editMove(c,x,y);                  //SL add eb 8
		}
	}
	
	// If we are in edit mode, dont check for captures (sgf defines)
	// qDebug("CAP BLACK %d, CAP WHITE %d", capturesBlack, capturesWhite);
	stoneHandler->toggleWorking(true);

	updateCurrentMatrix(c, x, y); //SL added eb 8 -> moved here because we nned an updated matrix to check liberties in 'addstone'
  
//	if (!stoneHandler->addStone(s, !shown, gameMode == modeNormal || gameMode == modeObserve || gameMode == modeMatch || gameMode == modeTeach) &&
/*
 * It might be advisable to process a CHECK_PTR on tree->getcurrent-> getmatrix
 * because we don't do it afterwards in addstone and beyond because it is so expensive ...
 */

	// We want to check wether there was a stone of the same color 2 moves before
	// This check will be used at the next step
	bool koStone = false;	
	if (currentMove > 1)
		koStone = ((StoneColor)tree->getCurrent()->parent->parent->getMatrix()->at(x-1, y-1) == c);
			


	if (!stoneHandler->addStone(s, !shown, gameMode == modeNormal || gameMode == modeObserve  || gameMode == modeMatch || gameMode == modeTeach || gameMode == modeComputer, tree->getCurrent()->getMatrix(), koStone) && //SL add eb 8
		(gameMode == modeNormal || gameMode == modeObserve || gameMode == modeMatch || gameMode == modeTeach || gameMode == modeComputer))
	{
		// Suicide move
		QApplication::beep();
		deleteNode();
		stoneHandler->toggleWorking(false);
		return;
	} 
	stoneHandler->toggleWorking(false);
	//updateCurrentMatrix(c, x, y);
	
	// Update captures
	tree->getCurrent()->setCaptures(capturesBlack, capturesWhite);
	/* qDebug("BoardHandler::addStone - setting captures to B %d, W %d",
	tree->getCurrent()->getCapturesBlack(), tree->getCurrent()->getCapturesWhite()); */

	if (do_not_show)                 //added eb 8
	{
		tree->setCurrent(remember);  
		//s->hide();
		stoneHandler->updateAll(remember->getMatrix());//SL added eb 10
	}                               //end add eb 8
  
	// Display data in GUI
	board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), getNumBrothers(), getNumSons(),
		hasParent(), hasPrevBrother(), hasNextBrother(),
		gameMode == modeNormal || gameMode == modeObserve || gameMode == modeMatch || gameMode == modeTeach || gameMode == modeComputer ? x : 0,
		gameMode == modeNormal || gameMode == modeObserve || gameMode == modeMatch || gameMode == modeTeach || gameMode == modeComputer ? y : 0);
		
	if (gameMode == modeNormal)
		board->getInterfaceHandler()->clearComment();
	board->getInterfaceHandler()->setCaptures(capturesBlack, capturesWhite);


   	
	board->updateCanvas();
	board->setModified();
}

void BoardHandler::addStoneSGF(StoneColor c, int x, int y, bool new_node)
{
	bool shown = false;
	
	/* qDebug("BoardHandler::addStoneSGF(StoneColor c, int x, int y) - %d %d/%d %d",
	c, x, y, gameMode); */
	
	if (hasStone(x, y) == 1)
	{
		// In edit mode, this overwrites an existing stone with another color.
		// This is different to the normal interface, when reading sgf files.
		if (gameMode == modeEdit &&
			tree->getCurrent() != NULL &&
			tree->getCurrent()->getMatrix()->at(x-1, y-1) != c)
		{
			if (!stoneHandler->removeStone(x, y, true))
				qWarning("   *** BoardHandler::addStoneSGF() Failed to remove stone! *** ");
			// updateCurrentMatrix(stoneNone, x, y);
		}
	}
	
	if ((tree->getCurrent()->parent != NULL && lastValidMove != NULL &&
		gameMode == modeNormal &&
		tree->getCurrent()->parent != lastValidMove) ||
		(gameMode == modeNormal &&
		tree->getCurrent()->parent->getGameMode() == modeEdit) ||
		(gameData->handicap > 0 && currentMove == 1) ||
		lastValidMove == NULL)
		stoneHandler->checkAllPositions();
	
	if ((x < 1 || x > board->getBoardSize() || y < 1 || y > board->getBoardSize()) && x != 20 && y != 20)
		qWarning("BoardHandler::addStoneSGF() - Invalid position: %d/%d", x, y);
	Stone *s = board->addStoneSprite(c, x, y, shown);
	if (s == NULL)
		return;
	
	// qDebug("Game Mode = %s", gameMode == modeNormal ? "NORMAL" : "EDIT");
	
	// Remember captures from move before adding the stone
	if (tree->getCurrent()->parent != NULL)
	{
		capturesBlack = tree->getCurrent()->parent->getCapturesBlack();
		capturesWhite = tree->getCurrent()->parent->getCapturesWhite();
	}
	else
		capturesBlack = capturesWhite = 0;
	
	if (gameMode == modeNormal || gameMode == modeObserve || gameMode == modeTeach)
	{
		currentMove++;
		// This is a hack to force the first  move to be #1. For example, sgf2misc starts with move 0.
		if (currentMove == 1)
			tree->getCurrent()->setMoveNumber(currentMove);
	}
	else if(gameMode == modeEdit)
		tree->getCurrent()->setMoveNumber(0);
	if (new_node)
	{
		// Set move data
		editMove(s->getColor(), s->posX(), s->posY());
		
		// Update move game mode
		if (gameMode != tree->getCurrent()->getGameMode())
			tree->getCurrent()->setGameMode(gameMode);
	}
	
	// If we are in edit mode, dont check for captures (sgf defines)
	stoneHandler->toggleWorking(true);
	stoneHandler->addStone(s, !shown, gameMode == modeNormal, NULL);
	stoneHandler->toggleWorking(false);
	updateCurrentMatrix(c, x, y);
	// Update captures
	tree->getCurrent()->setCaptures(capturesBlack, capturesWhite);
	lastValidMove = tree->getCurrent();
}

bool BoardHandler::removeStone(int x, int y, bool hide, bool new_node)
{
	// qDebug("BoardHandler::removeStone(int x, int y, bool hide)");
	
	// TODO: DEBUG
#ifndef NO_DEBUG
	if (gameMode != modeEdit)
		qFatal("OOPS");
#endif
	
	bool res = stoneHandler->removeStone(x, y, hide);
	
	if (res)
	{
		if (tree->getCurrent()->getGameMode() == modeNormal &&
			currentMove > 0)
		{
			if (new_node)  // false, when reading sgf
				addMove(stoneNone, x, y);
			updateCurrentMatrix(stoneErase, x, y);
		}
		else
			updateCurrentMatrix(stoneErase, x, y);
		
		board->checkLastMoveMark(x, y);
	}
	
	board->setModified();
	
	return res;
}

void BoardHandler::removeDeadStone(int x, int y)
{
	stoneHandler->removeStone(x, y, true);
	updateCurrentMatrix(stoneNone, x, y);
}

void BoardHandler::addMove(StoneColor c, int x, int y, bool clearMarks)
{
	// qDebug("BoardHandler::addMove - clearMarks = %d", clearMarks);
	
 	Matrix *mat = tree->getCurrent()->getMatrix();
	CHECK_PTR(mat);
	
	Move *m = new Move(c, x, y, currentMove, gameMode, *mat);       
	CHECK_PTR(m);
	
	if (tree->hasSon(m))
	{
		// qDebug("*** HAVE THIS SON ALREADY! ***");
		delete m;
		return;
	}
	
	// Remove all marks from this new move. We dont do that when creating
	// a new variation in edit mode.
	if (clearMarks)
	{
		m->getMatrix()->clearAllMarks();
		board->hideAllMarks();
	}
	
	if (tree->addSon(m) && setting->readIntEntry("VAR_GHOSTS") && getNumBrothers())
		updateVariationGhosts();
	lastValidMove = m;
}

void BoardHandler::createMoveSGF(GameMode mode, bool brother)
{
	// qDebug("BoardHandler::createMoveSGF() - %d", mode);
	
	Move *m;
	
	Matrix *mat = tree->getCurrent()->getMatrix();
	m = new Move(stoneBlack, -1, -1, tree->getCurrent()->getMoveNumber()+1, mode, *mat);
	
	if (!brother && tree->hasSon(m))
	{
		// qDebug("*** HAVE THIS SON ALREADY! ***");
		delete m;
		return;
	}
	
	if (mode == modeNormal)
		m->getMatrix()->clearAllMarks();
	
	if (!brother)
		tree->addSon(m);
	else
		tree->addBrother(m);
}

void BoardHandler::editMove(StoneColor c, int x, int y)
{
	// qDebug("BoardHandler::editMove");
	
	if ((x < 1 || x > board->getBoardSize() || y < 1 || y > board->getBoardSize()) && x != 20 && y != 20)
		return;
	
	Move *m = tree->getCurrent();
	CHECK_PTR(m);
	
	m->setX(x);
	m->setY(y);
	m->setColor(c);
	
	if (m->getMatrix() == NULL)
		qFatal("   *** BoardHandler::editMove() - Current matrix is NULL! ***");
	// if (x != 20 && y != 20)
	// m->getMatrix()->insertStone(x, y, c);
}

void BoardHandler::updateMove(Move *m, bool ignore_update)
{
	if (m == NULL)
	{
		m = tree->getCurrent();
	}
	//qDebug("BoardHandler::updateMove(Move *m)");

	CHECK_PTR(m);
	currentMove = m->getMoveNumber();
	int brothers = getNumBrothers();
	
	// Display move data and comment in the GUI
	if (m->getGameMode() == modeNormal)
		board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), brothers, getNumSons(),
		hasParent(), hasPrevBrother(), hasNextBrother(),
		m->getX(), m->getY());
	else
		board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), brothers, getNumSons(),
		hasParent(), hasPrevBrother(), hasNextBrother());
	if (board->get_isLocalGame())
		board->getInterfaceHandler()->displayComment(m->getComment());        // Update comment
	board->getInterfaceHandler()->setSliderMax(currentMove + tree->getBranchLength());  // Update slider branch length

  
	// Get rid of the varation ghosts
	if (setting->readIntEntry("VAR_GHOSTS"))
		board->removeGhosts();
	
	// Get rid of all marks except the last-move-mark
	board->hideAllMarks();
	
	// Remove territory marks
	if (tree->getCurrent()->isTerritoryMarked())
	{
		tree->getCurrent()->getMatrix()->clearTerritoryMarks();
		tree->getCurrent()->setTerritoryMarked(false);
	}
	
	// Unshade dead stones
	if (markedDead)
	{
		stoneHandler->removeDeadMarks();
		markedDead = false;
	}
	
	if (m->getGameMode() == modeNormal || m->getGameMode() == modeObserve )  //SL add eb 8
		// If the node is in normal mode, show the circle to mark the last move
		    board->updateLastMove(m->getColor(), m->getX(), m->getY());
	else                                                                      
		// ... if node is in edit mode, just delete that circle
	{
		board->removeLastMoveMark();
		board->setCurStoneColor();
	}
	
	// Update the ghosts indicating variations
	if (brothers && setting->readIntEntry("VAR_GHOSTS"))
		updateVariationGhosts();
	
	// Oops, something serious went wrong
	if (m->getMatrix() == NULL)
		qFatal("   *** Move returns NULL pointer for matrix! ***");
	
	// Synchronize the board with the current nodes matrix, provided we want to 
  if (!ignore_update)                //SL added eb 9 - this if we are browsing an observing game and an undo incomes
  	stoneHandler->updateAll(m->getMatrix());
	
	// Display captures or score in the GUI
	if (m->isScored())  // This move has been scored
		board->getInterfaceHandler()->setCaptures(m->getScoreBlack(), m->getScoreWhite(), true);
	else
		board->getInterfaceHandler()->setCaptures(m->getCapturesBlack(), m->getCapturesWhite());

	// Display times
	if (currentMove == 0)
	{
		if (gameData->timelimit == 0)
			board->getInterfaceHandler()->setTimes("00:00", "-1", "00:00", "-1");
		else
		{
			// set Black's and White's time to timelimit
			board->getInterfaceHandler()->setTimes(true, gameData->timelimit, -1);
			board->getInterfaceHandler()->setTimes(false, gameData->timelimit, -1);
		}
	}
	else if (m->getTimeinfo())
		board->getInterfaceHandler()->setTimes(!getBlackTurn(), m->getTimeLeft(), m->getOpenMoves() == 0 ? -1 : m->getOpenMoves());

	board->updateCanvas();
}

void BoardHandler::updateVariationGhosts()
{
	// qDebug("BoardHandler::updateVariationGhosts()");
	
	Move *m = tree->getCurrent()->parent->son;
	CHECK_PTR(m);
	
	do {
		if (m == tree->getCurrent())
			continue;
		board->setVarGhost(m->getColor(), m->getX(), m->getY());
	} while ((m = m->brother) != NULL);
}

bool BoardHandler::nextMove(bool autoplay)
{
	if (gameMode == modeScore)
		return false;
	
	CHECK_PTR(tree);
	
	Move *m = tree->nextMove();
	if (m == NULL)
		return false;
	
	if (autoplay)
		setting->qgo->playAutoPlayClick();
	
	updateMove(m);
	return true;
}

void BoardHandler::previousMove()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	Move *m = tree->previousMove();
	if (m != NULL)
		updateMove(m);
}

void BoardHandler::previousComment()            //added eb
{
	 if (gameMode == modeScore)
		return;

	//ASSERT(n >= 0);
	CHECK_PTR(tree);

	Move  *m = tree->getCurrent()->parent ;

	CHECK_PTR(m);

	while (m != NULL)
	{
		if (m->getComment() != "")
			break;
		if (m->parent == NULL)
			break;
		m = m->parent;
	}

	if (m != NULL)        
		gotoMove(m);
}

void BoardHandler::nextComment()
{
	if (gameMode == modeScore)
		return;

	//ASSERT(n >= 0);
	CHECK_PTR(tree);

	Move  *m = tree->getCurrent()->son ;
             
	CHECK_PTR(m);

	while (m != NULL)
	{
		if (m->getComment() != "" )
			break;
		if (m->son == NULL)
			break;
		m = m->son;
	}

	if (m != NULL)
		gotoMove(m);
}                                               //end add eb

void BoardHandler::nextVariation()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	Move *m = tree->nextVariation();
	if (m == NULL)
		return;
	
	updateMove(m);
}

void BoardHandler::previousVariation()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	Move *m = tree->previousVariation();
	if (m == NULL)
		return;
	
	updateMove(m);
}

void BoardHandler::gotoFirstMove()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
#if 0
	// Set tree to root
	tree->setToFirstMove();
	Move *m = tree->getCurrent();
	updateMove(m);
#else
	// We need to set the markers. So go the tree upwards
	Move *m = tree->getCurrent();
	CHECK_PTR(m);
	
	// Ascent tree until root reached
	while (m->parent != NULL)
		m = tree->previousMove();
	
	tree->setToFirstMove();  // Set move to root
	m = tree->getCurrent();
	updateMove(m);
#endif
}

void BoardHandler::gotoLastMove()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent();
	CHECK_PTR(m);
	
	// Descent tree to last son of main variation
	while (m->son != NULL)
		m = tree->nextMove();
	
	if (m != NULL)
		updateMove(m);
}

// this slot is used for edit window to navigate to last made move
void BoardHandler::gotoLastMoveByTime()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	tree->setToFirstMove();
	Move *m = tree->getCurrent();
	CHECK_PTR(m);
	
	// Descent tree to last son of latest variation
	while (m->son != NULL)
	{
		m = tree->nextMove();
		for (int i = 0; i < tree->getNumBrothers(); i++)
			m = tree->nextVariation();
/*		Move *n = m;
		while (n != NULL && (n = n->brother) != NULL)
		{
			m = n;
			if (n->getTimeLeft() <= m->getTimeLeft() ||
				n->getTimeLeft() != 0 && m->getTimeLeft() == 0)
				m = n;
		}*/
	}
	
	if (m != NULL)
		updateMove(m);
}

void BoardHandler::gotoMainBranch()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	if (tree->getCurrent()->parent == NULL)
		return;
	
	Move *m = tree->getCurrent(),
		*old = m,
		*lastOddNode = NULL;
	
	if (m == NULL)
		return;
	
	while ((m = m->parent) != NULL)
	{
		if (tree->getNumSons(m) > 1 && old != m->son)  // Remember a node when we came from a branch
			lastOddNode = m;
		m->marker = old;
		old = m;
	}
	
	if (lastOddNode == NULL)
		return;
	
	CHECK_PTR(lastOddNode);
	lastOddNode->marker = NULL;  // Clear the marker, so we can proceed in the main branch
	tree->setCurrent(lastOddNode);
	updateMove(lastOddNode);
}

void BoardHandler::gotoVarStart()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	if (tree->getCurrent()->parent == NULL)
		return;
	
	Move *tmp = tree->previousMove(),
		*m = NULL;
	
	if (tmp == NULL)
		return;
	
	// Go up tree until we find a node that has > 1 sons
	while ((m = tree->previousMove()) != NULL && tree->getNumSons() <= 1)
	tmp = m;  /* Remember move+1, as we set current to the
	* first move after the start of the variation */
	
	if (m == NULL)  // No such node found, so we reached root.
	{
		tmp = tree->getRoot();
		// For convinience, if we have Move 1, go there. Looks better.
		if (tmp->son != NULL)
			tmp = tree->nextMove();
	}
	
	// If found, set current to the first move inside the variation
	CHECK_PTR(tmp);
	tree->setCurrent(tmp);
	updateMove(tmp);
}

void BoardHandler::gotoNextBranch()
{
	if (gameMode == modeScore)
		return;
	
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*remember = m;  // Remember current in case we dont find a branch
	CHECK_PTR(m);
	
	// We are already on a node with 2 or more sons?
	if (tree->getNumSons() > 1)
	{
		m = tree->nextMove();
		updateMove(m);
		return;
	}
	
	// Descent tree to last son of main variation
	while (m->son != NULL && tree->getNumSons() <= 1)
		m = tree->nextMove();
	
	if (m != NULL && m != remember)
	{
		if (m->son != NULL)
			m = tree->nextMove();
		updateMove(m);
	}
	else
		tree->setCurrent(remember);
}

void BoardHandler::gotoNthMove(int n)
{
	if (gameMode == modeScore)
		return;
	
	ASSERT(n >= 0);
	CHECK_PTR(tree);
	
	Move *m = tree->getRoot(),
		*old = tree->getCurrent();
	CHECK_PTR(m);
	
	while (m != NULL)
	{
		if (m->getMoveNumber() == n)
			break;
		if (m->son == NULL)
			break;
		m = m->son;
	}
	
	if (m != NULL && m != old)
		gotoMove(m);
}

void BoardHandler::gotoNthMoveInVar(int n)
{
	if (gameMode == modeScore)
		return;
	
	ASSERT(n >= 0);
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*old = m;
	CHECK_PTR(m);
	
	while (m != NULL)
	{
		if (m->getMoveNumber() == n)
			break;
		if ((n >= currentMove && m->son == NULL && m->marker == NULL) ||
			(n < currentMove && m->parent == NULL))
			break;
		if (n > currentMove)
		{
			if (m->marker == NULL)
				m = m->son;
			else
				m = m->marker;
			m->parent->marker = m;
		}	    
		else
		{
			m->parent->marker = m;
			m = m->parent;
		}
	}
	
	if (m != NULL && m != old)
		gotoMove(m);
}

void BoardHandler::deleteNode()
{
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*remember = NULL,
		*remSon = NULL;
	CHECK_PTR(m);
	
	if (m->parent != NULL)
	{
		remember = m->parent;
		
		// Remember son of parent if its not the move to be deleted.
		// Then check for the brothers and fix the pointer connections, if we
		// delete a node with brothers. (It gets ugly now...)
		// YUCK! I hope this works.
		if (remember->son == m)                  // This son is our move to be deleted?
		{
			if (remember->son->brother != NULL)  // This son has a brother?
				remSon = remember->son->brother; // Reset pointer
		}
		else                                     // No, the son is not our move
		{
			remSon = remember->son;
			Move *tmp = remSon, *oldTmp = tmp;
			
			do {   // Loop through all brothers until we find our move
				if (tmp == m)
				{
					if (m->brother != NULL)            // Our move has a brother?
						oldTmp->brother = m->brother;  // Then set the previous move brother
					else                               // to brother of our move
						oldTmp->brother = NULL;        // No brother found.
					break;
				}
				oldTmp = tmp;
			} while ((tmp = tmp->brother) != NULL);
		}
	}
	else if (tree->hasPrevBrother())
	{
		remember = tree->previousVariation();
		if (m->brother != NULL)
			remember->brother = m->brother;
		else
			remember->brother = NULL;
	}
	else if (tree->hasNextBrother())
	{
		remember = tree->nextVariation();
		// Urgs, remember is now root.
		tree->setRoot(remember);
	}
	else
	{
		// Oops, first and only move. We delete everything
		tree->init(board->getBoardSize());
		board->hideAllStones();
		board->hideAllMarks();
		board->updateCanvas();
		lastValidMove = NULL;
		stoneHandler->clearData();
		updateMove(tree->getCurrent());
		return;
	}
	
	if (m->son != NULL)
		Tree::traverseClear(m->son);  // Traverse the tree after our move (to avoid brothers)
	delete m;                         // Delete our move
	tree->setCurrent(remember);       // Set current move to previous move
	remember->son = remSon;           // Reset son pointer
	remember->marker = NULL;          // Forget marker
	
	updateMove(tree->getCurrent(), !display_incoming_move);
	
	board->setModified();
}

void BoardHandler::createNode(const Matrix &mat, bool brother)
{
	Move *m = new Move(stoneNone, -1, -1, currentMove, modeEdit, mat);
	CHECK_PTR(m);

	// Remove all marks from this new move.
	board->hideAllMarks();
	board->removeLastMoveMark();
	
	bool res = false;
	if (brother)
		res = tree->addBrother(m);
	else
		res = tree->addSon(m);
	
	if (res && setting->readIntEntry("VAR_GHOSTS") && getNumBrothers())
		updateVariationGhosts();
	lastValidMove = m;
	
	int brothers = getNumBrothers();
	// Update the ghosts indicating variations
	if (brothers && setting->readIntEntry("VAR_GHOSTS"))
		updateVariationGhosts();
	
	board->getInterfaceHandler()->showEditGroup();
	board->getInterfaceHandler()->clearComment();
	board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), brothers, getNumSons(),
		hasParent(), hasPrevBrother(), hasNextBrother());
	
	stoneHandler->updateAll(tree->getCurrent()->getMatrix());
	board->updateCanvas();
	
	board->setModified();
}

bool BoardHandler::swapVariations()
{
	if (gameMode == modeScore)
		return false;
	
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent(),
		*parent = NULL,
		*prev = NULL,
		*next = NULL,
		*newSon = NULL;
	
	if (!tree->hasPrevBrother())
	{
		qWarning("   *** BoardHandler::swapVariations() - No prev brother. Aborting... ***");
		return false;
	}
	
	parent = m->parent;
	prev = tree->previousVariation();
	next = m->brother;
	
	// Check if our move has further prev-prev nodes. If yes, we don't change parents son
	if (!tree->hasPrevBrother(prev))
		newSon = m;
	else
		tree->previousVariation()->brother = m;
	
	// Do the swap
	m->brother = prev;
	prev->brother = next;
	
	if (newSon != NULL)
	{
		if (parent != NULL)
			parent->son = m;
		else
			tree->setRoot(m);
	}
	
	tree->setCurrent(m);
	updateMove(m);
	
	return true;
}

void BoardHandler::doPass(bool sgf)
{
	if (lastValidMove == NULL)
	{
		stoneHandler->checkAllPositions();
	}

	StoneColor c = getBlackTurn() ? stoneBlack : stoneWhite;
	
	currentMove++;
	if (setting->readIntEntry("VAR_GHOSTS") && !sgf)
		board->removeGhosts();
	
	if (!sgf)
		addMove(c, 20, 20);
	else  // Sgf reading
	{
		if (hasParent())
			c = tree->getCurrent()->parent->getColor() == stoneBlack ? stoneWhite : stoneBlack;
		else
			c = stoneBlack;
		editMove(c, 20, 20);
	}
	
	if (hasParent())
		tree->getCurrent()->setCaptures(tree->getCurrent()->parent->getCapturesBlack(),
		tree->getCurrent()->parent->getCapturesWhite());
	
	if (!sgf)
	{
		board->getInterfaceHandler()->setMoveData(currentMove, getBlackTurn(), getNumBrothers(), getNumSons(),
			hasParent(), hasPrevBrother(), hasNextBrother(), 20, 20);

		if (board->get_isLocalGame())
			board->getInterfaceHandler()->clearComment();
		
		board->updateLastMove(c, 20, 20);
		board->updateCanvas();
	}
	
	board->setModified();
}

#ifndef NO_DEBUG
void BoardHandler::debug()
{
	if (tree->getCurrent()->getMatrix() == NULL)
		qWarning("CURRENT MATRIX IS NULL");
	else
	{
		qDebug("X = %d, Y = %d, MODE = %d",
			tree->getCurrent()->getX(), tree->getCurrent()->getY(), tree->getCurrent()->getGameMode());
		qDebug("CAPTURES Black: %d White: %d",
			tree->getCurrent()->getCapturesBlack(),
			tree->getCurrent()->getCapturesWhite());
		tree->getCurrent()->getMatrix()->debug();
	}
}
#endif

bool BoardHandler::getBlackTurn()
{
	if (tree->getCurrent()->getPLinfo())
		// color of next stone is same as current
		return tree->getCurrent()->getPLnextMove() == stoneBlack;

	if (currentMove == 0)
	{
		// Handicap, so white starts
		if (gameData->handicap >= 2)
			return false;
			
		return true;
	}
	
	// Normal mode
	if (tree->getCurrent()->getGameMode() == modeNormal ||
		tree->getCurrent()->getGameMode() == modeObserve ||
		tree->getCurrent()->getGameMode() == modeMatch ||
		tree->getCurrent()->getGameMode() == modeTeach ||
    		tree->getCurrent()->getGameMode() == modeComputer)
	{
			// change color
			return tree->getCurrent()->getColor() == stoneWhite;
	}
	// Edit mode. Return color of parent move.
	else if (tree->getCurrent()->parent != NULL)
		return tree->getCurrent()->parent->getColor() == stoneWhite;
	// Crap happened. 50% chance this is correct .)
	qWarning("Oops, crap happened in BoardHandler::getBlackTurn() !");
	return true;
}

bool BoardHandler::loadSGF(const QString &fileName)
{
	// Load the sgf file
	if (!sgfParser->parse(fileName))
		return false;
	
	prepareBoard();
	return true;
}

bool BoardHandler::openComputerSession(QNewGameDlg *dlg,const QString &fileName, const QString &computer_path)
{
  // now we start the dialog
	gtp = new QGtp() ;

	if (gtp->openGtpSession(computer_path,dlg->getSize(),dlg->getKomi(),dlg->getHandicap(),dlg->getLevelBlack())==FAIL)
	{
		// if gnugo fails
//		QString mesg = QString(QObject::tr("Error opening program: %1\n")).arg(gtp->getLastMessage());
		QMessageBox msg(QObject::tr("Error"), //mesg,
			QString(QObject::tr("Error opening program: %1")).arg(gtp->getLastMessage()),
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, Qt::NoButton, Qt::NoButton);
		msg.setActiveWindow();
		msg.raise();
		msg.exec();

		return false ;
	}
	//if (dlg->getHandicap())
	// setHandicap(dlg->getHandicap());

	if (!(fileName.isNull() || fileName.isEmpty())) {

		if (!sgfParser->parse(fileName))
			return false;
		else
			gtp->loadsgf(fileName);
	}
	
	prepareComputerBoard();  

	return true;
}                              //end add eb 12

bool BoardHandler::saveBoard(const QString &fileName)
{
	CHECK_PTR(tree);
	return sgfParser->doWrite(fileName, tree);
}

void BoardHandler::updateComment(QString text)
{
	if (tree->getCurrent() == NULL)
		return;
	
	QString comment;

	if (board->get_isLocalGame())
	{
		// local game: comment field is input field containing only string of current node
		if (text.isEmpty())
			comment = board->getInterfaceHandler()->getComment();
		else
			return;
	}
	else
	{
		// online game: comment field shows all kibitzes since the beginning
		if (text.isEmpty())
			return;
		else if (!text.contains('\n'))
		{
			comment = tree->getCurrent()->getComment();
			if (!comment.isEmpty())
				comment += '\n';
			comment += text;
		}
		// show current line
		board->getInterfaceHandler()->displayComment(text);
	}
	
	tree->getCurrent()->setComment(comment);
}

void BoardHandler::editMark(int x, int y, MarkType t, const QString &txt)
{
	CHECK_PTR(tree->getCurrent()->getMatrix());
	
	if (t == markNone)
		tree->getCurrent()->getMatrix()->removeMark(x, y);
	else
		tree->getCurrent()->getMatrix()->insertMark(x, y, t);
	
	if ((t == markText || t == markNumber) && !txt.isNull() && !txt.isEmpty())
		tree->getCurrent()->getMatrix()->setMarkText(x, y, txt);
	
	board->setModified();
}

void BoardHandler::updateCurrentMatrix(StoneColor c, int x, int y)
{
	// Passing?
	if (x == 20 && y == 20)
		return;
	
	if (x < 1 || x > board->getBoardSize() || y < 1 || y > board->getBoardSize())
	{
		qWarning("   *** BoardHandler::updateCurrentMatrix() - Invalid move given: %d/%d at move %d ***",
			x, y, tree->getCurrent()->getMoveNumber());
		return;
	}
	
	CHECK_PTR(tree->getCurrent()->getMatrix());
	
	if (c == stoneNone)
		tree->getCurrent()->getMatrix()->removeStone(x, y);
	else if (c == stoneErase)
		tree->getCurrent()->getMatrix()->eraseStone(x, y);
	else
		tree->getCurrent()->getMatrix()->insertStone(x, y, c, gameMode);
}

void BoardHandler::exportASCII()
{
	TextView dlg(board, "textview", true);
	dlg.setMatrix(tree->getCurrent()->getMatrix(), setting->charset);
	dlg.exec();
}

bool BoardHandler::importASCII(const QString &fileName, bool fromClipboard)
{
#if 0
	QString filenameORstring = fromClipboard ? QApplication::clipboard()->text() : fileName;  
	
	if (gameMode == modeScore)
		return false;
	
	clearNode(setting->addImportAsBrother);
	
	if (!sgfParser->parseASCII(filenameORstring, setting->charset, !fromClipboard))
		return false;
	
	updateMove(tree->getCurrent());
	
	board->setModified();
	
	return true;
#endif
}

bool BoardHandler::importSGFClipboard()
{
	QString sgfString = QApplication::clipboard()->text();
	
	// qDebug("Text in clipboard:\n%s", sgfString.latin1());
	
	if (sgfString.isNull() || sgfString.isEmpty() ||
		!sgfParser->parseString(sgfString))
		return false;
	
	prepareBoard();
	return true;
}

bool BoardHandler::exportSGFtoClipB()
{
	QString str = "";
	if (!sgfParser->exportSGFtoClipB(&str, tree))
		return false;
	QApplication::clipboard()->setText(str);
	return true;
}

void BoardHandler::setHandicap(int handicap)
{
	GameMode store = getGameMode();
	//qDebug("set Handicap " + QString::number(handicap) + ", stored mode == " + QString::number(store));
	setMode(modeEdit);

	int size = getGameData()->size;
	int edge_dist = (size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (size + 1) / 2;
	int high = size + 1 - edge_dist;
	
	if (size > 25 || size < 7)
	{
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
		setMode(store);
		return;
	}
	
	// change: handicap is first move
	if (handicap > 1)
	{
		createNode(*tree->getCurrent()->getMatrix(), false);
		/* Move should already be set to 0 by board handler and the placement
		 * of handicap stones won't change that */
		//currentMove++;
		//tree->getCurrent()->setMoveNumber(1);
	}

	// extra:
	if (size == 19 && handicap > 9)
		switch (handicap)
		{
		case 13:  // Hehe, this is nuts... :)
			addStone(stoneBlack, 17, 17);
		case 12:
			addStone(stoneBlack, 3, 3);
		case 11:
			addStone(stoneBlack, 3, 17);
		case 10:
			addStone(stoneBlack, 17, 3);
			
		default:
			handicap = 9;
			break;
		}
	
	switch (size % 2)
	{
	// odd board size
	case 1:
		switch (handicap)
		{
		case 9:
			addStone(stoneBlack, middle, middle);
		case 8:
		case 7:
			if (handicap >= 8)
			{
				addStone(stoneBlack, middle, low);
				addStone(stoneBlack, middle, high);
			}
			else
				addStone(stoneBlack, middle, middle);
		case 6:
		case 5:
			if (handicap >= 6)
			{
				addStone(stoneBlack, low, middle);
				addStone(stoneBlack, high, middle);
			}
			else
				addStone(stoneBlack, middle, middle);
		case 4:
			addStone(stoneBlack, high, high);
		case 3:
			addStone(stoneBlack, low, low);
		case 2:
			addStone(stoneBlack, high, low);
			addStone(stoneBlack, low, high);
		case 1:
			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	// even board size
	case 0:
		switch (handicap)
		{
		case 4:
			addStone(stoneBlack, high, high);
		case 3:
			addStone(stoneBlack, low, low);
		case 2:
			addStone(stoneBlack, high, low);
			addStone(stoneBlack, low, high);
		case 1:
			if (store != modeObserve && store != modeMatch &&  store != modeTeach)
				gameData->komi = 0.5;
			break;
			
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;
		
	default:
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
				
	}
	
	// Change cursor stone color
	board->setCurStoneColor();
	gameData->handicap = handicap;
	setMode(store);
	board->getInterfaceHandler()->disableToolbarButtons();
}

bool BoardHandler::hasParent()
{
	return tree->getCurrent()->parent != NULL;
}

void BoardHandler::setCaptures(StoneColor c, int n)
{
	if (c == stoneBlack)
		capturesBlack += n;
	else
		capturesWhite += n;
}

void BoardHandler::countScore()
{
	// Switch to score mode
	setMode(modeScore);

	Matrix * current_matrix = tree->getCurrent()->getMatrix();
	current_matrix->clearTerritoryMarks();

	// capturesBlack -= caps_black;
	// capturesWhite -= caps_white;
	capturesBlack = tree->getCurrent()->getCapturesBlack();
	capturesWhite = tree->getCurrent()->getCapturesWhite();

	tree->getCurrent()->setScored(true);

	// Copy the current matrix
	Matrix *m = new Matrix(*current_matrix);
	CHECK_PTR(m);

	// Do some cleanups, we only need stones
	m->absMatrix();
	m->clearAllMarks();

	int caps_white = 0, caps_black = 0;
	int sz = board->getBoardSize();

	// Mark all dead stones in the matrix with negative number
	int i, j;
	for (i = 0; i < sz; i++)
		for (j = 0; j < sz; j++)
			if (stoneHandler->hasStone(i+1, j+1) == 1)
			{
				Stone *s = stoneHandler->getStoneAt(i+1, j+1);
				if (s->isDead()) {
					if (m->at(i, j) == stoneBlack)
						caps_white++;
					else
						caps_black++;
					m->set(i, j, m->at(i, j) * -1);
				}
				else if (s->isSeki())
					m->set(i, j, m->at(i, j) * MARK_SEKI);
			}

	int terrWhite = 0, terrBlack = 0;
			  
	while (m != NULL)
	{
		bool found = false;
					
		for (i = 0; i < sz; i++)
		{
			for (j = 0; j < sz; j++)
			{
				if (m->at(i, j) <= 0)
				{
					found = true;
					break;
				}
			}
			if (found)
				break;
		}
					
		if (!found)
			break;
					
		// Traverse the enclosed territory. Resulting color is in col afterwards
		StoneColor col = stoneNone;
		traverseTerritory(m, i, j, col);
					
		// Now turn the result into real territory or dame points
		for (i = 0; i < sz; i++)
		{
			for (j = 0; j < sz; j++)
			{
				if (m->at(i, j) == MARK_TERRITORY_VISITED)
				{
					// Black territory
					if (col == stoneBlack)
					{
						tree->getCurrent()->getMatrix()->removeMark(i+1, j+1);
						tree->getCurrent()->getMatrix()->insertMark(i+1, j+1, markTerrBlack);
						terrBlack ++;
						m->set(i, j, MARK_TERRITORY_DONE_BLACK);
					}
					// White territory
					else if (col == stoneWhite)
					{
						tree->getCurrent()->getMatrix()->removeMark(i+1, j+1);
						tree->getCurrent()->getMatrix()->insertMark(i+1, j+1, markTerrWhite);
						terrWhite ++;
						m->set(i, j, MARK_TERRITORY_DONE_WHITE);
					}
					// Dame
					else
						m->set(i, j, MARK_TERRITORY_DAME);
				}
			}
		}
	}

	// Finally, remove all false eyes that have been marked as territory. This
	// has to be here, as in the above loop we did not find all dame points yet.
	for (i = 0; i < board->getBoardSize(); i++) {
		for (j = 0; j < board->getBoardSize(); j++) {
			if (m->at(i, j) == MARK_TERRITORY_DONE_BLACK ||
			    m->at(i, j) == MARK_TERRITORY_DONE_WHITE) {
				int col = (m->at(i, j) == MARK_TERRITORY_DONE_BLACK ? stoneBlack : stoneWhite);
				if (stoneHandler->checkFalseEye(m, i, j, col)) {
					tree->getCurrent()->getMatrix()->removeMark(i + 1, j + 1);
					if (col == stoneBlack)
						terrBlack--;
					else
						terrWhite--;
				}
			}
		}
	}

	// Mark the move having territory marks
	tree->getCurrent()->setTerritoryMarked(true);
	// Paint the territory on the board
	stoneHandler->updateAll(tree->getCurrent()->getMatrix());
	board->updateCanvas();
				
	// Update Interface
	board->getInterfaceHandler()->setScore(terrBlack, capturesBlack + caps_black,
					       terrWhite, capturesWhite + caps_white,
					       gameData->komi);

	delete m;

	board->setModified();
}

void BoardHandler::traverseTerritory(Matrix *m, int x, int y, StoneColor &col)
{
	// Mark visited
	m->set(x, y, MARK_TERRITORY_VISITED);
	
	// North
	if (checkNeighbourTerritory(m, x, y-1, col))
		traverseTerritory(m, x, y-1, col);
	
	// East
	if (checkNeighbourTerritory(m, x+1, y, col))
		traverseTerritory(m, x+1, y, col);
	
	// South
	if (checkNeighbourTerritory(m, x, y+1, col))
		traverseTerritory(m, x, y+1, col);
	
	// West
	if (checkNeighbourTerritory(m, x-1, y, col))
		traverseTerritory(m, x-1, y, col);
}

bool BoardHandler::checkNeighbourTerritory(Matrix *m, const int &x, const int &y, StoneColor &col)
{
	// Off the board? Dont continue
	if (x < 0 || x >= board->getBoardSize() || y < 0 || y >= board->getBoardSize())
		return false;
	
	// No stone? Continue
	if (m->at(x, y) <= 0)
		return true;
	
	// A stone, but no color found yet? Then set this color and dont continue
	// The stone must not be marked as alive in seki.
	if (col == stoneNone && m->at(x, y) > 0 && m->at(x, y) < MARK_SEKI)
	{
		col = (StoneColor)m->at(x, y);
		return false;
	}
	
	// A stone, but wrong color? Set abort flag but continue to mark the rest of the dame points
	StoneColor tmpCol = stoneNone;
	if (col == stoneBlack)
		tmpCol = stoneWhite;
	else if (col == stoneWhite)
		tmpCol = stoneBlack;
	if ((tmpCol == stoneBlack || tmpCol == stoneWhite) && m->at(x, y) == tmpCol)
	{
		col = stoneErase;
		return false;
	}
	
	// A stone, correct color, or already visited, or seki. Dont continue
	return false;
}

void BoardHandler::enterScoreMode(int cb, int cw)
{
}

void BoardHandler::leaveScoreMode()
{
	// Remove territory marks
	if (tree->getCurrent()->isTerritoryMarked())
	{
		tree->getCurrent()->getMatrix()->clearTerritoryMarks();
		tree->getCurrent()->setTerritoryMarked(false);
		tree->getCurrent()->setScored(false);
	}
	
	// Unshade dead stones
	stoneHandler->removeDeadMarks();
	markedDead = false;
	
	updateMove(tree->getCurrent());	
}

void BoardHandler::markDeadStone(int x, int y)
{
	int caps = 0;
	StoneColor col;
	bool dead = false;
	
	if (stoneHandler->removeDeadGroup(x, y, caps, col, dead))
	{
		// Mark we have dead stones in this move
		if (dead)
			markedDead = true;
		
		// Add captures for opponent
		if (!dead)
			caps *= -1;
		setCaptures(col == stoneBlack ? stoneWhite : stoneBlack, caps);
		
		countScore();
	}
}

void BoardHandler::markSeki(int x, int y)
{
	int caps = 0;
	StoneColor col;
	bool seki = false;
	
	if (stoneHandler->markSekiGroup(x, y, caps, col, seki))
	{
		// Mark we have seki stones in this move
		if (seki)
			markedDead = true;
		
		// Remove captures if seki brought dead stones back to life
		if (caps)
			setCaptures(col == stoneBlack ? stoneWhite : stoneBlack, -caps);
		
		countScore();
	}
}

void BoardHandler::findMoveByPos(int x, int y)
{
	Move *m = tree->findMoveInMainBranch(x, y);
	
	if (m != NULL)
		gotoMove(m);
	else
		QApplication::beep();
}

void BoardHandler::findMoveByPosInVar(int x, int y)
{
	Move *m = tree->findMoveInBranch(x, y);
	
	if (m != NULL)
		gotoMove(m);
	else
		QApplication::beep();
}

void BoardHandler::gotoMove(Move *m)
{
	CHECK_PTR(m);
	tree->setCurrent(m);
	updateMove(m);
}

bool BoardHandler::findMoveInVar(int x, int y)
{
	// Init stack if not yet done
	if (nodeResults == NULL)
	{
		nodeResults = new Q3PtrStack<Move>;
		nodeResults->setAutoDelete(FALSE);
	}
	
	CHECK_PTR(nodeResults);
	nodeResults->clear();
	tree->traverseFind(tree->getCurrent(), x, y, *nodeResults);
	
	if (nodeResults->isEmpty())
	{
		QApplication::beep();
		return false;
	}
	
	return true;
}

void BoardHandler::numberMoves()
{
	if (gameMode == modeScore)
		return;
	CHECK_PTR(tree);
	
	// Move from current upwards to root and set a number mark
	Move *m = tree->getCurrent();
	if (m == NULL || m->getMoveNumber() == 0)
		return;
	
	do {
		board->setMark(m->getX(), m->getY(), markNumber, true, QString::number(m->getMoveNumber()));
	} while ((m = m->parent) != NULL && m->getMoveNumber() != 0);
	
	board->updateCanvas();
}

void BoardHandler::markVariations(bool sons)
{
	if (gameMode == modeScore)
		return;
	CHECK_PTR(tree);
	
	Move *m = tree->getCurrent();
	if (m == NULL)
		return;
	
	// Mark all sons of current move
	if (sons && m->son != NULL)
	{
		m = m->son;
		do {
			board->setMark(m->getX(), m->getY(), markText);
		} while ((m = m->brother) != NULL);
	}
	// Mark all brothers of current move
	else if (!sons && m->parent != NULL)
	{
		Move *tmp = m->parent->son;
		if (tmp == NULL)
			return;
		
		do {
			if (tmp != m)
				board->setMark(tmp->getX(), tmp->getY(), markText);
		} while ((tmp = tmp->brother) != NULL);
	}
	
	board->updateCanvas();
}

