/*
* board.cpp
*/
#include <vector>
#include <fstream>

#include <qmessagebox.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qlineedit.h>
#include <qcursor.h>

#include <QPixmap>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QWheelEvent>

#include "config.h"
#include "setting.h"
#include "qgo.h"
#include "board.h"
#include "globals.h"
#include "imagehandler.h"
#include "tip.h"
#include "mainwindow.h"
#include "miscdialogs.h"

Board::Board(QWidget *parent, QGraphicsScene *c)
	: QGraphicsView(c, parent), m_game (nullptr)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	viewport()->setMouseTracking(true);
	setUpdatesEnabled(true);

	board_size = DEFAULT_BOARD_SIZE;
	showCoords = setting->readBoolEntry("BOARD_COORDS");
	showSGFCoords = setting->readBoolEntry("SGF_BOARD_COORDS");
	antiClicko = setting->readBoolEntry("ANTICLICKO");

	setStyleSheet( "QGraphicsView { border-style: none; }" );

	// Create an ImageHandler instance.
	imageHandler = new ImageHandler();
	CHECK_PTR(imageHandler);

	// Init the canvas
	canvas = new QGraphicsScene(0,0, BOARD_X, BOARD_Y,this);
	setScene(canvas);
	gatter = new Gatter(canvas, board_size);

	// Init the gatter size and the imagehandler pixmaps
	calculateSize();

	imageHandler->init(square_size);

	// Initialize some class variables
	fastLoad = false;
	isModified = false;
	mouseState = Qt::NoButton;

	//coordsTip = new Tip(this);
#ifdef Q_OS_WIN
	resizeDelayFlag = false;
#endif
	curX = curY = -1;

	isLocalGame = true;

	lockResize = false;
	navIntersectionStatus = false;

	isHidingStones = false; // QQQ

	canvas->addItem (coverTop = new QGraphicsRectItem ());
	canvas->addItem (coverBot = new QGraphicsRectItem ());
	canvas->addItem (coverLeft = new QGraphicsRectItem ());
	canvas->addItem (coverRight = new QGraphicsRectItem ());

	coverTop->setBrush (QColor (0, 0, 0, 128));
	coverBot->setBrush (QColor (0, 0, 0, 128));
	coverLeft->setBrush (QColor (0, 0, 0, 128));
	coverRight->setBrush (QColor (0, 0, 0, 128));
	coverTop->setPen (QColor (0, 0, 0, 0));
	coverBot->setPen (QColor (0, 0, 0, 0));
	coverLeft->setPen (QColor (0, 0, 0, 0));
	coverRight->setPen (QColor (0, 0, 0, 0));

	setupCoords();
	setFocusPolicy(Qt::NoFocus);
}


void Board::setupCoords()
{
	QString hTxt,vTxt;

	// Init the coordinates
	for (int i=0; i<board_size; i++)
	{
		if (showSGFCoords)
		{
			vTxt = QString(QChar(static_cast<const char>('a' + i)));
			hTxt = QString(QChar(static_cast<const char>('a' + i)));
		} else {
			int real_i = i < 8 ? i : i + 1;
			vTxt = QString::number(i + 1);
			hTxt = QString(QChar(static_cast<const char>('A' + real_i)));
		}

		QGraphicsSimpleTextItem *t;
		vCoords1.append(t = new QGraphicsSimpleTextItem(vTxt));
		canvas->addItem (t);
		hCoords1.append(t = new QGraphicsSimpleTextItem(hTxt));
		canvas->addItem (t);
		vCoords2.append(t = new QGraphicsSimpleTextItem(vTxt));
		canvas->addItem (t);
		hCoords2.append(t = new QGraphicsSimpleTextItem(hTxt));
		canvas->addItem (t);
	}
}

void Board::clearCoords()
{
	QList<QGraphicsSimpleTextItem*>::const_iterator i;
#define FREE_ARRAY_OF_POINTERS(a)							\
	for (i = a.begin(); i != a.end(); ++i)					\
		delete *i;											\
	a.clear();												\

	FREE_ARRAY_OF_POINTERS(vCoords1);
	FREE_ARRAY_OF_POINTERS(hCoords1);
	FREE_ARRAY_OF_POINTERS(vCoords2);
	FREE_ARRAY_OF_POINTERS(hCoords2);
}

Board::~Board()
{
	clear_stones ();
	delete canvas;
	delete imageHandler;
}

// distance from table edge to wooden board edge
const int Board::margin = 2;

// distance from coords to surrounding elements
const int Board::coord_margin = 4;

void Board::calculateSize()
{
	// Calculate the size values

	int w = (int)canvas->width() - margin * 2;
	int h = (int)canvas->height() - margin * 2;

	table_size = w < h ? w : h;

	QGraphicsSimpleTextItem coordV (QString::number(board_size));
	QGraphicsSimpleTextItem coordH ("A");
	canvas->addItem (&coordV);
	canvas->addItem (&coordH);
	int coord_width = (int)coordV.boundingRect().width();
	int coord_height = (int)coordH.boundingRect().height();

	// space for coodinates if shown
	coord_offset = coord_width < coord_height ? coord_height : coord_width;

	//we need 1 more virtual 'square' for the stones on 1st and last line getting off the grid
	if (showCoords)
		square_size = (table_size - 2 * (coord_offset + coord_margin * 2));
	else
		square_size = table_size;
	square_size /= (float)board_size;
	// Should not happen, but safe is safe.
	if (square_size == 0)
		  square_size = 1;

	// grid size
	board_pixel_size = square_size * (board_size - 1);
	offset = (table_size - board_pixel_size)/2;

	// Center the board in canvas

	offsetX = margin + (w - board_pixel_size) / 2;
	offsetY = margin + (h - board_pixel_size) / 2;
}

