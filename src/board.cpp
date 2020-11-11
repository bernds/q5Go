/*
* board.cpp
*/
#include <vector>
#include <fstream>
#include <cmath>

#include <QMessageBox>
#include <QPainter>
#include <QLineEdit>
#include <QPixmap>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QWheelEvent>
#include <QFontMetrics>

#include "config.h"
#include "setting.h"
#include "board.h"
#include "imagehandler.h"
#include "mainwindow.h"
#include "miscdialogs.h"
#include "svgbuilder.h"
#include "ui_helpers.h"
#include "gotools.h"
#include "patternsearch.h"

BoardView::BoardView (QWidget *parent, bool no_sync, bool no_marks)
	: QGraphicsView(parent), m_vgrid_outline (1), m_hgrid_outline (1), m_dims (0, 0, DEFAULT_BOARD_SIZE, DEFAULT_BOARD_SIZE),
	  m_hoshis (1), m_never_sync (no_sync), m_no_marks (no_marks)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setUpdatesEnabled(true);

	m_show_hoshis = true;
	m_show_figure_caps = false;
	m_show_coords = setting->readBoolEntry ("BOARD_COORDS");
	m_sgf_coords = setting->readBoolEntry ("SGF_BOARD_COORDS");
	m_cont_view = pattern_cont_view::letters;
	setStyleSheet( "QGraphicsView { border-style: none; }" );

	// Create an ImageHandler instance.
	imageHandler = new ImageHandler();

	// Init the canvas
	canvas = new QGraphicsScene (0, 0, BOARD_X, BOARD_Y, this);
	setScene (canvas);

	imageHandler->init(10);

#ifdef Q_OS_WIN
	resizeDelayFlag = false;
#endif
	lockResize = false;

	canvas->addItem (&m_stone_layer);
	m_stone_layer.setZValue (10);

	canvas->addItem (coverTop = new QGraphicsRectItem ());
	canvas->addItem (coverBot = new QGraphicsRectItem ());
	canvas->addItem (coverLeft = new QGraphicsRectItem ());
	canvas->addItem (coverRight = new QGraphicsRectItem ());

	coverTop->setPen (QColor (0, 0, 0, 0));
	coverBot->setPen (QColor (0, 0, 0, 0));
	coverLeft->setPen (QColor (0, 0, 0, 0));
	coverRight->setPen (QColor (0, 0, 0, 0));

	setFocusPolicy(Qt::NoFocus);
}

BoardView::~BoardView()
{
	canvas->removeItem (&m_stone_layer);
	clear_graphics_elts ();
	delete canvas;
	delete imageHandler;
}


Board::Board (QWidget *parent)
	: BoardView (parent), GTP_Eval_Controller (parent)
{
	viewport()->setMouseTracking(true);
	curX = curY = -1;

	navIntersectionStatus = false;
}

void BoardView::calculateSize ()
{
	// Calculate the size values

	int w = canvas->width ();
	int h = canvas->height ();

	int table_size_x = w - 2 * m_margin;
	int table_size_y = h - 2 * m_margin;

	double cmargin = m_show_coords ? 4 * m_coord_margin : 0;
	int shown_size_x = m_crop.width () + 2 * n_dups_h ();
	int shown_size_y = m_crop.height () + 2 * n_dups_v ();
	double coord_factor = m_show_coords ? setting->readIntEntry ("COORDS_SIZE") / 100.0 : 0;
	double square_size_w = (table_size_x - cmargin) / (shown_size_x + 2 * coord_factor);
	double square_size_h = (table_size_y - cmargin) / (shown_size_y + 2 * coord_factor);
	square_size = std::min (square_size_w, square_size_h);

	// Should not happen, but safe is safe.
	if (square_size == 0)
		  square_size = 1;

	imageHandler->rescale (square_size);

	int board_pixel_size_x = square_size * (shown_size_x - 1);
	int board_pixel_size_y = square_size * (shown_size_y - 1);

	int real_tx = std::min ((double)table_size_x, round (square_size * (shown_size_x + 2 * coord_factor) + cmargin));
	int real_ty = std::min ((double)table_size_y, round (square_size * (shown_size_y + 2 * coord_factor) + cmargin));
#if 1
	if (table_size_x - real_tx < table_size_y - real_ty)
		real_tx = table_size_x;
	else
		real_ty = table_size_y;
#endif
	// Center the board in canvas
	m_wood_rect = QRect ((w - real_tx) / 2, (h - real_ty) / 2, real_tx, real_ty);
	m_board_rect = QRect ((w - board_pixel_size_x) / 2, (h - board_pixel_size_y) / 2,
			      board_pixel_size_x, board_pixel_size_y);
}

void BoardView::resizeBoard (int w, int h)
{
	if (w < 30 || h < 30)
		return;

	// Resize canvas
	canvas->setSceneRect (0, 0, w, h);

	// Recalculate the size values
	calculateSize ();

	// Redraw the board
	draw_background ();
	updateCovers ();

	sync_appearance ();
}

void BoardView::resizeEvent(QResizeEvent*)
{
#ifdef _WS_WIN_x
	if (!resizeDelayFlag)
	{
		resizeDelayFlag = true;
		// not necessary?
		QTimer::singleShot(50, this, SLOT(changeSize()));
	}
#else
	if (!lockResize)
		changeSize();
#endif
}

QImage BoardView::background_image ()
{
	int w = canvas->width ();
	int h = canvas->height ();

	m_wood = *setting->wood_image ();
	m_table = *setting->table_image ();

	// Create pixmap of appropriate size
	//QPixmap all(w, h);
	QImage image (w, h, QImage::Format_RGB32);

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin (&image);
	painter.setPen (Qt::NoPen);

	int board_x0 = m_wood_rect.x ();
	int board_y0 = m_wood_rect.y ();
	int board_x1 = board_x0 + m_wood_rect.width () - 1;
	int board_y1 = board_y0 + m_wood_rect.height () - 1;
	painter.drawTiledPixmap (0, 0, w, h, m_table);
	painter.setRenderHint (QPainter::SmoothPixmapTransform);
	if (setting->readBoolEntry ("SKIN_SCALE_WOOD"))
		painter.drawPixmap (m_wood_rect, m_wood);
	else
		painter.drawTiledPixmap (m_wood_rect, m_wood);

	// Modify the edges of the board so they appear slightly three-dimensional.
	int width = 3;

	for (int i = 0; i < width; i++) {
		double darken_factor = (double) (width - i) / width;
		double lighten_factor = (double) (width - i) / width;
		darken_factor *= darken_factor;
		lighten_factor *= lighten_factor;
		darken_factor = darken_factor * 0.6;
		lighten_factor = lighten_factor * 0.4;
		QColor darken (0, 0, 0, darken_factor * 255);
		QColor lighten (255, 255, 255, lighten_factor * 255);
		painter.setPen (lighten);
		painter.drawLine (board_x0 + i, board_y0 + i, board_x1 - i, board_y0 + i);
		painter.drawLine (board_x1 - i, board_y0 + i + 1, board_x1 - i, board_y1 - i);
		painter.setPen (darken);
		painter.drawLine (board_x0 + i, board_y0 + i + 1, board_x0 + i, board_y1 - i);
		painter.drawLine (board_x0 + i + 1, board_y1 - i, board_x1 - i - 1, board_y1 - i);
	}

	// Draw a shadow below the board
	width = 10;
	for (int i = 0; i < width; i++) {
		double darken_factor = (double) (width - i) / width;
		darken_factor = darken_factor * 0.5;
		QColor darken (0, 0, 0, darken_factor * 255);
		painter.setPen (darken);
		painter.drawLine (board_x0 - i, board_y0 + i, board_x0 - i, board_y1 + i);
		painter.drawLine (board_x0 - i + 1, board_y1 + i, board_x1 - i, board_y1 + i);
	}

	/* Draw the coordinates.  */
	if (m_show_coords && m_displayed != nullptr) {
		const go_board &b = m_displayed->get_board ();

		double coord_factor = setting->readIntEntry ("COORDS_SIZE") / 100.0;
		double coord_size = std::max (1.0, square_size * coord_factor);
		// centres the coordinates text within the remaining space at table edge
		const int center = coord_size / 2 + m_coord_margin;

		QFont f = setting->fontMarks;
		/* Throughout, we use the "double bounding rect" trick to obtain correct results.
		   Experience shows that a single boundingRect call produces too narrow a rectangle,
		   and googling shows that this is in fact a known problem.  */
		QFontMetrics fm (f);
		QRect max_rect;
		for (int tx = m_crop.x1; tx <= m_crop.x2; tx++) {
			auto name = b.coords_name (tx, 0, m_sgf_coords);
			QString nm = QString::fromStdString (name.first);
			QRect brect = fm.boundingRect (nm);
			brect = fm.boundingRect (brect, Qt::AlignCenter, nm);
			brect.moveTopLeft (max_rect.topLeft ());
			max_rect = max_rect.united (brect);
		}
		for (int ty = m_crop.y1; ty <= m_crop.y2; ty++) {
			auto name = b.coords_name (0, ty, m_sgf_coords);
			QString nm = QString::fromStdString (name.second);
			QRect brect = fm.boundingRect (nm);
			brect = fm.boundingRect (brect, Qt::AlignCenter, nm);
			brect.moveTopLeft (max_rect.topLeft ());
			max_rect = max_rect.united (brect);
		}
		double fac_x = max_rect.width () / coord_size;
		double fac_y = max_rect.height () / coord_size;
		double fac = std::max (fac_x, fac_y);
		if (fac != 0)
			f.setPointSize (f.pointSize () / fac);
		painter.setFont (f);
		QFontMetrics fm2 (f);

		int hdups = n_dups_h ();
		int vdups = n_dups_v ();
		painter.setPen (Qt::black);

		int shiftx = m_shift_x + m_crop.x1;
		int shifty = m_shift_y + m_crop.y1;

		for (int tx = m_crop.x1; tx <= m_crop.x2; tx++) {
			int x = (tx + shiftx) % m_dims.width ();
			auto name = b.coords_name (x, 0, m_sgf_coords);
			QString nm = QString::fromStdString (name.first);
			QRect brect = fm2.boundingRect (nm);
			brect = fm2.boundingRect (brect, Qt::AlignCenter, nm);
			brect.moveCenter (QPoint (m_board_rect.x () + square_size * (hdups + tx),
						  m_wood_rect.y () + center));
			painter.drawText (brect, Qt::AlignCenter, nm);
			brect.moveCenter (QPoint (m_board_rect.x () + square_size * (hdups + tx),
						  m_wood_rect.bottom () - center));
			painter.drawText (brect, Qt::AlignCenter, nm);
		}
		for (int ty = m_crop.y1; ty <= m_crop.y2; ty++) {
			int y = (ty + shifty) % m_dims.height ();
			auto name = b.coords_name (0, y, m_sgf_coords);
			QString nm = QString::fromStdString (name.second);
			QRect brect = fm2.boundingRect (nm);
			brect = fm2.boundingRect (brect, Qt::AlignCenter, nm);
			brect.moveCenter (QPoint (m_wood_rect.x () + center,
						  m_board_rect.y () + square_size * (vdups + ty)));
			painter.drawText (brect, Qt::AlignCenter, nm);
			brect.moveCenter (QPoint (m_wood_rect.right () - center,
						  m_board_rect.y () + square_size * (vdups + ty)));
			painter.drawText (brect, Qt::AlignCenter, nm);
		}
	}

	painter.end ();
	return image;
}

