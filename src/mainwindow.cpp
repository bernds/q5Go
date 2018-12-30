/*
* mainwindow.cpp - qGo's main window
*/

#include <QFileDialog>

#include <QLabel>
#include <QPixmap>
#include <QCloseEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QMenu>
#include <QWhatsThis>

#include "qgo.h"
#include "mainwin.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "board.h"
#include "tip.h"
#include "setting.h"
#include "icons.h"
#include "newgame_gui.h"
#include "interfacehandler.h"
#include "komispinbox.h"
#include "parserdefs.h"
#include "config.h"
#include "move.h"
#include "qnewgamedlg.h"
#include "qgo_interface.h"
#include <QShortcut>
#include <QAction>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <q3listbox.h>

#include <qcheckbox.h>
#include <qsplitter.h>
//#include <qmultilineedit.h>
#include <q3textedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qtabwidget.h>
#include <qlayout.h>

//#ifdef USE_XPM
#include ICON_PREFS
#include ICON_GAMEINFO
#include ICON_EXIT
#include ICON_FILENEWBOARD
#include ICON_FILENEW
#include ICON_FILEOPEN
#include ICON_FILESAVE
#include ICON_FILESAVEAS
#include ICON_TRANSFORM
#include ICON_CHARSET
#include ICON_RIGHTARROW
#include ICON_LEFTARROW
#include ICON_RIGHTCOMMENT
#include ICON_LEFTCOMMENT
#include ICON_TWO_RIGHTARROW
#include ICON_TWO_LEFTARROW
#include ICON_NEXT_VAR
#include ICON_PREV_VAR
#include ICON_MAIN_BRANCH
#include ICON_START_VAR
#include ICON_NEXT_BRANCH
#include ICON_AUTOPLAY
#include ICON_CUT
#include ICON_PASTE
#include ICON_DELETE
#include ICON_FULLSCREEN
#include ICON_WHATSTHIS
#include ICON_MANUAL
#include ICON_NAV_INTERSECTION
#include ICON_COMPUTER_PLAY
#include ICON_COORDS
#include ICON_SOUND_ON
#include ICON_SOUND_OFF
//#endif



MainWindow::MainWindow(QWidget* parent, const char* name, Qt::WFlags f)
	: QMainWindow(parent, name, f)
{
	setProperty("icon", setting->image0);

	parent_ = 0;

	isFullScreen = 0;
	setFocusPolicy(Qt::StrongFocus);

	setIcon(setting->image0);

	initActions();
	initMenuBar();
	initToolBar();
	initStatusBar();

	bool bb=setting->readBoolEntry("FILEBAR");
	bb=false;
	if (!setting->readBoolEntry("FILEBAR"))
		viewFileBar->setChecked(false);
	if (!setting->readBoolEntry("TOOLBAR"))
		viewToolBar->setChecked(false);
	if (!setting->readBoolEntry("EDITBAR"))
		viewEditBar->setChecked(false);

	if (!setting->readBoolEntry("STATUSBAR"))
		viewStatusBar->setChecked(false); //statusBar()->hide();

#if 0
	if (!setting->readBoolEntry("MENUBAR"))
		viewMenuBar->setChecked(false); //menuBar()->hide();
#endif

	interfaceHandler = 0;

	// VIEW_COMMENT: 0 = see BOARDVERTCOMMENT, 1 = hor, 2 = ver
	// BOARDVERCOMMENT: 0 = hor, 1 = ver, 2 = not shown
	if (setting->readIntEntry("VIEW_COMMENT") == 2 ||
		setting->readIntEntry("VIEW_COMMENT") == 0 && setting->readIntEntry("BOARDVERTCOMMENT_0"))
	{
		// show vertical comment
		splitter = new QSplitter(Qt::Horizontal, this);
		mainWidget = new MainWidget(this, splitter, "MainWidget");
		splitter_comment = new QSplitter(Qt::Vertical, splitter);
	}
	else
	{
		splitter = new QSplitter(Qt::Vertical, this);
		mainWidget = new MainWidget(this, splitter, "MainWidget");
		splitter_comment = new QSplitter(Qt::Horizontal, splitter);
	}
	splitter->setOpaqueResize(false);

//	mainWidget = new MainWidget(splitter, "MainWidget");

	mainWidgetGuiLayout = new QGridLayout(mainWidget, 1, 1, 0, 0);
	if (setting->readBoolEntry("SIDEBAR_LEFT"))
	{
		mainWidgetGuiLayout->addWidget(mainWidget->toolsFrame, 0, 0);
		mainWidgetGuiLayout->addWidget(mainWidget->boardFrame, 0, 1);

	}
	else
	{
		mainWidgetGuiLayout->addWidget(mainWidget->toolsFrame, 0, 1);
		mainWidgetGuiLayout->addWidget(mainWidget->boardFrame, 0, 0);
	}

	board = mainWidget->board;
	CHECK_PTR(board);
	// Connect the mouseMove event of the board with the status bar coords widget
	connect(board, SIGNAL(coordsChanged(int, int, int,bool)), statusTip, SLOT(slotStatusTipCoords(int, int, int,bool)));

	//commentEdit = new QTextEdit(splitter_comment, "comments");
	QWidget *commentWidget = new QWidget(splitter_comment);
	QVBoxLayout *commentLayout = new QVBoxLayout(commentWidget, 0,0,"commentLayout");
	commentEdit = new QTextEdit(commentWidget,  "comments");
	commentLayout->addWidget(commentEdit);
	commentEdit2 = new QLineEdit( commentWidget, "commentEdit2" );
	commentLayout->addWidget(commentEdit2);

	ListView_observers = new Q3ListView(splitter_comment, "observers");
	splitter->setResizeMode(mainWidget, QSplitter::KeepSize);
	splitter_comment->setResizeMode(ListView_observers, QSplitter::KeepSize);
	ListView_observers->addColumn(tr("Observers") + "     ");
	ListView_observers->setProperty("focusPolicy", (int)Qt::NoFocus );
	ListView_observers->setProperty("resizePolicy", (int)Q3ListView::AutoOneFit );
	ListView_observers->addColumn(tr("Rk"));
	ListView_observers->setProperty("focusPolicy", (int)Qt::NoFocus );
	ListView_observers->setProperty("resizePolicy", (int)Q3ListView::AutoOneFit );
	ListView_observers->setSorting(1);
	// disable sorting; else sort rank would sort by rank string (col 2) instead of rank key (col 3, invisible)
	ListView_observers->setSorting(-1);

	commentEdit->setWordWrapMode(QTextOption::WordWrap);
	//commentEdit2 = mainWidget->commentEdit2;
	//commentEdit2 = new QLineEdit( boardFrame, "commentEdit2" );

	//    connect(commentEdit2, SIGNAL(returnPressed()), board, SLOT(modifiedComment()));
	connect(commentEdit, SIGNAL(textChanged()), board, SLOT(updateComment()));
	connect(commentEdit2, SIGNAL(returnPressed()), board, SLOT(updateComment2()));

	// Connect Ctrl-E with MainWidget 'Edit' button. We need this to control the button
	// even when the sidebar is hidden.
//	connect(toggleEdit, SIGNAL(activated()), mainWidget->modeButton, SLOT(animateClick()));

	setCentralWidget(splitter);

	interfaceHandler = mainWidget->interfaceHandler;
	CHECK_PTR(interfaceHandler);
	interfaceHandler->fileImportASCII = fileImportASCII;
	interfaceHandler->fileImportASCIIClipB = fileImportASCIIClipB;
	interfaceHandler->fileImportSgfClipB = fileImportSgfClipB;
	interfaceHandler->navForward = navForward;
	interfaceHandler->navBackward = navBackward;
	interfaceHandler->navFirst = navFirst;
	interfaceHandler->navLast = navLast;
	interfaceHandler->navNextVar = navNextVar;
	interfaceHandler->navPrevVar = navPrevVar;
	interfaceHandler->navStartVar = navStartVar;
	interfaceHandler->navMainBranch = navMainBranch;
	interfaceHandler->navNextBranch = navNextBranch;
	interfaceHandler->navPrevComment = navPrevComment;
	interfaceHandler->navNextComment = navNextComment;
	interfaceHandler->navIntersection = navIntersection;
	interfaceHandler->editDelete = editDelete;
	interfaceHandler->navNthMove = navNthMove;
	interfaceHandler->navAutoplay = navAutoplay;
	interfaceHandler->navSwapVariations = navSwapVariations;
	interfaceHandler->commentEdit = commentEdit;
	interfaceHandler->commentEdit2 = commentEdit2;
	interfaceHandler->statusTurn = statusTurn;
	interfaceHandler->statusNav = statusNav;
	interfaceHandler->slider = mainWidget->slider;
	interfaceHandler->mainWidget = mainWidget;
	interfaceHandler->fileNew = fileNew;
	interfaceHandler->fileNewBoard = fileNewBoard ;
	interfaceHandler->fileOpen = fileOpen ;

	// Create a timer instance
	// timerInterval = 2;  // 1000 msec
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerForward()));
	timerIntervals[0] = (float) 0.1;
	timerIntervals[1] = 0.5;
	timerIntervals[2] = 1.0;
	timerIntervals[3] = 2.0;
	timerIntervals[4] = 3.0;
	timerIntervals[5] = 5.0;

	updateBoard();

	mainWidget->setGameMode (modeNormal);
	// restore board window
	reStoreWindowSize("0", false);

	toolBar->setFocus();
	updateFont();
}

