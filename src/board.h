/*
 * board.h
 */

#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <map>

#include <QGraphicsView>
#include <QDateTime>
#include <QPainter>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>

#include "defines.h"
#include "setting.h"
#include "goboard.h"
#include "gogame.h"
#include "qgtp.h"

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class QNewGameDlg;
class MainWindow;
class svg_builder;
class QFontInfo;
struct Engine;

/* We split the Board view into two classes: a BoardView, dealing only with display, and
   Board, which also interacts with the board window.
   The split is not 100% clean, as BoardView contains a few members that are modified only
   by the derived Board class (such as stuff related the rectangle selection), but it's
   good enough for the moment.  */

class BoardView : public QGraphicsView
{
	Q_OBJECT

	/* The navigable_observer defines m_game for navigation, so Board actually
	   ends up with two game pointers.  That's a little unfortunate.
	   But we want a reference to the game record of the displayed position to
	   ensure that it isn't deleted before a BoardView is.  */
	go_game_ptr m_displayed_game;

	void update_rect_select (int, int);
	/* Used by mouse event handlers during rectangle selection.  */
	int m_rect_down_x = -1, m_rect_down_y = -1;

protected:
	MainWindow *m_board_win {};

	/* Size of the (abstract) board.  */
	board_rect m_dims;
	bit_array m_hoshis;

	game_state *m_displayed {};

	/* A local copy of the board for when editing and scoring.  Null otherwise.  */
	go_board *m_edit_board = nullptr;
	stone_color m_edit_to_move;

	/* Collected when syncing the abstract position to the screen, and used
	   when placing marks.  */
	bit_array m_used_numbers = bit_array (256, false);
	bit_array m_used_letters = bit_array (52, false);

	/* Toroid support.  */
	int m_shift_x = 0, m_shift_y = 0;

	/* Variables related to rectangle selection and crop.  */
	board_rect m_sel;
	/* It can be somewhat scary to think about how crop would interact with shifts
	   and duplications.  The solution is: it does not.  Cropping is only allowable
	   on fairly plain boards without shift/duplicate, such as the figure view, or
	   tooltips.  */
	board_rect m_crop;

	/* A few board display options.  */
	bool m_figure_view = false;
	bool m_move_numbers = false;
	bool m_show_coords, m_sgf_coords, m_show_hoshis, m_show_figure_caps;

	/* Positioning of the board image inside the view.  The board rect gives the
	   exact size of the grid; stones and marks extend outside of it.  We also
	   need space for coordinates, so there is an additional wood rect to hold
	   the entire area drawn with a wooden background texture.  */
	QRect m_wood_rect;
	QRect m_board_rect;

	/* Distance from table edge to wooden board edge.  */
	int m_margin = 2;
	/* Margins around coordinates, on all four sides.  */
	static const int coord_margin;

	double square_size;

	/* Graphical elements on the board canvas.  */
	QGraphicsScene *canvas;

	int m_vars_type = 1;
	bool m_vars_children = false;

	QGraphicsPixmapItem m_stone_layer;

	virtual void sync_appearance (bool board_only = true);
	const QPixmap &choose_stone_pixmap (stone_color, stone_type, int);

	int n_dups_h ();
	int n_dups_v ();
	void alloc_graphics_elts (bool);
	void clear_graphics_elts ();
	void clear_stones ();

	bool have_selection ();
	/* Overridden by Board.  */
	virtual stone_color cursor_color (int, int, stone_color) { return none; }
	virtual int extract_analysis (go_board &) { return 0; }
	enum class ram_result { none, hide, nohide };
	virtual ram_result render_analysis_marks (svg_builder &, double, double, double, const QFontInfo &,
						  int, int, bool, int, int)
	{
		return ram_result::none;
	}
	virtual bool have_analysis () { return false; }
	std::pair<stone_color, stone_type> stone_to_display (const go_board &, const bit_array *visible,
							     stone_color to_move, int x, int y,
							     const go_board &vars, int var_type);

public:
	BoardView (QWidget *parent = nullptr);
	~BoardView ();
	/* Should be part of the constructor, but Qt doesn't seem to allow such a construction with the .ui files.  */
	void set_board_win (MainWindow *w) { m_board_win = w; }
	virtual void reset_game (go_game_ptr);
	virtual void set_displayed (game_state *);
	game_state *displayed () { return m_displayed; }

	stone_color to_move () { return m_displayed->to_move (); }

	void set_margin (int);
	QPixmap grabPicture ();
	QImage background_image ();
	QRect wood_rect () { return m_wood_rect; }
	QPixmap draw_position (int);
	QString render_ascii (bool, bool, bool);
	QByteArray render_svg (bool, bool);

	void clear_selection ();
	void clear_crop ();
	void set_crop (const board_rect &);

	bool lockResize;