void BoardView::draw_background (void)
{
	canvas->setBackgroundBrush (QBrush (background_image ()));
}

void Board::setMode (GameMode mode)
{
	m_game_mode = mode;

	/* Always needed when changing modes to update toolbar buttons etc.  */
	sync_appearance (false);
}

bool Board::show_cursor_p ()
{
	if (!setting->readBoolEntry("CURSOR") || navIntersectionStatus)
		return false;

	if (m_mark_rect || m_request_mark_rect || m_dragging)
		return false;

	GameMode mode = m_game_mode;
	if (m_edit_mark != mark::none)
		return false;

	if (mode == modeScore || mode == modeObserve)
		return false;

	if (!player_to_move_p ())
		return false;

	return true;
}

stone_color Board::cursor_color (int x, int y, stone_color to_move)
{
	if (x == curX && y == curY && show_cursor_p ()) {
		if (m_game_mode == modeEdit)
			/* @@@ */
			return black;
		return to_move;
	}
	return none;
}

static int collect_moves (go_board &b, game_state *startpos, game_state *stop_pos, bool primary)
{
	int mvnr = startpos->sgf_move_number ();
	int count = 0;
	game_state *st = startpos;
	int maxnr = 0;
	for (;;) {
		if (!st->was_move_p ())
			throw std::logic_error ("found non-move ");
		int x = st->get_move_x ();
		int y = st->get_move_y ();
		stone_color to_move = st->get_move_color ();
		stone_color present = b.stone_at (x, y);
		if (present == none)
			b.set_stone (x, y, present = to_move);
		if (present == to_move)
			b.set_mark (x, y, mark::num, maxnr = mvnr + count);

		if (st == stop_pos)
			break;
		++count;
		if (primary)
			st = st->next_primary_move ();
		else
			st = st->next_move ();
	}
	return maxnr;
}

static QString convert_letter_mark (mextra extra)
{
	if (extra < 26)
		return QString ('A' + extra);
	else
		return QString ('a' + extra - 26);
}

/* Render a mark in SVG at center position CX/CY in a square with a side length of FACTOR.
   M and ME represent the mark as present in the board.  SC is the color of the stone that
   has been rendered before, or NONE.  COUNT_VAL, if nonzero, together with MAX_NUMBER represents
   the move number to be displayed.  VAR_M and VAR_ME represent a variation mark, typically
   some letter.

   WHITE_BACKGROUND is true if we are doing this for svg export, it changes the display a
   little bit.  */
static bool add_mark_svg (svg_builder &svg, double cx, double cy, double factor,
			  mark m, mextra me, const std::string &mstr, mark var_m, mextra var_me,
			  stone_color sc, int count_val, int max_number,
			  bool was_last_move, bool white_background, const QFontInfo &fi,
			  QString color_override = QString ())
{
	QString mark_col = !color_override.isEmpty () ? color_override : sc == black ? "white" : "black";
	if (sc == none && white_background && (count_val != 0 || m != mark::none || var_m != mark::none)) {
		svg.square_at (cx, cy, factor * 0.9,
			       "white", "none");
	}
	if (count_val != 0) {
		int len = max_number > 99 ? 3 : max_number > 9 ? 2 : 1;
		svg.text_at (cx, cy, factor,
			     len, QString::number (count_val),
			     mark_col, fi);
		return true;
	}

	if (m == mark::none && var_m != mark::none) {
		m = var_m;
		me = var_me;
		mark_col = "blue";
	}

	/* We make an artificial mark for the last move.  Done late so as to not
	   override other marks.  */
	if (m == mark::none && was_last_move)
		m = mark::move;

	/* Convert the large number of conceptual marks into a smaller set of
	   visual ones.  */

	/* @@@ Could think about making these two look different.  Using red
	   as a color, for example, but that makes it less visible.  */
	if (m == mark::move) {
		m = mark::circle;
	} else if (m == mark::falseeye) {
		m = mark::triangle;
		mark_col = "red";
	} else if (m == mark::seki) {
		m = mark::square;
		mark_col = "blue";
	}

	if (m == mark::none)
		return false;

	switch (m) {
	case mark::circle:
		svg.circle_at (cx, cy, factor * 0.25,
			       "none", mark_col);
		break;
	case mark::square:
		svg.square_at (cx, cy, factor * 0.8 / M_SQRT2,
			       "none", mark_col);
		break;
	case mark::triangle:
		svg.triangle_at (cx, cy, factor * 0.8,
				 "none", mark_col);
		break;
	case mark::terr:
		if (sc == none && white_background)
			mark_col = "black";
		else
			mark_col = me == 0 ? "white" : "black";
		/* fall through */
	case mark::cross:
		svg.cross_at (cx, cy, factor * 0.8,
			      mark_col);
		break;
	case mark::num:
		svg.text_at (cx, cy, factor, 0,
			     QString::number (me),
			     mark_col, fi);
		break;
	case mark::letter:
		svg.text_at (cx, cy, factor, 0,
			     convert_letter_mark (me),
			     mark_col, fi);
		break;
	case mark::text:
		svg.text_at (cx, cy, factor, 0,
			     QString::fromStdString (mstr),
			     mark_col, fi);
		break;
	case mark::dead:
		return false;

	default:
		break;
	}
	return true;
}

QByteArray BoardView::render_svg (bool do_number, bool coords)
{
	const go_board &b = m_displayed->get_board ();
	/* Look back through previous moves to see if we should do numbering.  */
	bool numbering = do_number;
	bool have_figure = numbering && m_displayed->has_figure ();
	int fig_flags = have_figure ? m_displayed->figure_flags () : 0;
	QString fig_title = have_figure ? QString::fromStdString (m_displayed->figure_title ()) : QString ();
	int max_number = 0;
	go_board mn_board (b, mark::none);

	if (have_figure && numbering) {
		game_state *startpos = m_displayed;
		if (!startpos->was_move_p () && startpos->n_children () > 0)
			startpos = startpos->next_primary_move ();
		game_state *last = startpos;
		game_state *st = startpos->next_primary_move ();
		while (st && st->was_move_p () && !st->has_figure ()) {
			last = st;
			st = st->next_primary_move ();
		}
		if (last != startpos || startpos->was_move_p ()) {
			game_state *real_start = startpos->was_move_p () ? startpos->prev_move () : startpos;
			mn_board = (fig_flags & 256) == 0 ? last->get_board () : real_start->get_board ();
			max_number = collect_moves (mn_board, startpos, last, true);
		}
	} else if (numbering && !m_displayed->has_figure () && m_displayed->was_move_p ()) {
		game_state *startpos = m_displayed;
		game_state *first = startpos;
		while (startpos && startpos->was_move_p () && !startpos->has_figure ())
			first = startpos, startpos = startpos->prev_move ();
		max_number = collect_moves (mn_board, first, m_displayed, false);
	}

	double factor = 30;
	double margin = 10;
	double coord_margin = 4;
	double offset_x = margin + factor / 2;
	double offset_y = margin + factor / 2;
	int cols = m_sel.width ();
	int rows = m_sel.height ();
	int w = factor * cols + 2 * margin;
	int h = factor * rows + 2 * margin;
	double coord_factor = setting->readIntEntry ("COORDS_SIZE") / 100.0;
	if (coords) {
		if (m_sel.aligned_left (m_dims))
			offset_x += factor * coord_factor + 2 * coord_margin, w += factor * coord_factor + 2 * coord_margin;
		if (m_sel.aligned_top (m_dims))
			offset_y += factor * coord_factor + 2 * coord_margin, h += factor * coord_factor + 2 * coord_margin;
		if (m_sel.aligned_right (m_dims))
			w += factor * coord_factor + 2 * coord_margin;
		if (m_sel.aligned_bottom (m_dims))
			h += factor * coord_factor + 2 * coord_margin;
	}

	QFontInfo fi (setting->fontMarks);
	int full_h = h;
	if ((fig_flags & 2) == 0 && !fig_title.isEmpty ()) {
#if 0
		 /* @@@ This would be nice but svg apparently doesn't support word wrap and there
		    isn't really a nice QFontMetrics function to break a string into a QStringList.  */
		QFont f = setting->fontMarks;
		f.setPixelSize (factor * 0.75);
		QFontMetrics fm (f);
		QRect r1 (0, 0, w, factor * 80);
		QRect br = fm.boundingRect (r1, Qt::TextWordWrap, fig_title);
		int lines = round (br.height () / (double)fm.height ());
		full_h += lines * factor;
#endif
		full_h += factor;
	}
	svg_builder svg (w, full_h);
	/* A white background, since use white squares to clear the grid when showing marks.
	   Against a clear background that wouldn't look very good.  */
	svg.rect (0, 0, w, h, "white", "none");

	if ((fig_flags & 2) == 0 && !fig_title.isEmpty ()) {
		int fh = factor * 0.75;
		svg.fixed_height_text_at (w / 2, full_h, fh, fig_title, "black", fi, false);
	}

	if (coords) {
		double dist = margin + factor * coord_factor / 2 + coord_margin;
		for (int y = 0; y < rows; y++) {
			int ry = y + m_sel.y1;
			double center_y = offset_y + y * factor;
			if (m_sel.aligned_left (m_dims))
				svg.fixed_height_text_at (dist, center_y, factor * coord_factor,
							  QString::number (m_dims.height () - ry),
							  "black", fi, true);
			if (m_sel.aligned_right (m_dims))
				svg.fixed_height_text_at (w - dist, center_y, factor * coord_factor,
							  QString::number (m_dims.height () - ry),
							  "black", fi, true);
		}
		for (int x = 0; x < cols; x++) {
			int rx = x + m_sel.x1;
			double center_x = offset_x + x * factor;
			QString label;
			if (rx < 9)
				label = QChar ('A' + rx);
			else
				label = QChar ('A' + rx + 1);
			if (m_sel.aligned_top (m_dims))
				svg.fixed_height_text_at (center_x, dist, factor * coord_factor,
							  label, "black", fi, true);
			if (m_sel.aligned_bottom (m_dims))
				svg.fixed_height_text_at (center_x, h - dist, factor * coord_factor,
							  label, "black", fi, true);
		}
	}

	/* The grid.  */
	int top = !m_sel.aligned_top (m_dims) ? -factor / 2 : 0;
	int bot = !m_sel.aligned_bottom (m_dims) ? factor / 2 : 0;
	int lef = !m_sel.aligned_left (m_dims) ? -factor / 2 : 0;
	int rig = !m_sel.aligned_right (m_dims) ? factor / 2 : 0;
	for (int x = 0; x < cols; x++) {
		QString width = "1";
		if ((x == 0 && m_sel.aligned_left (m_dims))
		    || (x + 1 == cols && m_sel.aligned_right (m_dims)))
			width = "2";
		svg.line (offset_x + x * factor, offset_y + top,
			  offset_x + x * factor, offset_y + (rows - 1) * factor + bot,
			  "black", width);
	}
	for (int y = 0; y < rows; y++) {
		QString width = "1";
		if ((y == 0 && m_sel.aligned_top (m_dims))
		    || (y + 1 == rows && m_sel.aligned_bottom (m_dims)))
			width = "2";
		svg.line (offset_x + lef, offset_y + y * factor,
			  offset_x + (cols - 1) * factor + rig, offset_y + y * factor,
			  "black", width);
	}
	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			int rx = x + m_sel.x1;
			int ry = y + m_sel.y1;
			stone_color c = mn_board.stone_at (rx, ry);
			mark m = b.mark_at (rx, ry);
			mextra extra = b.mark_extra_at (rx, ry);
			int v = mn_board.mark_at (rx, ry) == mark::num ? mn_board.mark_extra_at (rx, ry) : 0;

			double center_x = offset_x + x * factor;
			double center_y = offset_y + y * factor;
			if (c != none) {
				svg.circle_at (center_x, center_y, factor * 0.45,
					       c == black ? "black" : "white",
					       c == black ? "none" : "black");
			}
			add_mark_svg (svg, center_x, center_y, factor,
				      m, extra, m == mark::text ? b.mark_text_at (rx, ry) : "",
				      mark::none, 0, c, v, max_number, false, true, fi);
		}
	}

	return svg;
}

