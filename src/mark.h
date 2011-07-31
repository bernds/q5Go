/*
* mark.h
*/

#ifndef MARK_H
#define MARK_H

#include "defines.h"
#include "setting.h"
#include "imagehandler.h"
#include <qcanvas.h>

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

class MarkSquare : public QCanvasRectangle, public Mark
{
public:
	MarkSquare(int x, int y, int size, QCanvas *canvas, QColor col=black);
	virtual ~MarkSquare() { hide(); }
	virtual int rtti() const { return RTTI_MARK_SQUARE; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setX(double x) { QCanvasRectangle::setX(x); }
	virtual void setY(double y) { QCanvasRectangle::setY(y); }
	virtual void setSize(double x, double y) { QCanvasRectangle::setSize((int)(x*0.55), (int)(y*0.55)); }
	virtual int getSizeX() const { return QCanvasRectangle::width(); }
	virtual int getSizeY() const { return QCanvasRectangle::height(); }
	virtual void show() { QCanvasRectangle::show(); }
	virtual void hide() { QCanvasRectangle::hide(); }
	virtual void setColor(const QColor &col) { QCanvasRectangle::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return QCanvasRectangle::pen().color(); }
};

class MarkCircle : public QCanvasEllipse, public Mark
{
public:
	MarkCircle(int x, int y, int size, QCanvas *canvas, QColor col=black, bool s=false);
	virtual ~MarkCircle() { hide(); }
	virtual int rtti() const { return RTTI_MARK_CIRCLE; }
	virtual MarkType getType() const { return markCircle; }
	virtual void setX(double x) { QCanvasEllipse::setX(x + width()/2.0); }
	virtual void setY(double y) { QCanvasEllipse::setY(y + width()/2.0); }
	virtual void setSize(double x, double y);
	virtual int getSizeX() const { return QCanvasEllipse::width(); }
	virtual int getSizeY() const { return QCanvasEllipse::height(); }
	virtual void show() { QCanvasEllipse::show(); }
	virtual void hide() { QCanvasEllipse::hide(); }
	virtual void setColor(const QColor &col) { QCanvasEllipse::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return QCanvasEllipse::pen().color(); }
	virtual void setSmall(bool s) { small = s; }
	
protected:
	virtual void drawShape(QPainter & p);
	bool small;
};

class MarkRedCircle : public MarkCircle // QQQ
{
public:
	MarkRedCircle(int x, int y, int size, QCanvas *canvas, QColor col=black, bool s=false)
	: MarkCircle(x,y,size,canvas,QColor(255,0,0),s) {}
	virtual ~MarkRedCircle() { hide(); }
};

class MarkTriangle : public QCanvasPolygon, public Mark
{
public:
	MarkTriangle(int x, int y, int size, QCanvas *canvas, QColor col=black);
	virtual ~MarkTriangle() { hide(); }
	virtual int rtti() const { return RTTI_MARK_TRIANGLE; }
	virtual MarkType getType() const { return markTriangle; }
	virtual void setX(double x) { QCanvasPolygon::setX(x); }
	virtual void setY(double y) { QCanvasPolygon::setY(y + size-1); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return size; }
	virtual int getSizeY() const { return size; }
	virtual void show() { QCanvasPolygon::show(); }
	virtual void hide() { QCanvasPolygon::hide(); }
	virtual void setColor(const QColor &col) { QCanvasPolygon::setPen(QPen(col, 1)); }
	virtual const QColor getColor() { return QCanvasPolygon::pen().color(); }
	
protected:
	virtual void drawShape(QPainter & p);
	
private:
	int size;
};

// Need to have a line with another rtti to differ this from the gatter lines
class MarkOtherLine : public QCanvasLine
{
public:
	MarkOtherLine(QCanvas *canvas)
		: QCanvasLine(canvas)
	{ }
	int rtti() const { return RTTI_MARK_OTHERLINE; }
};

class MarkCross : public QCanvasLine, public Mark
{
public:
	MarkCross(int x, int y, int s, QCanvas *canvas, QColor col=black, bool plus=false);
	virtual ~MarkCross();
	virtual int rtti() const { return RTTI_MARK_CROSS; }
	virtual MarkType getType() const { return markSquare; }
	virtual void setX(double x) { QCanvasLine::setX(x); if (ol != NULL) ol->setX(x); }
	virtual void setY(double y) { QCanvasLine::setY(y); if (ol != NULL) ol->setY(y); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return size; }
	virtual int getSizeY() const { return size; }
	virtual void show() { QCanvasLine::show(); }
	virtual void hide() { QCanvasLine::hide(); }
	virtual void setColor(const QColor &col);
	virtual const QColor getColor() { return QCanvasLine::pen().color(); }
	
private:
	MarkOtherLine *ol;
	int size;
	bool plussign;
};

class MarkText : public QCanvasText, public Mark
{
public:
	MarkText(ImageHandler *ih, int x, int y, int size, const QString &txt, QCanvas *canvas, QColor col=black,
		short c=-1, bool bold=false, bool overlay=true);
	virtual ~MarkText();
	virtual int rtti() const { return RTTI_MARK_TEXT; }
	virtual MarkType getType() const { return markText; }
	virtual void setX(double x) { QCanvasText::setX(x); if (rect != NULL) rect->setX(x); }
	virtual void setY(double y) { QCanvasText::setY(y); if (rect != NULL) rect->setY(y); }
	virtual void setSize(double, double);
	virtual int getSizeX() const { return width; }
	virtual int getSizeY() const { return height; }
	virtual void show() { QCanvasText::show(); }
	virtual void hide() { QCanvasText::hide(); }
	virtual void setColor(const QColor &col) { QCanvasText::setColor(col); }
	virtual const QColor getColor() { return QCanvasText::color(); }
	virtual void setText(const QString &txt) { QCanvasText::setText(txt);}
	virtual short getCounter() { return counter; }
	virtual void setCounter(short n) { counter = n; }

	static unsigned int maxLength;


private:
	QCanvasRectangle *rect;
	int width, height;
	double curSize;
	short counter;
	static bool useBold;
};

class MarkNumber : public MarkText
{
public:
	MarkNumber(ImageHandler *ih, int x, int y, int size, short num, QCanvas *canvas, QColor col=black, bool bold=false)
		: MarkText(ih, x, y, size, QString::number(num+1), canvas, col, num, bold)
	{ }
	virtual int rtti() const { return RTTI_MARK_NUMBER; }
	virtual MarkType getType() const { return markNumber; }
};

class MarkTerr : public MarkCross
{
public: 
	MarkTerr(int x, int y, int s, StoneColor c, QCanvas *canvas)
		: MarkCross(x, y, s, canvas, (c == stoneBlack ? black : white)),
		col(c)
	{ }
	virtual ~MarkTerr() { }
	virtual int rtti() const { return RTTI_MARK_TERR; }
	virtual MarkType getType() const { if (col == stoneBlack) return markTerrBlack; else return markTerrWhite; }
private:
	StoneColor col;
};

#endif
