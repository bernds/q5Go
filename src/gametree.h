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
class BoardView;

class GameTree : public QGraphicsView
{
	Q_OBJECT

	MainWindow *m_win {};
	BoardView *m_previewer {};

	int m_size = 30;
	std::shared_ptr<game_record> m_game {};
	game_state *m_active {};
	QGraphicsScene *m_scene;
	QGraphicsRectItem *m_sel {};
	QGraphicsPathItem *m_path {};
	QGraphicsLineItem *m_path_end {};
	QPixmap m_pm_w, m_pm_b, m_pm_wfig, m_pm_bfig;
	QPixmap m_pm_e, m_pm_box;
	QStandardItemModel m_headers;
	QHeaderView m_header_view;
	bool m_autocollapse = false;

	void do_autocollapse ();

protected:
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
	virtual void resizeEvent(QResizeEvent*) override;
	virtual bool event (QEvent *e) override;

public:
	GameTree(QWidget *parent);
	void set_board_win (MainWindow *win) { m_win = win; }
	void update (std::shared_ptr<game_record> gr, game_state *, bool force = false);
	void show_menu (int x, int y, const QPoint &pos);
	void item_clicked (int x, int y);
	void toggle_collapse (int x, int y, bool);
	void toggle_figure (int x, int y);

	void update_prefs ();

	virtual QSize sizeHint () const override;
};

#endif