/* Construct ASCII diagrams suitable for use on lifein19x19.com.
   Moves can be numbered 1-10.  When we do numbering, we split the moves up
   into a suitable number of diagrams, inserting breaks when 10 moves are
   exceeded or a stone is placed on an intersection which previously
   held something else.

   This should really be part of game_state.  */

QString BoardView::render_ascii (bool do_number, bool coords, bool go_tags)
{
	const go_board &db = m_displayed->get_board ();
	bool have_figure = m_figure_view && m_displayed->has_figure ();
	unsigned bitsz = db.bitsize ();
	int szx = db.size_x ();
	int szy = db.size_y ();
	QString result;

	/* Holds the position to be shown.  There is a difference between
	   numbering and plain mode: if numbering, this should be the position
	   _before_ the first move.  */
	go_board initial_board (db);

	int moves = 1;
	game_state *startpos = m_displayed;
	game_state *finalpos = m_displayed;

	if (do_number && have_figure) {
		if (!startpos->was_move_p () && startpos->n_children () > 0)
			startpos = startpos->next_primary_move ();
		game_state *st = startpos->next_primary_move ();
		while (st && st->was_move_p () && !st->has_figure ())
			st = st->next_primary_move ();
		finalpos = st;
		moves = startpos->sgf_move_number ();
		game_state *real_start = startpos->was_move_p () ? startpos->prev_move () : startpos;
		initial_board = go_board (real_start->get_board (), mark::none);
	} else if (do_number && !m_displayed->has_figure ()) {
		game_state *prev = startpos;
		while (prev && prev->was_move_p () && !startpos->has_figure ())
			startpos = prev, prev = prev->prev_move ();
		game_state *real_start = startpos->was_move_p () ? startpos->prev_move () : startpos;
		initial_board = go_board (real_start->get_board (), mark::none);
		finalpos = finalpos->next_move ();
	}

	std::vector<int> count_map (bitsz);
	do {
		std::fill (std::begin (count_map), std::end (count_map), 0);

		int n_mv = 0;
		game_state *next = startpos;
		stone_color first_col = none;
		stone_color prev_col = none;
		while (next != finalpos && n_mv < 10) {
			int x = next->get_move_x ();
			int y = next->get_move_y ();
			stone_color col = next->get_move_color ();
			if (first_col == none)
				first_col = col;
			else if (col == prev_col)
				break;
			prev_col = col;
			int bp = initial_board.bitpos (x, y);
			if (count_map[bp] != 0 || initial_board.stone_at (x, y) != none)
				break;
			count_map[bp] = ++n_mv;
			next = have_figure ? next->next_primary_move () : next->next_move ();
		}
		if (first_col == none)
			first_col = startpos->to_move ();
		if (go_tags)
			result += "[go]";
		result +="$$";
		result += first_col == black ? "B" : "W";
		if (coords) {
			result += "c" + QString::number (std::max (szx, szy));
		}
		if (moves > 1) {
			result += "m" + QString::number (moves);
		}
		result += "\n";
		if (m_sel.aligned_top (m_dims)) {
			result += "$$";
			if (m_sel.aligned_left (m_dims))
				result += " +";
			for (int i = 0; i < m_sel.width (); i++)
				result += "--";
			if (m_sel.aligned_right (m_dims))
				result += "-+";
			result += "\n";
		}
		for (int y = m_sel.y1; y <= m_sel.y2; y++) {
			result += "$$";
			if (m_sel.aligned_left (m_dims))
				result += " |";
			for (int x = m_sel.x1; x <= m_sel.x2; x++) {
				int bp = initial_board.bitpos (x, y);
				int v = count_map[bp];
				if (v != 0) {
					result += " " + QString::number (v % 10);
				} else {
					stone_color c = initial_board.stone_at (x, y);
					mark m = initial_board.mark_at (x, y);
					mextra me = initial_board.mark_extra_at (x, y);
					char rslt[] = " .";
					if (c == none) {
						if (m == mark::letter && me < 26)
							rslt[1] = 'a' + me;
						else switch (m) {
						case mark::circle: rslt[1] = 'C'; break;
						case mark::square: rslt[1] = 'S'; break;
						case mark::triangle: rslt[1] = 'T'; break;
						case mark::cross: rslt[1] = 'M'; break;
						default: break;
						}
					} else {
						switch (m) {
						case mark::square: rslt[1] = c == black ? '#' : '@'; break;
						case mark::triangle: rslt[1] = c == black ? 'Y' : 'Q'; break;
						case mark::circle: rslt[1] = c == black ? 'B' : 'W'; break;
						case mark::cross: rslt[1] = c == black ? 'Z' : 'P'; break;
						default: rslt[1] = c == black ? 'X' : 'O'; break;
						}
					}
					result += rslt;
				}
			}
			if (m_sel.aligned_right (m_dims))
				result += " |";
			result += "\n";
		}
		if (m_sel.aligned_bottom (m_dims)) {
			result += "$$";
			if (m_sel.aligned_left (m_dims))
				result += " +";
			for (int i = 0; i < m_sel.width (); i++)
				result += "--";
			if (m_sel.aligned_right (m_dims))
				result += "-+";
			result += "\n";
		}
		if (go_tags)
			result += "[/go]";
		result += "\n";

		for (int i = 0; i < n_mv; i++) {
			int x = startpos->get_move_x ();
			int y = startpos->get_move_y ();
			initial_board.add_stone (x, y, startpos->get_move_color ());
			startpos = have_figure ? startpos->next_primary_move () : startpos->next_move ();
		}
		moves += n_mv;
	} while (startpos != finalpos);
	return result;
}

void BoardView::draw_grid (QPainter &painter, bit_array hgrid, bit_array vgrid, const bit_array &hidden)
{
	int szx = m_dims.width ();
	int szy = m_dims.height ();
	int visx = m_crop.width ();
	int visy = m_crop.height ();
	int shiftx = m_shift_x + m_crop.x1;
	int shifty = m_shift_y + m_crop.y1;
	int dups_x = n_dups_h ();
	int dups_y = n_dups_v ();
	const go_board &b = m_displayed->get_board();

	int scaled_w = setting->readBoolEntry ("BOARD_LINESCALE") ? (int)square_size / 40 + 1 : 1;
	QPen pen (Qt::black);
	pen.setWidth (scaled_w);
	painter.setPen (pen);

	int hoshi_size = square_size / 8;
	if (hoshi_size % 2 == 0)
		hoshi_size--;
	if (hoshi_size <= 2)
		hoshi_size = 3;

	int line_offx = m_board_rect.left () - m_wood_rect.left ();
	int line_offy = m_board_rect.top () - m_wood_rect.top ();

	auto draw_vertical = [&] (int tx, bool center = false) {
		int real_x = (tx + shiftx + (szx - dups_x)) % szx;
		int begin_y = center ? dups_y : 0;
		int end_y = begin_y + visy + (center ? 0 : 2 * dups_y);
		int first = -2;
		int last;
		for (int ty = begin_y * 2; ty < end_y * 2; ty++) {
			int real_y = (ty / 2 + shifty + (szy - dups_y)) % szy;
			int off = ty % 2;
			int bp = real_x + (real_y * 2 + off) * szx;
			if (vgrid.test_bit (bp)) {
				if (first == -2)
					first = ty - 1;
				last = ty;
			} else if (first != -2) {
				painter.drawLine (line_offx + tx * square_size, line_offy + first * square_size / 2,
						  line_offx + tx * square_size, line_offy + last * square_size / 2);
				first = -2;
			}
		}
		if (first != -2)
			painter.drawLine (line_offx + tx * square_size, line_offy + first * square_size / 2,
					  line_offx + tx * square_size, line_offy + last * square_size / 2);

	};
	auto draw_horizontal = [&] (int ty, bool center = false) {
		int real_y = (ty + shifty + (szy - dups_y)) % szy;
		int begin_x = center ? dups_x : 0;
		int end_x = begin_x + visx + (center ? 0 : 2 * dups_x);
		int first = -2;
		int last;
		for (int tx = begin_x * 2; tx < end_x * 2; tx++) {
			int real_x = (tx / 2 + shiftx + (szx - dups_x)) % szx;
			int off = tx % 2;
			int bp = real_x * 2 + off + real_y * szx * 2;
			if (hgrid.test_bit (bp)) {
				if (first == -2)
					first = tx - 1;
				last = tx;
			} else if (first != -2) {
				painter.drawLine (line_offx + first * square_size / 2, line_offy + ty * square_size,
						  line_offx + last * square_size / 2, line_offy + ty * square_size);
				first = -2;
			}
		}
		if (first != -2)
			painter.drawLine (line_offx + first * square_size / 2, line_offy + ty * square_size,
					  line_offx + last * square_size / 2, line_offy + ty * square_size);

	};
	for (int tx = 0; tx < visx + 2 * dups_x; tx++) {
		draw_vertical (tx);
	}
	for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
		draw_horizontal (ty);
	}
	if (setting->readBoolEntry ("BOARD_LINEWIDEN")) {
		/* Adjust the grids to hold only the outline by removing lines from the
		   previous grids.  */
		vgrid.and1 (m_vgrid_outline);
		hgrid.and1 (m_hgrid_outline);
		pen.setWidth (scaled_w * 2);
		painter.setPen (pen);
		int tx1 = dups_x + (szx - shiftx) % szx;
		int tx2 = dups_x + (szx - shiftx - 1) % szx;
		int ty1 = dups_y + (szy - shifty) % szy;
		int ty2 = dups_y + (szy - shifty - 1) % szy;

		draw_vertical (tx1, true);
		draw_vertical (tx2, true);
		draw_horizontal (ty1, true);
		draw_horizontal (ty2, true);
	}
	if (!m_show_hoshis)
		return;
	/* Now draw the hoshi points.  */
	painter.setRenderHints (QPainter::Antialiasing);
	painter.setPen (Qt::NoPen);
	painter.setBrush (QBrush (Qt::black));
	for (int tx = 0; tx < visx; tx++)
		for (int ty = 0; ty < visy; ty++) {
			int x = (tx + shiftx) % szx;
			int y = (ty + shifty) % szy;
			int bp = b.bitpos (x, y);
			if (m_hoshis.test_bit (bp) && !hidden.test_bit (bp)) {
				painter.drawEllipse (QPoint (line_offx + (tx + dups_x) * square_size,
							     line_offy + (ty + dups_y) * square_size),
						     hoshi_size, hoshi_size);
			}
		}
}