MainWindow::~MainWindow()
{
	delete timer;
	delete commentEdit;
	delete mainWidget;
	delete splitter;

	// status bar
	delete statusMode;
	delete statusNav;
	delete statusTurn;
	delete statusTip;

	// tool bar;
	delete editBar;
	delete toolBar;
	delete fileBar;

	// menu bar
	delete helpMenu;
	delete viewMenu;
	delete settingsMenu;
	delete navMenu;
	delete editMenu;
	delete fileMenu;
	delete importExportMenu;

	// Actions
	delete escapeFocus;
	delete toggleEdit;
	delete toggleMarks;
	delete fileNewBoard;
	delete fileNew;
	delete fileOpen;
	delete fileSave;
	delete fileSaveAs;
	delete fileClose;
	delete fileImportASCII;
	delete fileImportASCIIClipB;
	delete fileExportASCII;
	delete fileImportSgfClipB;
	delete fileExportSgfClipB;
	delete fileExportPic;
	delete fileExportPicClipB;
	delete fileQuit;
	delete editDelete;
	delete editNumberMoves;
	delete editMarkBrothers;
	delete editMarkSons;
	delete navBackward;
	delete navForward;
	delete navFirst;
	delete navLast;
	delete navPrevVar;
	delete navNextVar;
	delete navMainBranch;
	delete navStartVar;
	delete navNextBranch;
	delete navPrevComment;
	delete navNextComment;
	delete navIntersection; //SL added eb 11
	delete navNthMove;
	delete navAutoplay;
	delete navSwapVariations;
	delete setPreferences;
	delete setGameInfo;
	delete viewFileBar;
	delete viewToolBar;
	delete viewEditBar;
	delete viewMenuBar;
	delete viewStatusBar;
	delete viewCoords;
	delete viewSidebar;
	delete viewComment;
	delete viewVertComment;
	delete viewSaveSize;
	delete viewFullscreen;
	delete helpManual;
	delete helpSoundInfo;
	delete helpAboutApp;
	delete helpAboutQt;
	delete whatsThis;
}

