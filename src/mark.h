/*
* mark.h
*/

#ifndef MARK_H
#define MARK_H

#include "defines.h"
#include "setting.h"
#include "graphicsitemstypes.h"
#include "imagehandler.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPolygonItem>
#include <QPen>
#include <QPainter>

class Mark
{
public:
	Mark(int x, int y);
	virtual ~Mark();
	virtual int type() const = 0;
	virtual MarkType getType() const = 0;
	virtual void setPos(double,double) = 0;
//	virtual void setY(double) = 0;
	virtual void setSize(double, double) = 0;
	virtual double getSizeX() const = 0;
	virtual double getSizeY() const = 0;
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void setColor(const QColor &col) = 0;
	virtual const QColor getColor() = 0;
	virtual void setText(const QString&) { }
	virtual short getCounter() { return -1; }
	virtual void setCounter(short) { }
	int posX() const { return myX; }
	int posY() const { return myY; }
	virtual void setSmall(bool) { }
//	QCanvasRectangle *rect;
protected:
	int myX, myY;


};

class MarkSquare : public QGraphicsRectItem, public Mark
{
public:
	MarkSquare(int x, int y, double size, QGraphicsScene *canvas, QColor col=Qt::black);
	virtual ~MarkSquare() {/* hide(); */}
	virtual int type() const { return RTTI_MARK_SQUARE; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setPos(double x, double y) { QGraphicsRectItem::setPos((qreal)x, (qreal)y); }
//	virtual void setY(double y) { QGraphicsRectItem::setY(y); }
	virtual void setSize(double x, double y) { setRect(rect().x()/*myX*/,rect().y()/* myY*/, x*0.5, y*0.5); }
	virtual double getSizeX() const { return (int) rect().width(); }
	virtual double getSizeY() const { return (int) rect().height(); }
	virtual void show() { QGraphicsRectItem::show(); }
	virtual void hide() { QGraphicsRectItem::hide(); }
	virtual void setColor(const QColor &col) { setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return pen().color(); }
};

class MarkCircle : public QGraphicsEllipseItem, public Mark
{
public:
	MarkCircle(int x, int y, double size, QGraphicsScene *canvas, QColor col=Qt::black, bool s=false);
	virtual ~MarkCircle() { /*hide();*/ }
	virtual int type() const { return RTTI_MARK_CIRCLE; }
	virtual MarkType getType() const { return markCircle; }
	virtual void setPos(double x, double y) { QGraphicsEllipseItem::setPos((qreal)x /*- rect().width()/2.0*/, (qreal)y /*- rect().height()*/); }
//	virtual void setX(double x) { QGraphicsEllipseItem::setX(x + width()/2.0); }
//	virtual void setY(double y) { QGraphicsEllipseItem::setY(y + width()/2.0); }
	virtual void setSize(double x, double y);
	virtual double getSizeX() const { return (int)rect().width(); }
	virtual double getSizeY() const { return (int)rect().height(); }
	virtual void show() { QGraphicsEllipseItem::show(); }
	virtual void hide() { QGraphicsEllipseItem::hide(); }
	virtual void setColor(const QColor &col) { setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return pen().color(); }
	virtual void setSmall(bool s) { small = s; }
	
protected:
//	virtual void drawShape(QPainter & p);
	void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) { painter->setRenderHints(QPainter::Antialiasing); QGraphicsEllipseItem::paint(painter, option, widget);}
	bool small;
};

class MarkRedCircle : public MarkCircle // QQQ
{
public:
	MarkRedCircle(int x, int y, int size, QGraphicsScene *canvas)
	: MarkCircle(x,y,size,canvas,QColor(255,0,0), false) {}
	virtual ~MarkRedCircle() { hide(); }
};

class MarkTriangle : public QGraphicsPolygonItem, public Mark
{
public:
	MarkTriangle(int x, int y, double size, QGraphicsScene *canvas, QColor col=Qt::black);
	virtual ~MarkTriangle() { /*hide();*/ }
	virtual int type() const { return RTTI_MARK_TRIANGLE; }
	virtual MarkType getType() const { return markTriangle; }
	virtual void setPos(double x, double y) { QGraphicsPolygonItem::setPos((qreal)x , (qreal)y); }
//	virtual void setX(double x) { QGraphicsPolygonItem::setX(x); }
//	virtual void setY(double y) { QGraphicsPolygonItem::setY(y + size-1); }
	virtual void setSize(double, double);
	virtual double getSizeX() const { return size; }
	virtual double getSizeY() const { return size; }
	virtual void show() { QGraphicsPolygonItem::show(); }
	virtual void hide() { QGraphicsPolygonItem::hide(); }
	virtual void setColor(const QColor &col) { setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return pen().color(); }
	
protected:
//	virtual void drawShape(QPainter & p);
	void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) { painter->setRenderHints(QPainter::Antialiasing); QGraphicsPolygonItem::paint(painter, option, widget);}
private:
	double size;
	QPolygon pa;
};

// Need to have a line with another rtti to differ this from the gatter lines
class MarkOtherLine : public QGraphicsLineItem
{
public:
	MarkOtherLine(QGraphicsScene *canvas)
		: QGraphicsLineItem(0,canvas)
	{ }
	int type() const { return RTTI_MARK_OTHERLINE; }
};