void Board::resizeBoard (int w, int h)
{
	if (w < 30 || h < 30)
		return;

	// Resize canvas
	canvas->setSceneRect (0, 0, w, h);

	// Recalculate the size values
	calculateSize ();

	// Rescale the pixmaps in the ImageHandler
	imageHandler->rescale (square_size);

	// Redraw the board
	drawBackground ();
	drawGatter ();
	drawCoordinates ();
	updateCovers ();

	sync_appearance ();
}

void Board::resizeEvent(QResizeEvent*)
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

void Board::drawBackground()
{
	int w = (int)canvas->width();
	int h = (int)canvas->height();

	m_wood = *setting->wood_image ();
	m_table = *setting->table_image ();

	// Create pixmap of appropriate size
	//QPixmap all(w, h);
	QImage image(w, h, QImage::Format_RGB32);

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin(&image);
	painter.setPen(Qt::NoPen);

	painter.drawTiledPixmap (0, 0, w, h, m_table);
	painter.drawTiledPixmap (offsetX - offset, offsetY - offset, table_size, table_size, m_wood);

	painter.end();

	// Modify the edges of the board so they appear slightly three-dimensional.
	//QImage image = all.toImage();
	int lighter=20;
	int darker=60;
	int width = 3;

	for(int x = 0; x < width; x++)
		for (int yi = x; yi < table_size - x; yi++)
		{
			int y = yi + offsetY - offset;
			int x1 = offsetX - offset + x;
			image.setPixel(x1, y,
				       QColor(image.pixel(x1,y)).dark(int(100 + darker*(width-x)*(width-x)/width/width)).rgb());

			int x2 = offsetX - offset + table_size - x - 1;
			image.setPixel(x2, y,
				       QColor(image.pixel(x2,y)).light(100+ int(lighter*(width-x)*(width-x)/width/width)).rgb());
		}
	for(int y = 0; y < width; y++)
		for (int xi = y; xi < table_size - y; xi++)
		{
			int x = xi + offsetX - offset;
			int y1 = offsetY - offset + y;
			image.setPixel(x, y1,
				       QColor(image.pixel(x, y1)).light(int(100 + lighter*(width-y)*(width-y)/width/width)).rgb());
			int y2 = offsetY - offset + table_size - y - 1;
			image.setPixel(x, y2,
				       QColor(image.pixel(x, y2)).dark(100+ int(darker*(width-y)*(width-y)/width/width)).rgb());
		}

	// Draw a shadow below the board
	width = 10;
	darker=50;

	for (int x = 0; x <= width && offsetX - offset - x > 0; x++)
		for (int yi = x; yi < table_size - x; yi++)
		{
			int y = yi + offsetY - offset;
			image.setPixel(offsetX - offset - 1 - x, y,
				       QColor(image.pixel(offsetX - offset - 1 - x,y)).dark(int(100 + darker*(width-x)/width)).rgb());
		}

	for (int y = 0; y <= width && offsetY + board_pixel_size + offset + y + 1 < h;y++)
		for (int x = (offsetX - offset - y > 0 ? offsetX - offset - y : 0); x < offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(x, offsetY + board_pixel_size + offset + y +1,
				QColor(image.pixel(x,offsetY + board_pixel_size + offset+y+1)).dark(100+ int(darker*(width-y)/width)).rgb());
		}

	canvas->setBackgroundBrush(QBrush(image));
}

void Board::drawGatter()
{
	gatter->resize(offsetX,offsetY,square_size);
}