void MainWindow::initActions()
{
	// Load the pixmaps
	QPixmap exitIcon, fileNewboardIcon, fileNewIcon, fileOpenIcon, fileSaveIcon, fileSaveAsIcon,
		transformIcon, charIcon, deleteIcon,
		nextCommentIcon, previousCommentIcon, navIntersectionIcon,
		rightArrowIcon, leftArrowIcon,two_rightArrowIcon, two_leftArrowIcon,
		prevVarIcon, nextVarIcon, startVarIcon,	mainBranchIcon, nextBranchIcon, autoplayIcon,
		prefsIcon, infoIcon, fullscreenIcon, manualIcon,
		coordsIcon, sound_onIcon, sound_offIcon;

	prefsIcon = QPixmap(package_settings_xpm);
	infoIcon = QPixmap(idea_xpm);
	exitIcon = QPixmap(exit_xpm);
	fileNewboardIcon = QPixmap(newboard_xpm);
	fileNewIcon = QPixmap(filenew_xpm);
	fileOpenIcon = QPixmap(fileopen_xpm);
	fileSaveIcon = QPixmap(filesave_xpm);
	fileSaveAsIcon = QPixmap(filesaveas_xpm);
	transformIcon = QPixmap(transform_xpm);
	charIcon = QPixmap(charset_xpm);
	deleteIcon = QPixmap(editdelete_xpm);
	rightArrowIcon = QPixmap(rightarrow_xpm);
	leftArrowIcon = QPixmap(leftarrow_xpm);
	nextCommentIcon = QPixmap(rightcomment_xpm);
	previousCommentIcon = QPixmap(leftcomment_xpm);
	two_rightArrowIcon = QPixmap(two_rightarrow_xpm);
	two_leftArrowIcon = QPixmap(two_leftarrow_xpm);
	nextVarIcon = QPixmap(down_xpm);
	prevVarIcon = QPixmap(up_xpm);
	mainBranchIcon = QPixmap(start_xpm);
	startVarIcon = QPixmap(top_xpm);
	nextBranchIcon = QPixmap(bottom_xpm);
	fullscreenIcon = QPixmap(window_fullscreen_xpm);
	manualIcon = QPixmap(help_xpm);
	autoplayIcon = QPixmap(player_pause_xpm);
	navIntersectionIcon  = QPixmap(navIntersection_xpm);  //SL added eb 11
  	coordsIcon= QPixmap(coords_xpm);
	sound_onIcon= QPixmap(sound_on_xpm);
	sound_offIcon= QPixmap(sound_off_xpm);

	/*
	* Global actions
	*/
	// Escape focus: Escape key to get the focus from comment field to main window.
 	escapeFocus = new QAction(this);
	escapeFocus->setShortcut(Qt::Key_Escape);
	connect(escapeFocus, SIGNAL(activated()), this, SLOT(setFocus()));

	// Toggle game mode: Normal / Edit - Ctrl-E
	toggleEdit = new QAction(this);
	toggleEdit->setShortcut(Qt::CTRL + Qt::Key_E);
	// connect in constructor, as we need the mainwidget instance first.

	// Toggle through the marks - Ctrl-T
	toggleMarks = new QAction(this);
	toggleMarks->setShortcut (Qt::CTRL + Qt::Key_T);
	connect(toggleMarks, SIGNAL(activated()), this, SLOT(slotToggleMarks()));

	/*
	* Menu File
	*/
	// File New Board
	fileNewBoard = new QAction(fileNewboardIcon, tr("New &Board"), this);
	fileNewBoard->setShortcut (Qt::CTRL + Qt::Key_B);
	fileNewBoard->setStatusTip(tr("Creates a new board"));
	fileNewBoard->setWhatsThis(tr("New\n\nCreates a new board."));
	connect(fileNewBoard, SIGNAL(activated()), this, SLOT(slotFileNewBoard()));

	// File New Game
	fileNew = new QAction(fileNewIcon, tr("&New game"), this);
	fileNew->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_N));
	fileNew->setStatusTip(tr("Creates a new game on this board"));
	fileNew->setWhatsThis(tr("New\n\nCreates a new game on this board."));
	connect(fileNew, SIGNAL(activated()), this, SLOT(slotFileNewGame()));

	// File Open
	fileOpen = new QAction(fileOpenIcon, tr("&Open"), this);
	fileOpen->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_O));
	fileOpen->setStatusTip(tr("Open a sgf file"));
	fileOpen->setWhatsThis(tr("Open\n\nOpen a sgf file."));
	connect(fileOpen, SIGNAL(activated()), this, SLOT(slotFileOpen()));

	// File Save
	fileSave = new QAction(fileSaveIcon, tr("&Save"), this);
	fileSave->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_S));
	fileSave->setStatusTip(tr("Save a sgf file"));
	fileSave->setWhatsThis(tr("Save\n\nSave a sgf file."));
	connect(fileSave, SIGNAL(activated()), this, SLOT(slotFileSave()));

	// File SaveAs
	fileSaveAs = new QAction(fileSaveAsIcon, tr("Save &As"), this);
	fileSaveAs->setStatusTip(tr("Save a sgf file under a new name"));
	fileSaveAs->setWhatsThis(tr("Save As\n\nSave a sgf file under a new name."));
	connect(fileSaveAs, SIGNAL(activated()), this, SLOT(slotFileSaveAs()));

	// File Close
	fileClose = new QAction(tr("&Close"), this);
	fileClose->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_W));
	fileClose->setStatusTip(tr("Close this board"));
	fileClose->setWhatsThis(tr("Exit\n\nClose this board."));
	connect(fileClose, SIGNAL(activated()), this, SLOT(slotFileClose()));

	// File ImportASCII
	fileImportASCII = new QAction(charIcon, tr("Import &ASCII"), this);
	fileImportASCII->setStatusTip(tr("Import an ASCII file as new variation"));
	fileImportASCII->setWhatsThis(tr("Import ASCII\n\nImport an ASCII file as new variation."));
	connect(fileImportASCII, SIGNAL(activated()), this, SLOT(slotFileImportASCII()));

	// File ImportASCIIClipB
	fileImportASCIIClipB = new QAction(charIcon, tr("Import ASCII from &clipboard"), this);
	fileImportASCIIClipB->setStatusTip(tr("Import an ASCII board as new variation from the clipboard"));
	fileImportASCIIClipB->setWhatsThis(tr("Import ASCII from clipboard\n\nImport an ASCII file as new variation from the clipboard."));
	connect(fileImportASCIIClipB, SIGNAL(activated()), this, SLOT(slotFileImportASCIIClipB()));

	// File ExportASCII
	fileExportASCII = new QAction(charIcon, tr("&Export ASCII"), this);
	fileExportASCII->setStatusTip(tr("Export current board to ASCII"));
	fileExportASCII->setWhatsThis(tr("Export ASCII\n\nExport current board to ASCII."));
	connect(fileExportASCII, SIGNAL(activated()), this, SLOT(slotFileExportASCII()));

	// File ImportSgfClipB
	fileImportSgfClipB = new QAction(fileOpenIcon, tr("Import SGF &from clipboard"), this);
	fileImportSgfClipB->setStatusTip(tr("Import a complete game in SGF format from clipboard"));
	fileImportSgfClipB->setWhatsThis(tr("Import SGF from clipboard\n\n"
		"Import a complete game in SGF format from clipboard."));
	connect(fileImportSgfClipB, SIGNAL(activated()), this, SLOT(slotFileImportSgfClipB()));

	// File ExportSgfClipB
	fileExportSgfClipB = new QAction(fileSaveIcon, tr("Export SGF &to clipboard"), this);
	fileExportSgfClipB->setStatusTip(tr("Export a complete game in SGF format to clipboard"));
	fileExportSgfClipB->setWhatsThis(tr("Export SGF to clipboard\n\n"
		"Export a complete game in SGF format to clipboard."));
	connect(fileExportSgfClipB, SIGNAL(activated()), this, SLOT(slotFileExportSgfClipB()));

	// File ExportPic
	fileExportPic = new QAction(transformIcon, tr("Export &Image"), this);
	fileExportPic->setStatusTip(tr("Export current board to an image"));
	fileExportPic->setWhatsThis(tr("Export Image\n\nExport current board to an image."));
	connect(fileExportPic, SIGNAL(activated()), this, SLOT(slotFileExportPic()));

	// File ExportPic
	fileExportPicClipB = new QAction(transformIcon, tr("E&xport Image to clipboard"), this);
	fileExportPicClipB->setStatusTip(tr("Export current board to the clipboard as image"));
	fileExportPicClipB->setWhatsThis(tr("Export Image to clipboard\n\nExport current board to the clipboard as image."));
	connect(fileExportPicClipB, SIGNAL(activated()), this, SLOT(slotFileExportPicClipB()));

	// File Quit
	fileQuit = new QAction(exitIcon, tr("E&xit"), this);
	fileQuit->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_Q));
	fileQuit->setStatusTip(tr("Quits the application"));
	fileQuit->setWhatsThis(tr("Exit\n\nQuits the application."));
	connect(fileQuit, SIGNAL(activated()), this, SLOT(slotFileClose()));//(qGo*)qApp, SLOT(quit()));

	/*
	* Menu Edit
	*/

	// Edit delete
	editDelete = new QAction (deleteIcon, tr("&Delete"), this);
	editDelete->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_D));
	editDelete->setStatusTip(tr("Delete this and all following positions"));
	editDelete->setWhatsThis(tr("Delete\n\nDelete this and all following positions."));
	connect(editDelete, SIGNAL(activated()), this, SLOT(slotEditDelete()));

	// Edit number moves
	editNumberMoves = new QAction(tr("&Number Moves"), this);
	editNumberMoves->setShortcut (Qt::SHIFT + Qt::Key_F2);
	editNumberMoves->setStatusTip(tr("Mark all moves with the number of their turn"));
	editNumberMoves->setWhatsThis(tr("Number moves\n\nMark all moves with the number of their turn."));
	connect(editNumberMoves, SIGNAL(activated()), this, SLOT(slotEditNumberMoves()));

	// Edit mark brothers
	editMarkBrothers = new QAction(tr("Mark &brothers"), this);
	editMarkBrothers->setShortcut (Qt::SHIFT + Qt::Key_F3);
	editMarkBrothers->setStatusTip(tr("Mark all brothers of the current move"));
	editMarkBrothers->setWhatsThis(tr("Mark brothers\n\nMark all brothers of the current move."));
	connect(editMarkBrothers, SIGNAL(activated()), this, SLOT(slotEditMarkBrothers()));

	// Edit mark sons
	editMarkSons = new QAction(tr("Mark &sons"), this);
	editMarkSons->setShortcut (Qt::SHIFT + Qt::Key_F4);
	editMarkSons->setStatusTip(tr("Mark all sons of the current move"));
	editMarkSons->setWhatsThis(tr("Mark sons\n\nMark all sons of the current move."));
	connect(editMarkSons, SIGNAL(activated()), this, SLOT(slotEditMarkSons()));

	/*
	* Menu Navigation
	*/
	// Navigation Backward
	navBackward = new QAction(leftArrowIcon, tr("&Previous move") + "\t" + tr("Left"), this);
	navBackward->setStatusTip(tr("To previous move"));
	navBackward->setWhatsThis(tr("Previous move\n\nMove one move backward."));
	connect(navBackward, SIGNAL(activated()), this, SLOT(slotNavBackward()));

	// Navigation Forward
	navForward = new QAction(rightArrowIcon, tr("&Next move") + "\t" + tr("Right"), this);
	navForward->setStatusTip(tr("To next move"));
	navForward->setWhatsThis(tr("Next move\n\nMove one move forward."));
	connect(navForward, SIGNAL(activated()), this, SLOT(slotNavForward()));

	// Navigation First
	navFirst = new QAction(two_leftArrowIcon, tr("&First move") + "\t" + tr("Home"), this);
	navFirst->setStatusTip(tr("To first move"));
	navFirst->setWhatsThis(tr("First move\n\nMove to first move."));
	connect(navFirst, SIGNAL(activated()), this, SLOT(slotNavFirst()));

	// Navigation Last
	navLast = new QAction(two_rightArrowIcon, tr("&Last move") + "\t" + tr("End"), this);
	navLast->setStatusTip(tr("To last move"));
	navLast->setWhatsThis(tr("Last move\n\nMove to last move."));
	connect(navLast, SIGNAL(activated()), this, SLOT(slotNavLast()));

	// Navigation previous variation
	navPrevVar = new QAction(prevVarIcon, tr("P&revious variation") + "\t" + tr("Up"), this);
	navPrevVar->setStatusTip(tr("To previous variation"));
	navPrevVar->setWhatsThis(tr("Previous variation\n\nMove to the previous variation of this move."));
	connect(navPrevVar, SIGNAL(activated()), this, SLOT(slotNavPrevVar()));

	// Navigation next variation
	navNextVar = new QAction(nextVarIcon, tr("N&ext variation") + "\t" + tr("Down"), this);
	navNextVar->setStatusTip(tr("To next variation"));
	navNextVar->setWhatsThis(tr("Next variation\n\nMove to the next variation of this move."));
	connect(navNextVar, SIGNAL(activated()), this, SLOT(slotNavNextVar()));

	// Navigation main branch
	navMainBranch = new QAction(mainBranchIcon, tr("&Main branch"), this);
	navMainBranch->setShortcut (Qt::Key_Insert);
	navMainBranch->setStatusTip(tr("To main branch"));
	navMainBranch->setWhatsThis(tr("Main Branch\n\nMove to the main branch where variation started."));
	connect(navMainBranch, SIGNAL(activated()), this, SLOT(slotNavMainBranch()));

	// Navigation variation start
	navStartVar = new QAction(startVarIcon, tr("Variation &start"), this);
	navStartVar->setShortcut (Qt::Key_PageUp);
	navStartVar->setStatusTip(tr("To top of variation"));
	navStartVar->setWhatsThis(tr("Variation start\n\nMove to the top variation of this branch."));
	connect(navStartVar, SIGNAL(activated()), this, SLOT(slotNavStartVar()));

	// Navigation next branch
	navNextBranch = new QAction(nextBranchIcon, tr("Next &branch"), this);
	navNextBranch->setShortcut (Qt::Key_PageDown);
	navNextBranch->setStatusTip(tr("To next branch starting a variation"));
	navNextBranch->setWhatsThis(tr("Next branch\n\nMove to the next branch starting a variation."));
	connect(navNextBranch, SIGNAL(activated()), this, SLOT(slotNavNextBranch()));

	// Navigation goto Nth move
	navNthMove = new QAction(tr("&Goto Move"), this);
	navNthMove->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_G));
	navNthMove->setStatusTip(tr("Goto a move of main branch by number"));
	navNthMove->setWhatsThis(tr("Goto move\n\nGoto a move of main branch by number."));
	connect(navNthMove, SIGNAL(activated()), this, SLOT(slotNavNthMove()));

	// Navigation Autoplay
	navAutoplay = new QAction(autoplayIcon, tr("&Autoplay"), this);
	navAutoplay->setCheckable (true);
	navAutoplay->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_A));
	navAutoplay->setChecked(false);
	navAutoplay->setStatusTip(tr("Start/Stop autoplaying current game"));
	navAutoplay->setWhatsThis(tr("Autoplay\n\nStart/Stop autoplaying current game."));
	connect(navAutoplay, SIGNAL(toggled(bool)), this, SLOT(slotNavAutoplay(bool)));

	// Navigation swap variations
	navSwapVariations = new QAction(tr("S&wap variations"), this);
	navSwapVariations->setStatusTip(tr("Swap current move with previous variation"));
	navSwapVariations->setWhatsThis(tr("Swap variations\n\nSwap current move with previous variation."));
	connect(navSwapVariations, SIGNAL(activated()), this, SLOT(slotNavSwapVariations()));

	// Navigation previous comment
	navPrevComment = new QAction(previousCommentIcon, tr("Previous &commented move"), this);
	navPrevComment->setStatusTip(tr("To previous comment"));
	navPrevComment->setWhatsThis(tr("Previous comment\n\nMove to the previous move that has a comment"));
	connect(navPrevComment, SIGNAL(activated()), this, SLOT(slotNavPrevComment()));

	// Navigation next comment
	navNextComment = new QAction(nextCommentIcon, tr("Next c&ommented move"), this);
	navNextComment->setStatusTip(tr("To next comment"));
	navNextComment->setWhatsThis(tr("Next comment\n\nMove to the next move that has a comment"));
	connect(navNextComment, SIGNAL(activated()), this, SLOT(slotNavNextComment()));

	// Navigation to clicked intersection
	navIntersection = new QAction(navIntersectionIcon, tr("Goto clic&ked move"), this);
	navIntersection->setStatusTip(tr("To clicked move"));
	navIntersection->setWhatsThis(tr("Click on a board intersection\n\nMove to the stone played at this intersection (if any)"));
	connect(navIntersection, SIGNAL(activated()), this, SLOT(slotNavIntersection()));

	/*
	* Menu Settings
	*/
	// Settings Preferences
	setPreferences = new QAction(prefsIcon, tr("&Preferences"), this);
	setPreferences->setShortcut (Qt::ALT + Qt::Key_P);
	setPreferences->setStatusTip(tr("Edit the preferences"));
	setPreferences->setWhatsThis(tr("Preferences\n\nEdit the applications preferences."));
	connect(setPreferences, SIGNAL(activated()), client_window, SLOT(slot_preferences()));

	// Setings GameInfo
	setGameInfo = new QAction(infoIcon, tr("&Game Info"), this);
	setGameInfo->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_I));
	setGameInfo->setStatusTip(tr("Display game information"));
	setGameInfo->setWhatsThis(tr("Game Info\n\nDisplay game information."));
	connect(setGameInfo, SIGNAL(activated()), this, SLOT(slotSetGameInfo()));

	//Toggling sound
	QIcon  OIC;
	OIC.setPixmap ( sound_offIcon, QIcon::Automatic, QIcon::Normal, QIcon::On);
	OIC.setPixmap ( sound_onIcon, QIcon::Automatic, QIcon::Normal, QIcon::Off );
	soundToggle = new QAction(OIC, tr("&Mute stones sound"), this);
	soundToggle->setCheckable (true);
	soundToggle->setChecked(!setting->readBoolEntry("SOUND_STONE"));
	soundToggle->setStatusTip(tr("Toggle stones sound on/off"));
	soundToggle->setWhatsThis(tr("Stones sound\n\nToggle stones sound on/off\nthis toggles only the stones sounds"));
	connect(soundToggle, SIGNAL(toggled(bool)), this, SLOT(slotSoundToggle(bool)));

	/*
	* Menu View
	*/
	// View Filebar toggle
	viewFileBar = new QAction(tr("&File toolbar"), this);
	viewFileBar->setCheckable (true);
	viewFileBar->setChecked(true);
	viewFileBar->setStatusTip(tr("Enables/disables the file toolbar"));
	viewFileBar->setWhatsThis(tr("File toolbar\n\nEnables/disables the file toolbar."));
	connect(viewFileBar, SIGNAL(toggled(bool)), this, SLOT(slotViewFileBar(bool)));

	// View Toolbar toggle
	viewToolBar = new QAction(tr("Navigation &toolbar"), this);
	viewToolBar->setCheckable (true);
	viewToolBar->setChecked(true);
	viewToolBar->setStatusTip(tr("Enables/disables the navigation toolbar"));
	viewToolBar->setWhatsThis(tr("Navigation toolbar\n\nEnables/disables the navigation toolbar."));
	connect(viewToolBar, SIGNAL(toggled(bool)), this, SLOT(slotViewToolBar(bool)));

	// View Editbar toggle
	viewEditBar = new QAction(tr("&Edit toolbar"), this);
	viewEditBar->setCheckable (true);
	viewEditBar->setChecked(true);
	viewEditBar->setStatusTip(tr("Enables/disables the edit toolbar"));
	viewEditBar->setWhatsThis(tr("Edit toolbar\n\nEnables/disables the edit toolbar."));
	connect(viewEditBar, SIGNAL(toggled(bool)), this, SLOT(slotViewEditBar(bool)));

