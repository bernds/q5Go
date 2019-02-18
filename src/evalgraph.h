/*
 * evalgraph.h
 */

#ifndef EVALGRAPH_H
#define EVALGRAPH_H

#include <QGraphicsView>
#include <memory>
#include "defines.h"
#include "setting.h"

class game_state;
class game_record;
class MainWindow;

class EvalGraph : public QGraphicsView
{
	Q_OBJECT

	MainWindow *m_win {};

	std::shared_ptr<game_record> m_game {};
	game_state *m_active {};
	QGraphicsScene *m_scene;
	QBrush *m_brush;
	double m_step;

protected:
	virtual void mousePressEvent (QMouseEvent *e) override;
	virtual void resizeEvent (QResizeEvent*) override;
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
public slots:
	void export_clipboard (bool);
	void export_file (bool);
public:
	EvalGraph (QWidget *parent);
	~EvalGraph () { delete m_brush; delete m_scene; }
	void set_board_win (MainWindow *win) { m_win = win; }
	void update (std::shared_ptr<game_record> gr, game_state *);

	void update_prefs ();

	virtual QSize sizeHint () const override;
};

#endif
