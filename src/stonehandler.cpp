/*
* stonehandler.cpp
*/

#include "stonehandler.h"
#include "boardhandler.h"
#include "group.h"
#include "board.h"
#include "imagehandler.h"
#include <stdlib.h>

StoneHandler::StoneHandler(BoardHandler *bh)
: boardHandler(bh)
{
	CHECK_PTR(boardHandler);
	
	stones = new QIntDict<Stone>(367);  // prime number larger than 361 (19*19)
	// TODO: Dynamic for different board size?
	stones->setAutoDelete(TRUE);
	
	groups = new QPtrList<Group>;
	groups->setAutoDelete(TRUE);
	
	workingOnNewMove = false;
}

StoneHandler::~StoneHandler()
{
	clearData();
	delete stones;
	delete groups;
}

void StoneHandler::clearData()
{
	workingOnNewMove = false;
	stones->clear();
	groups->clear();
}


bool StoneHandler::addStone(Stone *stone, bool toAdd, bool doCheck, Matrix *m, bool koStone)
{
//	CHECK_PTR(stone);
//	CHECK_PTR(m);
  
	if (toAdd)
		stones->insert(Matrix::coordsToKey(stone->posX(), stone->posY()), stone);
	
	if (doCheck)
		return checkPosition(stone, m, koStone);
	
	return true;
}

bool StoneHandler::removeStone(int x, int y, bool hide)
{
	if (!hasStone(x, y))
		return false;
	
	if (!hide)
	{
    // We delete the killed stones only if we want to update the board (i.e. : not if we are browsing the game)
    if (boardHandler->getDisplay_incoming_move())             //SL added eb 9
      if (!stones->remove(Matrix::coordsToKey(x, y)))
	    {
			   qWarning("   ***   Key for stone %d, %d not found!   ***", x, y);
			   return false;
		   }
	}
	else
	{
		Stone *s;
		if ((s = stones->find(Matrix::coordsToKey(x, y))) == NULL)
		{
			qWarning("   ***   Key for stone %d, %d not found!   ***", x, y);
			return false;
		}
		s->hide();
	}
	
	return true;
}

//  0 : no stone
// -1 : hidden
//  1 : shown
int StoneHandler::hasStone(int x, int y)
{
	Stone *s;
	
	if ((s = stones->find(Matrix::coordsToKey(x, y))) == NULL)
		return 0;
	
	if (s->visible())
		return 1;
	
	return -1;
}

