/*
* mainwindow.h
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <q3mainwindow.h>
#include <qlayout.h>
#include <q3listview.h>
//Added by qt3to4:
#include <QCloseEvent>
#include <Q3GridLayout>
#include <QLabel>
#include <Q3PopupMenu>
#include <QKeyEvent>
#include <Q3Action>

#include "preferences.h"
#include "board.h"
#include "mainwidget.h"
#include "setting.h"
#include "miscdialogs.h"

class Board;
class InterfaceHandler;
class QSplitter;
class Q3MultiLineEdit;
class StatusTip;
class qGoIF;
class QNewGameDlg; //SL added eb 12

struct ASCII_Import;

class MainWindow : public Q3MainWindow
{
	Q_OBJECT
		
public:
	MainWindow(QWidget* parent = 0, const char* name = 0, Qt::WFlags f = Qt::WType_TopLevel);
	~MainWindow();
	InterfaceHandler* getInterfaceHandler() const { return interfaceHandler; }
	Board* getBoard() const { return board; }
//	void setApplicationStyle();
	bool preferencesAccept();
	void doOpen(const QString &fileName, const QString &filter=0, bool storedir=true);
	bool startComputerPlay(QNewGameDlg * dlg,const QString &fileName, const QString &filter, const QString &computer_path);
	int checkModified(bool interactive=true);
	void updateFont();
	static QString getFileExtension(const QString &fileName, bool defaultExt=true);
	void doScore(bool toggle) { mainWidget->doScore(toggle); }
	void doRealScore(bool toggle) { mainWidget->doRealScore(toggle); }
	MainWidget *getMainWidget() { return mainWidget; }
	bool reStoreWindowSize(QString, bool);
	bool preferencesSave(PreferencesDialog*);
	void updateBoard();
	void addObserver(const QString &name);
	void clearObserver() { ListView_observers->clear(); }
	Q3ListView *getListView_observers() { return ListView_observers; }
	void updateObserverCnt();
	void setParent(qGoIF *w) { parent_ = w; }
	qGoIF *getParent() { return parent_; }
	void dlgSetPreferences(int tab=-1);
	Q3Action *get_fileQuit() { return fileQuit; }
	Q3Action *get_fileClose() { return fileClose; }
	int blackPlayerType, whitePlayerType ;
	bool doSave(QString fileName, bool force=false);

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
	
signals:
	void signal_closeevent();
	
public slots:
	void slotFileNewBoard();
	void slotFileNewGame();
	void slotFileOpen();
	bool slotFileSave();
	void slotFileClose();
	bool slotFileSaveAs();
	void slotFileImportASCII();
	void slotFileImportASCIIClipB();
	void slotFileExportASCII();
	void slotFileImportSgfClipB();
	void slotFileExportSgfClipB();
	void slotFileExportPic();
	void slotFileExportPicClipB();
	void slotEditCut();
	void slotEditPaste();
	void slotEditPasteBrother();
	void slotEditDelete();
	void slotEditHideStones(); // QQQ
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
	void slotNavNextComment();      //added
	void slotNavPrevComment();      //end add
	void slotNavIntersection(); //SL added eb 11
	void slotNavMainBranch();
	void slotNavStartVar();
	void slotNavNextBranch();
	void slotNavNthMove();
	void slotNavAutoplay(bool toggle);
	void slotNavEmptyBranch();
	void slotNavCloneNode();
	void slotNavSwapVariations();
	void slotSetPreferences();
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
	void slotViewPinComment(bool toggle);
	void slotViewIncreaseSize();
	void slotViewDecreaseSize();
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
	Board *board;
	InterfaceHandler *interfaceHandler;
	MainWidget *mainWidget;
  	QString style;
	
	//	HelpViewer *helpViewer;
	StatusTip *statusTip;
	QLabel *statusMode, *statusTurn, *statusMark, *statusNav;
	
	QSplitter *splitter, *splitter_comment;
//	QMultiLineEdit *commentEdit;
	Q3TextEdit *commentEdit;
	QLineEdit *commentEdit2;
	Q3ListView *ListView_observers;
	
	Q3ToolBar *fileBar, *toolBar, *editBar;

	Q3PopupMenu *fileMenu, *importExportMenu, *editMenu, *navMenu, *settingsMenu, *viewMenu, *helpMenu;

	Q3Action *escapeFocus, *toggleEdit, *toggleMarks;
	Q3Action *fileNewBoard,*fileNew, *fileOpen, *fileSave, *fileSaveAs, *fileClose,
		*fileImportASCII, *fileImportASCIIClipB,*fileExportASCII,
		*fileImportSgfClipB, *fileExportSgfClipB,
		*fileExportPic, *fileExportPicClipB,
		*fileQuit ;
	Q3Action *editCut, *editPaste, *editPasteBrother, *editDelete, *editHideStones, //QQQ
		*editNumberMoves, *editMarkBrothers,
		*editMarkSons;
	Q3Action *navBackward, *navForward, *navFirst, *navLast, *navNextVar, *navPrevVar,
		*navMainBranch, *navStartVar, *navNextBranch, *navNthMove, *navAutoplay, *navEmptyBranch,
		*navCloneNode, *navSwapVariations, *navNextComment, *navPrevComment, *navIntersection ;       //SL added eb 11                               // added eb the 2 last
	Q3Action *setPreferences, *setGameInfo, *soundToggle;
	Q3Action *viewFileBar, *viewToolBar, *viewEditBar, *viewMenuBar, *viewStatusBar, *viewCoords,
		*viewSlider, *viewSidebar, *viewComment, *viewVertComment, *viewPinComment, *viewIncreaseSize,
		*viewDecreaseSize, *viewSaveSize, *viewFullscreen;
	Q3Action *helpManual, *helpSoundInfo, *helpAboutApp, *helpAboutQt;
	QTimer *timer;
	
	float timerIntervals[6];
	bool isFullScreen;

	Q3GridLayout *mainWidgetGuiLayout;
};

#endif

