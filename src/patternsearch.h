#include <QMainWindow>
#include <QThreadPool>

#include "gamedb.h"
#include "gogame.h"
#include "pattern.h"
#include "ui_helpers.h"

/* Data associated with pattern search.  */
struct pattern_cont_entry
{
	long count = 0;
	int rank = 0;
};

struct pattern_cont_data
{
	pattern_cont_entry data[3];
	pattern_cont_entry &get (stone_color col)
	{
		int idx = col == none ? 0 : col == white ? 1 : 2;
		return data[idx];
	}
};

enum class pattern_cont_view {
	numbers, letters, percent
};

namespace Ui
{
	class PatternSearchWindow;
};

class PatternSearchWindow;
class QActionGroup;
class QGraphicsScene;
class QGraphicsSceneContextMenuEvent;
class QGraphicsRectItem;
class FigureView;

class search_runnable : public QObject
{
	Q_OBJECT

	gamedb_model *m_model;
	gamedb_model::search_result **m_result;
	std::atomic<long> *m_cur, *m_max;
	go_pattern m_pattern;

public:
	search_runnable (PatternSearchWindow *parent, gamedb_model *m, gamedb_model::search_result **r,
			 go_pattern &&p, std::atomic<long> *cur, std::atomic<long> *max);

public slots:
	void start_search ();
signals:
	void signal_completed ();
};

class PatternSearchWindow : public QMainWindow
{
	Q_OBJECT

	Ui::PatternSearchWindow *ui;
	QActionGroup *m_edit_group {}, *m_view_group {};

	gamedb_model m_model;
	go_game_ptr m_game, m_orig_game;
	go_pattern *last_pattern {};
	gamedb_model::search_result *m_result {};
	QThread search_thread;
	search_runnable *m_runnable;

	std::atomic<long> m_progress;
	std::atomic<long> m_progress_max;
	int m_progress_timer;

	QGraphicsScene *m_preview_scene {};
	QGraphicsScene *m_info_scene {};
	QGraphicsRectItem *m_cursor {};
	int m_preview_w = 0;
	int m_preview_h = 0;
	struct preview : public board_preview {
		go_pattern pat;
		board_rect selection;
		std::vector<pattern_cont_data> cont;
		bit_array games_result;

		preview (go_pattern p, go_game_ptr g, game_state *st, board_rect sel, bit_array r)
			: board_preview (g, st), pat (std::move (p)), selection (sel),
			  cont (g->get_root ()->get_board ().bitsize ()), games_result (r)
		{
		}
		preview (const preview &) = default;
		preview (preview &&) = default;
		preview &operator= (const preview &) = default;
		preview &operator= (preview &&) = default;
	};
	std::vector<preview> m_previews;
	const preview *m_cursor_preview {};
	FigureView *m_previewer {};

	void timerEvent (QTimerEvent *) override;

	void apply_game_result (const bit_array &);
	void update_preview (preview &);
	void set_preview_cursor (const preview &);
	void clear_preview_cursor ();
	void preview_clicked (const preview &);
	void preview_menu (QGraphicsSceneContextMenuEvent *e, const preview &);

	void pattern_search (bool);
	void handle_doubleclick ();
	void update_selection ();
	void update_caption ();

	void choose_color (bool);
	void update_actions ();

	go_game_ptr load_selected ();

public:
	PatternSearchWindow ();
	~PatternSearchWindow ();

	void do_search (go_game_ptr, game_state *, const board_rect &);
public slots:
	void do_open (bool);
	void choices_resized ();
	void slot_completed ();
	void slot_choose_color (bool);
	void slot_choose_view (bool);

	void slot_using ();

	void nav_next_move ();
	void nav_previous_move ();
	void nav_goto_first_move ();
	void nav_goto_last_move ();
	void nav_goto_cont ();

	void editDelete ();
signals:
	void signal_start_search ();
};

extern PatternSearchWindow *patsearch_window;

extern game_state *find_first_match (go_game_ptr, const go_pattern &, board_rect &);