void Board::drawCoordinates()
{
	QGraphicsSimpleTextItem *coord;
	int i;

	// centres the coordinates text within the remaining space at table edge
	const int coord_centre = coord_offset/2 + coord_margin;
	QString txt;

	for (i=0; i<board_size; i++)
	{
		// Left side
		coord = vCoords1.at(i);
		coord->setPos(offsetX - offset + coord_centre - coord->boundingRect().width()/2,
			      offsetY + square_size * (board_size - i - 1) - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Right side
		coord = vCoords2.at(i);
    		coord->setPos(offsetX + board_pixel_size + offset - coord_centre - coord->boundingRect().width()/2,
			      offsetY + square_size * (board_size - i - 1) - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Top
		coord = hCoords1.at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2,
			      offsetY - offset + coord_centre - coord->boundingRect().height()/2 );

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Bottom
		coord = hCoords2.at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2,
			      offsetY + offset + board_pixel_size - coord_centre - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();
	}
}

/* Handle a click on the Done button.  Return true if we should return to normal mode.  */
bool Board::doCountDone()
{
	game_state *new_st = m_state->add_child_edit (*m_edit_board, m_edit_to_move, true);
	m_state->transfer_observers (new_st);

	return true;
}

void Board::setMode (GameMode mode)
{
	game_state *new_st = m_state;

	GameMode old_mode = m_game_mode;
	m_game_mode = mode;

	if (mode == modeScore || mode == modeScoreRemote || mode == modeEdit) {
		m_edit_board = new go_board (m_state->get_board ());
		m_edit_changed = false;
		m_edit_to_move = m_state->to_move ();
		if (mode == modeScore || mode == modeScoreRemote) {
			m_edit_board->calc_scoring_markers_complex ();
		}
	} else if (old_mode == modeScore || old_mode == modeScoreRemote) {
		/* The only way the scored board is added to the game tree is through
		   doCountDone.  This may have happened at this point; in any case,
		   discard the board now.  */
		delete m_edit_board;
		m_edit_board = nullptr;
 	} else if (mode == modeNormal && old_mode == modeEdit) {
		m_edit_board->identify_units ();
		/* Normally, we add an edited board as a new child.  However, when the original position
		   was the root node, or an edit node, and has no children yet, just update it in-place.
		   It can be debated whether this behaviour would be surprising to users, but it seems like
		   the most useful way to handle the situation.  */
		if (m_state->n_children () == 0
		    && (m_state->root_node_p () || m_state->was_edit_p ()))
		{
			m_state->replace (*m_edit_board, m_edit_to_move);
		} else if (m_state->to_move () != m_edit_to_move || m_state->get_board () != *m_edit_board)
			new_st = m_state->add_child_edit (*m_edit_board, m_edit_to_move);
		delete m_edit_board;
		m_edit_board = nullptr;
	}
	if (new_st != m_state)
		m_state->transfer_observers (new_st);

	/* Always needed when changing modes to update toolbar buttons etc.  */
	sync_appearance (false);
}

bool Board::show_cursor_p ()
{
	if (!setting->readBoolEntry("CURSOR") || navIntersectionStatus)
		return false;

	if (m_mark_rect || m_request_mark_rect)
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

/* Construct ASCII diagrams suitable for use on lifein19x19.com.
   Moves can be numbered 1-10.  When we do numbering, we split the moves up
   into a suitable number of diagrams, inserting breaks when 10 moves are
   exceeded or a stone is placed on an intersection which previously
   held something else.

   This should really be part of game_state, but there is the added complication
   of the m_edit_board.  */

QString Board::render_ascii (bool do_number, bool coords)
{
	int sz = m_game->boardsize ();
	QString result;

	int *count_map = new int[sz * sz]();
	game_state *startpos = m_state;
	if (do_number && m_edit_board == nullptr && !m_state->get_start_count ()) {
		startpos = m_state;
		while (startpos
		       && (startpos->was_move_p () || startpos->root_node_p ())
		       && !startpos->get_start_count ())
		{
			startpos = startpos->prev_move ();
		}
		if (startpos == nullptr || (!startpos->was_move_p () && !startpos->root_node_p ()))
			startpos = m_state;
	}
	int moves = 1;
	do {
		const go_board &b = m_edit_board == nullptr ? startpos->get_board () : *m_edit_board;

		int n_mv = 0;
		game_state *next = startpos;
		memset (count_map, 0, sz * sz * sizeof *count_map);
		while (next != m_state && n_mv < 10) {
			game_state *nx2 = next->next_move ();
			int x = nx2->get_move_x ();
			int y = nx2->get_move_y ();
			int bp = b.bitpos (x, y);
			if (count_map[bp] != 0 || b.stone_at (x, y) != none)
				break;
			next = nx2;
			count_map[bp] = ++n_mv;
		}

		result += "[go]$$";
		result += startpos->to_move () == black ? "B" : "W";
		if (coords) {
			result += "c" + QString::number (sz);
		}
		if (moves > 1) {
			result += "m" + QString::number (moves);
		}
		result += "\n";
		if (m_rect_y1 == 1) {
			result += "$$";
			if (m_rect_x1 == 1)
				result += " +";
			for (int i = 0; i < m_rect_x2 - m_rect_x1 + 1; i++)
				result += "--";
			if (m_rect_x2 == sz)
				result += "-+";
			result += "\n";
		}
		for (int y = m_rect_y1; y <= m_rect_y2; y++) {
			result += "$$";
			if (m_rect_x1 == 1)
				result += " |";
			for (int x = m_rect_x1; x <= m_rect_x2; x++) {
				int bp = b.bitpos (x - 1, y - 1);
				int v = count_map[bp];
				if (v != 0) {
					result += " " + QString::number (v % 10);
				} else {
					stone_color c = b.stone_at (x - 1, y - 1);
					mark m = b.mark_at (x - 1, y - 1);
					mextra me = b.mark_extra_at (x - 1, y - 1);
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
			if (m_rect_x2 == sz)
				result += " |";
			result += "\n";
		}
		if (m_rect_y2 == sz) {
			result += "$$";
			if (m_rect_x1 == 1)
				result += " +";
			for (int i = 0; i < m_rect_x2 - m_rect_x1 + 1; i++)
				result += "--";
			if (m_rect_x2 == sz)
				result += "-+";
			result += "\n";
		}
		result += "[/go]\n";
		startpos = next;
		moves += n_mv;
	} while (startpos != m_state);
	return result;
}

static QString convert_letter_mark (mextra extra)
{
	if (extra < 27)
		return QString ('A' + extra);
	else
		return QString ('a' + extra - 26);
}

/* The central function for synchronizing visual appearance with the abstract board data.  */
void Board::sync_appearance (bool board_only)
{
	const go_board &b = m_edit_board == nullptr ? m_state->get_board () : *m_edit_board;
	stone_color to_move = m_edit_board == nullptr ? m_state->to_move () : m_edit_to_move;

	const go_board vars = m_vars_children ? m_state->child_moves (nullptr) : m_state->sibling_moves ();
	const bit_array st_w = b.get_stones_w ();
	const bit_array st_b = b.get_stones_b ();
	int sz = b.size ();

	/* Look back through previous moves to see if we should do numbering.  */
	int n_back = 0;
	int *count_map = new int[sz * sz]();
	game_state *startpos = nullptr;
	if (m_edit_board == nullptr && !m_state->get_start_count () && m_state->was_move_p ()) {
		startpos = m_state;
		while (startpos
		       && (startpos->was_move_p () || startpos->root_node_p ())
		       && !startpos->get_start_count ())
		{
			if (startpos->root_node_p ()) {
				startpos = nullptr;
				break;
			}
			int x = startpos->get_move_x ();
			int y = startpos->get_move_y ();
			int bp = b.bitpos (x, y);
			count_map[bp] = ++n_back;
			startpos = startpos->prev_move ();
		}
		if (startpos && !startpos->was_move_p () && !startpos->root_node_p ())
			startpos = nullptr;
	}
	m_used_letters.clear ();
	m_used_numbers.clear ();

	gatter->showAll ();

	for (int x = 0; x < sz; x++)
		for (int y = 0; y < sz; y++) {
			int bp = b.bitpos (x, y);
			stone_gfx *s = m_stones[bp];
			stone_color sc = b.stone_at (x, y);
			stone_type type = stone_type::live;
			mark mark_at_pos = b.mark_at (x, y);
			mextra extra = b.mark_extra_at (x, y);

			/* If we don't have a real stone, check for various possibilities of
			   ghost stones.  First, the mouse cursor, then territory marks,
			   then variation display.  */
			if (sc == none) {
				if (x == curX && y == curY && show_cursor_p ()) {
					sc = to_move;
					/* @@@ */
					if (m_game_mode == modeEdit)
						sc = black;
					type = stone_type::var;
				}
			} else if (mark_at_pos == mark::terr || mark_at_pos == mark::falseeye)
				type = stone_type::var;

			if (sc == none && m_vars_type == 1) {
				sc = vars.stone_at (x, y);
				if (sc != none)
					type = stone_type::var;
			}
			if (sc == none) {
				if (s) {
					s->hide ();
					s = nullptr;
				}
			}
			if (sc != none) {
				if (s == nullptr)
					s = new stone_gfx (canvas, imageHandler, sc, type, bp);
				else
					s->set_appearance (sc, type);
				s->show ();
			}
			if (s) {
				s->set_center(offsetX + square_size * x, offsetY + square_size * y);
				m_stones[bp] = s;
			}
			mark mark_type = mark::none;
			QColor mark_col = sc == black ? Qt::white : Qt::black;
			mark_gfx *m = m_marks[bp];
			mark_text *text_mark = m_text_marks[bp];
			QString mark_str;
			/* Used to ensure that all move numbers indicators have the same font size,
			   appropriately sized for the longest number.  */
			int len_override = 0;

			/* Now look at marks.  */

			/* See if we should number moves.  This is done relatively early because if the
			   user enables move numbering, we do want to override marks on the board.  */
			if (mark_type == mark::none && startpos) {
				int bp = b.bitpos (x, y);
				int v = count_map[bp];
				if (v != 0) {
					v = n_back - v + 1;
					mark_type = mark::text;
					mark_str = QString::number (v);
					len_override = n_back > 99 ? 3 : n_back > 9 ? 2 : 1;
				}
			} else {
				mark_type = mark_at_pos;
				/* Make sure this isn't entered if we made a move number mark,
				   which also becomes mark::text.  */
				if (mark_type == mark::text)
					mark_str = QString::fromStdString (b.mark_text_at (x, y));
			}


			/* Now, letter-style variation display.  The SGF spec says that this
			   should not overwrite any other marks, so we do it here.  */
			mark var_mark = vars.mark_at (x, y);
#if 0 /* Occasionally useful for debugging.  */
			if (var_mark != mark::none && mark_type != mark::none)
				mark_col = Qt::green;
#endif
			if (mark_type == mark::none && m_vars_type == 2 && var_mark != mark::none) {
				mark_type = var_mark;
				extra = vars.mark_extra_at (x, y);
				if (mark_type == mark::letter) {
					mark_str = convert_letter_mark (extra);
					mark_type = mark::text;
				}
				mark_col = Qt::blue;
			}

			/* We make an artificial mark for the last move.  Done late so as to not
			   override other marks.  */
			if (mark_type == mark::none && m_edit_board == nullptr && m_state->was_move_p ()) {
				int last_x = m_state->get_move_x ();
				int last_y = m_state->get_move_y ();
				if (last_x == x && last_y == y)
					mark_type = mark::move;
			}

			/* Convert the large number of conceptual marks into a smaller set of
			   visual ones.  */

			/* @@@ Could think about making these two look different.  Using red
			   as a color, for example, but that makes it less visible.  */
			if (mark_type == mark::move) {
				mark_type = mark::circle;
			} else if (mark_type == mark::falseeye) {
				mark_type = mark::triangle;
				mark_col = Qt::red;
			} else if (mark_type == mark::seki) {
				mark_type = mark::square;
				mark_col = Qt::blue;
			}
			if (mark_type == mark::num) {
				mark_str = QString::number (extra);
				m_used_numbers.set_bit (extra);
				mark_type = mark::text;
			} else if (mark_type == mark::letter) {
				mark_str = convert_letter_mark (extra);
				m_used_letters.set_bit (extra);
				mark_type = mark::text;
			}
			if (mark_type == mark::terr) {
				mark_type = mark::cross;
				mark_col = b.mark_extra_at (x, y) == 0 ? Qt::white : Qt::black;
			}

			if (mark_type != mark::text && text_mark != nullptr)
				text_mark->set_shown (false);

			if (mark_type == mark::none) {
				if (m)
					m->set_shown (false);
			} else {
				if (m && m->get_type () != mark_type) {
					delete m;
					m = nullptr;
				}

				mark_gfx *to_show = mark_type == mark::text ? text_mark : m;

				if (to_show == nullptr) {
					switch (mark_type) {
					case mark::circle:
						m = to_show = new mark_circle (canvas, square_size, mark_col);
						break;
					case mark::triangle:
						m = to_show = new mark_triangle (canvas, square_size, mark_col);
						break;
					case mark::square:
						m = to_show = new mark_square (canvas, square_size, mark_col);
						break;
					case mark::plus:
						m = to_show = new mark_cross (canvas, square_size, mark_col, true);
						break;
					case mark::cross:
						m = to_show = new mark_cross (canvas, square_size, mark_col, false);
						break;
					case mark::text:
						text_mark = new mark_text (canvas, square_size, mark_col, setting->fontMarks,
									   mark_str);
						to_show = text_mark;
						break;
					default:
						break;
					}
				} else {
					if (mark_type == mark::text)
						text_mark->set_text (mark_str);
					to_show->set_size_and_color (square_size, mark_col);
				}
				if (text_mark && len_override > 0)
					text_mark->set_max_len (len_override);

				if (to_show != nullptr) {
					gatter->hide (x + 1, y + 1);
					to_show->set_center(offsetX + square_size * x, offsetY + square_size * y);
					to_show->set_shown (true);
				}
			}
			m_marks[bp] = m;
			m_text_marks[bp] = text_mark;
		}

	updateCanvas();

	m_main_widget->recalc_scores (b, m_game_mode);
	if (!board_only)
		m_board_win->setMoveData (*m_state, b, m_game_mode);
	delete[] count_map;
}

void Board::observed_changed ()
{
	sync_appearance (false);
}

#ifndef NO_DEBUG
void Board::debug()
{
    qDebug("Board::debug()");
}
#endif

void Board::nextMove()
{
	game_state *next = m_state->next_move ();
	if (next == nullptr)
		return;
	move_state (next);
}

void Board::previousMove()
{
	game_state *next = m_state->prev_move ();
	if (next == nullptr)
		return;
	move_state (next);
}

void Board::previousComment()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->comment () != "") {
			move_state (st);
			break;
		}
	}
}

void Board::nextComment()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->comment () != "") {
			move_state (st);
			break;
		}
	}
}

