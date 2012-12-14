/*
* stonehandler.h
*/

#ifndef STONEHANDLER_H
#define STONEHANDLER_H

#include <Q3IntDict>
#include <Q3ValueList>
#include <Q3PtrList>
#include "defines.h"
#include "matrix.h"
#include "stone.h"

class Group;
class BoardHandler;

class StoneHandler
{
public:
	StoneHandler(BoardHandler *bh);
	~StoneHandler();
	void clearData();
	void toggleWorking(bool toggle) { workingOnNewMove = toggle; }
//  bool addStone(Stone *stone, bool toAdd=true, bool toCheck=true);  
	bool addStone(Stone *stone, bool toAdd=true, bool toCheck=true, Matrix *m=NULL, bool koStone = false);
	bool removeStone(int x, int y, bool hide=false);
	int hasStone(int x, int y);
	Stone* getStoneAt(int x, int y) { return stones->find(Matrix::coordsToKey(x, y)); }
	Q3IntDict<Stone>* getAllStones() const { return stones; }
	bool updateAll(Matrix *m, bool toDraw=true);
	void checkAllPositions();
	bool removeDeadGroup(int x, int y, int &caps, StoneColor &col, bool &dead);
	bool markSekiGroup(int x, int y, int &caps, StoneColor &col, bool &seki);
	void removeDeadMarks();
	void updateDeadMarks(int &black, int &white);
	bool checkFalseEye(Matrix *m, int x, int y, int col);
	int numStones() const { return stones->count(); } // TODO: For debugging only currently.
	// Needed later?
#ifndef NO_DEBUG
	void debug();
#endif
	
protected:
//  bool checkPosition(Stone *stone);
	bool checkPosition(Stone *stone,Matrix *m, bool koStone = false);       //SL added eb 8
	Group* assembleGroup(Stone *stone,Matrix *m);     //SL added eb 8
//  Group* assembleGroup(Stone *stone);
	Group* checkNeighbour(int x, int y, StoneColor color, Group *group,Matrix *m);
  	Group* checkNeighbour(int x, int y, StoneColor color, Group *group);
	int countLiberties(Group *group, Matrix *m);      //SL added eb 8
	int countLibertiesOnMatrix(Group *group, Matrix *m);
	void checkNeighbourLiberty(int x, int y, Q3ValueList<int> &libCounted, int &liberties, Matrix *m);
	void checkNeighbourLibertyOnMatrix(int x, int y, Q3ValueList<int> &libCounted, int &liberties, Matrix *m);
	
private:
	Q3IntDict<Stone> *stones;
	Q3PtrList<Group> *groups;
	bool workingOnNewMove;
	BoardHandler *boardHandler;
};

#endif
