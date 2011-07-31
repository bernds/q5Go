/*
* group.h
*/

#ifndef GROUP_H
#define GROUP_H

#include <qptrlist.h>

class Stone;

class Group : public QPtrList<Stone>
{
public:
	Group();
	~Group() {}
	
	int getLiberties() const { return liberties; }
	void setLiberties(int l) { liberties = l; }
	virtual int compareItems(Item d1, Item d2);
	bool isAttachedTo(Stone *s);
#ifndef NO_DEBUG
	void debug();
#endif
	
private:
	int liberties;
};

#endif
