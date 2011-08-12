/*
* stone.h
*/

#ifndef STONE_H
#define STONE_H

#include "defines.h"
#include <QGraphicsPixmapItem>

class Stone : public QGraphicsPixmapItem
{
public:
	Stone(const QList<QPixmap> *, QGraphicsScene *canvas, StoneColor c,
	      int x, int y, int numberOfImages=WHITE_STONES_NB,
	      bool has_shadow=false);
	Stone(const Stone &s);
	~Stone() ;
	
	StoneColor getColor() const { return color; }
	void setColor(StoneColor c=stoneBlack);
	void toggleOneColorGo(bool oneColor) ;
	int type() const { return RTTI_STONE; }
	int posX() const { return myX; }
	int posY() const { return myY; }
	void setPos(double x, double y);
	bool isDead() const { return dead; }
	void setDead(bool b=true) { dead = b; seki = false;}
	bool isSeki() const { return seki; }
	void setSeki(bool b=true) { seki = b; dead = false; }
	QGraphicsSimpleTextItem * getNum() {return moveNum;}
	void setCoord(int x, int y) { myX = x; myY = y; }
	void togglePixmap(const QList<QPixmap> *a, bool showShadow = TRUE);

	void setX(double x);
	void setY(double y);
	void show();
	void hide();
	
	bool checked;

 protected:
	int randomImage(StoneColor);
	
private:
	StoneColor color;
	int myX, myY;
	bool dead, seki;

	const QList<QPixmap> *pixmapList;
	QGraphicsSimpleTextItem *moveNum;
	QGraphicsPixmapItem *shadow;
};

#endif
