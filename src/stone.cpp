/*
* stone.cpp
*/


#include <QPen>
#include <QFont>
#include "defines.h"
#include "stone.h"
#include "imagehandler.h"
#include <stdlib.h>

stone_gfx::stone_gfx(QGraphicsScene *canvas, ImageHandler *ih, stone_color c, stone_type t, int rnd)
	: m_ih (ih), m_rnd (rnd)
{
	shadow = new QGraphicsPixmapItem;
	shadow->setPixmap(ih->getStonePixmaps ()->last ());
	shadow->setZValue(4);

	moveNum = new QGraphicsSimpleTextItem("",this);

	setZValue(5);
	set_appearance (c, t);
	canvas->addItem (this);
	canvas->addItem (shadow);
	show ();
}

stone_gfx::~stone_gfx()
{
    delete shadow;
}

void stone_gfx::set_appearance(stone_color c, stone_type t)
{
	m_color = c;
	m_type = t;
	moveNum->setPen (QPen (m_color == black ? Qt::white : Qt::black));

	if (t == stone_type::live) {
		const QList<QPixmap> *l = m_ih->getStonePixmaps ();
		int cnt = l->count ();
		if (c == black)
			setPixmap ((*l)[0]);
		else if (cnt <= 2)
			setPixmap ((*l)[1]);
		else
			setPixmap ((*l)[1 + m_rnd % (cnt - 2)]);
		shadow->setPixmap(l->last ());
		shadow->show ();
	} else {
		const QList<QPixmap> *l = m_ih->getGhostPixmaps ();
		if (c == black)
			setPixmap ((*l)[0]);
		else
			setPixmap ((*l)[1]);
		shadow->hide ();
	}
}

void stone_gfx::set_center(double x, double y)
{
	x -= boundingRect().width() / 2;
	y -= boundingRect().height() / 2;

	setPos(x, y);
	qreal offset;
	if (shadow) {
		 offset = shadow->boundingRect().height();
		 shadow->setPos((qreal)(x - offset / 8), (qreal)(y + offset / 8));
	}

	moveNum->setFont(QFont("",pixmap().width()/3, 1));

	qreal h = (boundingRect().height() - moveNum->boundingRect().height())/2 ;
	qreal w = (boundingRect().width() - moveNum->boundingRect().width())/2 ;

	moveNum->setPos(w,h);
}

void stone_gfx::hide()
{
	QGraphicsPixmapItem::hide();
	if (shadow)
		shadow->hide();
}

void stone_gfx::show()
{
	QGraphicsPixmapItem::show();
	if (shadow && m_type == stone_type::live)
		shadow->show();
}
