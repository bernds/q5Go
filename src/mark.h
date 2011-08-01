/*
* mark.h
*/

#ifndef MARK_H
#define MARK_H

#include "defines.h"
#include "setting.h"
#include "imagehandler.h"
#include <q3canvas.h>

class Mark
{
public:
	Mark(int x, int y);
	virtual ~Mark();
	virtual int rtti() const = 0;
	virtual MarkType getType() const = 0;
	virtual void setX(double) = 0;
	virtual void setY(double) = 0;
	virtual void setSize(double, double) = 0;
	virtual int getSizeX() const = 0;
	virtual int getSizeY() const = 0;
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

class MarkSquare : public Q3CanvasRectangle, public Mark
{
public:
	MarkSquare(int x, int y, int size, Q3Canvas *canvas, QColor col=Qt::black);
	virtual ~MarkSquare() { hide(); }
	virtual int rtti() const { return RTTI_MARK_SQUARE; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setX(double x) { Q3CanvasRectangle::setX(x); }
	virtual void setY(double y) { Q3CanvasRectangle::setY(y); }
	virtual void setSize(double x, double y) { Q3CanvasRectangle::setSize((int)(x*0.55), (int)(y*0.55)); }
	virtual int getSizeX() const { return Q3CanvasRectangle::width(); }
	virtual int getSizeY() const { return Q3CanvasRectangle::height(); }
	virtual void show() { Q3CanvasRectangle::show(); }
	virtual void hide() { Q3CanvasRectangle::hide(); }
	virtual void setColor(const QColor &col) { Q3CanvasRectangle::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return Q3CanvasRectangle::pen().color(); }
};

class MarkCircle : public Q3CanvasEllipse, public Mark
{
public:
	MarkCircle(int x, int y, int size, Q3Canvas *canvas, QColor col=Qt::black, bool s=false);
	virtual ~MarkCircle() { hide(); }
	virtual int rtti() const { return RTTI_MARK_CIRCLE; }
	virtual MarkType getType() const { return markCircle; }
	virtual void setX(double x) { Q3CanvasEllipse::setX(x + width()/2.0); }
	virtual void setY(double y) { Q3CanvasEllipse::setY(y + width()/2.0); }
	virtual void setSize(double x, double y);
	virtual int getSizeX() const { return Q3CanvasEllipse::width(); }
	virtual int getSizeY() const { return Q3CanvasEllipse::height(); }
	virtual void show() { Q3CanvasEllipse::show(); }
	virtual void hide() { Q3CanvasEllipse::hide(); }
	virtual void setColor(const QColor &col) { Q3CanvasEllipse::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return Q3CanvasEllipse::pen().color(); }
	virtual void setSmall(bool s) { small = s; }
	
protected:
	virtual void drawShape(QPainter & p);
	bool small;
};

class MarkRedCircle : public MarkCircle // QQQ
{
public:
	MarkRedCircle(int x, int y, int size, Q3Canvas *canvas, QColor col=Qt::black, bool s=false)
	: MarkCircle(x,y,size,canvas,QColor(255,0,0),s) {}
	virtual ~MarkRedCircle() { hide(); }
};

class MarkTriangle : public Q3CanvasPolygon, public Mark
{
public:
	MarkTriangle(int x, int y, int size, Q3Canvas *canvas, QColor col=Qt::black);
	virtual ~MarkTriangle() { hide(); }
	virtual int rtti() const { return RTTI_MARK_TRIANGLE; }
	virtual MarkType getType() const { return markTriangle; }
	virtual void setX(double x) { Q3CanvasPolygon::setX(x); }
	virtual void setY(double y) { Q3CanvasPolygon::setY(y + size-1); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return size; }
	virtual int getSizeY() const { return size; }
	virtual void show() { Q3CanvasPolygon::show(); }
	virtual void hide() { Q3CanvasPolygon::hide(); }
	virtual void setColor(const QColor &col) { Q3CanvasPolygon::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return Q3CanvasPolygon::pen().color(); }
	
protected:
	virtual void drawShape(QPainter & p);
	
private:
	int size;
};

// Need to have a line with another rtti to differ this from the gatter lines
class MarkOtherLine : public Q3CanvasLine
{
public:
	MarkOtherLine(Q3Canvas *canvas)
		: Q3CanvasLine(canvas)
	{ }
	int rtti() const { return RTTI_MARK_OTHERLINE; }
};

class MarkCross : public Q3CanvasLine, public Mark
{
public:
	MarkCross(int x, int y, int s, Q3Canvas *canvas, QColor col=Qt::black, bool plus=false);
	virtual ~MarkCross();
	virtual int rtti() const { return RTTI_MARK_CROSS; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setX(double x) { Q3CanvasLine::setX(x); if (ol != NULL) ol->setX(x); }
	virtual void setY(double y) { Q3CanvasLine::setY(y); if (ol != NULL) ol->setY(y); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return size; }
	virtual int getSizeY() const { return size; }
	virtual void show() { Q3CanvasLine::show(); }
	virtual void hide() { Q3CanvasLine::hide(); }
	virtual void setColor(const QColor &col);
	virtual const QColor getColor() { return Q3CanvasLine::pen().color(); }
	
private:
	MarkOtherLine *ol;
	int size;
	bool plussign;
};

class MarkText : public Q3CanvasText, public Mark
{
public:
	MarkText(ImageHandler *ih, int x, int y, int size, const QString &txt, Q3Canvas *canvas, QColor col=Qt::black,
		short c=-1, bool bold=false, bool overlay=true);
	virtual ~MarkText();
	virtual int rtti() const { return RTTI_MARK_TEXT; }
	virtual MarkType getType() const { return markText; }
	virtual void setX(double x) { Q3CanvasText::setX(x); if (rect != NULL) rect->setX(x); }
	virtual void setY(double y) { Q3CanvasText::setY(y); if (rect != NULL) rect->setY(y); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return width; }
	virtual int getSizeY() const { return height; }
	virtual void show() { Q3CanvasText::show(); }
	virtual void hide() { Q3CanvasText::hide(); }
	virtual void setColor(const QColor &col) { Q3CanvasText::setColor(col); }
	virtual const QColor getColor() { return Q3CanvasText::color(); }
	virtual void setText(const QString &txt) { Q3CanvasText::setText(txt);}
	virtual short getCounter() { return counter; }
	virtual void setCounter(short n) { counter = n; }

	static unsigned int maxLength;


private:
	Q3CanvasRectangle *rect;
	int width, height;
	double curSize;
	short counter;
	static bool useBold;
};

class MarkNumber : public MarkText
{
public:
	MarkNumber(ImageHandler *ih, int x, int y, int size, short num, Q3Canvas *canvas, QColor col=Qt::black, bool bold=false)
		: MarkText(ih, x, y, size, QString::number(num+1), canvas, col, num, bold)
	{ }
	virtual int rtti() const { return RTTI_MARK_NUMBER; }
	virtual MarkType getType() const { return markNumber; }
};

class MarkTerr : public MarkCross
{
public: 
	MarkTerr(int x, int y, int s, StoneColor c, Q3Canvas *canvas)
		: MarkCross(x, y, s, canvas, (c == stoneBlack ? Qt::black : Qt::white)),
		col(c)
	{ }
	virtual ~MarkTerr() { }
	virtual int rtti() const { return RTTI_MARK_TERR; }
	virtual MarkType getType() const { if (col == stoneBlack) return markTerrBlack; else return markTerrWhite; }
private:
	StoneColor col;
};

#endif