const QPixmap &BoardView::choose_stone_pixmap (stone_color c, stone_type type, int bp)
{
	if (type == stone_type::live) {
		const QList<QPixmap> *l = imageHandler->getStonePixmaps ();
		int cnt = l->count ();
		if (c == black)
			return (*l)[0];
		else if (cnt <= 2)
			return (*l)[1];
		else
			return (*l)[1 + bp % (cnt - 2)];
	} else {
		const QList<QPixmap> *l = imageHandler->getGhostPixmaps ();
		if (c == black)
			return (*l)[0];
		else
			return (*l)[1];
	}
}

std::pair<stone_color, stone_type> BoardView::stone_to_display (const go_board &b, const bit_array *visible,
								stone_color to_move, int x, int y,
								const go_board &vars, int var_type)
{
	if (visible != nullptr && !visible->test_bit (b.bitpos (x, y)))
		return std::make_pair (none, stone_type::live);

	stone_color sc = b.stone_at (x, y);
	stone_type type = stone_type::live;
	mark mark_at_pos = b.mark_at (x, y);

	/* If we don't have a real stone, check for various possibilities of
	   ghost stones: the mouse cursor, then territory marks, then variation display.  */

	if (sc == none) {
		sc = cursor_color (x, y, to_move);
		if (sc != none)
			type = stone_type::var;
	} else if (mark_at_pos == mark::terr || mark_at_pos == mark::falseeye || mark_at_pos == mark::dead)
		type = stone_type::var;

	if (sc == none && var_type == 1) {
		sc = vars.stone_at (x, y);
		if (sc != none)
			type = stone_type::var;
	}
	return std::make_pair (sc, type);
}

bool Board::have_analysis ()
{
	if (m_hide_analysis)
		return false;

	if (m_eval_state != nullptr)
		return !m_pause_eval;

	auto &c = m_displayed->children ();
	for (auto it: c) {
		if (it == c[0])
			continue;
		if (it->has_figure () && it->best_eval ().visits > 0)
			return true;
	}
	return false;
}

game_state *Board::analysis_primary_move ()
{
	game_state *eval_root = m_eval_state != nullptr ? m_eval_state : m_displayed;
	auto &c = eval_root->children ();
	for (auto it: c) {
		eval ev = m_eval_state != nullptr ? it->best_eval () : it->eval_from (m_an_id, true);
		if (it->has_figure () && ev.visits > 0 && it->was_move_p ())
			return it;
	}
	return nullptr;
}

game_state *Board::analysis_at (int x, int y, int &num, double &primary)
{
	game_state *eval_root = m_eval_state != nullptr ? m_eval_state : m_displayed;
	auto &c = eval_root->children ();
	num = 0;
	primary = 0;
	for (auto it: c) {
		eval ev = m_eval_state != nullptr ? it->best_eval () : it->eval_from (m_an_id, true);
		if (it->has_figure () && ev.visits > 0 && it->was_move_p ()) {
			if (num == 0)
				primary = ev.wr_black;
			if (it->get_move_x () == x && it->get_move_y () == y)
				return it;
			num++;
		}
	}
	return nullptr;
}

/* Examine the current cursor position, and extract a PV line into board B if
   we have one.  Return the number of moves added.  */
int Board::extract_analysis (go_board &b)
{
	int maxdepth = setting->readIntEntry ("ANALYSIS_DEPTH");

	int idx;
	double primary;
	auto display_pv = [this, &b] (game_state *pv, void (MainWindow::*set_eval) (const QString &, double, bool, double, stone_color, int))
		{
			if (pv == nullptr) {
				(m_board_win->*set_eval) (QString (), 0, false, 0, none, 0);
				return;
			}
			int x = pv->get_move_x ();
			int y = pv->get_move_y ();
			stone_color to_move = m_displayed->to_move ();
			eval ev = pv->best_eval ();
			double wr = ev.wr_black;
			int visits = ev.visits;
			if (to_move == white)
				wr = 1 - wr;
			if (x > 7)
				x++;
			QString move = QChar ('A' + x) + QString::number (b.size_y () - y);
			(m_board_win->*set_eval) (move, wr, ev.score_stddev != 0, ev.score_mean, to_move, visits);
		};
	game_state *primary_pv = analysis_primary_move ();
	display_pv (primary_pv, &MainWindow::set_eval);
	game_state *pv = analysis_at (curX, curY, idx, primary);
	display_pv (pv, &MainWindow::set_2nd_eval);
	if (pv == nullptr)
		return 0;

	int depth = 0;
	game_state *first = pv;
	game_state *last = pv;
	while (pv && (maxdepth == 0 || depth++ < maxdepth)) {
		last = pv;
		pv = pv->next_primary_move ();
	}
	return collect_moves (b, first, last, true);
}

Board::ram_result Board::render_analysis_marks (svg_builder &svg, double svg_factor, double cx, double cy, const QFontInfo &fi,
						int x, int y, bool child_mark, int v, int max_number)
{
	if (!have_analysis ())
		return ram_result::none;

	int analysis_vartype = setting->values.analysis_vartype;
	int winrate_for = setting->values.analysis_winrate;
	bool hideother = setting->values.analysis_hideother;

	int pv_idx;
	double primary;
	game_state *pv = analysis_at (x, y, pv_idx, primary);
	if (pv == nullptr || v > 0 || (max_number > 0 && hideother)) {
		if (child_mark) {
			svg.circle_at (cx, cy, svg_factor * 0.45, "none", "white", "1");
			return ram_result::nohide;
		}
		return ram_result::none;
	}

	stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;

	stone_color to_move = m_displayed->to_move ();
	eval ev = pv->best_eval ();
	double wr = ev.wr_black;
	double wrdiff = wr - primary;
	if (to_move == white)
		wr = 1 - wr, wrdiff = -wrdiff;

	/* Put a percentage or letter mark on variations returned by the analysis engine,
	   unless we are already showing a numbered variation and this intersection
	   has a number.  */
	QString wr_col = "lightblue";
	if (pv_idx > 0) {
		/* Use green for a difference of 0, red for any loss bigger than 12%.
		   The HSV angle between red and green is 120, so just multiply by 1000.  */
		int angle = std::min (120.0, std::max (0.0, 120 + 1000 * wrdiff));
		QColor col = QColor::fromHsv (angle, 255, 200);
		wr_col = col.name ();
	}
	svg.circle_at (cx, cy, svg_factor * 0.45, wr_col, child_mark ? "white" : "black", "1");

	if (analysis_vartype == 0) {
		QChar c = pv_idx >= 26 ? 'a' + pv_idx - 26 : 'A' + pv_idx;
		svg.text_at (cx, cy, svg_factor, 0, c,
			     "black", fi);
	} else {
		double shown_val = wrdiff;
		if (analysis_vartype == 2) {
			shown_val = wr;

			if (to_move == wr_swap_col)
				shown_val = 1 - shown_val;
		} else if (to_move == wr_swap_col)
			shown_val = -shown_val;

		svg.text_at (cx, cy, svg_factor, 4,
			     QString::number (shown_val * 100, 'f', 1),
			     "black", fi);
	}
	return ram_result::hide;
}

void BoardView::set_time_warning (int seconds)
{
	bool redraw = m_time != seconds;
	m_time = seconds;
	if (redraw)
		sync_appearance ();
}

