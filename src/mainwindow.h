/*
* mainwindow.h
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <utility>

#include <QGridLayout>
#include <QLabel>
#include <QAction>
#include <QMainWindow>
#include <QAbstractItemModel>

#include "qgo.h"
#include "preferences.h"
#include "board.h"
#include "setting.h"
#include "textview.h"
#include "figuredlg.h"
#include "qgtp.h"
#include "timing.h"

#include "ui_boardwindow_gui.h"

class Board;
class QSplitter;
class QToolBar;
struct Engine;
class GameTree;
class SlideView;

/* This keeps track of analyzer_ids, which are combinations of engine name and
   komi.  The evaluation graph shows one line per id.  */
class an_id_model : public QAbstractItemModel {
	typedef std::pair<analyzer_id, bool> entry;
	std::vector<entry> m_entries;
public:
	an_id_model ()
	{
	}
	void populate_list (go_game_ptr);

	const std::vector<entry> &entries () const { return m_entries; }
	void notice_analyzer_id (const analyzer_id &, bool);

	virtual QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QModelIndex index (int row, int col, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent (const QModelIndex &index ) const override;
	int rowCount (const QModelIndex &parent = QModelIndex()) const override;
	int columnCount (const QModelIndex &parent = QModelIndex()) const override;
	bool removeRows (int, int, const QModelIndex &parent = QModelIndex()) override;
	QVariant headerData (int section, Qt::Orientation orientation,
			     int role = Qt::DisplayRole) const override;

	// Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class undo_entry
{
protected:
	QString m_op;
public:
	undo_entry (const QString &op) : m_op (op)
	{
	}
	virtual ~undo_entry ()
	{
	}
	QString op_str ()
	{
		return m_op;
	}
	virtual game_state *apply_undo () = 0;
	virtual game_state *apply_redo () = 0;
};

class MainWindow : public QMainWindow, public Ui::BoardWindow
{
	Q_OBJECT

	bool m_allow_text_update_signal = false;
	bool m_sgf_var_style;

	QList<game_state *> m_figures;
	BoardView *m_svg_update_source;
	BoardView *m_ascii_update_source;

	bool showSlider, sliderSignalToggle;
	GameMode m_gamemode;
	GameMode m_remember_mode = modeNormal;
	int m_remember_tab;
	QGraphicsScene *m_eval_canvas;
	QGraphicsRectItem *m_eval_bar, *m_eval_mid;
	QGraphicsTextItem *m_w_time, *m_b_time;
	double m_eval;

	an_id_model m_an_id_model;

	go_score m_score;
	double m_result {};
	QString m_result_text;

	QColor m_default_text_color;

	/* When switching to edit/score mode, we make a temporary duplicate of the position.  */
	game_state *m_pos_before_edit;
protected:
	move_timer m_timer_white, m_timer_black;
	std::string m_tstr_white, m_tstr_black;

	go_game_ptr m_game;
	game_state *m_empty_state {};
	TextView m_ascii_dlg;
	SvgView m_svg_dlg;
	bool local_stone_sound;

	SlideView *slideView {};

	QLabel *statusCoords, *statusMode, *statusTurn, *statusNav;

	QAction *escapeFocus, *whatsThis;
	QAction *navSwapVariations;
	QActionGroup *editGroup, *engineGroup;
	QButtonGroup *scoreGroup;
	QList<QAction *> engine_actions;
	QMap<QAction *, Engine> engine_map;
	QTimer *timer;

	bool isFullScreen;

	void set_comment (const QString &);

private:
	std::vector<std::unique_ptr<undo_entry>> m_undo_stack;
	size_t m_undo_stack_pos = 0;

	template<class T> T *top_undo_entry ()
	{
		if (m_undo_stack.empty ())
			return nullptr;
		m_undo_stack.erase (std::begin (m_undo_stack) + m_undo_stack_pos, std::end (m_undo_stack));
		return dynamic_cast<T *>(m_undo_stack.back ().get ());
	}

	void update_undo_menus ();
	void perform_undo ();
	void perform_redo ();

	void toggleSliderSignal(bool b) { sliderSignalToggle = b; }

	void setToolsTabWidget(enum tabType=tabNormalScore, enum tabState=tabSet);
	void toggleSidebar (bool);
	void setSliderMax(int n);
	void updateCaption ();
	void update_font ();
	void populate_engines_menu ();
	void start_analysis ();
	void update_score_type ();

	void leave_edit_append ();
	void leave_edit_prepend ();
	void leave_edit_modify ();

	void push_undo (std::unique_ptr<undo_entry>);

	void hide_panes_for_mode ();
	QString visible_panes_key ();
	void restore_visibility_from_key (const QString &);
	void saveWindowLayout (bool);
	bool restoreWindowLayout (bool, const QString &scrkey = QString ());
	void update_view_menu ();
	void defaultPortraitLayout ();
	void defaultLandscapeLayout ();

	void grey_eval_bar ();
	void set_eval_bar (double);
	void clear_primary_eval ();
	void clear_2nd_eval ();
	QString format_eval (double, bool, double, stone_color);

	void update_ascii_dialog ();
	void update_svg_dialog ();

	static QString getFileExtension(const QString &fileName, bool defaultExt=true);

public:
	MainWindow (QWidget* parent, go_game_ptr, const QString opener_scrkey = QString (),
		    GameMode mode = modeNormal, time_settings ts = time_settings ());
	virtual ~MainWindow ();
	void init_game_record (go_game_ptr);
	Board* getBoard () const { return gfx_board; }
	int checkModified (bool interactive=true);

	virtual void update_settings ();
	void set_observer_model (QStandardItemModel *m);
	bool doSave (QString fileName, bool force=false);
	void update_pass_button ();
	void setGameMode (GameMode);

	void setMoveData (const game_state *);
	void mark_dead_external (int x, int y) { gfx_board->mark_dead_external (x, y); }
	/* Called when the record was changed by some external source (say, a Go server
	   providing a title string).  */
	void update_game_record ();
	void update_figure_display ();
	/* Called by the game tree or eval graph if the user wants to change the
	   displayed position.  */
	void set_game_position (game_state *);
	void done_rect_select ();

	/* Callback from the Remove Analysis dialog.  */
	void remove_nodes (const std::vector<game_state *> &);

	/* Called from external source.  */
	void append_comment (const QString &, QColor = QColor ());
	void refresh_comment ();
	void notice_mark_change (const go_board &new_board);
	typedef std::pair<game_state *, bool> move_result;
	move_result make_move (game_state *, int x, int y);
	void add_engine_pv (game_state *, game_state *);
	void update_analysis (analyzer);
	void update_game_tree ();
	void update_figures ();
	void update_analyzer_ids (const analyzer_id &, bool);

	void coords_changed (const QString &, const QString &);

	void recalc_scores (const go_board &);

	void set_eval (const QString &, double, bool, double, stone_color, int);
	void set_2nd_eval (const QString &, double, bool, double, stone_color, int);

	void setTimes (int btime, const QString &bstones, int wtime, const QString &wstones,
		       bool warn_b, bool warn_w, int);

	/* Navigation.  */
	void nav_next_move ();
	void nav_previous_move ();
	void nav_next_figure ();
	void nav_previous_figure ();
	void nav_next_variation ();
	void nav_previous_variation ();
	void nav_next_comment ();
	void nav_previous_comment ();
	void nav_goto_first_move ();
	void nav_goto_last_move ();
	void nav_goto_main_branch ();
	void nav_goto_var_start ();
	void nav_goto_next_branch ();
	void nav_goto_nth_move (int n);
	void nav_goto_nth_move_in_var (int n);
	void nav_find_move (int x, int y);

	/* Called when the user performed a board action.  player_move is
	   responsible for actually adding the move to the game.  Derived
	   classes override it to also send a GTP command or a message to
	   the server.  */
	virtual game_state *player_move (int, int);
	virtual void player_toggle_dead (int, int) { }

protected:
	void initActions ();
	void initMenuBar (GameMode);
	void initToolBar ();
	void initStatusBar ();

	GameMode game_mode () { return m_gamemode; };
	virtual bool comments_from_game_p ();

	/* Sidebar button actions.  */
	void doPass ();
	void doCountDone ();

	virtual void closeEvent (QCloseEvent *e) override;
	virtual void keyPressEvent (QKeyEvent*) override;
	virtual void keyReleaseEvent (QKeyEvent*) override;

	virtual void time_loss (stone_color);
	virtual void remove_connector ()
	{
		/* Called when the "Disconnect game" button is clicked.  Overridden in
		   MainWindow_IGS.  */
	}

signals:
	void signal_sendcomment(const QString&);
	void signal_closeevent();

public slots:
	void slotFocus (bool) { setFocus (); }
	void slotFileNewBoard(bool);
	void slotFileNewGame(bool);
	void slotFileNewVariantGame(bool);
	void slotFileOpen(bool);
	void slotFileOpenDB(bool);
	bool slotFileSave(bool = false);
	virtual void slotFileClose(bool);
	bool slotFileSaveAs(bool);
	void slotFileSaveVariations(bool);
	void slotFileExportASCII(bool);
	void slotFileExportSVG(bool);
	void slotFileImportSgfClipB(bool);
	void slotFileExportSgfClipB(bool);
	void slotFileExportPic(bool);
	void slotFileExportPicClipB(bool);

	void slotEditCopyPos(bool);
	void slotEditPastePos(bool);
	void slotEditDelete(bool);
	void slotEditFigure(bool);

	void slotNavIntersection(bool);
	void slotNavNthMove(bool);
	void slotNavSwapVariations(bool);

	void slotSetGameInfo(bool);
	void slotViewMenuBar(bool toggle);
	void slotViewStatusBar(bool toggle);
	void slotViewCoords(bool toggle);
	void slotViewSlider(bool toggle);
	void slotViewLeftSidebar();
	void slotViewSidebar(bool toggle);
	void slotViewFullscreen(bool toggle);
	void slotViewMoveNumbers(bool toggle);
	void slotViewDiagComments (bool);
	void slotViewConnections (bool);

	void slotTimerForward();
	void slotSoundToggle(bool toggle);
	void slotUpdateComment();
	void slotUpdateComment2();
	void slotEditGroup(bool);
	void slotEditRectSelect(bool);
	void slotEditClearSelect(bool);

	void slotPlayFromHere (bool);
	void slotEditAnalysis (bool);

	void slotDiagEdit (bool);
	void slotDiagASCII (bool);
	void slotDiagSVG (bool);
	void slotDiagChosen (int);

	void slotEngineGroup (bool);

	void slotToggleHideAnalysis (bool);

	void on_colorButton_clicked (bool);
	void doRealScore (bool);
	void doEdit ();
	void doEditPos (bool);
	void sliderChanged (int);

	void perform_search ();
};

class MainWindow_GTP : public MainWindow, public GTP_Eval_Controller
{
	GTP_Process *m_gtp_w {};
	GTP_Process *m_gtp_b {};
	std::vector<game_state *> m_start_positions;
	game_state *m_game_position;
	QString m_score_report;

	int m_starting_up = 0;
	int m_setting_up = 0;

	/* Track scores.  The two m_winner_ fields hold the winner reported by each
	   of the two engines, so we can track disagreements.  */
	stone_color m_winner_1, m_winner_2;
	int m_wins_w = 0;
	int m_wins_b = 0;
	int m_jigo = 0;
	int m_disagreements = 0;
	int m_n_games = 0;

	GTP_Process *single_engine ()
	{
		return m_gtp_w == nullptr ? m_gtp_b : m_gtp_w;
	}
	void enter_scoring ();
	void start_game (const Engine &, bool b_is_comp, bool w_is_comp, const go_board &);
	void setup_game ();
	void game_end (const QString &, stone_color);
	bool two_engines ()
	{
		return m_gtp_w != nullptr && m_gtp_b != nullptr;
	}
	void request_next_move ();

	virtual void time_loss (stone_color) override;
	bool stop_move_timer ();

	void player_undo ();
	void player_pass ();
	void player_resign ();
	void play_again ();

	void start_game (const Engine &program, const time_settings &, bool b_comp, bool w_comp);

public:
	MainWindow_GTP (QWidget *parent, go_game_ptr, QString opener_scrkey,
			const Engine &program, const time_settings &, bool b_comp, bool w_comp);
	MainWindow_GTP (QWidget *parent, go_game_ptr, game_state *, QString opener_scrkey,
			const Engine &program, const time_settings &, bool b_comp, bool w_comp);
	MainWindow_GTP (QWidget *parent, go_game_ptr, QString opener_scrkey,
			const Engine &program_w, const Engine &program_b, const time_settings &, int num_games, bool book);
	~MainWindow_GTP ();

	/* Virtuals from MainWindow.  */
	virtual game_state *player_move (int x, int y) override;

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_played_move (GTP_Process *p, int x, int y) override;
	virtual void gtp_played_resign (GTP_Process *p) override;
	virtual void gtp_played_pass (GTP_Process *p) override;
	virtual void gtp_startup_success (GTP_Process *p) override;
	virtual void gtp_setup_success (GTP_Process *p) override;
	virtual void gtp_exited (GTP_Process *p) override;
	virtual void gtp_failure (GTP_Process *p, const QString &) override;
	virtual void gtp_report_score (GTP_Process *p, const QString &) override;
	virtual void eval_received (const analyzer_id &id, const QString &, int, bool) override;
};

extern std::list<MainWindow *> main_window_list;
#endif
