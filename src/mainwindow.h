/*
* mainwindow.h
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qlayout.h>
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
	QVariant headerData (int section, Qt::Orientation orientation,
			     int role = Qt::DisplayRole) const override;

	// Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class MainWindow : public QMainWindow, public Ui::BoardWindow
{
	Q_OBJECT

	bool m_allow_text_update_signal;
	bool m_sgf_var_style;

	QList<game_state *> m_figures;
	BoardView *m_svg_update_source;
	BoardView *m_ascii_update_source;

	bool showSlider, sliderSignalToggle;
	GameMode m_gamemode;
	GameMode m_remember_mode;
	int m_remember_tab;
	QGraphicsScene *m_eval_canvas;
	QGraphicsRectItem *m_eval_bar, *m_eval_mid;
	QGraphicsTextItem *m_w_time, *m_b_time;
	double m_eval;

	an_id_model m_an_id_model;

	go_score m_score;
	double m_result {};
	QString m_result_text;

	void toggleSliderSignal(bool b) { sliderSignalToggle = b; }

	void setToolsTabWidget(enum tabType=tabNormalScore, enum tabState=tabSet);
	void toggleSidebar (bool);
	void setSliderMax(int n);
	void updateCaption ();
	void update_font ();
	void populate_engines_menu ();
	void start_analysis ();
	void update_score_type ();

public:
	MainWindow(QWidget* parent, go_game_ptr, const QString opener_scrkey = QString (),
		   GameMode mode = modeNormal);
	virtual ~MainWindow();
	Board* getBoard() const { return gfx_board; }
	int checkModified(bool interactive=true);

	void update_settings ();

	static QString getFileExtension(const QString &fileName, bool defaultExt=true);
	void hide_panes_for_mode ();
	QString visible_panes_key ();
	void restore_visibility_from_key (const QString &);
	void saveWindowLayout (bool);
	bool restoreWindowLayout (bool, const QString &scrkey = QString ());
	void defaultPortraitLayout ();
	void defaultLandscapeLayout ();

	void set_observer_model (QStandardItemModel *m);
	bool doSave(QString fileName, bool force=false);
	void setGameMode (GameMode);

	void setMoveData(game_state &, const go_board &, GameMode);
	void mark_dead_external (int x, int y) { gfx_board->mark_dead_external (x, y); }
	void init_game_record (go_game_ptr);
	/* Called when the record was changed by some external source (say, a Go server
	   providing a title string).  */
	void update_game_record ();
	void update_figure_display ();
	/* Called by the game tree or eval graph if the user wants to change the
	   displayed position.  */
	void set_game_position (game_state *);
	void done_rect_select (int minx, int miny, int maxx, int maxy);

	/* Called from external source.  */
	void append_comment (const QString &);

	void update_analysis (analyzer);
	void update_game_tree ();
	void update_figures ();
	void update_analyzer_ids (const analyzer_id &, bool);

	void coords_changed (const QString &, const QString &);

	void update_ascii_dialog ();
	void update_svg_dialog ();

	void recalc_scores (const go_board &);

	void grey_eval_bar ();
	void set_eval (double);
	void set_eval (const QString &, double, stone_color, int);
	void set_2nd_eval (const QString &, double, stone_color, int);

	void setTimes(const QString &btime, const QString &bstones,
		      const QString &wtime, const QString &wstones,
		      bool warn_b, bool warn_w, int);

protected:
	void initActions();
	void initMenuBar(GameMode);
	void initToolBar();
	void initStatusBar();

	virtual void closeEvent (QCloseEvent *e) override;
	virtual void keyPressEvent (QKeyEvent*) override;
	virtual void keyReleaseEvent (QKeyEvent*) override;

signals:
	void signal_sendcomment(const QString&);
	void signal_closeevent();

	void signal_undo();
	void signal_adjourn();
	void signal_resign();
	void signal_pass();
	void signal_done();
	void signal_editBoardInNewWindow();

public slots:
	void slotFocus (bool) { setFocus (); }
	void slotFileNewBoard(bool);
	void slotFileNewGame(bool);
	void slotFileNewVariantGame(bool);
	void slotFileOpen(bool);
	void slotFileOpenDB(bool);
	bool slotFileSave(bool = false);
	void slotFileClose(bool);
	bool slotFileSaveAs(bool);
	void slotFileExportASCII(bool);
	void slotFileExportSVG(bool);
	void slotFileImportSgfClipB(bool);
	void slotFileExportSgfClipB(bool);
	void slotFileExportPic(bool);
	void slotFileExportPicClipB(bool);

	void slotEditDelete(bool);
	void slotEditFigure(bool);

	void slotNavIntersection(bool);
	void slotNavAutoplay(bool toggle);
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

	void slotTimerForward();
	void slot_editBoardInNewWindow(bool);
	void slotSoundToggle(bool toggle);
	void slotUpdateComment();
	void slotUpdateComment2();
	void slotEditGroup(bool);
	void slotEditRectSelect(bool);
	void slotEditClearSelect(bool);

	void slotPlayFromHere (bool);

	void slotDiagEdit (bool);
	void slotDiagASCII (bool);
	void slotDiagSVG (bool);
	void slotDiagChosen (int);

	void slotEngineGroup(bool);

	virtual void doPass ();
	virtual void doCountDone ();
	virtual void doUndo ();
	virtual void doAdjourn ();
	virtual void doResign ();
	void on_colorButton_clicked (bool);
	void doRealScore (bool);
	void doEdit ();
	void doEditPos (bool);
	void sliderChanged (int);

protected:
	go_game_ptr m_game;
	game_state *m_empty_state {};
	TextView m_ascii_dlg;
	SvgView m_svg_dlg;
	bool local_stone_sound;

	SlideView *slideView {};

	QLabel *statusCoords, *statusMode, *statusTurn, *statusNav;

	QAction *escapeFocus, *whatsThis;
	QAction *navAutoplay, *navSwapVariations;
	QActionGroup *editGroup, *engineGroup;
	QButtonGroup *scoreGroup;
	QList<QAction *> engine_actions;
	QMap<QAction *, Engine> engine_map;
	QTimer *timer;

	float timerIntervals[6];
	bool isFullScreen;

public:
	/* Called when the user performed a board action.  Just plays an
	   (optional) sound, but is used in derived classes to send a GTP
	   command or a message to the server.  */
	virtual void player_move (stone_color, int, int);
	virtual void player_toggle_dead (int, int) { }
};

class MainWindow_GTP : public MainWindow, public GTP_Controller
{
	GTP_Process *m_gtp;
	game_state *m_first_pos;
public:
	MainWindow_GTP (QWidget *parent, go_game_ptr, QString opener_scrkey,
			const Engine &program, bool b_comp, bool w_comp);
	MainWindow_GTP (QWidget *parent, go_game_ptr, game_state *, QString opener_scrkey,
			const Engine &program, bool b_comp, bool w_comp);
	~MainWindow_GTP ();

	/* Virtuals from MainWindow.  */
	virtual void player_move (stone_color, int x, int y) override;
	virtual void doPass() override;
	// virtual void doUndo();
	virtual void doResign() override;

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_played_move (int x, int y) override;
	virtual void gtp_played_resign () override;
	virtual void gtp_played_pass () override;
	virtual void gtp_startup_success () override;
	virtual void gtp_setup_success () override;
	virtual void gtp_exited () override;
	virtual void gtp_failure (const QString &) override;
};

extern std::list<MainWindow *> main_window_list;
#endif