#if 0 /* After porting from Q3Action, these no longer trigger when the menu is hidden, so it can't be unhidden.  */
	// View Menubar toggle
	viewMenuBar = new QAction(tr("&Menubar"), this);
	viewMenuBar->setCheckable (true);
	viewMenuBar->setShortcut (Qt::Key_F7);
	viewMenuBar->setChecked(true);
	viewMenuBar->setStatusTip(tr("Enables/disables the menubar"));
	viewMenuBar->setWhatsThis(tr("Menubar\n\nEnables/disables the menubar."));
	connect(viewMenuBar, SIGNAL(toggled(bool)), this, SLOT(slotViewMenuBar(bool)));
#endif

	// View Statusbar toggle
	viewStatusBar = new QAction(tr("&Statusbar"), this);
	viewStatusBar->setCheckable (true);
	viewStatusBar->setChecked(true);
	viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
	viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar."));
	connect(viewStatusBar, SIGNAL(toggled(bool)), this, SLOT(slotViewStatusBar(bool)));

	// View Coordinates toggle
	viewCoords = new QAction(coordsIcon, tr("C&oordinates"), this);
	viewCoords->setCheckable (true);
	viewCoords->setShortcut (Qt::Key_F8);
	viewCoords->setChecked(false);
	viewCoords->setStatusTip(tr("Enables/disables the coordinates"));
	viewCoords->setWhatsThis(tr("Coordinates\n\nEnables/disables the coordinates."));
	connect(viewCoords, SIGNAL(toggled(bool)), this, SLOT(slotViewCoords(bool)));

	// View Slider toggle
	viewSlider = new QAction(tr("Sli&der"), this);
	viewSlider->setCheckable (true);
	viewSlider->setShortcut (Qt::CTRL + Qt::Key_F8);
	viewSlider->setChecked(false);
	viewSlider->setStatusTip(tr("Enables/disables the slider"));
	viewSlider->setWhatsThis(tr("Slider\n\nEnables/disables the slider."));
	connect(viewSlider, SIGNAL(toggled(bool)), this, SLOT(slotViewSlider(bool)));

	// View Sidebar toggle
	viewSidebar = new QAction(tr("Side&bar"), this);
	viewSidebar->setCheckable (true);
	viewSidebar->setShortcut (Qt::Key_F9);
	viewSidebar->setChecked(true);
	viewSidebar->setStatusTip(tr("Enables/disables the sidebar"));
	viewSidebar->setWhatsThis(tr("Sidebar\n\nEnables/disables the sidebar."));
	connect(viewSidebar, SIGNAL(toggled(bool)), this, SLOT(slotViewSidebar(bool)));

	// View Comment toggle
	viewComment = new QAction(tr("&Comment"), this);
	viewComment->setCheckable (true);
	viewComment->setShortcut (Qt::Key_F10);
	viewComment->setChecked(true);
	viewComment->setStatusTip(tr("Enables/disables the comment field"));
	viewComment->setWhatsThis(tr("Comment field\n\nEnables/disables the comment field."));
	connect(viewComment, SIGNAL(toggled(bool)), this, SLOT(slotViewComment(bool)));

	// View Vertical Comment toggle
	viewVertComment = new QAction(tr("&Vertical comment"), this);
	viewVertComment->setCheckable (true);
	viewVertComment->setShortcut (Qt::SHIFT + Qt::Key_F10);
	viewVertComment->setChecked(setting->readIntEntry("VIEW_COMMENT") == 2 ||
		setting->readIntEntry("VIEW_COMMENT") == 0 && setting->readIntEntry("BOARDVERTCOMMENT_0"));
	viewVertComment->setStatusTip(tr("Enables/disables a vertical direction of the comment field"));
	viewVertComment->setWhatsThis(tr("Vertical comment field\n\n"
		"Enables/disables a vertical direction of the comment field.\n\nNote: This setting is temporary for this board. In order to set permanent horizontal/vertical comment use 'Preferences'."));
	connect(viewVertComment, SIGNAL(toggled(bool)), this, SLOT(slotViewVertComment(bool)));

	// View Save Size
	viewSaveSize = new QAction(tr("Save si&ze"), this);
	viewSaveSize->setShortcut (Qt::ALT + Qt::Key_0);
	viewSaveSize->setStatusTip(tr("Save the current window size"));
	viewSaveSize->setWhatsThis(tr("Save size\n\n"
		"Saves the current window size and restores it on the next program start.\n\nUse ALT + <number key> to store own sizes\nRestore with CTRL + <number key>\n\n<0> is default value at program start.\n<9> is default for edit window."));
	connect(viewSaveSize, SIGNAL(activated()), this, SLOT(slotViewSaveSize()));

	// View Fullscreen
	viewFullscreen = new QAction(fullscreenIcon, tr("&Fullscreen"), this);
	viewFullscreen->setCheckable (true);
	viewFullscreen->setShortcut (Qt::Key_F11);
	viewFullscreen->setChecked(false);
	viewFullscreen->setStatusTip(tr("Enable/disable fullscreen mode"));
	viewFullscreen->setWhatsThis(tr("Fullscreen\n\nEnable/disable fullscreen mode."));
	connect(viewFullscreen, SIGNAL(toggled(bool)), this, SLOT(slotViewFullscreen(bool)));

	/*
	* Menu Help
	*/
	// Help Manual
	helpManual = new QAction(manualIcon, tr("&Manual"), this);
	helpManual->setShortcut (Qt::Key_F1);
	helpManual->setStatusTip(tr("Opens the manual"));
	helpManual->setWhatsThis(tr("Help\n\nOpens the manual of the application."));
	connect(helpManual, SIGNAL(activated()), this, SLOT(slotHelpManual()));

	// Sound Info
	helpSoundInfo = new QAction(tr("&Sound"), this);
	helpSoundInfo->setStatusTip(tr("Short info on sound availability"));
	helpSoundInfo->setWhatsThis(tr("Sound Info\n\nViews a message box with a short comment about sound."));
	connect(helpSoundInfo, SIGNAL(activated()), this, SLOT(slotHelpSoundInfo()));

	// Help About
	helpAboutApp = new QAction(tr("&About..."), this);
	helpAboutApp->setStatusTip(tr("About the application"));
	helpAboutApp->setWhatsThis(tr("About\n\nAbout the application."));
	connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));

	// Help AboutQt
	helpAboutQt = new QAction(tr("About &Qt..."), this);
	helpAboutQt->setStatusTip(tr("About Qt"));
	helpAboutQt->setWhatsThis(tr("About Qt\n\nAbout Qt."));
	connect(helpAboutQt, SIGNAL(activated()), this, SLOT(slotHelpAboutQt()));

	// Disable some toolbuttons at startup
	navForward->setEnabled(false);
	navBackward->setEnabled(false);
	navFirst->setEnabled(false);
	navLast->setEnabled(false);
	navPrevVar->setEnabled(false);
	navNextVar->setEnabled(false);
	navMainBranch->setEnabled(false);
	navStartVar->setEnabled(false);
	navNextBranch->setEnabled(false);
	navSwapVariations->setEnabled(false);
	navPrevComment->setEnabled(false);
	navNextComment->setEnabled(false);
  	navIntersection->setEnabled(false);     //SL added eb 11

	whatsThis = QWhatsThis::createAction (this);
}

