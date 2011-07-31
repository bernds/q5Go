/*
* tree.h
*/

#ifndef TREE_H
#define TREE_H

#include <qptrstack.h>
class Move;

template<class type> class QPtrStack;

class Tree
{
public:
	Tree(int board_size);
	~Tree();
	void init(int board_size);
	bool addBrother(Move *node);
	bool addSon(Move *node);
	int getNumBrothers();
	int getNumSons(Move *m=0);
	int getBranchLength(Move *node=0);
	Move* nextMove();
	Move* previousMove();
	Move* nextVariation();
	Move* previousVariation();
	bool hasSon(Move *m);
	bool hasPrevBrother(Move *m=0);
	bool hasNextBrother();
	void clear();
	static void traverseClear(Move *m);
	int count();
	Move* getCurrent() const { return current; }
	void setCurrent(Move *m) { current = m; }
	void setToFirstMove();
	Move* getRoot() const { return root; }
	void setRoot(Move *m) { root = m; }
	int mainBranchSize();
	Move* findMoveInMainBranch(int x, int y) { return findMove(root, x, y, false); }
	Move* findMoveInBranch(int x, int y) { return findMove(current, x, y, true); }
	Move* findLastMoveInMainBranch();   //SL added eb 9
	void traverseFind(Move *m, int x, int y, QPtrStack<Move> &result);
	
protected:
	Move* findMove(Move *start, int x, int y, bool checkmarker);
	
private:
	Move *root, *current;
};

#endif
