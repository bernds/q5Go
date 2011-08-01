/*
* group.h
*/

#ifndef GROUP_H
#define GROUP_H

#include <QList>

class Stone;

class Group : public QList<Stone *>
{
public:
	Group();
	~Group() {}
	
	int getLiberties() const { return liberties; }
	void setLiberties(int l) { liberties = l; }
	bool isAttachedTo(Stone *s);
#ifndef NO_DEBUG
	void debug();
#endif
	
private:
	int liberties;
};

#endif
