/*
* mainwindow.h
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qlayout.h>
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QAction>
#include <QTreeWidget>
#include <QMainWindow>

#include "preferences.h"
#include "board.h"
#include "mainwidget.h"
#include "setting.h"
#include "miscdialogs.h"
#include "qgtp.h"

class Board;
class InterfaceHandler;
class QSplitter;
class StatusTip;
class qGoIF;
class QNewGameDlg;
class QToolBar;

struct ASCII_Import;

extern QString screen_key ();

class MainWindow : public QMainWindow
{
	Q_OBJECT

	bool m_allow_text_update_signal;

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
	void addObserver(const QString &name);
	void clearObserver() { ListView_observers->clear(); }
	void updateObserverCnt();
	int blackPlayerType, whitePlayerType ;
	bool doSave(QString fileName, bool force=false);
	void setGameMode (GameMode);

	void setMoveData(const game_state &, const go_board &, GameMode);
	void mark_dead_external (int x, int y) { gfx_board->mark_dead_external (x, y); }
	void update_game_record (std::shared_ptr<game_record>) { /* @@@ */ }
	void updateCaption (bool modified);

	void done_rect_select (int minx, int miny, int maxx, int maxy);

	/* Called from external source.  */
	void append_comment (const QString &);

protected:
	void initActions();
	void initMenuBar();
	void initToolBar();
	void initStatusBar();
	void keyPressEvent(QKeyEvent *e);
	void closeEvent(QCloseEvent *e);
//	bool doSave(QString fileName, bool force=false);
	void rememberLastDir(const QString &file);
//  bool eventFilter( QObject *obj, QEvent *ev ); //SL added eb 11
	const QString getStatusMarkText(MarkType t);

signals:
	void signal_sendcomment(const QString&);
	void signal_closeevent();

	void signal_undo();
	void signal_adjourn();
	void signal_resign();
	void signal_pass();
	void signal_done();
	void signal_refresh();
	void signal_editBoardInNewWindow();

public slots:
	void slotFocus (bool) { setFocus (); }
	void slotFileNewBoard(bool);
	void slotFileNewGame(bool);
	void slotFileOpen(bool);
	bool slotFileSave(bool = false);
	void slotFileClose(bool);
	bool slotFileSaveAs(bool);
	void slotFileExportASCII(bool);
	void slotFileImportSgfClipB(bool);
	void slotFileExportSgfClipB(bool);
	void slotFileExportPic(bool);
	void slotFileExportPicClipB(bool);

	void slotEditDelete(bool);
	void slotEditMarkBrothers(bool);
	void slotEditMarkSons(bool);
	void slotEdit123(bool);
	void slotNavBackward(bool = false);
	void slotNavForward(bool = false);
	void slotNavFirst(bool = false);
	void slotNavLast(bool = false);
	void slotNavLastByTime(bool = false);
	void slotNavNextVar(bool = false);
	void slotNavPrevVar(bool = false);
	void slotNavNextComment(bool);
	void slotNavPrevComment(bool);
	void slotNavIntersection(bool);
	void slotNavMainBranch(bool);
	void slotNavStartVar(bool = false);
	void slotNavNextBranch(bool = false);
	void slotNavNthMove(bool);
	void slotNavAutoplay(bool toggle);
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
	void slotHelpManual(bool);
	void slotHelpAbout(bool);
	void slotHelpAboutQt(bool);
	void slotTimerForward();
	void slot_editBoardInNewWindow(bool);
	void slot_animateClick(bool);
	void slotSoundToggle(bool toggle);
	void slotUpdateComment();
	void slotUpdateComment2();
	void slotEditGroup(bool);
	void slotEditRectSelect(bool);
	void slotEditClearSelect(bool);

	virtual void doPass();
	virtual void doCountDone();
	virtual void doUndo();
	virtual void doAdjourn();
	virtual void doResign();

protected:
	qGoIF *parent_;
	std::shared_ptr<game_record> m_game;
	Board *gfx_board;
	MainWidget *mainWidget;
	TextView m_ascii_dlg;
	bool local_stone_sound;

	//	HelpViewer *helpViewer;
	StatusTip *statusTip;
	QLabel *statusMode, *statusTurn, *statusMark, *statusNav;

	QSplitter *splitter, *splitter_comment;
	QWidget *comments_widget;
	QLayout *comments_layout;
	QTextEdit *commentEdit;
	QLineEdit *commentEdit2;
	QTreeWidget *ListView_observers;

	QToolBar *fileBar, *toolBar, *editBar;

	QMenu *fileMenu, *importExportMenu, *editMenu, *navMenu, *settingsMenu, *viewMenu, *helpMenu;

	QAction *escapeFocus;
	QAction *fileNewBoard, *fileNew, *fileOpen, *fileSave, *fileSaveAs, *fileClose,
		*fileExportASCII, *fileImportSgfClipB, *fileExportSgfClipB,
		*fileExportPic, *fileExportPicClipB,
		*fileQuit;
	QAction *editDelete, *editStone, *editTriangle, *editSquare, *editCircle, *editCross, *editNumber, *editLetter;
	QAction *editRectSelect, *editClearSelect, *edit123;
	QAction *editMarkBrothers, *editMarkSons;
	QAction *navBackward, *navForward, *navFirst, *navLast, *navNextVar, *navPrevVar,
		*navMainBranch, *navStartVar, *navNextBranch, *navNthMove, *navAutoplay, *navEmptyBranch,
		*navCloneNode, *navSwapVariations, *navNextComment, *navPrevComment, *navIntersection ;       //SL added eb 11                               // added eb the 2 last
	QAction *setPreferences, *setGameInfo, *soundToggle;
	QAction *viewFileBar, *viewToolBar, *viewEditBar, *viewMenuBar, *viewStatusBar, *viewCoords,
		*viewSlider, *viewSidebar, *viewComment, *viewVertComment, *viewSaveSize, *viewFullscreen;
	QAction *helpManual, *helpAboutApp, *helpAboutQt, *whatsThis;
	QActionGroup *editGroup;
	QTimer *timer;

	float timerIntervals[6];
	bool isFullScreen;

	QGridLayout *mainWidgetGuiLayout;

public:
	/* Called when the user performed a board action.  Used in derived
	   classes, to send a GTP command or a message to the server.  */
	virtual void player_move (stone_color, int, int) { }
	virtual void player_toggle_dead (int, int) { }
};

class MainWindow_GTP : public MainWindow, public Gtp_Controller
{
	QGtp *m_gtp;
public:
	MainWindow_GTP (QWidget *parent, std::shared_ptr<game_record>, const QString &program,
			bool b_comp, bool w_comp);
	~MainWindow_GTP ();

	/* Virtuals from MainWindow.  */
	virtual void player_move (stone_color, int x, int y) override;
	virtual void doPass();
	// virtual void doUndo();
	virtual void doResign();

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_played_move (int x, int y);
	virtual void gtp_played_resign ();
	virtual void gtp_played_pass ();
	virtual void gtp_startup_success ();
	virtual void gtp_exited ();
	virtual void gtp_failure (const gtp_exception &);
};

extern std::list<MainWindow *> main_window_list;
#endif