void Board::previousCount()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->get_start_count ()) {
			move_state (st);
			break;
		}
	}
}

void Board::nextCount()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->get_start_count ()) {
			move_state (st);
			break;
		}
	}
}

void Board::nextVariation()
{
	game_state *next = m_state->next_sibling (true);
	if (next == nullptr)
		return;
	if (next != m_state)
		move_state (next);
}

void Board::previousVariation()
{
	game_state *next = m_state->prev_sibling (true);
	if (next == nullptr)
		return;
	if (next != m_state)
		move_state (next);
}

void Board::gotoFirstMove()
{
	game_state *st = m_state;

	for (;;) {
		game_state *next = st->prev_move ();
		if (next == nullptr) {
			if (st != m_state)
				move_state (st);
			break;
		}
		st = next;
	}
}

void Board::gotoLastMove()
{
	game_state *st = m_state;

	for (;;) {
		game_state *next = st->next_move ();
		if (next == nullptr) {
			if (st != m_state)
				move_state (st);
			break;
		}
		st = next;
	}
}

void Board::findMove(int x, int y)
{
	game_state *st = m_state;
	if (!st->was_move_p () || st->get_move_x () != x || st->get_move_y () != y)
		st = m_game->get_root ();

	for (;;) {
		st = st->next_move ();
		if (st == nullptr)
			return;
		if (st->was_move_p () && st->get_move_x () == x && st->get_move_y () == y)
			break;
	}
	move_state (st);
}

