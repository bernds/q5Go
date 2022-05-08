/*
 * gametree.h
 */

#ifndef GAMETREE_H
#define GAMETREE_H

#include <memory>
#include <unordered_map>
#include <QGraphicsView>
#include "defines.h"
#include "setting.h"
#include <textview.h>

class ImageHandler;
class game_state;
class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class MainWindow;
class FigureView;

struct game_tree_pixmaps
{
	QPixmap w, b, wfig, bfig;
	QPixmap e;
};

class GameTree : public QGraphicsView
{
	Q_OBJECT

	MainWindow *m_win {};
	/* We used to use a QHeaderView.  That didn't work terribly well for a
	   variety of reasons, the main one being that on Windows, the cells had a
	   minimum size which could be larger than the width of the game tree nodes.
	   That meant that the two would not be in sync, which obviously defeats the
	   point.
	   Also, QGraphicsView doesn't seem to handle viewportMargins terribly well,
	   leading to problems with ensureVisible and mapToScene.
	   Hence, this solution: a separate QGraphicsView, linked to our viewport
	   width and our horizontal scrollbar.  */
	QGraphicsView *m_header_view {};
	FigureView *m_previewer {};

	int m_size = 30;
	go_game_ptr m_game {};
	game_state *m_active {};
	QGraphicsScene *m_scene;
	QGraphicsScene *m_header_scene;
	QGraphicsRectItem *m_sel {};
	QGraphicsPathItem *m_path {};
	QGraphicsLineItem *m_path_end {};
	QGraphicsItem *m_pixmap_holder {};
	QFont m_header_font;
	QStandardItemModel m_headers;
	bool m_autocollapse = false;

	game_tree_pixmaps m_pm;
	game_tree_pixmaps m_pm_comment;
	QPixmap m_box_pm;

	void do_autocollapse ();
	void resize_header ();
	void clear_scene ();

protected:
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual void changeEvent (QEvent *) override;
	virtual bool event (QEvent *e) override;

public:
	GameTree (QWidget *parent);
	void set_board_win (MainWindow *win, QGraphicsView *header);
	void update (go_game_ptr gr, game_state *, bool force = false);
	void show_menu (game_state *, const QPoint &pos);
	void item_clicked (game_state *);
	void toggle_collapse (game_state *, bool);
	void toggle_figure (game_state *);

	void update_prefs ();

	virtual QSize sizeHint () const override;
};

#endif