/* The central function for synchronizing visual appearance with the abstract board data.  */
QPixmap BoardView::draw_position (int default_vars_type)
{
	bool numbering = !have_analysis () && m_move_numbers;
	bool have_figure = m_figure_view && !have_analysis () && m_displayed->has_figure ();
	int print_num = m_displayed->print_numbering_inherited ();

	bool analysis_children = setting->values.analysis_children;

	int var_type = (have_analysis () && analysis_children) || have_figure ? 0 : default_vars_type;
	bool exclude_diag = setting->readBoolEntry ("VAR_IGNORE_DIAGS");
	const go_board child_vars = m_displayed->child_moves (nullptr, exclude_diag);
	const go_board sibling_vars = m_displayed->sibling_moves (exclude_diag);
	const go_board &vars = m_vars_children ? child_vars : sibling_vars;

	const go_board &b = m_displayed->get_board ();
	stone_color to_move = m_displayed->to_move ();
	const bit_array *visible = m_figure_view ? m_displayed->visible_inherited () : nullptr;

	/* There are several ways we can get move numbering: showing a PV line from analysis, or
	   when displaying a figure, or displaying numbers on a normal board.  The first and
	   second are mutually exclusive, and have priority.  */
	int max_number = 0;
	go_board mn_board (b, mark::none);

	if (have_analysis ())
		max_number = extract_analysis (mn_board);

	bool skip_last_move_mark = false;
	if (setting->values.time_warn_board && m_time != 0 && m_displayed->was_move_p ()) {
		if (m_time < 100) {
			mn_board.set_mark (m_displayed->get_move_x (), m_displayed->get_move_y (),
					   mark::num, m_time);
			skip_last_move_mark = true;
		}
	}
	if (have_figure && print_num != 0) {
		game_state *startpos = m_displayed;
		if (!startpos->was_move_p () && startpos->n_children () > 0)
			startpos = startpos->next_primary_move ();
		game_state *last = startpos;
		game_state *st = startpos->next_primary_move ();
		while (st && st->was_move_p () && !st->has_figure ()) {
			last = st;
			st = st->next_primary_move ();
		}
		if (last != startpos || startpos->was_move_p ()) {
			int fig_flags = m_displayed->figure_flags ();
			game_state *real_start = startpos->was_move_p () ? startpos->prev_move () : startpos;
			mn_board = (fig_flags & 256) == 0 ? last->get_board () : real_start->get_board ();
			max_number = collect_moves (mn_board, startpos, last, true);
		}
	} else if (numbering && !m_displayed->has_figure () && m_displayed->was_move_p ()) {
		game_state *startpos = m_displayed;
		game_state *first = startpos;
		while (startpos && startpos->was_move_p () && !first->has_figure ())
			first = startpos, startpos = startpos->prev_move ();
		max_number = collect_moves (mn_board, first, m_displayed, false);
	}

	/* Handle marks first.  They go into an svgbuilder which we'll render at the end,
	   but we also use this to decide which parts of the grid to hide for better
	   readability.  */
	m_used_letters.clear ();
	m_used_numbers.clear ();

	int szx = b.size_x ();
	int szy = b.size_y ();
	int visx = m_crop.width ();
	int visy = m_crop.height ();
	int shiftx = m_shift_x + m_crop.x1;
	int shifty = m_shift_y + m_crop.y1;
	int dups_x = n_dups_h ();
	int dups_y = n_dups_v ();

	/* The factor is the size of a square in svg, it gets scaled later.  It should have
	   an optically pleasant relation with the stroke width (2 for marks).  */
	double svg_factor = 30;
	QFontInfo fi (setting->fontMarks);
	svg_builder svg (svg_factor * (visx + 2 * dups_x), svg_factor * (visy + 2 * dups_y));

	bit_array grid_hidden (b.bitsize ());

	if (!m_no_marks && m_cont == nullptr)
		for (int tx = 0; tx < visx + 2 * dups_x; tx++)
			for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
				int x = (tx + shiftx + (szx - dups_x)) % szx;
				int y = (ty + shifty + (szy - dups_y)) % szy;
				mark mark_at_pos = b.mark_at (x, y);
				mextra extra = b.mark_extra_at (x, y);
				bool was_last_move = false;

				/* Transfer dead marks to the mn_board which was initialized without marks.  */
				if (mark_at_pos == mark::dead)
					mn_board.set_mark (x, y, mark_at_pos, 0);

				auto stone_display = stone_to_display (mn_board, visible, to_move, x, y, vars, var_type);
				stone_color sc = stone_display.first;

				if (!have_figure && !skip_last_move_mark && m_displayed->was_move_p ()) {
					int last_x = m_displayed->get_move_x ();
					int last_y = m_displayed->get_move_y ();
					if (last_x == x && last_y == y)
						was_last_move = true;
				}

				mark var_mark = var_type == 2 ? vars.mark_at (x, y) : mark::none;
				mextra var_me = vars.mark_extra_at (x, y);

				if (mark_at_pos == mark::num)
					m_used_numbers.set_bit (extra);
				else if (mark_at_pos == mark::letter)
					m_used_letters.set_bit (extra);

				double cx = svg_factor / 2 + svg_factor * tx;
				double cy = svg_factor / 2 + svg_factor * ty;

				int v = mn_board.mark_at (x, y) == mark::num ? mn_board.mark_extra_at (x, y) : 0;

				bool added = false;
				bool an_child_mark = analysis_children && v == 0 && max_number == 0 && child_vars.stone_at (x, y) == to_move;
				ram_result rs = render_analysis_marks (svg, svg_factor, cx, cy, fi,
								       x, y, an_child_mark, v, max_number);
				if (rs == ram_result::none) {
					added = add_mark_svg (svg, cx, cy, svg_factor,
							      mark_at_pos, extra,
							      mark_at_pos == mark::text ? b.mark_text_at (x, y) : "",
							      var_mark, var_me,
							      sc, v, max_number, was_last_move, false, fi);
				} else
					added = rs == ram_result::hide;

				if (added)
					grid_hidden.set_bit (b.bitpos (x, y));
			}
	if (m_cont != nullptr) {
		double total = 0, total_w = 0, total_b = 0;
		for (unsigned i = 0; i < b.bitsize (); i++) {
			auto data = (*m_cont)[i];
			total += data.get (none).count;
			total_w += data.get (white).count;
			total_b += data.get (black).count;
		}
		for (int tx = 0; tx < visx + 2 * dups_x; tx++)
			for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
				int x = (tx + shiftx + (szx - dups_x)) % szx;
				int y = (ty + shifty + (szy - dups_y)) % szy;
				int bp = b.bitpos (x, y);
				auto data = (*m_cont)[bp];
				auto entry = data.get (none);
				if (entry.count <= 0)
					continue;
				if (entry.rank > 52)
					continue;
				auto entry_w = data.get (white);
				auto entry_b = data.get (black);
				QString text_col = (entry_w.count > 0 && entry_b.count > 0 ? "#555555"
						    : entry_w.count > 0 ? "white" : "black");
				auto stone_display = stone_to_display (mn_board, visible, to_move, x, y, vars, var_type);
				stone_color sc = stone_display.first;

				double cx = svg_factor / 2 + svg_factor * tx;
				double cy = svg_factor / 2 + svg_factor * ty;

				if (m_cont_view == pattern_cont_view::letters) {
					mextra val = entry.rank;
					add_mark_svg (svg, cx, cy, svg_factor, mark::letter, val, "", mark::none, 0,
						      sc, 0, 0, false, false, fi, text_col);
				} else if (m_cont_view == pattern_cont_view::numbers) {
					std::string t = QString::number (entry.count).toStdString ();
					add_mark_svg (svg, cx, cy, svg_factor, mark::text, 0, t, mark::none, 0,
						      sc, 0, 0, false, false, fi, text_col);
				} else {
					std::string t = QString::number (entry.count / total * 100, 'f', 1).toStdString ();
					add_mark_svg (svg, cx, cy, svg_factor, mark::text, 0, t, mark::none, 0,
						      sc, 0, 0, false, false, fi, text_col);
				}
				grid_hidden.set_bit (bp);
			}
	}
	/* Every grid point is connected to up to four line segments.  Create bitmaps
	   to indicate whether these should be drawn, one for horizontal lines and one
	   for vertical lines, each holding two bits for every point.  */
	std::shared_ptr<const bit_array> board_mask = m_game->get_board_mask ();
	bit_array vgrid (szx * szy * 2, true);
	bit_array hgrid (szx * szy * 2, true);

	for (int x = 0; x < szx; x++)
		for (int y = 0; y < szy; y++) {
			int bph = x * 2 + y * szx * 2;
			int bph_left = (x + szx - 1) % szx * 2 + y * szx * 2;
			int bph_right = (x + 1) % szx * 2 + y * szx * 2;
			int bpv = x + y * szx * 2;
			int bpv_above = x + ((y + szy - 1) % szy) * szx * 2;
			int bpv_below = x + ((y + 1) % szy) * szx * 2;
			int bp = b.bitpos (x, y);
			if (board_mask != nullptr && board_mask->test_bit (bp)) {
				vgrid.clear_bit (bpv);
				vgrid.clear_bit (bpv + szx);
				vgrid.clear_bit (bpv_below);
				vgrid.clear_bit (bpv_above + szx);
				hgrid.clear_bit (bph);
				hgrid.clear_bit (bph + 1);
				hgrid.clear_bit (bph_left + 1);
				hgrid.clear_bit (bph_right);
				continue;
			}

			if ((visible != nullptr && !visible->test_bit (bp)) || grid_hidden.test_bit (bp)) {
				vgrid.clear_bit (bpv);
				vgrid.clear_bit (bpv + szx);
				hgrid.clear_bit (bph);
				hgrid.clear_bit (bph + 1);
				continue;
			}
			if (x == 0 && !b.torus_h ())
				hgrid.clear_bit (bph);
			if (x + 1 == szx && !b.torus_h ())
				hgrid.clear_bit (bph + 1);
			if (y == 0 && !b.torus_v ())
				vgrid.clear_bit (bpv);
			if (y + 1 == szy && !b.torus_v ())
				vgrid.clear_bit (bpv + szx);
		}

	/* Now we're ready to draw the grid.  */
	QPixmap image (m_wood_rect.size ());
	image.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&image);

	draw_grid (painter, std::move (hgrid), std::move (vgrid), grid_hidden);

	if (m_visualize_connections) {
		int line_offx = m_board_rect.left () - m_wood_rect.left ();
		int line_offy = m_board_rect.top () - m_wood_rect.top ();
		int scaled_w = (int)square_size * 2 / 3 + 1;
		QPen black_pen (Qt::black);
		QPen white_pen (Qt::white);
		black_pen.setWidth (scaled_w);
		white_pen.setWidth (scaled_w);
		for (int tx = 0; tx + 1 < visx + 2 * dups_x; tx++)
			for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
				int x = (tx + shiftx + (szx - dups_x)) % szx;
				int y = (ty + shifty + (szy - dups_y)) % szy;
				int xp1 = (tx + 1 + shiftx + (szx - dups_x)) % szx;
				stone_color c = b.stone_at (x, y);
				if (c == none)
					continue;
				painter.setPen (c == white ? white_pen : black_pen);
				if (b.stone_at (xp1, y) == c) {
					painter.drawLine (line_offx + tx * square_size, line_offy + ty * square_size,
							  line_offx + (tx + 1) * square_size, line_offy + ty * square_size);
				}
			}
		for (int tx = 0; tx < visx + 2 * dups_x; tx++)
			for (int ty = 0; ty + 1 < visy + 2 * dups_y; ty++) {
				int x = (tx + shiftx + (szx - dups_x)) % szx;
				int y = (ty + shifty + (szy - dups_y)) % szy;
				int yp1 = (ty + 1 + shifty + (szy - dups_y)) % szy;
				stone_color c = b.stone_at (x, y);
				if (c == none)
					continue;
				painter.setPen (c == white ? white_pen : black_pen);
				if (b.stone_at (x, yp1) == c) {
					painter.drawLine (line_offx + tx * square_size, line_offy + ty * square_size,
							  line_offx + tx * square_size, line_offy + (ty + 1) * square_size);
				}
			}
	}

	/* Now, draw stones.  Do this in two passes, with shadows first.  */
	painter.setPen (Qt::NoPen);
	int shadow_offx = m_board_rect.left () - m_wood_rect.left () - square_size / 2 - square_size / 8;
	int shadow_offy = m_board_rect.top () - m_wood_rect.top () - square_size / 2 + square_size / 8;
	for (int tx = 0; tx < visx + 2 * dups_x; tx++)
		for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
			int x = (tx + shiftx + (szx - dups_x)) % szx;
			int y = (ty + shifty + (szy - dups_y)) % szy;
			auto stone_display = stone_to_display (mn_board, visible, to_move, x, y, vars, var_type);
			stone_color sc = stone_display.first;
			stone_type type = stone_display.second;
			if (sc != none && type == stone_type::live) {
				painter.drawPixmap (shadow_offx + tx * square_size,
						    shadow_offy + ty * square_size,
						    imageHandler->getStonePixmaps ()->last ());
			}
		}

	int stone_offx = m_board_rect.left () - m_wood_rect.left () - square_size / 2;
	int stone_offy = m_board_rect.top () - m_wood_rect.top () - square_size / 2;
	for (int tx = 0; tx < visx + 2 * dups_x; tx++)
		for (int ty = 0; ty < visy + 2 * dups_y; ty++) {
			int x = (tx + shiftx + (szx - dups_x)) % szx;
			int y = (ty + shifty + (szy - dups_y)) % szy;
			auto stone_display = stone_to_display (mn_board, visible, to_move, x, y, vars, var_type);
			stone_color sc = stone_display.first;
			stone_type type = stone_display.second;
			if (sc != none) {
				int bp = b.bitpos (x, y);
				painter.drawPixmap (stone_offx + tx * square_size,
						    stone_offy + ty * square_size,
						    choose_stone_pixmap (sc, type, bp));
			}
		}

	/* Now render the marks on top of all that.  */
	if (!m_no_marks) {
		QTransform transform;
		transform.translate (m_board_rect.x () - m_wood_rect.x () - square_size / 2,
				     m_board_rect.y () - m_wood_rect.y () - square_size / 2);
		transform.scale (((double)m_board_rect.width () + square_size) / m_wood_rect.width (),
				 ((double)m_board_rect.height () + square_size) / m_wood_rect.height ());
		painter.setWorldTransform (transform);
		QSvgRenderer renderer (svg);
		renderer.render (&painter);
	}

	board_rect visible_sel = m_drawn_sel;
	visible_sel.intersect (m_crop);
	if (visible_sel != m_crop && visible_sel.height () > 0 && visible_sel.width () > 0) {
		visible_sel.translate (-m_crop.x1, -m_crop.y1);
		int x1 = m_wood_rect.left () + visible_sel.x1 * square_size;
		int x2 = m_wood_rect.left () + (visible_sel.x2 + 1) * square_size;
		int y1 = m_wood_rect.top () + visible_sel.y1 * square_size;
		int y2 = m_wood_rect.top () + (visible_sel.y2 + 1) * square_size;
		QRect sel_rect (x1, y1, x2 - x1 + 1, y2 - y1 + 1);
		QRegion r (m_wood_rect);
		r -= sel_rect;
		painter.setClipRegion (r);
		painter.setOpacity (0.5);
		painter.fillRect (m_wood_rect, Qt::black);
	}

	painter.end ();
	return image;
}