//bool StoneHandler::checkPosition(Stone *stone)
bool StoneHandler::checkPosition(Stone *stone, Matrix *m, bool koStone)
{
	//CHECK_PTR(stone);
 // CHECK_PTR(m); // SL added eb 8
 
	if (!stone->visible())
		return true;
	
	Group *active = NULL;
	
	// No groups existing? Create one.
	if (groups->isEmpty())
	{
		Group *g = assembleGroup(stone,m);
		CHECK_PTR(g);
		groups->append(g);
		active = g;
	}
	// We already have one or more groups.
	else
	{
		bool flag = false;
		Group *tmp;
		
		for (unsigned int i=0; i<groups->count(); i++)
		{
			tmp = groups->at(i);
			//CHECK_PTR(tmp);
			
			// Check if the added stone is attached to an existing group.
			// If yes, update this group and replace the old one.
			// If the stone is attached to two groups, remove the second group.
			// This happens if the added stone connects two groups.
			if (tmp->isAttachedTo(stone))
			{
				// Group attached to stone
				if (!flag)
				{
					if (!groups->remove(i))
						qFatal("StoneHandler::checkPosition(Stone *stone):"
						"Oops, removing an attached group failed.");
					active = assembleGroup(stone,m);
					groups->insert(i, active);
					flag = true;
				}
				// Groups connected, remove one
				else
				{
					if (active != NULL && active == groups->at(i))
						active = tmp;
					if (!groups->remove(i))
						qFatal("StoneHandler::checkPosition(Stone *stone): "
						"Oops, removing a connected group failed.");
					i--;
				}
			}
		}
		
		// The added stone isnt attached to an existing group. Create a new group.
		if (!flag)
		{
			Group *g = assembleGroup(stone,m);
			CHECK_PTR(g);
			groups->append(g);
			active = g;
		}
	}
	
	// active->debug();
	
	
	// Now we have to sort the active group as last in the groups QPtrList,
	// so if this one is out of liberties, we beep and abort the operation.
	// This prevents suicide moves.
	groups->append(groups->take(groups->findRef(active)));
	
	// Check the liberties of every group. If a group has zero liberties, remove it.
	for (unsigned int i=0; i<groups->count(); i++)
	{
		Group *tmp = groups->at(i);
		//CHECK_PTR(tmp);


 		tmp->setLiberties(countLiberties(tmp, m));          //SL added eb 8

    
		// qDebug("Group #%d with %d liberties:", i, tmp->getLiberties());
		// tmp->debug();
		
		// Oops, zero liberties.
 		if (tmp->getLiberties() == 0)
		{
			// Suicide move?
			if (tmp == active)
			{
				if (active->count() == 1)
				{
					groups->remove(i);
					removeStone(stone->posX(), stone->posY(), false);
				}
				return false;
			}
			
			//was it a forbidden ko move ?
			if ((tmp->count() == 1) && koStone && ((countLiberties(active, m) == 0)))
			{
				active->debug();
				groups->remove(groups->findRef(active));
				removeStone(stone->posX(), stone->posY(), false);
				return false ;
			}

			int stoneCounter = 0;
			
			// Erase the stones of this group from the stones table.
			QListIterator<Stone> it(*tmp);
			for (; it.current(); ++it)
			{
				Stone *s = it.current();
				CHECK_PTR(s);
				if (workingOnNewMove)
					boardHandler->updateCurrentMatrix(stoneNone, s->posX(), s->posY());
				removeStone(s->posX(), s->posY());
				stoneCounter ++;
			}
			
			// Remove the group from the groups list.
			// qDebug("Oops, a group got killed. Removing killed group #%d", i);
			if (tmp == active)
				active = NULL;
			if (!groups->remove(i))
				qFatal("StoneHandler::checkPosition(Stone *stone): "
				"Oops, removing a killed group failed.");
			i--;
			
			// Tell the boardhandler about the captures
			boardHandler->setCaptures(stone->getColor(), stoneCounter);
		}
	}
	
	return true;
}


Group* StoneHandler::assembleGroup(Stone *stone, Matrix *m)
{
	if (stones->isEmpty())
		qFatal("StoneHandler::assembleGroup(Stone *stone): No stones on the board!");
	
	Group *group = new Group();
	CHECK_PTR(group);
	
//	CHECK_PTR(stone);
//  CHECK_PTR(m);                   //SL added eb 8
	group->append(stone);
	
	unsigned int mark = 0;
	
	// Walk through the horizontal and vertical directions and assemble the
	// attached stones to this group.
	while (mark < group->count())
	{
		stone = group->at(mark);
		// we use preferably the matrix
		if ((m==NULL && stone->visible())|| (m!= NULL || m->at(stone->posX() - 1, stone->posY() - 1) != stoneNone ))
		{
			int stoneX = stone->posX(),
				stoneY = stone->posY();
			StoneColor col = stone->getColor();
			
			// North
			group = checkNeighbour(stoneX, stoneY-1, col, group,m);      //SL added eb 8
			
			// West
			group = checkNeighbour(stoneX-1, stoneY, col, group,m);       //SL added eb 8
			
			// South
			group = checkNeighbour(stoneX, stoneY+1, col, group,m);       //SL added eb 8
			
			// East
			group = checkNeighbour(stoneX+1, stoneY, col, group,m);        //SL added eb 8
		}
		mark ++;
	}
	
	return group;
}

Group* StoneHandler::checkNeighbour(int x, int y, StoneColor color, Group *group, Matrix *m) //SL added eb 8
{
//	CHECK_PTR(group);
//	CHECK_PTR(m); //added eb 8
  bool visible = false ;
  int size = boardHandler->board->getBoardSize();     //end add eb 8

  Stone *tmp = stones->find(Matrix::coordsToKey(x, y));
  
  // Okay, this is dirty and synthetic :
  // Because we use this function where the matrix can be NULL, we need to check this
  // Furthermore, since this has been added after the first code, we keep the 'stone->visible' test where one should only use the 'matrix' code
  if (m != NULL && x-1 >= 0 && x-1 < size && y-1 >= 0 && y-1 < size) //SL added eb 8
   visible = (m->at(x - 1, y - 1) != stoneNone); //SL added eb 9 We do this in order not to pass a null matrix to the matrix->at function (seen in handicap games)

  // again we priviledge matrix over stone visibility (we might be browsing a game)
	if (tmp != NULL && tmp->getColor() == color && (tmp->visible()|| visible)) //SL added eb 8
	{
		if (!group->contains(tmp))
		{
			group->append(tmp);
			tmp->checked = true;
		}
	}
	return group;
}

