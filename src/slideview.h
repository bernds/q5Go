#include <memory>

#include <QDialog>

class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class game_state;
class BoardView;
class FigureView;
class QTextEdit;
class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;

namespace Ui {
	class SlideViewDialog;
	class SlideshowDialog;
};

template<class UI, class Base>
class BaseSlideView : public Base
{
protected:
	UI *ui;
	FigureView *m_board_exporter;
	QGraphicsScene *m_scene;

	QGraphicsPixmapItem *m_bg_item {}, *m_board_item {}, *m_text_item {};

	QFont m_font;
	double m_aspect = 1.8;
	QSize m_displayed_sz;

	go_game_ptr m_game;

	bool m_builtin_tutorial = false;
	bool m_bold_title = true;
	bool m_italic_title = true;
	bool m_white_text = true;
	bool m_coords = false;
	int m_margin = 2;
	int m_n_lines = 10;

	BaseSlideView (QWidget *);
	~BaseSlideView ();

	QPixmap render_text (int, int);
	void redraw ();
	void view_resized ();
	void update_text_font ();

public:
	void set_game (go_game_ptr gr);
	void set_active (game_state *st);
};

class SlideView : public BaseSlideView<Ui::SlideViewDialog, QDialog>
{
	QPixmap render_export ();
	void inputs_changed ();
	void save_with_progress (std::vector<game_state *> &collection);

	bool save ();
	void save_all (bool commented);
	void save_mainline (bool commented);
	void save_as ();
	void choose_file ();

public:
	SlideView (QWidget *parent);

	void update_prefs ();
};
