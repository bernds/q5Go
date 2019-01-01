/*
* mainwindow.h
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qlayout.h>
#include <q3listview.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <Q3PopupMenu>
#include <QKeyEvent>
#include <QAction>

#include "preferences.h"
#include "board.h"
#include "mainwidget.h"
#include "setting.h"
#include "miscdialogs.h"

class Board;
class InterfaceHandler;
class QSplitter;
class StatusTip;
class qGoIF;
class QNewGameDlg;

extern QString screen_key ();

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = 0, const char* name = 0, Qt::WFlags f = Qt::WType_TopLevel);
	~MainWindow();
	InterfaceHandler* getInterfaceHandler() const { return interfaceHandler; }
	Board* getBoard() const { return gfx_board; }
	void doOpen(const QString &fileName, const QString &filter=0, bool storedir=true);
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
	Q3ListView *getListView_observers() { return ListView_observers; }
	void updateObserverCnt();
	QAction *get_fileQuit() { return fileQuit; }
	QAction *get_fileClose() { return fileClose; }
	int blackPlayerType, whitePlayerType ;
	bool doSave(QString fileName, bool force=false);
	void setGameMode (GameMode);

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
	void signal_closeevent();

public slots:
	void slotFileNewBoard();
	void slotFileNewGame();
	void slotFileOpen();
	bool slotFileSave();
	void slotFileClose();
	bool slotFileSaveAs();
	void slotFileExportASCII();
	void slotFileImportSgfClipB();
	void slotFileExportSgfClipB();
	void slotFileExportPic();
	void slotFileExportPicClipB();
	void slotEditDelete();
	void slotEditNumberMoves();
	void slotEditMarkBrothers();
	void slotEditMarkSons();
	void slotNavBackward();
	void slotNavForward();
	void slotNavFirst();
	void slotNavLast();
	void slotNavLastByTime();
	void slotNavNextVar();
	void slotNavPrevVar();
	void slotNavNextComment();
	void slotNavPrevComment();
	void slotNavIntersection();
	void slotNavMainBranch();
	void slotNavStartVar();
	void slotNavNextBranch();
	void slotNavNthMove();
	void slotNavAutoplay(bool toggle);
	void slotNavSwapVariations();
	void slotSetGameInfo();
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
	void slotViewSaveSize();
	void slotViewFullscreen(bool toggle);
	void slotHelpManual();
	void slotHelpSoundInfo();
	void slotHelpAbout();
	void slotHelpAboutQt();
	void slotToggleMarks();
	void slotTimerForward();
	void slot_editBoardInNewWindow();
	void slot_animateClick();
	void slotSoundToggle(bool toggle);

private:
	qGoIF *parent_;
	Board *gfx_board;
	InterfaceHandler *interfaceHandler;
	MainWidget *mainWidget;

	//	HelpViewer *helpViewer;
	StatusTip *statusTip;
	QLabel *statusMode, *statusTurn, *statusMark, *statusNav;

	QSplitter *splitter, *splitter_comment;
	QWidget *comments_widget;
	QLayout *comments_layout;
	QTextEdit *commentEdit;
	QLineEdit *commentEdit2;
	Q3ListView *ListView_observers;

	QToolBar *fileBar, *toolBar, *editBar;

	QMenu *fileMenu, *importExportMenu, *editMenu, *navMenu, *settingsMenu, *viewMenu, *helpMenu;

	QAction *escapeFocus, *toggleEdit, *toggleMarks;
	QAction *fileNewBoard, *fileNew, *fileOpen, *fileSave, *fileSaveAs, *fileClose,
		*fileExportASCII, *fileImportSgfClipB, *fileExportSgfClipB,
		*fileExportPic, *fileExportPicClipB,
		*fileQuit;
	QAction *editDelete, *editNumberMoves, *editMarkBrothers, *editMarkSons;
	QAction *navBackward, *navForward, *navFirst, *navLast, *navNextVar, *navPrevVar,
		*navMainBranch, *navStartVar, *navNextBranch, *navNthMove, *navAutoplay, *navEmptyBranch,
		*navCloneNode, *navSwapVariations, *navNextComment, *navPrevComment, *navIntersection;
	QAction *setPreferences, *setGameInfo, *soundToggle;
	QAction *viewFileBar, *viewToolBar, *viewEditBar, *viewMenuBar, *viewStatusBar, *viewCoords,
		*viewSlider, *viewSidebar, *viewComment, *viewVertComment, *viewSaveSize, *viewFullscreen;
	QAction *helpManual, *helpSoundInfo, *helpAboutApp, *helpAboutQt, *whatsThis;
	QTimer *timer;

	float timerIntervals[6];
	bool isFullScreen;

	QGridLayout *mainWidgetGuiLayout;
};

#endif

