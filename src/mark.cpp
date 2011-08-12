/*
* mark.cpp
*/

#include "mark.h"
#include "imagehandler.h"
#include "qgo.h"
//Added by qt3to4:
#include <Q3PointArray>
#include "setting.h"
#include <qpainter.h>

/*
* Mark
*/

Mark::Mark(int x, int y)
: myX(x), myY(y)
{
}

Mark::~Mark()
{
}

/*
* MarkSquare
*/

MarkSquare::MarkSquare(int x, int y, double size, QGraphicsScene *canvas, QColor col)
: QGraphicsRectItem(0,canvas), Mark(x, y)
{
	double size_d = (double)size;

	if (setting->readBoolEntry("SMALL_MARKS"))
		size_d *= 0.85;

	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
	else
		setPen(QPen(col, 1));
	setSize(size_d, size_d);
	setZValue(10);
}

/*
* MarkCircle
*/

MarkCircle::MarkCircle(int x, int y, double size, QGraphicsScene *canvas, QColor col, bool s)
: QGraphicsEllipseItem(0, canvas), Mark(x, y), small(s)
{
	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
	else
		setPen(QPen(col, 1));
	setSize((double)size, (double)size);
	setZValue(10);
}

void MarkCircle::setSize(double x, double y)
{
	double m;
	if (setting->readBoolEntry("SMALL_MARKS"))
		m = 0.4;
	else
		m = 0.5;

	if (!small)
	{
		x *= 1.25*m;
		y *= 1.25*m;
	}
	else
	{
		x *= m;
		y *= m;
	}
 
	QGraphicsEllipseItem::setRect(rect().x(), rect().y(), x, y);
}

/*
* MarkTriangle
*/
MarkTriangle::MarkTriangle(int x, int y, double s, QGraphicsScene *canvas, QColor col)
: QGraphicsPolygonItem(0, canvas), Mark(x, y)
{
	if (setting->readBoolEntry("BOLD_MARKS"))
		setPen(QPen(col, 2));
	else
		setPen(QPen(col, 1));
	pa = QPolygon(3);
	setSize(s, s);
	setZValue(10);
}

void MarkTriangle::setSize(double w, double)
{
	if (setting->readBoolEntry("SMALL_MARKS"))
		size = (int)(w*0.45);
	else
		size = (int)(w*0.55);
	
	pa.setPoint(0,(int)(size/2), 0);
	pa.setPoint(1,0, (int)size);
	pa.setPoint(2,(int)size, (int)size);
	setPolygon(pa);
}

/*
* MarkCross
*/

MarkCross::MarkCross(int x, int y, double s, QGraphicsScene *canvas, QColor col, bool plus)
: QGraphicsLineItem(0, canvas), Mark(x, y), size(s)
{
	plussign = plus;
	ol = NULL;

	int penWidth;
	if (setting->readBoolEntry("BOLD_MARKS"))
		penWidth = 2;
	else
		penWidth = 1;

	setPen(QPen(col, penWidth));
	setSize((double)s, (double)s);
	setZValue(10);
	
	ol = new MarkOtherLine(canvas);
	if (plussign)
		ol->setLine(0, size/2, size, size/2);
	else
		ol->setLine(0, size, size, 0);
	ol->setPen(QPen(col, penWidth));
	ol->setZValue(10);
	ol->show();
}

MarkCross::~MarkCross()
{
	if (ol != NULL)
		ol->hide();
#if 0
	// qGo2 has a comment about double free.
	delete ol;
#endif
	hide();
}

void MarkCross::setSize(double w, double)
{
	if (setting->readBoolEntry("SMALL_MARKS"))
		size = (int)(w*0.45);
	else
		size = (int)(w*0.55);
	
	if (plussign)
	{
		setLine(size/2, 0, size/2, size);
		if (ol != NULL)
			ol->setLine(0, size/2, size, size/2);
	}
	else
	{
		setLine(0, 0, size, size);
		if (ol != NULL)
			ol->setLine(0, size, size, 0);
	}
}

void MarkCross::setColor(const QColor &col)
{
	QGraphicsLineItem::setPen(col);
	if (ol != NULL)
		ol->setPen(col);
}

/*
* MarkText
*/

bool MarkText::useBold = false;
int MarkText::maxLength = 1;

MarkText::MarkText( int x, int y, double size, const QString &txt,
			 QGraphicsScene *canvas, QColor col, short c, bool bold, bool  /*overlay*/)
			 : QGraphicsSimpleTextItem(txt,0, canvas),
			 Mark(x, y), curSize(size), counter(c)
{
	useBold = bold;
	
	if (text().length() > maxLength)
		maxLength = text().length();
	
	setSize(size, size);
	setColor(col);

	setZValue(10);
}

MarkText::~MarkText()
{
}

void MarkText::setSize(double x, double)
{
	curSize = x;
	
	if (setting->readBoolEntry("SMALL_MARKS"))
		x *= 0.5;
	else
		x *= 0.6;
	
	// adjust font size, so all labels have the same size.
	if (setting->readBoolEntry("ADJ_FONT") && maxLength > 1)
		x /= (double)(maxLength) * 0.6;

	QFont f = setting->fontMarks;
	if (setting->readBoolEntry("VAR_FONT"))
		f.setPointSize((int)x);
	setFont(f);

	width = boundingRect().width();
	height = boundingRect().height();
}