void MainWindow::initMenuBar()
{
	// submenu Import/Export
	importExportMenu = new QMenu(tr("&Import/Export"));
	importExportMenu->insertTearOffHandle();
	importExportMenu->addAction (fileImportASCII);
	importExportMenu->addAction (fileImportASCIIClipB);
	importExportMenu->addAction (fileExportASCII);
	importExportMenu->addAction (fileImportSgfClipB);
	importExportMenu->addAction (fileExportSgfClipB);
	importExportMenu->addAction (fileExportPic);
	importExportMenu->addAction (fileExportPicClipB);

	importExportMenu->insertSeparator(fileImportSgfClipB);
	importExportMenu->insertSeparator(fileExportPic);

	// menuBar entry fileMenu
	fileMenu = new QMenu(tr("&File"));
	fileMenu->insertTearOffHandle();
	fileMenu->addAction (fileNewBoard);
	fileMenu->addAction (fileNew);
	fileMenu->addAction (fileOpen);
	fileMenu->addAction (fileSave);
	fileMenu->addAction (fileSaveAs);
	fileMenu->addAction (fileClose);
	// Some slight weirdness in the construction here since we
	// can't insert a separator before a menu.
	fileMenu->insertSeparator(fileQuit);
	fileMenu->addMenu (importExportMenu);
	fileMenu->insertSeparator(fileQuit);
	fileMenu->addAction (fileQuit);

	// menuBar entry editMenu
	editMenu = new QMenu(tr("&Edit"));
	editMenu->insertTearOffHandle();
	editMenu->addAction (editDelete);
	editMenu->addAction (editNumberMoves);
	editMenu->addAction (editMarkBrothers);
	editMenu->addAction (editMarkSons);

	editMenu->insertSeparator(editNumberMoves);

	// menuBar entry navMenu
	navMenu = new QMenu(tr("&Navigation"));
	navMenu->insertTearOffHandle();
	navMenu->addAction (navFirst);
	navMenu->addAction (navBackward);
	navMenu->addAction (navForward);
	navMenu->addAction (navLast);
	navMenu->addAction (navMainBranch);
	navMenu->addAction (navStartVar);
	navMenu->addAction (navPrevVar);
	navMenu->addAction (navNextVar);
	navMenu->addAction (navNextBranch);
	navMenu->addAction (navNthMove);
	navMenu->addAction (navAutoplay);
	navMenu->addAction (navSwapVariations);
	navMenu->addAction (navPrevComment);
	navMenu->addAction (navNextComment);

	navMenu->insertSeparator(navMainBranch);
	navMenu->insertSeparator(navNthMove);
	navMenu->insertSeparator(navSwapVariations);
	navMenu->insertSeparator(navPrevComment);

	// menuBar entry settingsMenu
	settingsMenu = new QMenu(tr("&Settings"));
	settingsMenu->insertTearOffHandle();
	settingsMenu->addAction (setPreferences);
	settingsMenu->addAction (setGameInfo);
	settingsMenu->addAction (soundToggle);

	settingsMenu->insertSeparator(soundToggle);

	// menuBar entry viewMenu
	viewMenu = new QMenu(tr("&View"));
	viewMenu->insertTearOffHandle();
	viewMenu->addAction (viewFileBar);
	viewMenu->addAction (viewToolBar);
	viewMenu->addAction (viewEditBar);
#if 0
	viewMenu->addAction (viewMenuBar);
#endif
	viewMenu->addAction (viewStatusBar);
	viewMenu->addAction (viewCoords);
	viewMenu->addAction (viewSlider);
	viewMenu->addAction (viewSidebar);
	viewMenu->addAction (viewComment);
	viewMenu->addAction (viewVertComment);
	viewMenu->addAction (viewSaveSize);
	viewMenu->addAction (viewFullscreen);

	viewMenu->insertSeparator(viewSaveSize);
	viewMenu->insertSeparator(viewFullscreen);

	// menuBar entry helpMenu
	helpMenu = new QMenu(tr("&Help"));
	helpMenu->addAction (helpManual);
	helpMenu->addAction (whatsThis);
	helpMenu->addAction (helpSoundInfo);
	helpMenu->addAction (helpAboutApp);
	helpMenu->addAction (helpAboutQt);

	helpMenu->insertSeparator(helpSoundInfo);
	helpMenu->insertSeparator(helpAboutApp);

	// menubar configuration
	menuBar()->addMenu(fileMenu);
	menuBar()->addMenu(editMenu);
	menuBar()->addMenu(navMenu);
	menuBar()->addMenu(settingsMenu);
	menuBar()->addMenu(viewMenu);
	menuBar()->addMenu(helpMenu);
}

void MainWindow::initToolBar()
{
	// File toolbar
	fileBar = addToolBar ("filebar");

	fileBar->addAction (fileNew);
	fileBar->addAction (fileOpen);
	fileBar->addAction (fileSave);
	fileBar->addAction (fileSaveAs);

	// Navigation toolbar
	toolBar = addToolBar ("toolbar");

	toolBar->addAction (navFirst);
	toolBar->addAction (navBackward);
	toolBar->addAction (navForward);
	toolBar->addAction (navLast);

	toolBar->addSeparator();

	toolBar->addAction (navMainBranch);
	toolBar->addAction (navStartVar);
	toolBar->addAction (navPrevVar);
	toolBar->addAction (navNextVar);
	toolBar->addAction (navNextBranch);

	toolBar->addSeparator();
	toolBar->addAction (navPrevComment);        // added eb 2
	toolBar->addAction (navNextComment);
	toolBar->addAction (navIntersection);       //SL added eb 11
	toolBar->addSeparator();               //end add eb 2

	toolBar->addAction (navAutoplay);

	toolBar->addSeparator();

	toolBar->addAction (soundToggle);
	toolBar->addAction (viewCoords);

	toolBar->addSeparator();

	toolBar->addAction (whatsThis);
	toolBar->addSeparator();

//	toolBar->addAction (setPreferences);
	toolBar->addAction (setGameInfo);

	// Edit toolbar
	editBar = addToolBar ("editbar");
	editBar->addAction (editDelete);
}

void MainWindow::initStatusBar()
{
	// The coords widget
	statusTip = new StatusTip(statusBar());
	statusBar()->addWidget(statusTip);
	//statusBar()->show();
	statusBar()->setSizeGripEnabled(true);
	statusBar()->showMessage(tr("Ready."));  // Normal indicator
	connect(statusTip, SIGNAL(clearStatusBar()), statusBar(), SLOT(clear()));
	
	// The turn widget
	statusTurn = new QLabel(statusBar());
	statusTurn->setAlignment(Qt::AlignCenter | Qt::TextSingleLine);
	statusTurn->setText(" 0 ");
	statusBar()->addWidget(statusTurn, 0, true);  // Permanent indicator
	QToolTip::add(statusTurn, tr("Current move"));
	QWhatsThis::add(statusTurn, tr("Move\nDisplays the number of the current turn and the last move played."));
	
	// The nav widget
	statusNav = new QLabel(statusBar());
	statusNav->setAlignment(Qt::AlignCenter | Qt::TextSingleLine);
	statusNav->setText(" 0/0 ");
	statusBar()->addWidget(statusNav, 0, true);  // Permanent indicator
	QToolTip::add(statusNav, tr("Brothers / sons"));
	QWhatsThis::add(statusNav, tr("Navigation\nShows the brothers and sons of the current move."));
	
	// The mode widget
	statusMode = new QLabel(statusBar());
	statusMode->setAlignment(Qt::AlignCenter | Qt::TextSingleLine);
	statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
	statusBar()->addWidget(statusMode, 0, true);  // Permanent indicator
	QToolTip::add(statusMode, tr("Current mode"));
	QWhatsThis::add(statusMode,
		tr("Mode\nShows the current mode. 'N' for normal mode, 'E' for edit mode."));
}

void MainWindow::slotFileNewBoard()
{
	setting->qgo->addBoardWindow();
}

void MainWindow::slotFileNewGame()
{
	if (!checkModified())
		return;
	
	if (board->getGameMode() == modeNormal)
	{
		NewLocalGameDialog dlg(this, tr("newgame"), true);
		
		if (dlg.exec() == QDialog::Accepted)
		{
			GameData *d = new GameData;
			d->size = dlg.boardSizeSpin->value();
			d->komi = dlg.komiSpin->value();
			d->handicap = dlg.handicapSpin->value();
			d->playerBlack = dlg.playerBlackEdit->text();
			d->rankBlack = dlg.playerBlackRkEdit->text();
			d->playerWhite = dlg.playerWhiteEdit->text();
			d->rankWhite = dlg.playerWhiteRkEdit->text();
			d->gameName = "";
			d->gameNumber = 0;
			d->fileName = "";
			d->byoTime = dlg.byoTimeSpin->value();
			d->style = 1;
			board->initGame(d);
		}
	}
	else
	{
		NewGameDialog dlg(this, tr("newgame"), true);
		
		if (dlg.exec() == QDialog::Accepted)
		{
			GameData *d = new GameData;
			d->size = dlg.boardSizeSpin->value();
			d->komi = dlg.komiSpin->value();
			d->handicap = dlg.handicapSpin->value();
//			d->playerBlack = dlg.playerBlackEdit->text();
//			d->playerWhite = dlg.playerWhiteEdit->text();
			d->gameName = "";
			d->gameNumber = 0;
			d->fileName = "";
			d->byoTime = dlg.byoTimeSpin->value();
			d->style = 1;
			board->initGame(d);
		}
	}

	interfaceHandler->normalTools->komi->setText(QString::number(board->getGameData()->komi));
	interfaceHandler->normalTools->handicap->setText(QString::number(board->getGameData()->handicap));
	
	statusBar()->showMessage(tr("New board prepared."));
}

void MainWindow::slotFileOpen()
{
	if (!checkModified())
		return;
	QString fileName(QFileDialog::getOpenFileName(setting->readEntry("LAST_DIR"),
		tr("SGF Files (*.sgf *.SGF);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)"), this));
	if (fileName.isEmpty())
		return;
	doOpen(fileName, getFileExtension(fileName));
}

QString MainWindow::getFileExtension(const QString &fileName, bool defaultExt)
{
	QString filter;
	if (defaultExt)
		filter = tr("SGF");
	else
		filter = "";
	
	int pos=0, oldpos=-1, len = fileName.length();
	
	while ((pos = fileName.indexOf('.', ++pos)) != -1 && pos < len)
		oldpos = pos;
	
	if (oldpos != -1)
		filter = fileName.mid(oldpos+1, fileName.length()-pos).upper();
	
	return filter;
}

void MainWindow::doOpen(const QString &fileName, const QString &filter, bool storedir)
{
	// qDebug("doOpen - fileName: %s - filter: %s", fileName.latin1(), filter.latin1());
	
	if (setting->readBoolEntry("REM_DIR") && storedir)
		rememberLastDir(fileName);
	
	if (board->openSGF(fileName))
		statusBar()->showMessage(fileName + " " + tr("loaded."));
}