/* The central function for synchronizing visual appearance with the abstract board data.  */
void BoardView::sync_appearance (bool)
{
	if (m_never_sync)
		return;
	QPixmap stones = draw_position (m_vars_type);
	m_stone_layer.setPixmap (stones);
	m_stone_layer.setPos (m_wood_rect.x (), m_wood_rect.y ());
}

void Board::sync_appearance (bool board_only)
{
	if (!board_only) {
		setup_analyzer_position ();
	}
	if (!have_analysis ())
		m_board_win->set_eval (QString (), 0, false, 0, none, 0);

	BoardView::sync_appearance (board_only);
	const go_board &b = m_displayed->get_board ();
	m_board_win->recalc_scores (b);
	if (!board_only) {
		m_board_win->setMoveData (m_displayed);
		m_board_win->update_game_tree ();
	}
}

void Board::set_hide_analysis (bool on)
{
	if (m_hide_analysis == on)
		return;
	m_hide_analysis = on;
	sync_appearance (true);
}

void Board::set_analyzer_id (analyzer_id id)
{
	bool changed = m_an_id != id;
	m_an_id = id;
	if (changed)
		sync_appearance (true);
}

void Board::leaveEvent(QEvent*)
{
	curX = curY = -1;
	sync_appearance (true);
}

int BoardView::coord_vis_to_board_x (int p, bool small_hitbox)
{
	p -= m_board_rect.x () - square_size / 2;
	if (p < 0)
		return -1;
	if (small_hitbox) {
		int off = p - (int)(p / square_size) * square_size;
		off = off * 100. / square_size;
		if (off < 15 || off > 85)
			return -1;
	}
	p /= square_size;
	p -= n_dups_h ();
	if (p < 0)
		return -1;
	if (p >= m_dims.width ())
		return m_dims.width ();
	p += m_shift_x;
	p %= m_dims.width ();
	return p;
}

int BoardView::coord_vis_to_board_y (int p, bool small_hitbox)
{
	p -= m_board_rect.y () - square_size / 2;
	if (p < 0)
		return -1;
	if (small_hitbox) {
		int off = p - (int)(p / square_size) * square_size;
		off = off * 100. / square_size;
		if (off < 15 || off > 85)
			return -1;
	}
	p /= square_size;
	p -= n_dups_v ();
	if (p < 0)
		return -1;
	if (p >= m_dims.height ())
		return m_dims.height ();
	p += m_shift_y;
	p %= m_dims.height ();
	return p;
}

void BoardView::updateCovers ()
{
	coverLeft->setVisible (!m_sel.aligned_left (m_dims) || n_dups_h () > 0);
	coverRight->setVisible (!m_sel.aligned_right (m_dims) || n_dups_h () > 0);
	coverTop->setVisible (!m_sel.aligned_top (m_dims) || n_dups_v () > 0);
	coverBot->setVisible (!m_sel.aligned_bottom (m_dims) || n_dups_v () > 0);

	QRectF sceneRect = canvas->sceneRect ();
	int top_edge = m_board_rect.y () + square_size * (m_sel.y1 + n_dups_v () - 0.5);
	int bot_edge = m_board_rect.y () + square_size * (m_sel.y2 + n_dups_v () + 0.5);
	int left_edge = m_board_rect.x () + square_size * (m_sel.x1 + n_dups_h () - 0.5);
	int right_edge = m_board_rect.x () + square_size * (m_sel.x2 + n_dups_h () + 0.5);

	int sides_top_edge = coverTop->isVisible () ? top_edge : 0;
	int sides_bot_edge = coverBot->isVisible () ? bot_edge : sceneRect.bottom();
	coverTop->setRect (0, 0, sceneRect.right(), top_edge);
	coverBot->setRect (0, bot_edge, sceneRect.right(), sceneRect.bottom () - bot_edge);
	coverLeft->setRect (0, sides_top_edge, left_edge, sides_bot_edge - sides_top_edge);
	coverRight->setRect (right_edge, sides_top_edge,
			     sceneRect.right() - right_edge, sides_bot_edge - sides_top_edge);

	int alpha = n_dups_h () + n_dups_v () > 0 ? 32 : 128;
	coverTop->setBrush (QColor (0, 0, 0, alpha));
	coverBot->setBrush (QColor (0, 0, 0, alpha));
	coverLeft->setBrush (QColor (0, 0, 0, alpha));
	coverRight->setBrush (QColor (0, 0, 0, alpha));
}

void BoardView::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button () != Qt::LeftButton)
		return;
	m_rect_down_x = -1;
}

bool BoardView::have_selection ()
{
	return m_dims != m_sel;
}

void BoardView::update_rect_select (int x, int y)
{
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= m_dims.width ())
		x = m_dims.width () - 1;
	if (y >= m_dims.height ())
		y = m_dims.height () - 1;

	if (m_rect_down_x == -1)
		m_rect_down_x = x, m_rect_down_y = y;

	int minx = m_rect_down_x;
	int miny = m_rect_down_y;
	if (x < minx)
		std::swap (minx, x);
	if (y < miny)
		std::swap (miny, y);
	m_sel.x1 = minx;
	m_sel.y1 = miny;
	m_sel.x2 = x;
	m_sel.y2 = y;
	updateCovers ();
	canvas->update ();
}

go_pattern BoardView::selected_pattern ()
{
	return go_pattern (m_displayed->get_board (), m_sel.x2 + 1 - m_sel.x1, m_sel.y2 + 1 - m_sel.y1,
			   m_sel.x1, m_sel.y1);
}

void BoardView::mouseMoveEvent(QMouseEvent *e)
{
	if (m_rect_down_x == -1)
		return;

	int x = coord_vis_to_board_x (e->x ());
	int y = coord_vis_to_board_y (e->y ());

	update_rect_select (x, y);
}

void BoardView::mousePressEvent(QMouseEvent *e)
{
	if (e->button () != Qt::LeftButton)
		return;

	int x = coord_vis_to_board_x (e->x ());
	int y = coord_vis_to_board_y (e->y ());

	update_rect_select (x, y);
}

void Board::update_shift (int x, int y)
{
	const go_board &b = m_game->get_root ()->get_board ();
	if (!b.torus_h ())
		x = 0;
	if (!b.torus_v ())
		y = 0;
	if (m_shift_x == x && m_shift_y == y)
		return;
	m_shift_x = x;
	m_shift_y = y;
	draw_background ();
	sync_appearance ();
}

void Board::mouseMoveEvent(QMouseEvent *e)
{
	if (m_mark_rect) {
		BoardView::mouseMoveEvent (e);
	} else if (m_dragging) {
		int dist_x = (m_down_x - e->x ()) / square_size;
		int dist_y = (m_down_y - e->y ()) / square_size;
		dist_x += m_drag_begin_x;
		dist_y += m_drag_begin_y;
		while (dist_x < 0)
			dist_x += m_dims.width ();
		while (dist_y < 0)
			dist_y += m_dims.height ();
		dist_x %= m_dims.width ();
		dist_y %= m_dims.height ();
		update_shift (dist_x, dist_y);
		return;
	}

	int x = coord_vis_to_board_x (e->x (), setting->values.clicko_hitbox && m_game_mode == modeMatch);
	int y = coord_vis_to_board_y (e->y (), setting->values.clicko_hitbox && m_game_mode == modeMatch);

	// Outside the valid board?
	std::shared_ptr<const bit_array> board_mask = m_game->get_board_mask ();
	const go_board &b = m_game->get_root ()->get_board ();
	if (!m_dims.contained (x, y) || (board_mask != nullptr && board_mask->test_bit (b.bitpos (x, y))))
	{
		x = -1;
		y = -1;
	}

	bool show = show_cursor_p ();
	if (curX == x && curY == y && m_cursor_shown == show)
		return;
	curX = x;
	curY = y;
	m_cursor_shown = show;

	// Update the statusbar coords tip
	if (x != -1) {
		auto name = b.coords_name (curX, curY, m_sgf_coords);
		m_board_win->coords_changed (QString::fromStdString (name.first), QString::fromStdString (name.second));
	} else
		m_board_win->coords_changed ("", "");
	sync_appearance (true);
}

