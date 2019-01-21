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

class GameTree : public QGraphicsView
{
	Q_OBJECT

	int m_size = 30;
	std::shared_ptr<game_record> m_game {};
	game_state *m_active {};
	QGraphicsScene *m_scene;
	QGraphicsRectItem *m_sel {};
	QGraphicsPathItem *m_path {};
	QGraphicsLineItem *m_path_end {};
	QPixmap *m_pm_w, *m_pm_b, *m_pm_e, *m_pm_box;

public:
	GameTree(QWidget *parent);
	void update (std::shared_ptr<game_record> gr, game_state *);
	void show_menu (int x, int y, const QPoint &pos);
	void item_clicked (int x, int y);
	void toggle_collapse (int x, int y, bool);

	virtual QSize sizeHint () const override;
};

#endif
