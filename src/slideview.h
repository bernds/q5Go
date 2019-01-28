#include <memory>

#include <QDialog>
#include "ui_slideview_gui.h"

class game_record;
class game_state;
class BoardView;
class FigureView;
class QTextEdit;
class QGraphicsView;
class QGraphicsScene;

class SlideView: public QDialog, public Ui::SlideViewDialog
{
	Q_OBJECT

	FigureView *m_board_exporter;
	std::shared_ptr<game_record> m_game;
	QFont m_font;
	double m_aspect;
	QSize m_displayed_sz;

	QGraphicsPixmapItem *m_bg_item {}, *m_board_item {}, *m_text_item {};

	QGraphicsScene *m_scene;
	QPixmap render_export ();
	QPixmap render_text (int, int);
	void redraw ();
	void view_resized ();
	void update_text_font ();
	void inputs_changed ();
	void save_with_progress (std::vector<game_state *> &collection);

public slots:
	bool save ();
	void save_all (bool commented);
	void save_mainline (bool commented);
	void save_as ();
	void choose_file ();

public:
	SlideView (QWidget *parent);
	~SlideView ();
	void set_game (std::shared_ptr<game_record> gr);
	void set_active (game_state *st);

	void update_prefs ();
};