	void set_show_move_numbers (bool);
	void set_show_hoshis (bool);
	void set_show_figure_caps (bool);
	void set_show_coords (bool);
	void set_sgf_coords (bool);
	void set_vardisplay (bool children, int type);

	void update_prefs ();

	void resizeBoard(int w, int h);

public slots:
	void changeSize();

protected:
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void resizeEvent(QResizeEvent*) override;

	void calculateSize ();
	void draw_background ();
	void draw_grid (QPainter &, bit_array &, int);

	int coord_vis_to_board_x (int, bool = false);
	int coord_vis_to_board_y (int, bool = false);

	void updateCovers ();

	/* Local copies of the pixmaps held by the settings.  I'm not entirely
	   clear on ownership issues of QPainter; this is to avoid any potential
	   use-after free if the picture changes and settings deletes the old
	   pixmaps.  */
	QPixmap m_wood, m_table;
	QGraphicsRectItem *coverTop, *coverLeft, *coverRight, *coverBot;

	ImageHandler *imageHandler;

#ifdef Q_OS_WIN
	bool resizeDelayFlag;
#endif
};

class Board : public BoardView, public navigable_observer, public GTP_Eval_Controller
{
	Q_OBJECT

	GameMode m_game_mode = modeNormal;

	/* Controls whether moves are allowed for either color.  */
	bool m_player_is_b = true;
	bool m_player_is_w = true;

	/* A local copy of the board for when editing and scoring.  Null otherwise.  */
	bool m_edit_changed;
	mark m_edit_mark = mark::none;

	/* Anti-clicko, remember position on mouse button down.  */
	int m_down_x, m_down_y;
	int m_cumulative_delta = 0;

	/* Toroid support.  */
	bool m_dragging = false;
	int m_drag_begin_x, m_drag_begin_y;

	/* Variables related to rectangle selection.  */
	bool m_mark_rect = false;
	bool m_request_mark_rect = false;

	/* Given to us by the main window, indicates the analyzer selected in the eval graph
	   list view.  */
	analyzer_id m_an_id;

	bool m_hide_analysis = false;

	bool show_cursor_p ();
	void update_shift (int x, int y);

	void play_one_move (int x, int y);
	void setup_analyzer_position ();

public:
	Board (QWidget *parent = nullptr);
	~Board ();

	virtual void reset_game (go_game_ptr) override;
	virtual void set_displayed (game_state *) override;

	void set_analyzer_id (analyzer_id id);
	void setModified (bool m=true);
	void external_move (game_state *st) { move_state (st); }
	void mark_dead_external (int x, int y);
	void update_comment(const QString &);

	void setMode(GameMode mode);
	GameMode getGameMode () { return m_game_mode; }
	void setMarkType(mark t) { m_edit_mark = t; }

	void navIntersection();
	void set_rect_select (int on) { m_request_mark_rect = on; }

	void start_analysis (const Engine &);
	void stop_analysis ();
	void pause_analysis (bool);
	void set_hide_analysis (bool);

	bool player_to_move_p ()
	{
		return !m_displayed->was_score_p () && (m_displayed->to_move () == black ? m_player_is_b : m_player_is_w);
	}
	void play_external_move (int x, int y);
	void play_external_pass ();

	stone_color swap_edit_to_move ();
	void deleteNode();
	void doPass();
	bool doCountDone();

	void set_player_colors (bool w, bool b) { m_player_is_w = w; m_player_is_b = b; }
	bool player_is (stone_color c) { return c == black ? m_player_is_b : m_player_is_w; }

	virtual void observed_changed () override;

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_startup_success (GTP_Process *) override;
	virtual void gtp_exited (GTP_Process *) override;
	virtual void gtp_failure (GTP_Process *, const QString &) override;

protected:
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void leaveEvent(QEvent*) override;

	virtual bool have_analysis () override;
	game_state *analysis_at (int x, int y, int &, double &);
	virtual stone_color cursor_color (int x, int y, stone_color to_move) override;
	virtual ram_result render_analysis_marks (svg_builder &, double svg_factor, double cx, double cy, const QFontInfo &,
						  int x, int y, bool child_mark,
						  int v, int max_number) override;
	virtual void sync_appearance (bool board_only = true) override;

	virtual int extract_analysis (go_board &) override;
	virtual void eval_received (const QString &, int, bool) override;

private:
	void click_add_mark (QMouseEvent *, int, int);

	QTime wheelTime;
	Qt::MouseButtons mouseState = Qt::NoButton;

	bool navIntersectionStatus = false;
	short curX, curY;
};

class FigureView : public BoardView
{
	Q_OBJECT

public:
	FigureView (QWidget *parent = nullptr);

protected:
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
public slots:
	void hide_unselected (bool);
	void make_all_visible (bool);
};

#endif
