/*
* gametree.cpp
*/

#include "config.h"
#include "setting.h"
#include "goboard.h"
#include "gogame.h"
#include "evalgraph.h"
#include "mainwindow.h"
#include "svgbuilder.h"

#define GRADIENT_WIDTH 16

EvalGraph::EvalGraph (QWidget *parent)
	: QGraphicsView (parent)
{
	setFocusPolicy (Qt::NoFocus);
	m_scene = new QGraphicsScene (0, 0, 30, 30, this);
	setScene (m_scene);

	int w = width ();
	int h = height ();

	QLinearGradient gradient (QPointF (0, 0), QPointF (0, 1));
	gradient.setColorAt (0, Qt::white);
	gradient.setColorAt (1, Qt::black);
	gradient.setCoordinateMode (QGradient::StretchToDeviceMode);
	QBrush gbr (gradient);
	m_grect = m_scene->addRect (0, 0, GRADIENT_WIDTH, h, Qt::NoPen, gbr);
	m_grect->setZValue (-2);

	m_line = m_scene->addLine (0, h / 2, w, h / 2);
	setAlignment (Qt::AlignTop | Qt::AlignLeft);

	setToolTip (tr ("The evaluation graph.\nDisplays evaluation data found in the game record."));
}

void EvalGraph::update_prefs ()
{
}

void EvalGraph::mousePressEvent (QMouseEvent *e)
{
	if (m_game == nullptr || e->button () != Qt::LeftButton)
		return;

	game_state *st = m_game->get_root ();
	int pos = e->x () - GRADIENT_WIDTH;
	if (pos < 0)
		return;

	pos /= m_step;
	for (int i = 0; st && i < pos; i++) {
		st = st->next_primary_move ();
	}
	if (st && m_active != st)
		m_win->set_game_position (st);
}

void EvalGraph::resizeEvent (QResizeEvent*)
{
	update (m_game, m_active);
}

QSize EvalGraph::sizeHint () const
{
	return QSize (100, 100);
}

void EvalGraph::show_menu (int x, int y, const QPoint &pos)
{
#if 0
	game_state *st = m_game->get_root ()->locate_by_vis_coords (x, y, 0, 0);
	QMenu menu;
	if (st->vis_collapsed ()) {
		menu.addAction (QIcon (m_pm_box), QObject::tr ("Expand subtree"), [=] () { toggle_collapse (x, y, false); });
		menu.addAction (QObject::tr ("Expand one level of child nodes"), [=] () { toggle_collapse (x, y, true); });
	} else
		menu.addAction (QIcon (m_pm_box), QObject::tr ("Collapse subtree"), [=] () { toggle_collapse (x, y, false); });
	if (st->has_figure ())
		menu.addAction (QIcon (":/BoardWindow/images/boardwindow/figure.png"),
				QObject::tr("Clear diagram status for this node"),
				[=] () { toggle_figure (x, y); m_win->update_figures (); });
	else
		menu.addAction (QIcon (":/BoardWindow/images/boardwindow/figure.png"),
				QObject::tr("Set this move to be the start of a diagram"),
				[=] () { toggle_figure (x, y); m_win->update_figures (); });
	menu.addAction (QObject::tr ("Navigate to this node"), [=] () { item_clicked (x, y); });
	menu.exec (pos);
#endif
}

void EvalGraph::update (std::shared_ptr<game_record> gr, game_state *active)
{
	int w = width ();
	int h = height ();
	m_line->setLine (0, h / 2, w, h / 2);
	m_grect->setRect (0, 0, GRADIENT_WIDTH, h);

	if (gr == nullptr)
		return;

	w -= GRADIENT_WIDTH;

	game_state *r = gr->get_root ();

	m_game = gr;
	m_active = active;

	size_t count = 0;
	int active_point = -1;
	for (game_state *st = r; st != nullptr; st = st->next_primary_move ()) {
		if (active == st)
			active_point = count;
		count++;
	}

	m_step = (double)w / count;

	QPainterPath path;
	bool on_path = false;
	game_state *st = r;
	for (int x = 0;; x++) {
		int v = st == nullptr ? 0 : st->eval_visits ();
		if (v > 0) {
			double wr = st->eval_wr_black ();
			if (on_path)
				path.lineTo (GRADIENT_WIDTH + x * m_step, h * wr);
			else
				path.moveTo (GRADIENT_WIDTH + x * m_step, h * wr);
		}
		on_path = v > 0;

		if (st == nullptr)
			break;
		st = st->next_primary_move ();
	}

	QPen pen;
	pen.setWidth (2);
	pen.setColor (Qt::blue);
	delete m_path;
	m_path = m_scene->addPath (path, pen);
	m_path->setZValue (3);

	delete m_sel;
	if (active_point == -1) {
		m_sel = nullptr;
	} else {
		m_sel = m_scene->addRect (GRADIENT_WIDTH + active_point * m_step, 0, m_step, h,
					  Qt::NoPen, QBrush (Qt::gray));
		m_sel->setZValue (-1);
	}
	m_scene->update ();
}