bool MainWindow::startComputerPlay(QNewGameDlg * dlg, const QString &fileName, const QString &computer_path)
{
	GameData *d = new GameData;
	d->size = dlg->getSize();
	d->komi = dlg->getKomi();
	d->handicap = dlg->getHandicap();
	d->playerBlack = dlg->getPlayerBlackName();
	//d->rankBlack = dlg.playerBlackRkEdit->text();
	d->playerWhite = dlg->getPlayerWhiteName();
	//d->rankWhite = dlg.playerWhiteRkEdit->text();
	d->gameName = "";
	d->gameNumber = 0;
	d->fileName = "";
	d->byoTime = dlg->getTime();
	d->style = 1;
	d->oneColorGo = dlg->getOneColorGo();

	blackPlayerType = dlg->getPlayerBlackType();
	whitePlayerType = dlg->getPlayerWhiteType();

	
	//if (fileName.isNull() || fileName.isEmpty())
	//	board->initGame(d);

	if (!board->startComputerPlay(dlg,fileName, computer_path))
		return false;

	return true;
}

bool MainWindow::slotFileSave()
{
	QString fileName;
	if ((fileName = board->getGameData()->fileName).isEmpty())
	{
		if (setting->readBoolEntry("REM_DIR"))
			fileName = setting->readEntry("LAST_DIR");
		else
			fileName = QString::null;
		return doSave(fileName, false);
	}
	else
		return doSave(board->getGameData()->fileName, true);
}

bool MainWindow::slotFileSaveAs()
{
//	if (setting->readBoolEntry("REM_DIR"))
//		return doSave(setting->readEntry("LAST_DIR"), false);
	return doSave(0, false);
}

bool MainWindow::doSave(QString fileName, bool force)
{
	if (!force)
  	{
     		if  (fileName.isNull() || fileName.isEmpty() || QDir(fileName).exists())
            	{
              		QString base = board->getCandidateFileName();
              		if (fileName.isNull() || fileName.isEmpty())
                		fileName = base;
              		else
                		fileName.append(base);

		}
		fileName = QFileDialog::getSaveFileName(fileName, tr("SGF Files (*.sgf);;All Files (*)"), this);
	}
	
	if (fileName.isEmpty())
		return false;
	
	if (getFileExtension(fileName, false).isEmpty())
		fileName.append(".sgf");
	
	// Confirm overwriting file.
	if (!force && QFile(fileName).exists())
		if (QMessageBox::information(this, PACKAGE,
			tr("This file already exists. Do you want to overwrite it?"),
			tr("Yes"), tr("No"), 0, 0, 1) == 1)
			return false;
		
	board->getGameData()->fileName = fileName;
		
	if (setting->readBoolEntry("REM_DIR"))
		rememberLastDir(fileName);
		
		if (!board->saveBoard(fileName))
		{
			QMessageBox::warning(this, PACKAGE, tr("Cannot save SGF file."));
			return false;
		}
		
	statusBar()->showMessage(fileName + " " + tr("saved."));
	board->setModified(false);
	return true;
}

void MainWindow::slotFileClose()
{
	if (checkModified() == 1)
	{
		board->setModified(false);  // Prevent to ask a second time in qGo::quit()
		close();
	}
}

void MainWindow::slotFileImportSgfClipB()
{
	// check wheter it's an edit board during online game
	if (getInterfaceHandler()->refreshButton->text() != tr("Update") && !checkModified())
		return;
	
	if (!board->importSGFClipboard())
		QMessageBox::warning(this, PACKAGE, tr("Cannot load from clipboard. Is it empty?"));
	else
		statusBar()->showMessage(tr("SGF imported."));
}

void MainWindow::slotFileExportSgfClipB()
{
	if (!board->exportSGFtoClipB())
		QMessageBox::warning(this, PACKAGE, tr("Failed to export SGF to clipboard."));
	else
		statusBar()->showMessage(tr("SGF exported."));
}

void MainWindow::slotFileImportASCII()
{
	QString fileName(QFileDialog::getOpenFileName(QString::null,
		tr("Text Files (*.txt);;All Files (*)"),
		this));
	if (fileName.isEmpty())
		return;
	
	board->importASCII(fileName);
	statusBar()->showMessage(tr("ASCII imported."));
}

void MainWindow::slotFileImportASCIIClipB()
{
	if (!board->importASCII(NULL, true))
		QMessageBox::warning(this, PACKAGE, tr("Importing ASCII failed. Clipboard empty?"));
	else
		statusBar()->showMessage(tr("ASCII imported."));
}