void Board::wheelEvent(QWheelEvent *e)
{
	// leave if observing or playing
	if (m_game_mode != modeObserve && m_game_mode != modeNormal)
		return;

#if 0 /* Let's try without this, and with cumulative angles instead.  */
	// Check delay
	if (QTime::currentTime() < wheelTime)
		return;
#endif
	// Needs an extra check on variable mouseState as state() does not work on Windows.
	QPoint numDegrees = e->angleDelta();
	m_cumulative_delta += numDegrees.y ();
	if (m_cumulative_delta < -60) // wheel down to next
	{
		if (e->buttons () == Qt::RightButton || mouseState == Qt::RightButton)
			m_board_win->nav_next_variation ();
		else
			m_board_win->nav_next_move ();
		m_cumulative_delta = 0;
	} else if (m_cumulative_delta > 60) {
		if (e->buttons () == Qt::RightButton || mouseState == Qt::RightButton)
			m_board_win->nav_previous_variation ();
		else
			m_board_win->nav_previous_move ();
		m_cumulative_delta = 0;
	}

	// Delay of 100 msecs to avoid too fast scrolling
	wheelTime = QTime::currentTime();
	wheelTime = wheelTime.addMSecs(50);

	e->accept();
}

void Board::play_one_move (int x, int y)
{
	if (!player_to_move_p ())
		return;

	m_board_win->player_move (x, y);
}

void Board::mouseReleaseEvent(QMouseEvent* e)
{
	mouseState = Qt::NoButton;

	if (e->button () == Qt::RightButton)
		pause_eval_updates (false);

	if (m_dragging) {
		m_dragging = false;
		m_down_x = -1;
	}

	if (m_mark_rect) {
		m_mark_rect = false;
		BoardView::mouseReleaseEvent (e);
		m_board_win->done_rect_select ();
		return;
	}
	int x = coord_vis_to_board_x (e->x ());
	int y = coord_vis_to_board_y (e->y ());

	if (m_down_x == -1 || x != m_down_x || y != m_down_y)
		return;

	//qDebug("Mouse should be released after %d,%03d", wheelTime.second(),wheelTime.msec());
	//qDebug("Mouse released at time         %d,%03d", QTime::currentTime().second(),QTime::currentTime().msec());

	if (m_game_mode != modeMatch || QTime::currentTime() <= wheelTime)
		return;

	play_one_move (x, y);
}

void Board::click_add_mark (QMouseEvent *e, int x, int y)
{
	if (e->button () != Qt::RightButton && e->button () != Qt::LeftButton)
		return;

	mark mark_to_set = m_edit_mark;
	mextra mark_extra = 0;

	if (e->button () == Qt::RightButton)
		mark_to_set = mark::none;

	go_board b = m_displayed->get_board ();

	if (mark_to_set == mark::letter && e->modifiers () == Qt::ShiftModifier) {
		TextEditDialog dlg (m_board_win);
		dlg.textLineEdit->setFocus();

		if (b.mark_at (x, y) == mark::text)
			dlg.textLineEdit->setText (QString::fromStdString (b.mark_text_at (x, y)));

		if (dlg.exec() == QDialog::Accepted) {
			b.set_text_mark (x, y, dlg.textLineEdit->text().toStdString ());
			if (m_game_mode != modeEdit) {
				m_board_win->notice_mark_change (b);
				setModified ();
			}
			m_displayed->replace (b, m_displayed->to_move ());
			sync_appearance (true);
		}

		return;
	}
	if (mark_to_set == mark::num) {
		int i;
		for (i = 1; i < 256; i++)
			if (!m_used_numbers.test_bit (i))
				break;
		if (i == 256)
			return;
		mark_extra = i;
	} else if (m_edit_mark == mark::letter) {
		int i;
		for (i = 0; i < 52; i++)
			if (!m_used_letters.test_bit (i))
				break;
		if (i == 52)
			return;
		mark_extra = i;
	}

	bool changed = b.set_mark (x, y, mark_to_set, mark_extra);
	if (changed) {
		if (m_game_mode != modeEdit) {
			m_board_win->notice_mark_change (b);
			setModified ();
		}
		m_displayed->replace (b, m_displayed->to_move ());
		sync_appearance (true);
	}
}

void SimpleBoard::wheelEvent (QWheelEvent *e)
{
	QPoint numDegrees = e->angleDelta ();
	m_cumulative_delta += numDegrees.y ();
	if (m_cumulative_delta < -60) {
		emit signal_nav_forward ();
		m_cumulative_delta = 0;
	} else if (m_cumulative_delta > 60) {
		emit signal_nav_backward ();
		m_cumulative_delta = 0;
	}

	e->accept ();
}

void SimpleBoard::mouseReleaseEvent (QMouseEvent *e)
{
	if (e->button () == Qt::RightButton)
		end_rect_select ();
}

void SimpleBoard::mousePressEvent (QMouseEvent *e)
{
	if (e->button () == Qt::RightButton) {
		int x = coord_vis_to_board_x (e->x ());
		int y = coord_vis_to_board_y (e->y ());

		update_rect_select (x, y);
		return;
	}

	int x = coord_vis_to_board_x (e->x ());
	int y = coord_vis_to_board_y (e->y ());
	std::shared_ptr<const bit_array> board_mask = m_game->get_board_mask ();
	const go_board &b = m_game->get_root ()->get_board ();
	if (!m_dims.contained (x, y) || (board_mask != nullptr && board_mask->test_bit (b.bitpos (x, y))))
		return;

	if (e->button () == Qt::LeftButton) {
		game_state *st = m_displayed;
		stone_color col = m_forced_color;
		if (col == none)
			col = (e->modifiers () == Qt::ShiftModifier ? black
			       : e->modifiers () == Qt::ControlModifier ? white
			       : st->to_move ());
		if (!st->valid_move_p (x, y, col))
			return;
		for (game_state *c: st->children ())
			if (c->was_move_p () && c->get_move_x () == x && c->get_move_y () == y) {
				transfer_displayed (st, c);
				emit signal_move_made ();
				return;
		}
		go_board new_board (st->get_board (), mark::none);
		new_board.add_stone (x, y, col);
		game_state *st_new = st->add_child_move (new_board, col, x, y);
		transfer_displayed (st, st_new);
		emit signal_move_made ();
	}
}

void Board::mousePressEvent(QMouseEvent *e)
{
	mouseState = e->button ();

	if (e->button () == Qt::RightButton)
		pause_eval_updates (true);

	if (m_request_mark_rect && e->button () == Qt::LeftButton) {
		BoardView::mousePressEvent (e);
		m_mark_rect = true;
		m_request_mark_rect = false;
		return;
	}

	if (e->button () == Qt::MiddleButton) {
		const go_board &b = m_game->get_root ()->get_board ();
		if (b.torus_h () || b.torus_v ()) {
			m_down_x = e->x ();
			m_down_y = e->y ();
			m_dragging = true;
			m_drag_begin_x = m_shift_x;
			m_drag_begin_y = m_shift_y;
			return;
		}
	}

	m_down_x = m_down_y = -1;

	int x = coord_vis_to_board_x (e->x (), setting->values.clicko_hitbox && m_game_mode == modeMatch);
	int y = coord_vis_to_board_y (e->y (), setting->values.clicko_hitbox && m_game_mode == modeMatch);
	std::shared_ptr<const bit_array> board_mask = m_game->get_board_mask ();
	const go_board &b = m_game->get_root ()->get_board ();
	if (!m_dims.contained (x, y) || (board_mask != nullptr && board_mask->test_bit (b.bitpos (x, y))))
		return;

	m_down_x = x;
	m_down_y = y;

	if (navIntersectionStatus) {
		navIntersectionStatus = false;
		unsetCursor ();
		m_board_win->nav_find_move (x, y);
		return;
	}
	if (m_eval_state != nullptr
	    && ((e->modifiers () == Qt::ShiftModifier && e->button () == Qt::LeftButton)
		|| e->button () == Qt::MiddleButton))
	{
		game_state *evchild = m_eval_state->find_child_move (x, y);
		if (evchild != nullptr) {
			eval ev = evchild->best_eval ();
			game_state *st = m_displayed;
			game_state *dup = evchild->duplicate (st);
			QString comment = tr ("Live evaluation: W %1%2 B %3%4 at %5 visits");
			double bwr = ev.wr_black;
			double wwr = 1 - bwr;
			comment = (comment.arg (QString::number (100 * wwr, 'f', 1)).arg ('%')
				   .arg (QString::number (100 * bwr, 'f', 1)).arg ('%')
				   .arg (QString::number (ev.visits)));
			if (ev.id.engine.length () > 0)
				comment += QString (" (%1)").arg (QString::fromStdString (ev.id.engine));
			dup->set_figure (256, comment.toStdString ());
			m_board_win->add_engine_pv (st, dup);

			sync_appearance ();
			return;
		}
	}
	if (e->modifiers () == Qt::ControlModifier
	    && (m_game_mode == modeNormal || m_game_mode == modeObserve))
	{
		/* @@@ Previous code made a distinction between left and right button.  */
		m_board_win->nav_find_move (x, y);
		return;
	}
	/* All modes where marks are not allowed, including scoring, force the editStone
	   button instead of the marks.  So this is a simple test.  */
	if (m_edit_mark != mark::none) {
		click_add_mark (e, x, y);
		return;
	}
	if (m_game_mode == modeObserve)
		return;

	 /* See if this is a simple click that should add a stone.
	    @@@ teaching mode is untested; best guess.  */
	if (m_game_mode == modeNormal || m_game_mode == modeComputer || m_game_mode == modeTeach) {
		if (e->button () == Qt::LeftButton)
			play_one_move (x, y);
		return;
	}

	if (m_game_mode == modeMatch) {
		// Delay of 250 msecs to avoid clickos
		wheelTime = QTime::currentTime ();
		//qDebug("Mouse pressed at time %d,%03d", wheelTime.second(),wheelTime.msec());
		if (setting->values.clicko_delay)
			wheelTime = wheelTime.addMSecs (250);
		return;
	}

	/* Deal with edit/score modes.  */
	stone_color existing_stone;
	go_board shown_b = m_displayed->get_board ();
	switch (m_game_mode)
	{
	case modeEdit:
		existing_stone = shown_b.stone_at (x, y);
		switch (e->button())
		{
		case Qt::LeftButton:
			if (existing_stone == black)
				shown_b.set_stone (x, y, none);
			else
				shown_b.set_stone (x, y, black);
			m_displayed->replace (shown_b, m_displayed->to_move ());
			sync_appearance (true);
			break;
		case Qt::RightButton:
			if (existing_stone == white)
				shown_b.set_stone (x, y, none);
			else
				shown_b.set_stone (x, y, white);
			m_displayed->replace (shown_b, m_displayed->to_move ());
			sync_appearance (true);
			break;

		default:
			break;
		}
		break;

	case modeScoreRemote:
		if (e->button () == Qt::LeftButton) {
			m_board_win->player_toggle_dead (x, y);
#if 0 /* We get our own toggle back from the server.  No need to do anything like this here.  */
			shown_b,toggle_alive (x, y);
			/* See comment in mark_dead_external.  */
			shown_b,calc_scoring_markers_simple ();
			m_displayed->replace (shown_b, m_displayed->to_move ());
			observed_changed ();
#endif
		}
		break;
	case modeScore:
		switch (e->button())
		{
		case Qt::LeftButton:
			shown_b.toggle_alive (x, y);
			shown_b.calc_scoring_markers_complex ();
			m_displayed->replace (shown_b, m_displayed->to_move ());
			sync_appearance ();
			break;
		case Qt::RightButton:
			shown_b.toggle_seki (x, y);
			shown_b.calc_scoring_markers_complex ();
			m_displayed->replace (shown_b, m_displayed->to_move ());
			sync_appearance ();
			break;
		default:
			break;
		}
		break;

	default:
		qWarning("   *** Invalid game mode! ***");
    }
}