int StoneHandler::countLiberties(Group *group, Matrix *m)     //SL added eb 8
{
//	CHECK_PTR(group);
//	CHECK_PTR(m); // SL added eb 8
  
	int liberties = 0;
	QValueList<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for (unsigned int i=0; i<group->count(); i++)
	{
		Stone *tmp = group->at(i);
		//CHECK_PTR(tmp);
		
		int x = tmp->posX(),
			y = tmp->posY();
		
		// North
		checkNeighbourLiberty(x, y-1, libCounted, liberties,m);        //SL added eb 8
		
		// West
		checkNeighbourLiberty(x-1, y, libCounted, liberties,m);         //SL added eb 8
		
		// South
		checkNeighbourLiberty(x, y+1, libCounted, liberties,m);          //SL added eb 8
		
		// East
		checkNeighbourLiberty(x+1, y, libCounted, liberties,m);         //SL added eb 8
	}
	return liberties;
}

void StoneHandler::checkNeighbourLiberty(int x, int y, QValueList<int> &libCounted, int &liberties, Matrix *m)    //SL added eb 8
{
	if (!x || !y)
		return;
	
	Stone *s;
//  CHECK_PTR(m); // SL added eb 8

  if (m==NULL) //added eb 8 -> we don't have a matrix passed here, so we check on the board
  {
	  if (x <= boardHandler->board->getBoardSize() && y <= boardHandler->board->getBoardSize() && x >= 0 && y >= 0 &&
		  !libCounted.contains(100*x + y) &&
  		((s = stones->find(Matrix::coordsToKey(x, y))) == NULL ||
	  	!s->visible()))
	  {
		  libCounted.append(100*x + y);
		  liberties ++;
	  }
  }  
  else                                      
  {
    if (x <= boardHandler->board->getBoardSize() && y <= boardHandler->board->getBoardSize() && x >= 0 && y >= 0 &&
	    !libCounted.contains(100*x + y) &&
	    (m->at(x - 1, y - 1) == stoneNone ))         // ?? check stoneErase ?
	  {
		  libCounted.append(100*x + y);
		  liberties ++;
	  }            // end add eb 8
  }
}

int StoneHandler::countLibertiesOnMatrix(Group *group, Matrix *m)    
{
	CHECK_PTR(group);
	CHECK_PTR(m);
	
	int liberties = 0;
	QValueList<int> libCounted;
	
	// Walk through the horizontal and vertial directions, counting the
	// liberties of this group.
	for (unsigned int i=0; i<group->count(); i++)
	{
		Stone *tmp = group->at(i);
		CHECK_PTR(tmp);
		
		int x = tmp->posX(),
			y = tmp->posY();
		
		// North
		checkNeighbourLibertyOnMatrix(x, y-1, libCounted, liberties, m);
		
		// West
		checkNeighbourLibertyOnMatrix(x-1, y, libCounted, liberties, m);
		
		// South
		checkNeighbourLibertyOnMatrix(x, y+1, libCounted, liberties, m);
		
		// East
		checkNeighbourLibertyOnMatrix(x+1, y, libCounted, liberties, m);
	}
	return liberties;
}

void StoneHandler::checkNeighbourLibertyOnMatrix(int x, int y, QValueList<int> &libCounted, int &liberties, Matrix *m)
{
	if (!x || !y)
		return;
	
//	Stone *s;
	
	if (x <= boardHandler->board->getBoardSize() && y <= boardHandler->board->getBoardSize() && x >= 0 && y >= 0 &&
	    !libCounted.contains(100*x + y) &&
	    (m->at(x - 1, y - 1) == MARK_TERRITORY_DONE_BLACK ||
	     m->at(x - 1, y - 1) == MARK_TERRITORY_DONE_WHITE))
	{
		libCounted.append(100*x + y);
		liberties ++;
	}
}

