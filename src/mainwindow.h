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

#include "qgo.h"
#include "preferences.h"
#include "board.h"
#include "mainwidget.h"
#include "setting.h"
#include "textview.h"
#include "figuredlg.h"
#include "qgtp.h"

class Board;
class QSplitter;
class QToolBar;
class Engine;
class GameTree;

extern QString screen_key ();

class MainWindow : public QMainWindow
{
	Q_OBJECT

	bool m_allow_text_update_signal;
	bool m_sgf_var_style;

	QList<game_state *> m_figures;
	BoardView *m_svg_update_source;
	BoardView *m_ascii_update_source;
public:
	MainWindow(QWidget* parent, std::shared_ptr<game_record>, GameMode mode = modeNormal);
	virtual ~MainWindow();
	Board* getBoard() const { return gfx_board; }
	bool doOpen(const char *fileName);
	int checkModified(bool interactive=true);
	void updateFont();
	static QString getFileExtension(const QString &fileName, bool defaultExt=true);
#if 0
	void doScore(bool toggle) { mainWidget->doRealScore(toggle); }
#endif
	void doRealScore(bool toggle) { mainWidget->doRealScore(toggle); }
	MainWidget *getMainWidget() { return mainWidget; }
	void saveWindowSize ();
	bool restoreWindowSize ();
	void updateBoard();

	void set_observer_model (QStandardItemModel *m);
	bool doSave(QString fileName, bool force=false);
	void setGameMode (GameMode);

	void setMoveData(game_state &, const go_board &, GameMode);
	void mark_dead_external (int x, int y) { gfx_board->mark_dead_external (x, y); }
	void init_game_record (std::shared_ptr<game_record>);
	/* Called when the record was changed by some external source (say, a Go server
	   providing a title string).  */
	void update_game_record ();
	void updateCaption (bool modified);
	void update_figure_display ();
	void done_rect_select (int minx, int miny, int maxx, int maxy);

	/* Called from external source.  */
	void append_comment (const QString &);

	void update_analysis (analyzer);
	void update_game_tree ();
	void update_figures ();

	void coords_changed (const QString &, const QString &);

	void update_ascii_dialog ();
	void update_svg_dialog ();

protected:
	void initActions();
	void initMenuBar(GameMode);
	void initToolBar();
	void initStatusBar();
	void keyPressEvent(QKeyEvent *e);
	void closeEvent(QCloseEvent *e);
//	bool doSave(QString fileName, bool force=false);
	void rememberLastDir(const QString &file);

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
	void slotViewFileBar(bool toggle);
	void slotViewToolBar(bool toggle);
	void slotViewEditBar(bool toggle);
	void slotViewMenuBar(bool toggle);
	void slotViewStatusBar(bool toggle);
	void slotViewCoords(bool toggle);
	void slotViewSlider(bool toggle);
	void slotViewLeftSidebar();
	void slotViewSidebar(bool toggle);
	void slotViewComment(bool toggle);
	void slotViewVertComment(bool toggle);
	void slotViewFullscreen(bool toggle);
	void slotViewMoveNumbers(bool toggle);
	void slotViewFigures(bool toggle);
	void slotTimerForward();
	void slot_editBoardInNewWindow(bool);
	void slot_animateClick(bool);
	void slotSoundToggle(bool toggle);
	void slotUpdateComment();
	void slotUpdateComment2();
	void slotEditGroup(bool);
	void slotEditRectSelect(bool);
	void slotEditClearSelect(bool);

	void slotDiagEdit (bool);
	void slotDiagASCII (bool);
	void slotDiagSVG (bool);
	void slotDiagChosen (int);

	virtual void doPass();
	virtual void doCountDone();
	virtual void doUndo();
	virtual void doAdjourn();
	virtual void doResign();

protected:
	std::shared_ptr<game_record> m_game;
	game_state *m_empty_state {};
	Board *gfx_board;
	MainWidget *mainWidget;
	TextView m_ascii_dlg;
	SvgView m_svg_dlg;
	bool local_stone_sound;

	QLabel *statusCoords, *statusMode, *statusTurn, *statusNav;

	QSplitter *splitter, *splitter_comment;
	QWidget *comments_widget;
	QLayout *comments_layout;
	QTextEdit *commentEdit;
	QLineEdit *commentEdit2;
	QTreeView *ListView_observers;
	GameTree *gameTreeView;

	QToolBar *fileBar, *toolBar, *editBar;

	QMenu *fileMenu, *importExportMenu, *editMenu, *navMenu, *settingsMenu, *viewMenu, *anMenu, *helpMenu;

	QAction *escapeFocus;
	QAction *fileNewBoard, *fileNew, *fileNewVariant, *fileOpen, *fileSave, *fileSaveAs, *fileClose,
		*fileExportASCII, *fileExportSVG, *fileImportSgfClipB, *fileExportSgfClipB,
		*fileExportPic, *fileExportPicClipB,
		*fileQuit;
	QAction *editDelete, *editStone, *editTriangle, *editSquare, *editCircle, *editCross, *editNumber, *editLetter;
	QAction *editRectSelect, *editClearSelect, *editFigure;
	QAction *navBackward, *navForward, *navFirst, *navLast, *navNextVar, *navPrevVar,
		*navMainBranch, *navStartVar, *navNextBranch, *navNthMove, *navAutoplay, *navEmptyBranch,
		*navCloneNode, *navSwapVariations, *navNextComment, *navPrevComment, *navIntersection;
	QAction *navPrevFigure, *navNextFigure;
	QAction *setPreferences, *setGameInfo, *soundToggle;
	QAction *anConnect, *anDisconnect, *anPause;
	QAction *viewFileBar, *viewToolBar, *viewEditBar, *viewMenuBar, *viewStatusBar, *viewCoords,
		*viewSlider, *viewSidebar, *viewComment, *viewVertComment, *viewSaveSize, *viewFullscreen,
		*viewNumbers, *viewFigures;
	QAction *helpManual, *helpAboutApp, *helpAboutQt, *whatsThis;
	QActionGroup *editGroup;
	QTimer *timer;

	float timerIntervals[6];
	bool isFullScreen;

	QGridLayout *mainWidgetGuiLayout;

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
public:
	MainWindow_GTP (QWidget *parent, std::shared_ptr<game_record>, const Engine &program,
			bool b_comp, bool w_comp);
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
	virtual void gtp_exited () override;
	virtual void gtp_failure (const QString &) override;
};

extern std::list<MainWindow *> main_window_list;
#endif
