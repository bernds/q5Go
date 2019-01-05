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
#include "gatter.h"
#include "goboard.h"

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class GameData;
class QNewGameDlg;
class MainWindow;
class MainWidget;

class Board : public QGraphicsView, public game_state::observer
{
	Q_OBJECT
	MainWindow *m_board_win;
	MainWidget *m_main_widget;

	GameMode m_game_mode = modeNormal;

	std::shared_ptr<game_record> m_game;

	/* A local copy of the board for when editing and scoring.  Null otherwise.  */
	go_board *m_edit_board = nullptr;
	stone_color m_edit_to_move;
	bool m_edit_changed;
	mark m_edit_mark = mark::none;

	bit_array m_used_numbers = bit_array (256, false);
	bit_array m_used_letters = bit_array (52, false);

	/* Anti-clicko, remember position on mouse button down.  */
	int m_down_x, m_down_y;
	int m_cumulative_delta = 0;

	int m_rect_x1, m_rect_x2;
	int m_rect_y1, m_rect_y2;
	bool m_mark_rect = false;
	bool m_request_mark_rect = false;

	std::vector<stone_gfx *> m_stones;
	std::vector<mark_gfx *> m_marks;
	std::vector<mark_text *> m_text_marks;
	bool m_player_is_b = true;
	bool m_player_is_w = true;

	int m_vars_type = 1;
	bool m_vars_children = false;

	void observed_changed ();
	void sync_appearance (bool board_only = true);
	void play_one_move (int x, int y);

public:
	Board(QWidget *parent=0, QGraphicsScene *c = 0);
	~Board();
	/* Should be part of the constructor, but Qt doesn't seem to allow such a construction with the .ui files.  */
	void init2 (MainWindow *w, MainWidget *mw) { m_board_win = w; m_main_widget = mw; }
	void clearData();
	void setModified(bool m=true);
	ImageHandler* getImageHandler() { return imageHandler; }
	void updateCanvas() { canvas->update(); }
	void clear_stones ();
	void reset_game (std::shared_ptr<game_record>);
	const std::shared_ptr<game_record> get_record () { return m_game; }
	const game_state *get_state () { return m_state; }
	void external_move (game_state *st) { move_state (st); }
	void mark_dead_external (int x, int y);
	bool show_cursor_p ();
	void setMode(GameMode mode);
	GameMode getGameMode () { return m_game_mode; }
	void setMarkType(mark t) { m_edit_mark = t; }

	void nextMove();
	void previousMove();
	void nextCount();
	void previousCount();
	void nextVariation();
	void previousVariation();
	void nextComment();
	void previousComment();
	void gotoFirstMove();
	void gotoLastMove();
	void gotoLastMoveByTime();
	void gotoMainBranch();
	void gotoVarStart();
	void gotoNextBranch();
	void gotoNthMove(int n);
	void gotoNthMoveInVar(int n);
	void findMove(int x, int y);
	void navIntersection();
	void set_start_count (bool on) { m_state->set_start_count (on); }
	void set_rect_select (int on) { m_request_mark_rect = on; }
	void clear_selection ();

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
	short getCurrentX() const { return curX; }
	short getCurrentY() const { return curY; }
	QPixmap grabPicture();
	QString render_ascii (bool, bool);
	bool doCountDone();
#ifndef NO_DEBUG
	void debug();
#endif

	bool fastLoad, isModified, lockResize;

	// in case of match
	void set_player_colors (bool w, bool b) { m_player_is_w = w; m_player_is_b = b; }
	bool player_is (stone_color c) { return c == black ? m_player_is_b : m_player_is_w; }

	void set_isLocalGame(bool isLocal);
	bool get_isLocalGame() { return isLocalGame; }

	void setShowCoords(bool b);
	void setShowSGFCoords(bool b);
	void set_antiClicko(bool b) { antiClicko = b; }
	void set_vardisplay (bool children, int type);

public slots:
	void update_comment(const QString &, bool append);
	void changeSize();

signals:
	void coordsChanged(int, int, int,bool);

protected:
	void calculateSize();
	void drawBackground();
	void drawGatter();
	void drawCoordinates();
	void drawStarPoint(int x, int y);
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
	void setupCoords();
	void clearCoords();
	void updateCovers ();

	/* Local copies of the pixmaps held by the settings.  I'm not entirely
	   clear on ownership issues of QPainter; this is to avoid any potential
	   use-after free if the picture changes and settings deletes the old
	   pixmaps.  */
	QPixmap m_wood, m_table;
	QGraphicsRectItem *coverTop, *coverLeft, *coverRight, *coverBot;

	QGraphicsScene *canvas;
	QList<QGraphicsSimpleTextItem*> hCoords1, hCoords2 ,vCoords1, vCoords2;
	Gatter *gatter;
	ImageHandler *imageHandler;
	static const int margin, coord_margin;
	int board_size, offset, offsetX, offsetY, board_pixel_size, table_size;
	int coord_offset;
	double square_size;
	bool showCoords;
	bool showSGFCoords;
	bool antiClicko;
	short curX, curY;

	QTime wheelTime;
	Qt::MouseButtons mouseState;

	bool isHidingStones; // QQQ

#ifdef Q_OS_WIN
	bool resizeDelayFlag;
#endif
	bool isLocalGame;
	bool navIntersectionStatus;
};

#endif