class MarkCross : public QGraphicsLineItem, public Mark
{
public:
	MarkCross(int x, int y, double s, QGraphicsScene *canvas, QColor col=Qt::black, bool plus=false);
	virtual ~MarkCross();
	virtual int type() const { return RTTI_MARK_CROSS; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setPos(double x, double y) { QGraphicsLineItem::setPos((qreal)x , (qreal)y); if (ol != NULL) ol->setPos(x,y); }
//	virtual void setX(double x) { QGraphicsLineItem::setX(x); if (ol != NULL) ol->setX(x); }
//	virtual void setY(double y) { QGraphicsLineItem:setY(y); if (ol != NULL) ol->setY(y); }
	virtual void setSize(double, double);
	virtual double getSizeX() const { return size; }
	virtual double getSizeY() const { return size; }
	virtual void show() { QGraphicsLineItem::show(); }
	virtual void hide() { QGraphicsLineItem::hide(); }
	virtual void setColor(const QColor &col);
	virtual const QColor getColor() { return pen().color(); }
	
private:
	MarkOtherLine *ol;
	double size;
	bool plussign;
};

class MarkText : public QGraphicsSimpleTextItem, public Mark
{
public:
	MarkText( int x, int y, double size, const QString &txt, QGraphicsScene *canvas, QColor col=Qt::black,
		short c=-1, bool bold=false, bool overlay=true);
	virtual ~MarkText();
	virtual int type() const { return RTTI_MARK_TEXT; }
	virtual MarkType getType() const { return markText; }
	virtual void setPos(double x, double y) { QGraphicsSimpleTextItem::setPos((qreal)x , (qreal)y);/* if (rect != NULL) rect->setPos(x,y);*/ }
//	virtual void setX(double x) { QGraphicsRectItem::setX(x); if (rect != NULL) rect->setX(x); }
//	virtual void setY(double y) { QGraphicsRectItem::setY(y); if (rect != NULL) rect->setY(y); }
	virtual void setSize(double, double);
	virtual double getSizeX() const { return width; }
	virtual double getSizeY() const { return height; }
	virtual void show() { QGraphicsSimpleTextItem::show(); }
	virtual void hide() { QGraphicsSimpleTextItem::hide(); }
	virtual void setColor(const QColor &col) { setPen(QPen(col, 1));}//setColor(col); }
	virtual const QColor getColor() { return  pen().color();}//QCanvasText::color(); }
//	virtual void setText(const QString &txt) { QCanvasText::setText(txt);}
	virtual short getCounter() { return counter; }
	virtual void setCounter(short n) { counter = n; }

	static int maxLength;

protected:
	void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) { painter->setRenderHints(QPainter::SmoothPixmapTransform); QGraphicsSimpleTextItem::paint(painter, option, widget);}

private:
//	QGraphicsRectItem *rect;
	int width, height;
	double curSize;
	short counter;
	static bool useBold;
};

class MarkNumber : public MarkText
{
public:
	MarkNumber( int x, int y, double size, short num, QGraphicsScene *canvas, QColor col=Qt::black, bool bold=false)
		: MarkText( x, y, size, QString::number(num+1), canvas, col, num, bold)
	{ }
	virtual int type() const { return RTTI_MARK_NUMBER; }
	virtual MarkType getType() const { return markNumber; }
};

class MarkTerr : public MarkCross
{
public: 
	MarkTerr(int x, int y, double s, StoneColor c, QGraphicsScene *canvas)
		: MarkCross(x, y, s, canvas, (c == stoneBlack ? Qt::black :  Qt::white)),
		col(c)
	{ }
	virtual ~MarkTerr() { }
	virtual int type() const { return RTTI_MARK_TERR; }
	virtual MarkType getType() const { if (col == stoneBlack) return markTerrBlack; else return markTerrWhite; }
private:
	StoneColor col;
};

class MarkSmallStoneTerr : public Mark, public QGraphicsPixmapItem
{
	public: 
		MarkSmallStoneTerr(int x, int y, double s, StoneColor c, QList<QPixmap> * p, QGraphicsScene *canvas);
		virtual ~MarkSmallStoneTerr() {};
		virtual int type() const { return RTTI_MARK_TERR; }
		virtual MarkType getType() const { if (col == stoneBlack) return markTerrBlack; else return markTerrWhite; }
		int getX() const { return _x; };
		int getY() const { return _y; };
		virtual void setPos(double x, double y) { QGraphicsPixmapItem::setPos((qreal)x , (qreal)y); }
		virtual void setSize(double, double ) {};
		virtual double getSizeX() const { return size; }
		virtual double getSizeY() const { return size; }
		virtual void show() { QGraphicsPixmapItem::show(); };
		virtual void hide() { QGraphicsPixmapItem::hide(); };
		virtual void setColor(const QColor &) {};
		virtual const QColor getColor() { return QColor(0, 0, 0);}
		void setPixmap(QList <QPixmap> * p);
	private:
		int _x, _y;
		StoneColor col;
		double size;
};

#endif
