#ifndef STONE_H
#define STONE_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPen>

#include "defines.h"
#include "goboard.h"
#include "imagehandler.h"

class stone_gfx : public QGraphicsPixmapItem
{
public:
	stone_gfx(QGraphicsScene *canvas, ImageHandler *, stone_color c, stone_type t, int rnd);
	~stone_gfx();

	void set_appearance (stone_color c, stone_type t);
	void set_center (double x, double y);

	void show();
	void hide();

private:
	ImageHandler *m_ih;
	stone_color m_color;
	stone_type m_type;
	int m_rnd;

	QGraphicsSimpleTextItem *moveNum;
	QGraphicsPixmapItem *shadow;
};

#endif