bool StoneHandler::updateAll(Matrix *m, bool toDraw)
{
	// qDebug("StoneHandler::updateAll(Matrix *m) - toDraw = %d", toDraw);
	
	CHECK_PTR(m);
	
	// m->debug();
	
	Stone *stone;
	bool modified = false, fake = false;
	short data;
	
	/*
	* Synchronize the matrix with the stonehandler data and
	* update the canvas.
	* This is usually called when navigating through the tree.
	*/
	
	for (int y=1; y<=boardHandler->board->getBoardSize(); y++)
	{
		for (int x=1; x<=boardHandler->board->getBoardSize(); x++)
		{
			// Extract the data for the stone from the matrix
			data = abs(m->at(x-1, y-1) % 10);
			switch (data)
			{
			case stoneBlack:
				if ((stone = stones->find(Matrix::coordsToKey(x, y))) == NULL)
				{
					addStone(boardHandler->board->addStoneSprite(stoneBlack, x, y, fake), true, false);
					modified = true;
					break;
				}
				else if (!stone->visible())
				{
					stone->show();
					modified = true;
				}
				
				if (stone->getColor() == stoneWhite)
				{
					stone->setColor(stoneBlack);
					modified = true;
				}
				
				break;
				
			case stoneWhite:
				if ((stone = stones->find(Matrix::coordsToKey(x, y))) == NULL)
				{
					addStone(boardHandler->board->addStoneSprite(stoneWhite, x, y, fake), true, false);
					modified = true;
					break;
				}
				else if (!stone->visible())
				{
					stone->show();
					modified = true;
				}
				
				if (stone->getColor() == stoneBlack)
				{
					stone->setColor(stoneWhite);
					modified = true;
				}
				
				break;
				
			case stoneNone:
			case stoneErase:
				if ((stone = stones->find(Matrix::coordsToKey(x, y))) != NULL &&
					stone->visible())
				{
					stone->hide();
					modified = true;
				}
				break;
				
			default:
				qWarning("Bad matrix data <%d> at %d/%d in StoneHandler::updateAll(Matrix *m) !",
					data, x, y);
			}
			
			// Skip mark drawing when reading sgf
			if (!toDraw)
				continue;
			
			// Extract the mark data from the matrix
			data = abs(m->at(x-1, y-1) / 10);
			switch (data)
			{
			case markSquare:
				modified = true;
				boardHandler->board->setMark(x, y, markSquare, false);
				break;
				
			case markCircle:
				modified = true;
				boardHandler->board->setMark(x, y, markCircle, false);
				break;
				
			case markTriangle:
				modified = true;
				boardHandler->board->setMark(x, y, markTriangle, false);
				break;
				
			case markCross:
				modified = true;
				boardHandler->board->setMark(x, y, markCross, false);
				break;
				
			case markText:
				modified = true;
				boardHandler->board->setMark(x, y, markText, false, m->getMarkText(x, y));
				break;
				
			case markNumber:
				modified = true;
				boardHandler->board->setMark(x, y, markNumber, false, m->getMarkText(x, y));
				break;
				
			case markTerrBlack:
				modified = true;
				boardHandler->board->setMark(x, y, markTerrBlack, false);
				break;
				
			case markTerrWhite:
				modified = true;
				boardHandler->board->setMark(x, y, markTerrWhite, false);
				break;
				
			case markNone:
				if (boardHandler->board->hasMark(x, y))
				{
					modified = true;
					boardHandler->board->removeMark(x, y, false);
				}
			}
	}
    }
    
    return modified;
}

void StoneHandler::checkAllPositions()
{
	// Traverse all stones and check their positions.
	// Yuck, I suppose this is expensive...
	// Called when jumping through the variations.
	
	groups->clear();
	
	QIntDictIterator<Stone> it(*stones);
	Stone *s;
	
	// Unmark stones as checked
	while (it.current())
	{
		it.current()->checked = false;
		++it;
	}
	
	it.toFirst();
	
	while (it.current())
	{
		s = it.current();
		if (!s->checked)
			checkPosition(s,NULL);            //SL added eb 8
		++it;
	}
}

#ifndef NO_DEBUG
void StoneHandler::debug()
{
	qDebug("StoneHandler::debug()");
	
#if 0
	QIntDictIterator<Stone> its(*stones);
	Stone *s;
	
	while (its.current())
	{
		s = its.current();
		qDebug("%d -> %s", its.currentKey(), s->getColor() == stoneBlack ? "Black" : "White");
		++its;
	}
#endif
	
	QListIterator<Group> it(*groups);
	for (; it.current(); ++it)
	{
		Group *g = it.current();
		g->debug();
	}
}
#endif