// this slot is used for edit window to navigate to last made move
void Board::gotoLastMoveByTime()
{
#if 0
	if (m_game_mode == modeScore)
		return;

	CHECK_PTR(tree);

	tree->setToFirstMove();
	Move *m = tree->getCurrent();
	CHECK_PTR(m);

	// Descent tree to last son of latest variation
	while (m->son != nullptr)
	{
		m = tree->nextMove();
		for (int i = 0; i < tree->getNumBrothers(); i++)
			m = tree->nextVariation();
/*		Move *n = m;
		while (n != NULL && (n = n->brother) != NULL)
		{
			m = n;
			if (n->getTimeLeft() <= m->getTimeLeft() ||
				n->getTimeLeft() != 0 && m->getTimeLeft() == 0)
				m = n;
		}*/
	}

	if (m != nullptr)
		updateMove(m);
#endif
}

void Board::gotoNthMove(int n)
{
	game_state *st = m_game->get_root ();
	while (st->move_number () < n && st->n_children () > 0) {
		st = st->next_move (true);
	}
	move_state (st);
}

void Board::gotoNthMoveInVar(int n)
{
	game_state *st = m_state;

	while (st->move_number () != n) {
		game_state *next = st->move_number () < n ? st->next_move () : st->prev_move ();
		/* Some added safety... this should not happen.  */
		if (next == nullptr)
			break;
		st = next;
	}
	move_state (st);
}

