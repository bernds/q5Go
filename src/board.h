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
#include "stone.h"
#include "goboard.h"
#include "gogame.h"
#include "qgtp.h"

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class QNewGameDlg;
class MainWindow;
class MainWidget;
class Grid;
class CoordDisplay;

enum class analyzer { disconnected, starting, running, paused };

class Board : public QGraphicsView, public navigable_observer, public Gtp_Controller
{
	Q_OBJECT
	MainWindow *m_board_win;
	MainWidget *m_main_widget;

	GameMode m_game_mode = modeNormal;

	/* Size of the (abstract) board.  */
	int board_size_x, board_size_y;

	/* Controls whether moves are allowed for either color.  */
	bool m_player_is_b = true;
	bool m_player_is_w = true;

	/* A local copy of the board for when editing and scoring.  Null otherwise.  */
	go_board *m_edit_board = nullptr;
	stone_color m_edit_to_move;
	bool m_edit_changed;
	mark m_edit_mark = mark::none;

	/* Collected when syncing the abstract position to the screen, and used
	   when placing marks.  */
	bit_array m_used_numbers = bit_array (256, false);
	bit_array m_used_letters = bit_array (52, false);

	/* Anti-clicko, remember position on mouse button down.  */
	int m_down_x, m_down_y;
	int m_cumulative_delta = 0;

	/* Variables related to rectangle selection.  */
	int m_rect_x1, m_rect_x2;
	int m_rect_y1, m_rect_y2;
	bool m_mark_rect = false;
	bool m_request_mark_rect = false;

	/* Positioning of the board image inside the view.  The board rect gives the
	   exact size of the grid; stones and marks extend outside of it.  We also
	   need space for coordinates, so there is an additional wood rect to hold
	   the entire area drawn with a wooden background texture.  */
	QRect m_wood_rect;
	QRect m_board_rect;
	static const int margin, coord_margin;
	int coord_offset;
	double square_size;

	/* Graphical elements on the board canvas.  */
	std::vector<stone_gfx *> m_stones;
	QGraphicsScene *canvas;
	Grid *m_grid {};
	CoordDisplay *m_coords {};

	int m_vars_type = 1;
	bool m_vars_children = false;

	bool m_pause_eval = false;
	double m_primary_eval;
	game_state *m_eval_state {};
	double *m_winrate {};
	int *m_visits {};
	QGraphicsPixmapItem *m_mark_layer {};

	GTP_Process *m_analyzer {};

	void observed_changed () override;
	bool show_cursor_p ();
	void sync_appearance (bool board_only = true);
	void play_one_move (int x, int y);
	void setup_analyzer_position ();
	void clear_eval_data ()
	{
		delete m_eval_state;
		m_eval_state = nullptr;
		delete m_winrate;
		m_winrate = nullptr;
		delete m_visits;
		m_visits = nullptr;
	}
public:
	Board(QWidget *parent=0, QGraphicsScene *c = 0);
	~Board();
	/* Should be part of the constructor, but Qt doesn't seem to allow such a construction with the .ui files.  */
	void init2 (MainWindow *w, MainWidget *mw) { m_board_win = w; m_main_widget = mw; }
	void setModified(bool m=true);
	ImageHandler* getImageHandler() { return imageHandler; }
	void updateCanvas() { canvas->update(); }
	void clear_stones ();
	void reset_game (std::shared_ptr<game_record>);
	const std::shared_ptr<game_record> get_record () { return m_game; }
	const game_state *get_state () { return m_state; }
	void external_move (game_state *st) { move_state (st); }
	void mark_dead_external (int x, int y);
	void setMode(GameMode mode);
	GameMode getGameMode () { return m_game_mode; }
	void setMarkType(mark t) { m_edit_mark = t; }

	void navIntersection();
	void set_start_count (bool on) { m_state->set_start_count (on); }
	void set_rect_select (int on) { m_request_mark_rect = on; }
	void clear_selection ();

	analyzer analyzer_state ();
	void start_analysis ();
	void stop_analysis ();
	void pause_analysis (bool);

	bool player_to_move_p ()
	{
		return !m_state->was_score_p () && (m_state->to_move () == black ? m_player_is_b : m_player_is_w);
	}
	void play_external_move (int x, int y);
	void play_external_pass ();

	go_board *dup_current_board () { return new go_board (m_state->get_board ()); }
	stone_color to_move () { return m_state->to_move (); }
	stone_color swap_edit_to_move ();
	void deleteNode();
	void doPass();
	QPixmap grabPicture();
	QString render_ascii (bool, bool);
	QByteArray render_svg (bool, bool);
	bool doCountDone();
#ifndef NO_DEBUG
	void debug();
#endif

	bool isModified, lockResize;

	// in case of match
	void set_player_colors (bool w, bool b) { m_player_is_w = w; m_player_is_b = b; }
	bool player_is (stone_color c) { return c == black ? m_player_is_b : m_player_is_w; }

	void setShowCoords(bool b);
	void setShowSGFCoords(bool b);
	void set_antiClicko(bool b) { antiClicko = b; }
	void set_vardisplay (bool children, int type);

	void update_images () {	imageHandler->rescale (square_size); sync_appearance (); }

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_played_move (int, int) override { /* Should not happen.  */ }
	virtual void gtp_played_resign () override { /* Should not happen.  */ }
	virtual void gtp_played_pass () override { /* Should not happen.  */ }
	virtual void gtp_startup_success () override;
	virtual void gtp_exited () override;
	virtual void gtp_failure (const QString &) override;
	virtual void gtp_eval (const QString &) override;

public slots:
	void update_comment(const QString &);
	void changeSize();

protected:
	void calculateSize ();
	void draw_background ();
	void draw_grid_and_coords ();

	void resizeBoard(int w, int h);
	int convertCoordsToPoint(int c, int o);
	void updateRectSel(int, int);
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent*) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void leaveEvent(QEvent*) override;
	virtual void resizeEvent(QResizeEvent*) override;

private:
	void click_add_mark (QMouseEvent *, int, int);
	void updateCovers ();

	/* Local copies of the pixmaps held by the settings.  I'm not entirely
	   clear on ownership issues of QPainter; this is to avoid any potential
	   use-after free if the picture changes and settings deletes the old
	   pixmaps.  */
	QPixmap m_wood, m_table;
	QGraphicsRectItem *coverTop, *coverLeft, *coverRight, *coverBot;

	ImageHandler *imageHandler;
	bool showCoords;
	bool showSGFCoords;
	bool antiClicko;
	short curX, curY;

	QTime wheelTime;
	Qt::MouseButtons mouseState;

#ifdef Q_OS_WIN
	bool resizeDelayFlag;
#endif
	bool navIntersectionStatus;
};

#endif
