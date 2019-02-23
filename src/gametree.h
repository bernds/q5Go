/*
 * gametree.h
 */

#ifndef GAMETREE_H
#define GAMETREE_H

#include <QGraphicsView>
#include "defines.h"
#include "setting.h"
#include <textview.h>

class ImageHandler;
class game_state;
class game_record;
class MainWindow;
class FigureView;

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
	std::shared_ptr<game_record> m_game {};
	game_state *m_active {};
	QGraphicsScene *m_scene;
	QGraphicsScene *m_header_scene;
	QGraphicsRectItem *m_sel {};
	QGraphicsPathItem *m_path {};
	QGraphicsLineItem *m_path_end {};
	QPixmap m_pm_w, m_pm_b, m_pm_wfig, m_pm_bfig;
	QPixmap m_pm_e, m_pm_box;
	QFont m_header_font;
	QStandardItemModel m_headers;
	bool m_autocollapse = false;

	void do_autocollapse ();
	void resize_header ();

protected:
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual bool event (QEvent *e) override;

public:
	GameTree(QWidget *parent);
	void set_board_win (MainWindow *win, QGraphicsView *header);
	void update (std::shared_ptr<game_record> gr, game_state *, bool force = false);
	void show_menu (int x, int y, const QPoint &pos);
	void item_clicked (int x, int y);
	void toggle_collapse (int x, int y, bool);
	void toggle_figure (int x, int y);

	void update_prefs ();

	virtual QSize sizeHint () const override;
};

#endif