void MainWindow::slotFileExportASCII()
{
	board->exportASCII();
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotFileExportPic()
{
   QString *filter = new QString("");
   QString fileName = QFileDialog::getSaveFileName(
    "",
		"PNG (*.png);;BMP (*.bmp);;XPM (*.xpm);;XBM (*.xbm);;PNM (*.pnm);;GIF (*.gif);;JPEG (*.jpeg);;MNG (*.mng)",
		this,
    "qGo",
    tr("Export image as"),
    filter,
    true);


		if (fileName.isEmpty())
			return;

    //fileName.append(".").append(filter->left(3).lower());
    
		// Confirm overwriting file.
		if ( QFile::exists( fileName ) )
			if (QMessageBox::information(this, PACKAGE,
				tr("This file already exists. Do you want to overwrite it?"),
				tr("Yes"), tr("No"), 0, 0, 1) == 1)
				return;
			
			//QString filter = dlg.selectedFilter().left(3);
			board->exportPicture(fileName, filter->left(3));
//	}
}

void MainWindow::slotFileExportPicClipB()
{
	board->exportPicture(NULL, NULL, true);	
}

void MainWindow::slotEditDelete()
{
	board->deleteNode();
}

void MainWindow::slotEditNumberMoves()
{
	board->numberMoves();
}

void MainWindow::slotEditMarkBrothers()
{
	board->markVariations(false);
}

void MainWindow::slotEditMarkSons()
{
	board->markVariations(true);
}

void MainWindow::slotNavBackward()
{
	board->previousMove();
}

void MainWindow::slotNavForward()
{
	board->nextMove();
}

void MainWindow::slotNavFirst()
{
	board->gotoFirstMove();
}

void MainWindow::slotNavLast()
{
	board->gotoLastMove();
}

// this slot is used for edit window to navigate to last made move
void MainWindow::slotNavLastByTime()
{
	board->gotoLastMoveByTime();
}

void MainWindow::slotNavNextVar()
{
	board->nextVariation();
}

void MainWindow::slotNavPrevVar()
{
	board->previousVariation();
}

void MainWindow::slotNavNextComment()    //added eb
{
	board->nextComment();
}

void MainWindow::slotNavPrevComment()
{
	board->previousComment();
}                                        //end add eb

void MainWindow::slotNavStartVar()
{
	board->gotoVarStart();
}

void MainWindow::slotNavMainBranch()
{
	board->gotoMainBranch();
}

void MainWindow::slotNavNextBranch()
{
	board->gotoNextBranch();
}

void MainWindow::slotNavIntersection()       // added eb 11
{
    board->navIntersection();
}
                                     // end add eb 11


void MainWindow::slotNavNthMove()
{
	NthMoveDialog dlg(this, tr("entermove"), true);
	dlg.moveSpinBox->setValue(board->getCurrentMoveNumber());
	dlg.moveSpinBox->setFocus();
	
	if (dlg.exec() == QDialog::Accepted)
		board->gotoNthMove(dlg.moveSpinBox->value());
}

void MainWindow::slotNavAutoplay(bool toggle)
{
	if (!toggle)
	{
		timer->stop();
		statusBar()->showMessage(tr("Autoplay stopped."));
	}
	else
	{
		if (setting->readIntEntry("TIMER_INTERVAL") < 0)
			setting->writeIntEntry("TIMER_INTERVAL", 0);
		else if (setting->readIntEntry("TIMER_INTERVAL") > 10)
			setting->writeIntEntry("TIMER_INTERVAL", 10);
		// check if time info available from sgf file
		if (setting->readBoolEntry("SGF_TIME_TAGS") && board->getGameData()->timeSystem != time_none)
			// set time to 1 sec
			timer->start(1000);
		else
			// set time interval as selected
			timer->start(int(timerIntervals[setting->readIntEntry("TIMER_INTERVAL")] * 1000));
		statusBar()->showMessage(tr("Autoplay started."));
	}
}

void MainWindow::slotNavSwapVariations()
{
	if (board->swapVariations())
		statusBar()->showMessage(tr("Variations swapped."));
	else
		statusBar()->showMessage(tr("No previous variation available."));
}

void MainWindow::updateBoard()
{

	viewSlider->setChecked(setting->readBoolEntry("SLIDER"));
	viewSidebar->setChecked(setting->readBoolEntry("SIDEBAR"));
	viewCoords->setChecked(setting->readBoolEntry("BOARD_COORDS"));
	board->setShowSGFCoords(setting->readBoolEntry("SGF_BOARD_COORDS"));
	board->set_antiClicko(setting->readBoolEntry("ANTICLICKO"));
	viewComment->setChecked(setting->readIntEntry("VIEW_COMMENT"));

	if (setting->readIntEntry("VIEW_COMMENT"))
	{
//		viewVertComment->setEnabled(true);
		viewVertComment->setChecked(setting->readIntEntry("VIEW_COMMENT") == 2);
	}

#if 0 // @@@
	QToolTip::setEnabled(setting->readBoolEntry("TOOLTIPS"));
#endif
	if (timer->isActive())
	{
		if (setting->readBoolEntry("SGF_TIME_TAGS") && board->getGameData()->timeSystem != time_none)
			timer->changeInterval(1000);
		else
			timer->changeInterval(int(timerIntervals[setting->readIntEntry("TIMER_INVERVAL")] * 1000));
	}
	
	slotViewLeftSidebar();
	board->setVariationDisplay(static_cast<VariationDisplay>(setting->readIntEntry("VAR_GHOSTS")));
	board->setShowCursor(setting->readBoolEntry("CURSOR"));
	board->changeSize();  // For smaller stones
}

void MainWindow::slotSetGameInfo()
{
	GameInfoDialog dlg(this, "gameinfo", true);
	
	dlg.playerWhiteEdit->setText(board->getGameData()->playerWhite);
	
	dlg.playerBlackEdit->setText(board->getGameData()->playerBlack);
	dlg.whiteRankEdit->setText(board->getGameData()->rankWhite);
	dlg.blackRankEdit->setText(board->getGameData()->rankBlack);
	dlg.komiSpin->setValue(board->getGameData()->komi);
	dlg.handicapSpin->setValue(board->getGameData()->handicap);
	dlg.resultEdit->setText(board->getGameData()->result);
	dlg.dateEdit->setText(board->getGameData()->date);
	dlg.placeEdit->setText(board->getGameData()->place);
	dlg.copyrightEdit->setText(board->getGameData()->copyright);
	dlg.gameNameEdit->setText(board->getGameData()->gameName);

	if (dlg.exec() == QDialog::Accepted)
	{
		board->getGameData()->playerWhite = dlg.playerWhiteEdit->text();
		board->getGameData()->playerBlack = dlg.playerBlackEdit->text();
		board->getGameData()->rankWhite = dlg.whiteRankEdit->text();
		board->getGameData()->rankBlack = dlg.blackRankEdit->text();
		board->getGameData()->komi = dlg.komiSpin->value();
		board->getGameData()->handicap = dlg.handicapSpin->value();
		board->getGameData()->result = dlg.resultEdit->text();
		board->getGameData()->date = dlg.dateEdit->text();
		board->getGameData()->place = dlg.placeEdit->text();
		board->getGameData()->copyright = dlg.copyrightEdit->text();
		board->getGameData()->gameName = dlg.gameNameEdit->text();
		
		board->isModified = true;
		board->updateCaption();  // Update caption in any case
	}

	interfaceHandler->normalTools->komi->setText(QString::number(board->getGameData()->komi));
	interfaceHandler->normalTools->handicap->setText(QString::number(board->getGameData()->handicap));
}

void MainWindow::slotViewFileBar(bool toggle)
{
	if (!toggle)
		fileBar->hide();
	else
		fileBar->show();
	
	setting->writeBoolEntry("FILEBAR", toggle);
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewToolBar(bool toggle)
{

	if (!toggle)
		toolBar->hide();
	else
		toolBar->show();

	setting->writeBoolEntry("TOOLBAR", toggle);
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewEditBar(bool toggle)
{
	if (!toggle)
		editBar->hide();
	else
		editBar->show();

	setting->writeBoolEntry("EDITBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewMenuBar(bool toggle)
{
	menuBar()->setVisible (toggle);

	setting->writeBoolEntry("MENUBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewStatusBar(bool toggle)
{
	if (!toggle)
	{
		statusBar()->hide();
		// Disconnect this signal, if the statusbar is hidden, we dont need it
		//disconnect(board, SIGNAL(coordsChanged(int, int, int)), statusTip, SLOT(slotStatusTipCoords(int, int, int)));
	}
	else
	{
		statusBar()->show();
		// Connect the mouseMove event of the board with the status bar coords widget
		connect(board, SIGNAL(coordsChanged(int, int, int,bool)), statusTip, SLOT(slotStatusTipCoords(int, int, int,bool)));
	}
	
	setting->writeBoolEntry("STATUSBAR", toggle);
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewCoords(bool toggle)
{
	if (!toggle)
		board->setShowCoords(false);
	else
		board->setShowCoords(true);
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewSlider(bool toggle)
{
	if (!toggle)
		mainWidget->toggleSlider(false);
	else
		mainWidget->toggleSlider(true);
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewComment(bool toggle)
{
	setting->writeIntEntry("VIEW_COMMENT", toggle ? viewVertComment->isChecked() ? 2 : 1 : 0);
	if (!toggle)
	{
		commentEdit->hide();
		commentEdit2->hide();
		viewVertComment->setEnabled(false);

		ListView_observers->hide();

		setFocus();
	}
	else
	{
		commentEdit->show();
		commentEdit2->show();
		viewVertComment->setEnabled(true);

		ListView_observers->show();

		setFocus();
	}
	
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewVertComment(bool toggle)
{
	setting->writeIntEntry("VIEW_COMMENT", toggle ? 2 : 1);
	splitter->setOrientation(toggle ? Qt::Horizontal : Qt::Vertical);
	splitter->setResizeMode(mainWidget, QSplitter::KeepSize);
	splitter_comment->setOrientation(!toggle ? Qt::Horizontal : Qt::Vertical);
	splitter_comment->setResizeMode(ListView_observers, QSplitter::KeepSize);
}

// set sidbar left or right
void MainWindow::slotViewLeftSidebar()
{
	mainWidgetGuiLayout->remove(mainWidget->boardFrame);
	mainWidgetGuiLayout->remove(mainWidget->toolsFrame);

	if (setting->readBoolEntry("SIDEBAR_LEFT"))
	{
		mainWidgetGuiLayout->addWidget(mainWidget->toolsFrame, 0, 0);
		mainWidgetGuiLayout->addWidget(mainWidget->boardFrame, 0, 1);

	}
	else
	{
		mainWidgetGuiLayout->addWidget(mainWidget->toolsFrame, 0, 1);
		mainWidgetGuiLayout->addWidget(mainWidget->boardFrame, 0, 0);
	}
}

void MainWindow::slotViewSidebar(bool toggle)
{
	interfaceHandler->toggleSidebar(toggle);
	setting->writeBoolEntry("SIDEBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewSaveSize()
{
	reStoreWindowSize("0", true);
}

void MainWindow::slotViewFullscreen(bool toggle)
{
	if (!toggle)
		showNormal();
	else
		showFullScreen();

	isFullScreen = toggle;
}

void MainWindow::slotHelpManual()
{
	setting->qgo->openManual();
}

void MainWindow::slotHelpSoundInfo()
{
	// show info
	setting->qgo->testSound(true);
}

void MainWindow::slotHelpAbout()
{
	setting->qgo->slotHelpAbout();
}

void MainWindow::slotHelpAboutQt()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::slotToggleMarks()
{
	interfaceHandler->toggleMarks();
}

void MainWindow::slotTimerForward()
{
	static int eventCounter = 0;
	static int moveHasTimeInfo = 0;

	if (timer->isActive() && setting->readBoolEntry("SGF_TIME_TAGS") && board->getGameData()->timeSystem != time_none)
	{
		bool isBlacksTurn = board->getBoardHandler()->getBlackTurn();

		// decrease time info
		QString tmp;
		int seconds;

		if (isBlacksTurn)
			tmp = interfaceHandler->normalTools->pb_timeBlack->text();
		else
			tmp = interfaceHandler->normalTools->pb_timeWhite->text();

		int pos1 = 0;
		int pos2;
		seconds = 0;
		switch (tmp.count(":"))
		{
			case 2:
				// case: 0:23:56
				pos1 = tmp.indexOf(":") ;
				seconds += tmp.left(pos1).toInt()*3600;
				pos1++;
			case 1:
				pos2 = tmp.lastIndexOf(":", -1);
				seconds += tmp.mid(pos1, pos2-pos1).toInt()*60;
				seconds += tmp.mid(pos2+1, 2).toInt();
				break;
		}
		if (tmp.contains("-"))
			seconds = -seconds;
		seconds--;

		int openMoves = ((pos1 = tmp.indexOf("/")) != -1 ? tmp.right(tmp.length() - pos1 - 1).toInt() : -1);

		// set stones using sgf's time info
		Move *m = board->getBoardHandler()->getTree()->getCurrent();

		interfaceHandler->setTimes(isBlacksTurn, seconds, openMoves);

		if (m->getMoveNumber() == 0)
			board->nextMove(setting->readBoolEntry("SOUND_AUTOPLAY"));
		else if (m->son == 0)
		{
			timer->stop();
			navAutoplay->setChecked(false);
			statusBar()->showMessage(tr("Autoplay stopped."));
		}
		else if (!m->son->getTimeinfo())
		{
			// no time info at this node; use settings
			int time = int(timerIntervals[setting->readIntEntry("TIMER_INTERVAL")]);
			if ((time > 1 && (++eventCounter%time == 0) || time <= 1) &&
				!board->nextMove(setting->readBoolEntry("SOUND_AUTOPLAY")))
			{
				timer->stop();
				navAutoplay->setChecked(false);
				statusBar()->showMessage(tr("Autoplay stopped."));
			}

			// indicate move to have time Info
			moveHasTimeInfo = 2;
		}
		else if (m->son->getTimeLeft() >= seconds || moveHasTimeInfo > 0)
		{
			// check if byoyomi period changed
			if (board->getGameData()->timeSystem == canadian &&
				m->son->getOpenMoves() > openMoves+1 && moveHasTimeInfo == 0)
			{
				if (seconds > (m->son->getTimeLeft() - board->getGameData()->byoTime))
					return;
			}

			if (!board->nextMove(setting->readBoolEntry("SOUND_AUTOPLAY")))
			{
				timer->stop();
				navAutoplay->setChecked(false);
				statusBar()->showMessage(tr("Autoplay stopped."));
			}

			if (moveHasTimeInfo > 0)
				moveHasTimeInfo--;
		}
	}
	else if ((!board->nextMove(setting->readBoolEntry("SOUND_AUTOPLAY")) || !isActiveWindow())
		&& timer->isActive())
	{
		timer->stop();
		navAutoplay->setChecked(false);
		statusBar()->showMessage(tr("Autoplay stopped."));
	}
}

// store and restore window properties
bool MainWindow::reStoreWindowSize(QString strKey, bool store)
{
	if (store)
	{
		// store window size, format, comment format
		setting->writeIntEntry("BOARDVERTCOMMENT_" + strKey, !viewComment->isChecked() ? 2 : viewVertComment->isChecked());
		setting->writeBoolEntry("BOARDFULLSCREEN_" + strKey, isFullScreen);
		
		setting->writeEntry("BOARDWINDOW_" + strKey,
			QString::number(pos().x()) + DELIMITER +
			QString::number(pos().y()) + DELIMITER +
			QString::number(size().width()) + DELIMITER +
			QString::number(size().height()));
		
		setting->writeEntry("BOARDSPLITTER_" + strKey,
			QString::number(splitter->sizes().first()) + DELIMITER +
			QString::number(splitter->sizes().last()) + DELIMITER +
			QString::number(splitter_comment->sizes().first()) + DELIMITER +
			QString::number(splitter_comment->sizes().last()));
		
		statusBar()->showMessage(tr("Window size saved.") + " (" + strKey + ")");
	}
	else
	{
		// restore board window
		QString s = setting->readEntry("BOARDWINDOW_" + strKey);
		if (s.length() > 5)
		{
			// do not resize until end of this procedure
			board->lockResize = true;

			if (setting->readBoolEntry("BOARDFULLSCREEN_" + strKey))
				viewFullscreen->setChecked(true);
			else
			{
				viewFullscreen->setChecked(false);
				QPoint p;
				p.setX(s.section(DELIMITER, 0, 0).toInt());
				p.setY(s.section(DELIMITER, 1, 1).toInt());
				QSize sz;
				sz.setWidth(s.section(DELIMITER, 2, 2).toInt());
				sz.setHeight(s.section(DELIMITER, 3, 3).toInt());
				resize(sz);
				move(p);
			}

			if (setting->readIntEntry("BOARDVERTCOMMENT_" + strKey) == 2)
			{
				// do not view comment
				viewComment->setChecked(false);
			}
			else
			{
				// view comment
				viewComment->setChecked(true);

				viewVertComment->setChecked(setting->readIntEntry("VIEW_COMMENT") == 2 ||
					setting->readIntEntry("VIEW_COMMENT") == 0 && setting->readIntEntry("BOARDVERTCOMMENT_" + strKey));

				// restore splitter in board window
				s = setting->readEntry("BOARDSPLITTER_" + strKey);
				if (s.length() > 5)
				{
					int i, j;

					i = s.section(DELIMITER, 2, 2).toInt();
					j = s.section(DELIMITER, 3, 3).toInt();
					QList<int> w1;
					w1 << i << j;
					splitter_comment->setSizes(w1);

					w1.clear();
					i = s.section(DELIMITER, 0, 0).toInt();
					j = s.section(DELIMITER, 1, 1).toInt();
					if (i && j)
						w1 << i << j;
					splitter->setSizes(w1);
				}
			}

			// do some other stuff
			// maybe not correct set at startup time
			slotViewCoords(viewCoords->isChecked());

			// ok, resize
			board->lockResize = false;
			board->changeSize();

			statusBar()->showMessage(tr("Window size restored.") + " (" + strKey + ")");

			// update current move
			board->refreshDisplay();
		}
		else
			// window sizes not found
			return false;
	}

	return true;
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	// check for window resize command = number button
	if (e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9)
	{
		QString strKey = QString::number(e->key() - Qt::Key_0);
		
		if (e->state() & Qt::AltModifier)
		{
			// true -> store
			reStoreWindowSize(strKey, true);
			return;
		}
		else if (e->state() & Qt::ControlModifier)
		{
			// false -> restore
			if (!reStoreWindowSize(strKey, false))
			{
				// sizes not found -> leave
				e->ignore();
				return;
			}
			
			return;
		}
	}
	
	bool localGame = true;
	// don't view last moves while observing or playing
	if (getBoard()->getGameMode() == modeObserve ||
		getBoard()->getGameMode() == modeMatch ||
		getBoard()->getGameMode() == modeTeach)
		localGame = false;
	
	switch (e->key())
	{
/*
		// TODO: DEBUG
#ifndef NO_DEBUG
	case Key_W:
		board->debug();
		break;
		
	case Key_L:
		board->openSGF("foo.sgf");
		break;
		
	case Key_S:
		board->saveBoard("foo.sgf");
		break;
		
	case Key_X:
		board->openSGF("foo.xml", "XML");
		break;
		// /DEBUG
#endif
*/
		
	case Qt::Key_Left:
		if (localGame || (getBoard()->getGameMode() == modeObserve))
			slotNavBackward();
		break;
		
	case Qt::Key_Right:
		if (localGame || (getBoard()->getGameMode() == modeObserve))
			slotNavForward();
		break;
		
	case Qt::Key_Up:
		if (localGame)
			slotNavPrevVar();
		break;
		
	case Qt::Key_Down:
		if (localGame)
			slotNavNextVar();
		break;
		
	case Qt::Key_Home:
		if (localGame || (getBoard()->getGameMode() == modeObserve))
			slotNavFirst();
		break;
		
	case Qt::Key_End:
		if (localGame || (getBoard()->getGameMode() == modeObserve))
			slotNavLast();
		break;

	default:
		e->ignore();
	}

	e->accept();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
	// qDebug("MainWindow::closeEvent(QCloseEvent *e)");
	if (getBoard()->getGameMode() == modeObserve || checkModified() == 1)
	{
		emit signal_closeevent();
		//qGo::removeBoardWindow(this);
		if (parent_)
			QTimer::singleShot(1000, (QWidget*)parent_, SLOT(slot_closeevent()));
		else
			qWarning("*** BOARD CANNOT BE DELETED");
		e->accept();
	}
	else
		e->ignore();
}

int MainWindow::checkModified(bool interactive)
{
	if (!board->isModified)
		return 1;
	
	if (!interactive)
		return 0;
	
	switch (QMessageBox::warning(this, PACKAGE,
		tr("You modified the game.\nDo you want to save your changes?"),
		tr("Yes"), tr("No"), tr("Cancel"),
		0, 2))
	{
	case 0:
		return slotFileSave() && !board->isModified;
		
	case 1:
		return 1;
		
	case 2:
		return 2;
		
	default:
		qWarning("Unknown messagebox input.");
		return 0;
	}
	
	return 1;
}

void MainWindow::rememberLastDir(const QString &file)
{
	int pos = 0, lastpos = -1;
	
	while ((pos =  file.indexOf('/', pos)) != -1 && pos++ < static_cast<int>(file.length()))
		lastpos = pos;
	
	if (lastpos == -1)
	{
		setting->writeEntry("LAST_DIR", "");
	}
	else
		setting->writeEntry("LAST_DIR", file.left(lastpos));
	
	// qDebug("LAST DIR: %s", qGo::getSettings()->lastDir.latin1());
}

void MainWindow::updateFont()
{
	// editable fields
	setFont(setting->fontComments);

	// observer
	ListView_observers->setFont(setting->fontLists);

	// rest: standard font
	mainWidget->setFont(setting->fontStandard);
	mainWidget->normalTools->pb_timeWhite->setFont(setting->fontClocks);
	mainWidget->normalTools->pb_timeBlack->setFont(setting->fontClocks);
}

// used in slot_editBoardInNewWindow()
void MainWindow::slot_animateClick()
{
	getInterfaceHandler()->refreshButton->animateClick();
}

void MainWindow::slot_editBoardInNewWindow()
{
	// online mode -> don't score, open new Window instead
	MainWindow *w = setting->qgo->addBoardWindow();
	w->reStoreWindowSize("9", false);
	
	CHECK_PTR(w);
	w->setGameMode (modeNormal);

	// create update button
	w->getInterfaceHandler()->refreshButton->setText(tr("Update"));
	QToolTip::add(w->getInterfaceHandler()->refreshButton, tr("Update from online game"));
	QWhatsThis::add(w->getInterfaceHandler()->refreshButton, tr("Update from online game to local board and supersede own changes."));
	w->getInterfaceHandler()->refreshButton->setEnabled(true);
	connect(w->getInterfaceHandler()->refreshButton, SIGNAL(clicked()), this, SLOT(slotFileExportSgfClipB()));
	connect(w->getInterfaceHandler()->refreshButton, SIGNAL(clicked()), w, SLOT(slotFileImportSgfClipB()));
	connect(w->getInterfaceHandler()->refreshButton, SIGNAL(clicked()), w, SLOT(slotNavLastByTime()));
	QTimer::singleShot(100, w, SLOT(slot_animateClick()));
}

void MainWindow::updateObserverCnt()
{
	ListView_observers->setColumnText(0, tr("Observers") + " (" + QString::number(ListView_observers->childCount()) + ")");
	ListView_observers->setSorting(2);
	ListView_observers->sort();
	ListView_observers->setSorting(-1);
}

void MainWindow::addObserver(const QString &name)
{
	QString name_without_rank = name.section(' ', 0, 0);
	QString rank = name.section(' ', 1, 1);
	QString rankkey = rkToKey(rank) + name_without_rank;
	
	new Q3ListViewItem(ListView_observers, name_without_rank, rank, rankkey);
//	ListView_observer->addItem
}

void MainWindow::slotSoundToggle(bool toggle)
{
	board->getBoardHandler()->local_stone_sound = !toggle ;
}

void MainWindow::setGameMode(GameMode mode)
{
	mainWidget->setGameMode (mode);
	switch (mode)
	{
	case modeEdit:
//		modeButton->setEnabled(true);
		commentEdit->setReadOnly(false);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
		break;

	case modeNormal:
//		modeButton->setEnabled(true);
		commentEdit->setReadOnly(false);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("E", "Board status line: edit mode") + " ");
		break;

	case modeObserve:
//		modeButton->setDisabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
    		editDelete->setEnabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("O", "Board status line: observe mode") + " ");
		break;

	case modeMatch:
//		modeButton->setDisabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modeComputer:           // added eb 12
//		modeButton->setDisabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modeTeach:
//		modeButton->setDisabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("T", "Board status line: teach mode") + " ");
		break;

	case modeScore:
//		modeButton->setDisabled(true);
		commentEdit->setReadOnly(true);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("S", "Board status line: score mode") + " ");
		break;
	}
}

