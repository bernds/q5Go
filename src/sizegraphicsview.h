#ifndef SIZEGRAPHICSVIEW_H
#define SIZEGRAPHICSVIEW_H

/* A collection of a few useful graphics classes, slightly extending their Qt
   base classes.  */

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>

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

class ClickablePixmap : public QGraphicsPixmapItem
{
	std::function<void ()> m_callback;
	std::function<bool (QGraphicsSceneContextMenuEvent *e)> m_menu_callback;
public:
	ClickablePixmap (const QPixmap &pm, std::function<void ()> f, std::function<bool (QGraphicsSceneContextMenuEvent *e)> m)
		: QGraphicsPixmapItem (pm), m_callback (std::move (f)), m_menu_callback (std::move (m))
	{
		/* Supposedly faster.  */
		setShapeMode (QGraphicsPixmapItem::BoundingRectShape);
	}

protected:
	void mousePressEvent (QGraphicsSceneMouseEvent *e) override
	{
		if (e->button () == Qt::LeftButton)
			m_callback ();
		else
			QGraphicsPixmapItem::mousePressEvent (e);
	}
	void contextMenuEvent (QGraphicsSceneContextMenuEvent *e) override
	{
		if (!m_menu_callback (e))
			QGraphicsPixmapItem::contextMenuEvent (e);
	}
};

#endif