void Board::gotoMainBranch()
{
	game_state *st = m_state;
	game_state *go_to = st;
	while (st != nullptr) {
		while (st->has_prev_sibling ())
			st = go_to = st->prev_sibling (false);
		st = st->prev_move ();
	}
	if (go_to != st)
		move_state (go_to);
}

void Board::gotoVarStart()
{
	game_state *st = m_state->prev_move ();
	if (st == nullptr)
		return;
	/* Walk back, ending either at the root or at a node which has
	   more than one sibling.  */
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *prev = st->prev_move ();
		if (prev == nullptr)
			break;
		st = prev;
	}
	move_state (st);
}

void Board::gotoNextBranch()
{
	game_state *st = m_state;
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *next = st->next_move ();
		if (next == nullptr)
			break;
		st = next;
	}
	if (st != m_state)
		move_state (st);
}

void Board::deleteNode()
{
	game_state *st = m_state;
	if (st->root_node_p ())
		return;
	game_state *parent = st->prev_move ();
	st->transfer_observers (parent);
	if (m_state != parent)
		throw std::logic_error ("should have updated to parent");
	delete st;
	const go_board &b = m_state->get_board ();
	m_board_win->setMoveData (*m_state, b, m_game_mode);
	setModified ();
}

void Board::leaveEvent(QEvent*)
{
	curX = curY = -1;
	sync_appearance (true);
}

int Board::convertCoordsToPoint(int c, int o)
{
	int p = c - o + square_size/2;
	if (p >= 0)
		return p / square_size + 1;
	else
		return -1;
}

void Board::updateCovers ()
{
	QRectF sceneRect = canvas->sceneRect ();
	int top_edge = 0;
	if (m_rect_y1 > 1)
		top_edge = offsetY + square_size * (m_rect_y1 - 1.5);
	int bot_edge = sceneRect.bottom();
	if (m_rect_y2 < board_size)
		bot_edge = offsetY + square_size * (m_rect_y2 - 0.5);
	int left_edge = 0;
	if (m_rect_x1 > 1)
		left_edge = offsetX + square_size * (m_rect_x1 - 1.5);
	int right_edge = sceneRect.right();
	if (m_rect_x2 < board_size)
		right_edge = offsetX + square_size * (m_rect_x2 - 0.5);

	coverLeft->setVisible (m_rect_x1 > 1);
	coverRight->setVisible (m_rect_x2 < board_size);
	coverTop->setVisible (m_rect_y1 > 1);
	coverBot->setVisible (m_rect_y2 < board_size);

	coverTop->setRect (0, 0, sceneRect.right(), top_edge);
	coverBot->setRect (0, bot_edge, sceneRect.right(), sceneRect.bottom () - bot_edge);
	coverLeft->setRect (0, top_edge, left_edge, bot_edge - top_edge);
	coverRight->setRect (right_edge, top_edge, sceneRect.right() - right_edge, bot_edge - top_edge);
}

void Board::updateRectSel (int x, int y)
{
	if (x < 1)
		x = 1;
	if (y < 1)
		y = 1;
	if (x > board_size)
		x = board_size;
	if (y > board_size)
		y = board_size;
	int minx = m_down_x;
	int miny = m_down_y;
	if (x < minx)
		std::swap (minx, x);
	if (y < miny)
		std::swap (miny, y);
	m_rect_x1 = minx;
	m_rect_y1 = miny;
	m_rect_x2 = x;
	m_rect_y2 = y;
	updateCovers ();
}

void Board::mouseMoveEvent(QMouseEvent *e)
{
	int x = convertCoordsToPoint (e->x (), offsetX);
	int y = convertCoordsToPoint (e->y (), offsetY);

	if (m_mark_rect)
		updateRectSel (x, y);

	// Outside the valid board?
	if (x < 1 || x > board_size || y < 1 || y > board_size)
	{
		x = -1;
		y = -1;
	} else {
		x--;
		y--;
	}


	if (curX == x && curY == y)
		return;
#if 0 /* Would be nice to make this work, but things are fast enough as-is.  */
	int prev_x = curX;
	int prev_y = curY;
#endif
	curX = x;
	curY = y;

	// Update the statusbar coords tip
	emit coordsChanged (x, y, board_size, showSGFCoords);
#if 0
	sync_appearance (prev_x, prev_y);
	sync_appearance (x, y);
#else
	sync_appearance (true);
#endif
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
			nextVariation();
		else
			nextMove();
		m_cumulative_delta = 0;
	} else if (m_cumulative_delta > 60) {
		if (e->buttons () == Qt::RightButton || mouseState == Qt::RightButton)
			previousVariation();
		else
			previousMove();
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

	game_state *st = m_state;
	stone_color col = st->to_move ();

	game_state *st_new = st->add_child_move (x, y);
	if (st_new == nullptr)
		/* Invalid move.  */
		return;
	setModified();
	st->transfer_observers (st_new);
	m_board_win->player_move (col, x, y);
}

void Board::play_external_move (int x, int y)
{
	game_state *st = m_state;
	stone_color col = st->to_move ();

	setModified();

	game_state *st_new = st->add_child_move (x, y);
	st->transfer_observers (st_new);
}

void Board::mouseReleaseEvent(QMouseEvent* e)
{
	mouseState = Qt::NoButton;

	if (m_mark_rect) {
		m_mark_rect = false;
		m_board_win->done_rect_select (m_rect_x1, m_rect_y1, m_rect_x2, m_rect_y2);
		return;
	}
	int x = convertCoordsToPoint(e->x(), offsetX);
	int y = convertCoordsToPoint(e->y(), offsetY);

	if (m_down_x == -1 || x != m_down_x || y != m_down_y)
		return;

	//qDebug("Mouse should be released after %d,%03d", wheelTime.second(),wheelTime.msec());
	//qDebug("Mouse released at time         %d,%03d", QTime::currentTime().second(),QTime::currentTime().msec());

	if (m_game_mode != modeMatch || QTime::currentTime() <= wheelTime)
		return;

	play_one_move (x - 1, y - 1);
}

