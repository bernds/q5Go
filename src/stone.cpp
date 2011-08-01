/*
* stone.cpp
*/

#include "defines.h"
#include "stone.h"
#include "imagehandler.h"
#include <stdlib.h>

Stone::Stone(Q3CanvasPixmapArray *a, Q3Canvas *canvas, StoneColor c, int x, int y, int numberOfImages, bool has_shadow)
: Q3CanvasSprite(a, canvas), color(c), myX(x), myY(y)
{
	setFrame(color == stoneBlack ? 0 : (rand() % numberOfImages) + 1);
	
	shadow = NULL;
	
	if (has_shadow) {
		shadow = new Q3CanvasSprite(a, canvas);
		shadow->setFrame(numberOfImages + 1);
		shadow->setZ(4);
	}
	
	setZ(5);
	show();
	
	dead = false;
	seki = false;
	checked = false;
	

}

Stone::~Stone()
{
    delete shadow;
} 

void Stone::setColor(StoneColor c)
{
	setFrame(c == stoneBlack ? 0 : (rand() % 8) + 1);
	color = c;
}

void Stone::toggleOneColorGo(bool oneColor)
{
  // if we play one color go, the black stones are repainted white, else normal black
  if (color ==  stoneBlack) 
    setFrame(oneColor ? (rand() % 8) + 1 : 0);
}

void Stone::setX(double x)
{
	int offset;
	Q3CanvasSprite::setX(x);
	if (shadow) {
		 offset = shadow->boundingRect().width();
		 shadow->setX(x - offset / 8);
	}
}



void Stone::setY(double y)
{
	Q3CanvasSprite::setY(y);
	int offset;
	if (shadow) {
		 offset = shadow->boundingRect().height();
		 shadow->setY(y + offset / 8);
	}
	
}

void Stone::hide()
{
	Q3CanvasSprite::hide();
	if (shadow) 	 shadow->hide();
	
}

void Stone::show()
{
	Q3CanvasSprite::show();
	if (shadow) 	 shadow->show();
	
}
