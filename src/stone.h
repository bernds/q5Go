/*
* stone.h
*/

#ifndef STONE_H
#define STONE_H

#include "defines.h"
#include <q3canvas.h>

class Stone : public Q3CanvasSprite
{
public:
	Stone(Q3CanvasPixmapArray *a, Q3Canvas *canvas, StoneColor c, int x, int y,
	      int numberOfImages=WHITE_STONES_NB, bool has_shadow=false);
	~Stone() ;
	
	StoneColor getColor() const { return color; }
	void setColor(StoneColor c=stoneBlack);
	void toggleOneColorGo(bool oneColor) ;
	int rtti() const { return RTTI_STONE; }
	int posX() const { return myX; }
	int posY() const { return myY; }
	void setPos(int x, int y) { myX = x; myY = y; }
	bool isDead() const { return dead; }
	void setDead(bool b=true) { dead = b; seki = false;}
	bool isSeki() const { return seki; }
	void setSeki(bool b=true) { seki = b; dead = false; }
	
	void setX(double x);
	void setY(double y);
	void show();
	void hide();
	
	bool checked;
	
	Q3CanvasSprite *shadow;
	
private:
	StoneColor color;
	int myX, myY;
	bool dead, seki;
};

#endif
