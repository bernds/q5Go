#include "qgo.h"
#include "defines.h"
#include "mainwidget.h"

ClockView::ClockView (QWidget *parent)
	: QGraphicsView (parent)
{
	setFocusPolicy (Qt::NoFocus);
	QFontMetrics m (setting->fontClocks);
	int w = width ();
	int h = m.height ();
	setMinimumHeight (h);
	setMaximumHeight (h);
	m_scene = new QGraphicsScene (0, 0, w, h, this);
	setScene (m_scene);
	m_text = m_scene->addText ("00:00", setting->fontClocks);
	m_text->setDefaultTextColor (Qt::white);
	update_pos ();
}

void ClockView::resizeEvent (QResizeEvent *)
{
	update_pos ();
}

void ClockView::mousePressEvent (QMouseEvent *e)
{
	if (e->button () == Qt::LeftButton)
		emit clicked ();
}

void ClockView::update_pos ()
{
	QRectF br = m_text->boundingRect ();
	m_text->setPos ((width () - br.width ()) / 2, (height () - br.height ()) / 2);
	m_scene->update ();
}

void ClockView::update_font (const QFont &f)
{
	m_text->setFont (f);
	QFontMetrics m (f);
	int h = m.height ();
	setMinimumHeight (h);
	setMaximumHeight (h);

	update_pos ();
}

void ClockView::set_text (const QString &s)
{
	m_text->setPlainText (s);
	update_pos ();
}

void ClockView::flash (bool on)
{
	setBackgroundBrush (QBrush (on ? Qt::red : Qt::black));
	m_text->setDefaultTextColor (on ? Qt::black : Qt::white);
}

