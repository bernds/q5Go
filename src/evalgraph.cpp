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
	m_brush = new QBrush (gradient);
	QGraphicsRectItem *r = m_scene->addRect (0, 0, GRADIENT_WIDTH, h, Qt::NoPen, *m_brush);
	r->setZValue (-2);
	m_scene->addLine (0, h / 2, w, h / 2);
	setAlignment (Qt::AlignTop | Qt::AlignLeft);

	setToolTip (tr ("The evaluation graph.\nDisplays evaluation data found in the game record."));
}

void EvalGraph::update_prefs ()
{
}

void EvalGraph::mousePressEvent (QMouseEvent *e)
{
	if (m_game == nullptr || e->buttons () != Qt::LeftButton)
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

void EvalGraph::mouseMoveEvent (QMouseEvent *e)
{
	mousePressEvent (e);
}

void EvalGraph::resizeEvent (QResizeEvent*)
{
	update (m_game, m_active, m_id_idx);
}

QSize EvalGraph::sizeHint () const
{
	return QSize (100, 100);
}

void EvalGraph::contextMenuEvent (QContextMenuEvent *e)
{
	QMenu menu;
	menu.addAction (tr ("Export image to clipboard"), this, &EvalGraph::export_clipboard);
	menu.addAction (tr ("Export image to file"), this, &EvalGraph::export_file);
	menu.addSeparator ();
	QAction *a1 = menu.addAction (tr ("Show scores"), this, &EvalGraph::show_scores);
	a1->setCheckable (true);
	QAction *a2 = menu.addAction (tr ("Show winrates"), this, &EvalGraph::show_winrates);
	a2->setCheckable (true);
	QActionGroup *g = new QActionGroup (this);
	g->addAction (a1);
	g->addAction (a2);
	if (m_show_scores)
		a1->setChecked (true);
	else
		a2->setChecked (true);
	menu.exec (e->globalPos ());
}

void EvalGraph::show_scores (bool)
{
	m_show_scores = true;
	update (m_game, m_active, m_id_idx);
}

void EvalGraph::show_winrates (bool)
{
	m_show_scores = false;
	update (m_game, m_active, m_id_idx);
}

void EvalGraph::export_clipboard (bool)
{
	QPixmap pm = grab ();
	QApplication::clipboard()->setPixmap (pm);
}

void EvalGraph::export_file (bool)
{
	QString filter;
	QString fileName = QFileDialog::getSaveFileName (this, tr ("Export evaluation graph image as"),
							 setting->readEntry ("LAST_DIR"),
							 "PNG (*.png);;BMP (*.bmp);;XPM (*.xpm);;XBM (*.xbm);;PNM (*.pnm);;GIF (*.gif);;JPG (*.jpg);;MNG (*.mng)",
							 &filter);


	if (fileName.isEmpty())
		return;

	filter.truncate (3);
	QPixmap pm = grab ();
	if (!pm.save (fileName, filter.toLatin1 ()))
		QMessageBox::warning (this, PACKAGE, tr("Failed to save image!"));
}

void EvalGraph::update (go_game_ptr gr, game_state *active, int sel_idx)
{
	int w = width ();
	int h = height ();

	m_scene->clear ();
	QGraphicsRectItem *gradr = m_scene->addRect (0, 0, GRADIENT_WIDTH, h, Qt::NoPen, *m_brush);
	gradr->setZValue (-2);
	QGraphicsLineItem *line = m_scene->addLine (0, h / 2, w, h / 2);
	line->setZValue (2);

	if (gr == nullptr)
		return;

	w -= GRADIENT_WIDTH;

	game_state *r = gr->get_root ();

	m_game = gr;
	m_active = active;
	m_id_idx = sel_idx;

	size_t count = 0;
	int active_point = -1;
	for (game_state *st = r; st != nullptr; st = st->next_primary_move ()) {
		if (active == st)
			active_point = count;
		count++;
	}

	m_step = (double)w / count;

	QGraphicsTextItem *type = m_scene->addText (m_show_scores ? tr ("Score") : tr ("Win rate"));
	QRectF trect = type->boundingRect ();
	type->setPos (w - trect.width (), h - trect.height ());
	type->setZValue (4);
	for (int idnr = 0; idnr < m_model->rowCount (); idnr++) {
		if (m_show_scores && idnr != sel_idx)
			continue;
		auto e = m_model->entries ()[idnr];
		auto id = e.first;
		QPainterPath path;
		bool on_path = false;
		game_state *st = r;
		double prev = 0;

		QPen pen;
		pen.setWidth (2);
		QVariant v = m_model->data (m_model->index (idnr, 0), Qt::DecorationRole);
		pen.setColor (v.value<QColor> ());
		QBrush sdbrush (v.value<QColor> ().lighter ());
		for (int x = 0; st != nullptr; x++, st = st->next_primary_move ()) {
			eval ev;
			if (!st->find_eval (id, ev)) {
				on_path = false;
				continue;
			}
			double val;
			if (m_show_scores) {
				if (ev.score_stddev == 0) {
					on_path = false;
					continue;
				}
				val = (ev.score_mean + 15.) / 30;
				double vmin = val - ev.score_stddev / 30.;
				double vmax = val + ev.score_stddev / 30.;
				val = std::min (1.0, std::max (val, 0.0));
				double vminb = std::min (1.0, std::max (vmin, 0.0));
				double vmaxb = std::min (1.0, std::max (vmax, 0.0));
				QGraphicsRectItem *sdrect;
				sdrect = m_scene->addRect (GRADIENT_WIDTH + x * m_step, (h - 2) * vminb,
							   m_step, (h - 2) * (vmaxb - vminb), Qt::NoPen, sdbrush);
				sdrect->setZValue (1);
			} else
				val = ev.wr_black;

			if (on_path) {
				path.lineTo (GRADIENT_WIDTH + x * m_step, (h - 2) * val);
				double chg = val - prev;
				if (!m_show_scores && chg != 0 && st->was_move_p () && idnr == sel_idx) {
					QBrush br (st->get_move_color () == black ? Qt::black : Qt::white);
					/* One idea was to offset this by half the width of a step, so as to
					   make the change appear between moves, but I found that confusing.  */
					m_scene->addRect (GRADIENT_WIDTH + x * m_step, h / 2, m_step, h * chg,
							  Qt::NoPen, br);
				}
			} else
				path.moveTo (GRADIENT_WIDTH + x * m_step, (h - 2) * val);
			prev = val;
			on_path = true;
		}

		QGraphicsPathItem *p = m_scene->addPath (path, pen);
		p->setZValue (3);
	}
	QGraphicsRectItem *sel = m_scene->addRect (GRADIENT_WIDTH + (int)(active_point * m_step), 0, round (m_step), h,
						   Qt::NoPen, QBrush (Qt::gray));
	sel->setZValue (2);

	m_scene->update ();
}

void EvalGraph::changeEvent (QEvent *e)
{
	QGraphicsView::changeEvent (e);
	if (e->type () != QEvent::EnabledChange)
		return;
	if (isEnabled ())
		setForegroundBrush (QBrush (Qt::transparent));
	else
		setForegroundBrush (QBrush (Qt::lightGray, Qt::Dense6Pattern));
}
