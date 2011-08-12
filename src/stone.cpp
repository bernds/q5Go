/*
* stone.cpp
*/


#include <QPen>
#include <QFont>
#include "defines.h"
#include "stone.h"
#include "imagehandler.h"
#include <stdlib.h>

Stone::Stone(const Stone &s)
	: QGraphicsPixmapItem(0, s.scene()), myX (s.myX), myY (s.myY),
	  pixmapList(s.pixmapList)
{
	
}

Stone::Stone(const QList<QPixmap> *images, QGraphicsScene *canvas, StoneColor c,
	     int x, int y, int numberOfImages, bool has_shadow)
: QGraphicsPixmapItem(0, canvas), myX(x), myY(y), pixmapList(images)
{
	shadow = NULL;
	
	if (has_shadow) {
		shadow = new QGraphicsPixmapItem(0, canvas);
		shadow->setPixmap(images->last());
		shadow->setZValue(4);
	}

	setColor(c);

	moveNum = new QGraphicsSimpleTextItem("",this);
	moveNum->setPen(QPen(( color == stoneBlack ? Qt::white : Qt::black)));

	setZValue(5);
	show();
	
	dead = false;
	seki = false;
	checked = false;
}

int Stone::randomImage(StoneColor c)
{
	if (c == stoneBlack)
		return 0;
	if (pixmapList->count() <= 2)
		return 1;

	return rand() % (pixmapList->count() -2) + 1;
}

Stone::~Stone()
{
    delete shadow;
} 

void Stone::setColor(StoneColor c)
{
	setPixmap((*pixmapList)[randomImage(c)]);
	color = c;
	
	if (shadow)
		shadow->setPixmap(pixmapList->last());
}

void Stone::toggleOneColorGo(bool oneColor)
{
	// if we play one color go, the black stones are repainted white, else normal black
	if (color == stoneBlack) 
		setPixmap((*pixmapList)[randomImage(stoneWhite)]);
}

void Stone::setPos(double x, double y)
{
	QGraphicsPixmapItem::setPos((qreal)x,(qreal)y);
	qreal offset;
	if (shadow) {
		 offset = shadow->boundingRect().height();
		 shadow->setPos((qreal)(x-offset / 8 ), (qreal)(y+ offset / 8));
	}
	
	moveNum->setFont(QFont("",pixmap().width()/3, 1));

	qreal h = (boundingRect().height() - moveNum->boundingRect().height())/2 ;
	qreal w = (boundingRect().width() - moveNum->boundingRect().width())/2 ;

	moveNum->setPos(w,h);
}

void Stone::hide()
{
	QGraphicsPixmapItem::hide();
	if (shadow) 	 shadow->hide();
}

void Stone::show()
{
	QGraphicsPixmapItem::show();
	if (shadow) 	 shadow->show();
}

void Stone::togglePixmap(const QList<QPixmap> *a, bool showShadow)
{
	pixmapList = a;
	setColor(color);

	if (shadow)
	{
		if (showShadow)
			shadow->show();
		else 
			shadow->hide();
	}
}