void Board::mark_dead_external (int x, int y)
{
	m_edit_board->toggle_alive (x, y, false);
	/* There's evidence to suggest that the IGS algorithm at least has
	   no fancy tricks to find false eyes and such, and we should at
	   least try to match the final result that the server will
	   caclulate.
	   See also the modeScoreRemote case in the mouse event handler.  */
	m_edit_board->calc_scoring_markers_simple ();
	observed_changed ();
}

stone_color Board::swap_edit_to_move ()
{
	if (m_edit_board != nullptr)
		return m_edit_to_move = m_edit_to_move == black ? white : black;
	stone_color newcol = m_state->to_move () == black ? white : black;
	m_state->set_to_move (newcol);
	m_board_win->setMoveData (*m_state, m_state->get_board (), m_game_mode);
	return newcol;
}

void Board::click_add_mark (QMouseEvent *e, int x, int y)
{
	if (e->button () != Qt::RightButton && e->button () != Qt::LeftButton)
		return;

	mark mark_to_set = m_edit_mark;
	mextra mark_extra = 0;

	if (e->button () == Qt::RightButton)
		mark_to_set = mark::none;

	if (mark_to_set == mark::letter && e->modifiers () == Qt::ShiftModifier) {
		TextEditDialog dlg (m_board_win);
		dlg.textLineEdit->setFocus();
		const go_board &b = m_edit_board ? *m_edit_board : m_state->get_board ();

		if (b.mark_at (x, y) == mark::text)
			dlg.textLineEdit->setText (QString::fromStdString (b.mark_text_at (x, y)));

		if (dlg.exec() == QDialog::Accepted) {
			/* This is all a bit clunky; it's the price to pay for having game_state's get_board
			   return a const go_board.  */
			if (m_edit_board)
				m_edit_board->set_text_mark (x, y, dlg.textLineEdit->text().toStdString ());
			else
				m_state->set_text_mark (x, y, dlg.textLineEdit->text().toStdString ());
			setModified ();
			sync_appearance (true);
		}

		return;
	}
	if (mark_to_set  == mark::num) {
		int i;
		for (i = 0; i < 256; i++)
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

	bool changed;
	if (m_edit_board)
		changed = m_edit_board->set_mark (x, y, mark_to_set, mark_extra);
	else
		changed = m_state->set_mark (x, y, mark_to_set, mark_extra);
	if (changed) {
		setModified ();
		sync_appearance (true);
	}
}

void Board::mousePressEvent(QMouseEvent *e)
{
	mouseState = e->button();

	int x = convertCoordsToPoint (e->x(), offsetX);
	int y = convertCoordsToPoint (e->y(), offsetY);

	if (m_request_mark_rect && e->button () == Qt::LeftButton) {
		m_mark_rect = true;
		m_request_mark_rect = false;
		if (x < 1)
			x = 1;
		if (y < 1)
			y = 1;
		if (x > board_size)
			x = board_size;
		if (y > board_size)
			y = board_size;
		m_down_x = x;
		m_down_y = y;
		updateRectSel (x, y);
		return;
	}

	m_down_x = m_down_y = -1;

	if (x < 1 || x > board_size || y < 1 || y > board_size)
		return;

	m_down_x = x;
	m_down_y = y;

	if (navIntersectionStatus) {
		navIntersectionStatus = false;
		unsetCursor ();
		findMove (x - 1, y - 1);
		return;
	}
	if (e->modifiers () == Qt::ControlModifier
	    && (m_game_mode == modeNormal || m_game_mode == modeObserve))
	{
		/* @@@ Previous code made a distinction between left and right button.  */
		findMove (x - 1, y - 1);
		return;
	}
	mark mark_to_set = m_edit_mark;
	stone_color existing_stone;

	// resume normal proceeding
	switch (m_game_mode)
	{
	case modeNormal:
		if (mark_to_set != mark::none)
		{
			click_add_mark (e, x - 1, y - 1);
			break;
		}

		/* fall through */
	case modeTeach: /* @@@ teaching mode is untested; best guess.  */
	case modeComputer:
		switch (e->button())
		{
		case Qt::LeftButton:
			play_one_move (x - 1, y - 1);
			break;

		default:
			break;
		}
		break;

	case modeEdit:
		if (mark_to_set != mark::none)
		{
			click_add_mark (e, x - 1, y - 1);
			break;
		}

		existing_stone = m_edit_board->stone_at (x - 1, y - 1);
		switch (e->button())
		{
		case Qt::LeftButton:
			if (existing_stone == black)
				m_edit_board->set_stone (x - 1, y - 1, none);
			else
				m_edit_board->set_stone (x - 1, y - 1, black);
			setModified();
			sync_appearance (true);
			break;
		case Qt::RightButton:
			if (existing_stone == white)
				m_edit_board->set_stone (x - 1, y - 1, none);
			else
				m_edit_board->set_stone (x - 1, y - 1, white);
			setModified ();
			sync_appearance (true);
			break;

		default:
			break;
		}
		break;

	case modeScoreRemote:
		if (e->button () == Qt::LeftButton) {
			m_board_win->player_toggle_dead (x - 1, y - 1);
#if 0 /* We get our own toggle back from the server, at least in tests with NNGS.  */
			m_edit_board->toggle_alive (x - 1, y - 1);
			/* See comment in mark_dead_external.  */
			m_edit_board->calc_scoring_markers_simple ();
			observed_changed ();
#endif
		}
		break;
	case modeScore:
		switch (e->button())
		{
		case Qt::LeftButton:
			m_edit_board->toggle_alive (x - 1, y - 1);
			m_edit_board->calc_scoring_markers_complex ();
			observed_changed ();
			break;
		case Qt::RightButton:
			m_edit_board->toggle_seki (x - 1, y - 1);
			m_edit_board->calc_scoring_markers_complex ();
			observed_changed ();
			break;
		default:
			break;
		}
		break;

	case modeObserve:
		// do nothing but observe
		break;

	case modeMatch:
		// Delay of 250 msecs to avoid clickos
    		wheelTime = QTime::currentTime();
    		//qDebug("Mouse pressed at time %d,%03d", wheelTime.second(),wheelTime.msec());
		if (antiClicko)
			wheelTime = wheelTime.addMSecs(250);
		break;

	default:
		qWarning("   *** Invalid game mode! ***");
    }
}

void Board::changeSize()
{
#ifdef Q_OS_WIN
    resizeDelayFlag = false;
#endif
    resizeBoard(width(), height());
}

void Board::clear_stones ()
{
	size_t sz = m_stones.size ();
	for (size_t i = 0; i < sz; i++) {
		stone_gfx *s = m_stones[i];
		delete s;
		m_stones[i] = nullptr;
		mark_gfx *m = m_marks[i];
		delete m;
		m_marks[i] = nullptr;
		mark_text *mt = m_text_marks[i];
		delete mt;
		m_text_marks[i] = nullptr;
	}
}

void Board::clear_selection ()
{
	m_request_mark_rect = false;
	m_rect_x1 = m_rect_y1 = 1;
	m_rect_x2 = m_rect_y2 = m_game->boardsize ();
	updateCovers ();
}

void Board::reset_game (std::shared_ptr<game_record> gr)
{
	stop_observing ();

	game_state *root = gr->get_root ();

	const go_board &b = root->get_board ();
	int sz = b.size ();
	board_size = sz;

	clear_stones ();
	m_stones.resize (sz * sz);
	m_marks.resize (sz * sz);
	m_text_marks.resize (sz * sz);
	for (int i = 0; i < sz * sz; i++)
		m_stones[i] = nullptr, m_marks[i] = nullptr, m_text_marks[i] = nullptr;

	m_game = gr;

	clear_selection ();

	delete gatter;
	gatter = new Gatter(canvas, board_size);
	clearCoords ();
	setupCoords ();

	calculateSize ();

	// Rescale the pixmaps in the ImageHandler
	imageHandler->rescale(square_size);

	// Redraw the board
	drawBackground();
	drawGatter();
	drawCoordinates();

	start_observing (root);

	canvas->update();
	setModified(false);
}

void Board::clearData()
{
	clear_stones ();
	canvas->update();
	isModified = false;
	clearCoords();
}

void Board::update_comment(const QString &qs, bool append)
{
	std::string s = qs.toStdString ();
	if (append)
		s = m_state->comment () + s;
	m_state->set_comment (s);
	setModified (true);
}

void Board::setShowCoords(bool b)
{
	bool old = showCoords;
	showCoords = b;
	if (old != showCoords)
		changeSize();  // Redraw the board if the value changed.
}

void Board::setShowSGFCoords(bool b)
{
	bool old = showSGFCoords;
	showSGFCoords = b;
	if (old == showSGFCoords)
		return;
	clearCoords ();
	setupCoords ();
	drawCoordinates ();
}

void Board::set_vardisplay (bool children, int type)
{
	m_vars_children = children;
	m_vars_type = type;

	sync_appearance (true);
}

void Board::setModified(bool m)
{
	if (m == isModified || m_game_mode == modeObserve)
		return;

	isModified = m;
	m_board_win->updateCaption (isModified);
}

QPixmap Board::grabPicture()
{
	int sz = m_game->boardsize ();
	int minx = offsetX - offset + 2;
	int miny = offsetY - offset + 2;
	int maxx = minx + board_pixel_size + offset*2 - 4;
	int maxy = miny + board_pixel_size + offset*2 - 4;
	if (m_rect_x1 > 1)
		minx = coverLeft->rect ().right ();
	if (m_rect_x2 < sz)
		maxx = coverRight->rect ().left ();
	if (m_rect_y1 > 1)
		miny = coverTop->rect ().bottom ();
	if (m_rect_y2 < sz)
		maxy = coverBot->rect ().top ();
	return grab (QRect (minx, miny, maxx - minx, maxy - miny));
}

// button "Pass" clicked
void Board::doPass()
{
	if (!player_to_move_p ())
		return;
	if (m_game_mode == modeNormal || m_game_mode == modeComputer) {
		game_state *st = m_state->add_child_pass ();
		m_state->transfer_observers (st);
	}
}

void Board::play_external_pass ()
{
	game_state *st = m_state->add_child_pass ();
	m_state->transfer_observers (st);
}

void Board::set_isLocalGame(bool isLocal)
{
	isLocalGame = isLocal;
}

void Board::navIntersection()
{
	setCursor(Qt::PointingHandCursor);
	navIntersectionStatus = true;
}
