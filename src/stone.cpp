/*
* stone.cpp
*/

#include "defines.h"
#include "stone.h"
#include "imagehandler.h"
#include <stdlib.h>

Stone::Stone(QCanvasPixmapArray *a, QCanvas *canvas, StoneColor c, int x, int y, int numberOfImages, bool has_shadow)
: QCanvasSprite(a, canvas), color(c), myX(x), myY(y)
{
	setFrame(color == stoneBlack ? 0 : (rand() % numberOfImages) + 1);
	
	shadow = NULL;
	
	if (has_shadow) {
		shadow = new QCanvasSprite(a, canvas);
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
	QCanvasSprite::setX(x);
	if (shadow) {
		 offset = shadow->boundingRect().width();
		 shadow->setX(x - offset / 8);
	}
}



void Stone::setY(double y)
{
	QCanvasSprite::setY(y);
	int offset;
	if (shadow) {
		 offset = shadow->boundingRect().height();
		 shadow->setY(y + offset / 8);
	}
	
}

void Stone::hide()
{
	QCanvasSprite::hide();
	if (shadow) 	 shadow->hide();
	
}

void Stone::show()
{
	QCanvasSprite::show();
	if (shadow) 	 shadow->show();
	
}