void BoardView::changeSize ()
{
#ifdef Q_OS_WIN
	resizeDelayFlag = false;
#endif
	resizeBoard (width (), height ());
}

void BoardView::set_margin (int m)
{
	if (m_margin == m)
		return;
	m_margin = m;
	changeSize ();
}

void BoardView::clear_selection ()
{
	m_sel = m_dims;
	m_drawn_sel = m_dims;
	updateCovers ();
}

int BoardView::n_dups_h ()
{
	const go_board &b = m_displayed->get_board ();
	if (!m_figure_view && b.torus_h ())
		return std::min (b.size_x (), setting->values.toroid_dups);
	return 0;
}

int BoardView::n_dups_v ()
{
	const go_board &b = m_displayed->get_board ();
	if (!m_figure_view && b.torus_v ())
		return std::min (b.size_y (), setting->values.toroid_dups);
	return 0;
}

void BoardView::clear_graphics_elts ()
{
	m_shift_x = m_shift_y = 0;
}

void BoardView::alloc_graphics_elts (bool reset)
{
	const go_board &b = m_displayed->get_board ();

	m_dims = board_rect (b);

	if (reset)
		clear_crop ();
	calculateSize ();
}

void BoardView::reset_game (go_game_ptr gr)
{
	clear_graphics_elts ();

	m_game = gr;
	game_state *root = gr->get_root ();
	m_displayed = root;
	m_cont = nullptr;
	const go_board &b = root->get_board ();
	m_dims = board_rect (b);
	/* Add hoshis unless we have masked off intersections from the board.  */
	if (gr->get_board_mask () == nullptr)
		m_hoshis = calculate_hoshis (b);
	else
		m_hoshis = bit_array (b.bitsize ());
	/* Precompute outline bitmaps for use in draw_grid.  */
	int szx = b.size_x ();
	int szy = b.size_y ();
	m_vgrid_outline = bit_array (b.bitsize () * 2);
	m_hgrid_outline = bit_array (b.bitsize () * 2);
	for (int y = 1; y < szy * 2 - 1; y++) {
		m_vgrid_outline.set_bit (y * szx);
		m_vgrid_outline.set_bit (szx - 1 + y * szx);
	}
	for (int x = 1; x < szx * 2 - 1; x++) {
		m_hgrid_outline.set_bit (x);
		m_hgrid_outline.set_bit (x + (szy - 1) * szx * 2);
	}

	alloc_graphics_elts (true);

	draw_background ();

	clear_selection ();

	canvas->update ();
	sync_appearance ();
}

void BoardView::clear_crop ()
{
	m_crop = m_dims;
}

void BoardView::set_crop (const board_rect &r)
{
	if (r == m_crop)
		return;
	m_crop = r;
	clear_graphics_elts ();
	alloc_graphics_elts (false);
}

void BoardView::set_displayed (game_state *st)
{
	if (st == m_displayed)
		return;
	m_cont = nullptr;
	m_displayed = st;
	sync_appearance (false);
}

void BoardView::replace_displayed (const go_board &b)
{
	m_displayed->replace (b, m_displayed->to_move ());
	sync_appearance (false);
}

void BoardView::transfer_displayed (game_state *from, game_state *to)
{
	if (m_displayed == from)
		set_displayed (to);
}

void Board::reset_game (go_game_ptr gr)
{
	BoardView::reset_game (gr);

	m_dragging = false;
	m_request_mark_rect = false;
}

void BoardView::update_prefs ()
{
	clear_graphics_elts ();
	alloc_graphics_elts (false);

	draw_background ();
	updateCovers ();

	sync_appearance ();
}

void BoardView::set_show_coords (bool b)
{
	bool old = m_show_coords;
	m_show_coords = b;
	if (old == m_show_coords)
		return;

	changeSize();
}

void BoardView::set_visualize_connections (bool b)
{
	bool old = m_visualize_connections;
	m_visualize_connections = b;
	if (old == m_visualize_connections)
		return;

	sync_appearance ();
}

void BoardView::set_show_hoshis (bool b)
{
	bool old = m_show_hoshis;
	m_show_hoshis = b;
	if (old == m_show_hoshis)
		return;

	sync_appearance ();
}

void BoardView::set_show_move_numbers (bool b)
{
	bool old = m_move_numbers;
	m_move_numbers = b;
	if (old == m_move_numbers)
		return;

	sync_appearance ();
}

void BoardView::set_show_figure_caps (bool b)
{
	bool old = m_show_figure_caps;
	m_show_figure_caps = b;
	if (old == m_show_figure_caps)
		return;

	sync_appearance ();
}

void BoardView::set_sgf_coords (bool b)
{
	bool old = m_sgf_coords;
	m_sgf_coords = b;
	if (old == m_sgf_coords)
		return;

	draw_background ();
}

void Board::mark_dead_external (int x, int y)
{
	go_board b = m_displayed->get_board ();
	b.toggle_alive (x, y, false);
	/* There's evidence to suggest that the IGS algorithm at least has
	   no fancy tricks to find false eyes and such, and we should at
	   least try to match the final result that the server will
	   caclulate.
	   See also the modeScoreRemote case in the mouse event handler.  */
	b.calc_scoring_markers_simple ();
	m_displayed->replace (b, m_displayed->to_move ());
	sync_appearance ();
}

void BoardView::set_vardisplay (bool children, int type)
{
	m_vars_children = children;
	m_vars_type = type;

	sync_appearance (true);
}

void Board::setModified (bool m)
{
	if (m == m_game->modified () || m_game_mode == modeObserve)
		return;

	m_game->set_modified (m);
	m_board_win->update_game_record ();
}

QPixmap BoardView::grabPicture()
{
	int minx = m_wood_rect.x () + 2;
	int miny = m_wood_rect.y () + 2;
	int maxx = minx + m_wood_rect.width () - 4;
	int maxy = miny + m_wood_rect.height () - 4;
	if (!m_sel.aligned_left (m_dims))
		minx = coverLeft->rect ().right ();
	if (!m_sel.aligned_right (m_dims))
		maxx = coverRight->rect ().left ();
	if (!m_sel.aligned_top (m_dims))
		miny = coverTop->rect ().bottom ();
	if (!m_sel.aligned_bottom (m_dims))
		maxy = coverBot->rect ().top ();
	return grab (QRect (minx, miny, maxx - minx, maxy - miny));
}

void Board::navIntersection()
{
	setCursor(Qt::PointingHandCursor);
	navIntersectionStatus = true;
}

void Board::setup_analyzer_position ()
{
	if (analyzer_state () == analyzer::running)
		request_analysis (m_game, m_displayed);
}

void Board::gtp_startup_success (GTP_Process *)
{
	m_board_win->update_analysis (analyzer::running);
	setup_analyzer_position ();
}

void Board::gtp_failure (GTP_Process *, const QString &err)
{
	clear_eval_data ();
	m_board_win->update_analysis (analyzer::disconnected);
	QMessageBox msg(QString (QObject::tr("Error")), err,
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default,
			Qt::NoButton, Qt::NoButton);
	msg.exec();
}

void Board::gtp_exited (GTP_Process *)
{
	clear_eval_data ();
	m_board_win->update_analysis (analyzer::disconnected);
	QMessageBox::warning (this, PACKAGE, QObject::tr ("GTP process exited unexpectedly."));
}

void Board::eval_received (const analyzer_id &id, const QString &move, int visits, bool have_score)
{
	m_board_win->update_analyzer_ids (id, have_score);
	m_displayed->update_eval (*m_eval_state);
	m_board_win->set_eval (move, m_primary_eval, m_primary_have_score, m_primary_score,
			       m_displayed->to_move (), visits);
	sync_appearance ();
}

void Board::start_analysis (const Engine &e)
{
	if (!m_dims.is_square ()) {
		QMessageBox::warning (this, PACKAGE, tr ("Analysis is supported only for square boards!"));
		return;
	}

	start_analyzer (e, m_dims.width (), 7.5);
	m_board_win->update_analysis (analyzer::starting);
}

void Board::stop_analysis ()
{
	stop_analyzer ();
	m_board_win->update_analysis (analyzer::disconnected);
}

void Board::pause_analysis (bool on)
{
	if (on)
		pause_analyzer ();
	else
		request_analysis (m_game, m_displayed);
	m_board_win->update_analysis (analyzer_state ());
}

FigureView::FigureView(QWidget *parent, bool no_sync, bool no_marks)
	: BoardView (parent, no_sync, no_marks)
{
	m_figure_view = true;
}

void FigureView::hide_unselected (bool)
{
	const go_board &b = m_displayed->get_board ();
	bit_array arr (b.bitsize ());
	for (int x = 0; x < b.size_x (); x++)
		for (int y = 0; y < b.size_y (); y++) {
			if (m_sel.contained (x, y))
				arr.set_bit (b.bitpos (x, y));
		}
	m_displayed->set_visible (new bit_array (arr));
	sync_appearance (true);
}

void FigureView::make_all_visible (bool)
{
	if (m_displayed->root_node_p () || m_displayed->prev_move ()->visible_inherited () == nullptr) {
		m_displayed->set_visible (nullptr);
	} else {
		const go_board &b = m_displayed->get_board ();
		bit_array arr (b.bitsize (), true);
		m_displayed->set_visible (new bit_array (arr));
	}
	sync_appearance (true);
}

void FigureView::contextMenuEvent (QContextMenuEvent *e)
{
	QMenu menu;
	menu.addAction (QObject::tr ("Edit diagram options..."), m_board_win, &MainWindow::slotDiagEdit);
	menu.addSeparator ();
	menu.addAction (QObject::tr ("Export &ASCII..."), m_board_win, &MainWindow::slotDiagASCII);
	menu.addAction (QObject::tr ("Export S&VG..."), m_board_win, &MainWindow::slotDiagSVG);
	if (have_selection ()) {
		menu.addSeparator ();
		menu.addAction (QObject::tr ("&Clear selection"), [this] (bool) { clear_selection (); });
		menu.addAction (QObject::tr ("&Hide unselected"), this, &FigureView::hide_unselected);
	}
	if (m_displayed->visible_inherited () != nullptr) {
		menu.addSeparator ();
		menu.addAction (QObject::tr ("Make all &visible"), this, &FigureView::make_all_visible);
		if (m_displayed->visible () != nullptr)
			menu.addAction (QObject::tr ("C&lear visibility state, inherit from parent"),
					[this] (bool) { m_displayed->set_visible (nullptr); sync_appearance (true); });
	}
	menu.exec (e->globalPos ());
}