bool StoneHandler::removeDeadGroup(int x, int y, int &caps, StoneColor &col, bool &dead)
{
	if (hasStone(x, y) != 1)
		return false;
	
	Stone *s = getStoneAt(x, y);
	CHECK_PTR(s);
	col = s->getColor();
	
	if (!s->isDead())
		dead = true;
	
	Group *g = assembleGroup(s, NULL);        //SL added eb 8
	CHECK_PTR(g);
	
	caps = g->count();
	
	// Mark stones of this group as dead or alive again
	QListIterator<Stone> it(*g);
	for (; it.current(); ++it)
	{
		s = it.current();
		CHECK_PTR(s);
		s->setDead(dead);
		if (dead)
		{
			s->setSequence(boardHandler->board->getImageHandler()->getGhostPixmaps());
			s->shadow->hide();
		}
		else
		{
			s->setSequence(boardHandler->board->getImageHandler()->getStonePixmaps());
			s->shadow->show();
		}
	}
	
	delete g;
	return true;
}

bool StoneHandler::markSekiGroup(int x, int y, int &caps, StoneColor &col, bool &seki)
{
	if (hasStone(x, y) != 1)
		return false;
	
	Stone *s = getStoneAt(x, y);
	CHECK_PTR(s);
	col = s->getColor();
	
	if (!s->isSeki())
		seki = true;
	
	Group *g = assembleGroup(s, NULL);        //SL added eb 8
	CHECK_PTR(g);
	
	// Mark stones of this group as seki
	QListIterator<Stone> it(*g);
	for (; it.current(); ++it)
	{
		s = it.current();
		CHECK_PTR(s);
		if (seki && s->isDead())
			caps ++;
		s->setSeki(seki);
		if (seki)
		{
			s->setSequence(boardHandler->board->getImageHandler()->getGhostPixmaps());
			s->shadow->hide();
		}	
		else
		{
			s->setSequence(boardHandler->board->getImageHandler()->getStonePixmaps());
			s->shadow->show();
		}
	}
	
	delete g;
	return true;
}

void StoneHandler::removeDeadMarks()
{
	QIntDictIterator<Stone> it(*stones);
	Stone *s;
	
	while (it.current())
	{
		s = it.current();
		CHECK_PTR(s);
		if (s->isDead() || s->isSeki())
		{
			s->setDead(false);
			s->setSeki(false);
			s->setSequence(boardHandler->board->getImageHandler()->getStonePixmaps());
			s->shadow->show();
		}
		++it;
	}
}

void StoneHandler::updateDeadMarks(int &black, int &white)
{
	QIntDictIterator<Stone> it(*stones);
	Stone *s;
	
	while (it.current())
	{
		s = it.current();
		CHECK_PTR(s);
		if (s->isDead())
		{
			if (s->getColor() == stoneBlack)
				white ++;
			else
				black ++;
		}
		++it;
	}    
}

bool StoneHandler:: checkFalseEye(Matrix *m, int x, int y, int col)
{
    int bsize = m->getSize();

    // Stone to the North?
    if (y - 1 >= 0 && m->at(x, y - 1) == col)
	if (countLibertiesOnMatrix(assembleGroup(getStoneAt(x + 1, y),NULL), m) == 1) //SL added eb 8
	    return true;

    // Stone to the west?
    if (x - 1 >= 0 && m->at(x - 1, y) == col)                                    //SL added eb 8
	if (countLibertiesOnMatrix(assembleGroup(getStoneAt(x, y + 1),NULL), m) == 1)
	    return true;
    
    // Stone to the south?
    if (y + 1 < bsize && m->at(x, y + 1) == col)
	if (countLibertiesOnMatrix(assembleGroup(getStoneAt(x + 1, y + 2),NULL), m) == 1)    //SL added eb 8
	    return true;
    
    // Stone to the east?
    if (x + 1 < bsize && m->at(x + 1, y) == col)
	if (countLibertiesOnMatrix(assembleGroup(getStoneAt(x + 2, y + 1),NULL), m) == 1)     //SL added eb 8
	    return true;
    
    return false;
}



  
