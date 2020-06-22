#include <QGraphicsTextItem>
#include <QMouseEvent>

#include "defines.h"
#include "gogame.h"
#include "setting.h"
#include "clockview.h"

/* Convert SGF style time information to something we can display on the clock.  */
static QString time_string (const std::string &timeleft, const std::string &stonesleft)
{
	QString timestr;
	if (timeleft.length () == 0)
		return QString ();
	try {
		double t = stod (timeleft);
		int seconds = (int)t;
		bool neg = seconds < 0;
		if (neg)
			seconds = -seconds;

		int h = seconds / 3600;
		seconds -= h*3600;
		int m = seconds / 60;
		int s = seconds - m*60;

		QString sec = QString::number (s);
		QString min = QString::number (m);
		if ((h || m) && s < 10)
			sec = "0" + sec;
		if (h)
		{
			if (m < 10)
				min = "0" + min;
			timestr = (neg ? "-" : "") + QString::number(h) + ":" + min + ":" + sec;
		} else
			timestr = (neg ? "-" : "") + min + ":" + sec;

	} catch (...) {
		return QString ();
	}
	if (stonesleft.length () > 0)
		timestr += " / " + QString::fromStdString (stonesleft);
	return timestr;
}


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

void ClockView::set_time (const game_state *gs, stone_color col)
{
	QString tstr = time_string (gs->time_left (col), gs->stones_left (col));
	if (tstr.isNull () && !gs->root_node_p ()) {
		tstr = time_string (gs->prev_move ()->time_left (col),
				    gs->prev_move ()->stones_left (col));
	}
	set_text (tstr.isNull () ? "--:--" : tstr);
}

void ClockView::flash (bool on)
{
	setBackgroundBrush (QBrush (on ? Qt::red : Qt::black));
	m_text->setDefaultTextColor (on ? Qt::black : Qt::white);
}

