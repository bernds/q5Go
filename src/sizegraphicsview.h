#ifndef SIZEGRAPHICSVIEW_H
#define SIZEGRAPHICSVIEW_H

#include <QGraphicsView>

class SizeGraphicsView: public QGraphicsView
{
	Q_OBJECT

signals:
	void resized ();
protected:
	virtual void resizeEvent(QResizeEvent*) override
	{
		emit resized ();
	}
public:
	SizeGraphicsView (QWidget *parent) : QGraphicsView (parent)
	{
	}
};

class AspectContainer: public QWidget
{
	Q_OBJECT
	double m_aspect = 1;
	QWidget *m_child {};

	void fix_aspect ();
protected:
	virtual void resizeEvent (QResizeEvent *e) override
	{
		fix_aspect ();
		QWidget::resizeEvent (e);
	}
public:
	using QWidget::QWidget;
	void set_aspect (double v) { m_aspect = v == 0 ? 1 : v; fix_aspect (); }
	void set_child (QWidget *c) { m_child = c; fix_aspect (); }
};

#endif
