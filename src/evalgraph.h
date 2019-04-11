/*
 * evalgraph.h
 */

#ifndef EVALGRAPH_H
#define EVALGRAPH_H

#include <QGraphicsView>
#include <memory>
#include "defines.h"
#include "setting.h"
#include "goeval.h"

class game_state;
class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class MainWindow;
class an_id_model;

class EvalGraph : public QGraphicsView
{
	Q_OBJECT

	MainWindow *m_win {};
	const an_id_model *m_model {};

	go_game_ptr m_game {};
	game_state *m_active {};
	int m_id_idx = 0;
	QGraphicsScene *m_scene;
	QBrush *m_brush;
	double m_step;

protected:
	virtual void mouseMoveEvent (QMouseEvent *e) override;
	virtual void mousePressEvent (QMouseEvent *e) override;
	virtual void resizeEvent (QResizeEvent*) override;
	virtual void contextMenuEvent (QContextMenuEvent *e) override;
	virtual void changeEvent (QEvent *) override;

public slots:
	void export_clipboard (bool);
	void export_file (bool);
public:
	EvalGraph (QWidget *parent);
	~EvalGraph () { delete m_brush; delete m_scene; }
	void set_board_win (MainWindow *win, const an_id_model *m) { m_win = win; m_model = m; }
	void update (go_game_ptr gr, game_state *, int id_idx);

	void update_prefs ();

	virtual QSize sizeHint () const override;
};

#endif
