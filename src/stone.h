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


class mark_gfx
{
public:
	mark_gfx(QColor c, mark t, double sz) : m_color (c), m_type (t), m_size (sz)
	{
	}
	virtual ~mark_gfx () { }
	int line_width ()
	{
		return (int)(m_size / 30) + 1;
	}
	void set_color (QColor c)
	{
		m_color = c;
		set_size (m_size);
	}
	mark get_type () { return m_type; }
	virtual void set_center (double x, double y) = 0;
	virtual void set_size (double) = 0;
	virtual void set_shown (bool) = 0;
	void set_size_and_color (double sz, QColor c)
	{
		m_color = c;
		set_size (sz);
	}
protected:
	QColor m_color;
	mark m_type;
	double m_size;
};

class mark_square : public mark_gfx, public QGraphicsRectItem
{
public:
	mark_square (QGraphicsScene *canvas, double size, QColor c)
		: mark_gfx (c, mark::square, size)
	{
		set_size (size);
		setZValue (10);
		canvas->addItem (this);
	}
	~mark_square ()
	{
		scene ()->removeItem (this);
	}
	void set_size (double size)
	{
		m_size = size;
		setPen (QPen (m_color, line_width ()));
		size *= 0.55;
		/* Do some rounding and ensure an odd size for better centering.  */
		int sz = size / 2;
		sz = sz * 2 + line_width () % 2;
		setRect (0, 0, sz, sz);
	}
	void set_center (double x, double y)
	{
		QRectF br = boundingRect ();
		setPos (x - br.width () / 2 + line_width () / 2, y - br.height () / 2 + line_width () / 2);
	}
	void set_shown (bool do_show)
	{
		if (do_show)
			show ();
		else
			hide ();
	}
};

class mark_circle : public mark_gfx, public QGraphicsEllipseItem
{
public:
	mark_circle (QGraphicsScene *canvas, double size, QColor c)
		: mark_gfx (c, mark::circle, size)
	{
		set_size (size);
		setZValue (10);
		canvas->addItem (this);
	}
	~mark_circle ()
	{
		scene ()->removeItem (this);
	}
	void set_size (double size)
	{
		m_size = size;
		setPen (QPen (m_color, line_width ()));
		size *= 0.55;
		/* Do some rounding and ensure an odd size for better centering.  */
		int sz = size / 2;
		sz = sz * 2 + line_width () % 2;
		setRect (0, 0, sz, sz);
	}
	void set_center (double x, double y)
	{
		QRectF br = boundingRect ();
		setPos (x - br.width () / 2 + line_width () / 2, y - br.height () / 2 + line_width () / 2);
	}
	void set_shown (bool do_show)
	{
		if (do_show)
			show ();
		else
			hide ();
	}
};

class mark_triangle : public mark_gfx, public QGraphicsPolygonItem
{
	QPolygon m_poly;

public:
	mark_triangle (QGraphicsScene *canvas, double size, QColor c)
		: mark_gfx (c, mark::triangle, size), m_poly (3)
	{
		set_size (size);
		setZValue (10);
		canvas->addItem (this);
	}
	~mark_triangle ()
	{
		scene ()->removeItem (this);
	}
	void set_size (double size)
	{
		m_size = size;
		setPen (QPen (m_color, line_width ()));

		size *= 0.55;
		int sz = size / 2;

		m_poly.setPoint (0, sz, 0);
		m_poly.setPoint (1, 0, sz * 2);
		m_poly.setPoint (2, sz * 2, sz * 2);
		setPolygon (m_poly);
	}
	void set_center (double x, double y)
	{
		QRectF br = boundingRect ();
		setPos (x - br.width () / 2 + line_width () / 2, y - br.height () / 1.7 + line_width () / 2);
	}
	void set_shown (bool do_show)
	{
		if (do_show)
			show ();
		else
			hide ();
	}
};

class mark_cross : public mark_gfx, public QGraphicsLineItem
{
	QGraphicsLineItem m_other;

public:
	mark_cross (QGraphicsScene *canvas, double size, QColor c, bool as_plus)
		: mark_gfx (c, as_plus ? mark::plus : mark::cross, size)

	{
		set_size (size);
		setZValue (10);
		m_other.setZValue (10);
		canvas->addItem (this);
		canvas->addItem (&m_other);
	}
	~mark_cross ()
	{
		scene ()->removeItem (&m_other);
		scene ()->removeItem (this);
	}
	void set_size (double size)
	{
		m_size = size;
		QPen p (m_color, line_width ());
		setPen (p);
		m_other.setPen (p);

		size *= 0.55;
		if (get_type () == mark::cross)
			size *= 0.767;

		int sz = size / 2;
		if (get_type () == mark::plus) {
			setLine (0, sz, 2 * sz, sz);
			m_other.setLine (sz, 0, sz, size);
		} else {
			setLine (0, 0, sz * 2, sz * 2);
			m_other.setLine (sz * 2, 0, 0, sz * 2);
		}
	}
	void set_center (double x, double y)
	{
		QRectF br = boundingRect ();
		QRectF obr = m_other.boundingRect ();
		setPos (x - br.width () / 2 + line_width () / 2, y - br.height () / 2 + line_width () / 2);
		m_other.setPos (x - obr.width () / 2 + line_width () / 2, y - obr.height () / 2 + line_width () / 2);
	}
	void set_shown (bool do_show)
	{
		if (do_show) {
			show ();
			m_other.show ();
		} else {
			hide ();
			m_other.hide ();
		}
	}
};

class mark_text : public mark_gfx, public QGraphicsSimpleTextItem
{
	QFont m_font;
	int m_len;

public:
	mark_text (QGraphicsScene *canvas, double size, QColor c, const QFont &f, const QString &txt)
		: mark_gfx (c, mark::text, size), QGraphicsSimpleTextItem (txt), m_font (f)
	{
		m_len = txt.length ();
		set_size (size);
		setZValue (10);
		canvas->addItem (this);
	}
	~mark_text ()
	{
		scene ()->removeItem (this);
	}
	/* Change the font size so that we can hope to fit strings up to LEN characters
	   into the space for the mark.  */
	void set_max_len (int len)
	{
		m_len = len;
		set_size (m_size);
	}
	void set_text (const QString &txt)
	{
		m_len = txt.length ();
		setText (txt);
		set_size (m_size);
	}
	void set_size (double size)
	{
		m_size = size;
		setPen (QPen (m_color, 1));
		setBrush (m_color);
		size *= 0.65;
//	if (setting->readBoolEntry("VAR_FONT"))
		double factor = (m_len - 1) * 0.4 + 1;
		int pt_size = size / factor;
		m_font.setPointSize (pt_size);
		setFont (m_font);
	}
	void set_center (double x, double y)
	{
		QRectF br = boundingRect ();
		setPos (x - br.width () / 2, y - br.height () / 2);
	}
	void set_shown (bool do_show)
	{
		if (do_show)
			show ();
		else
			hide ();
	}
};

#endif
