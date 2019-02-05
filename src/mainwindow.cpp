/*
* mainwindow.cpp - qGo's main window
*/

#include "qgo.h"

#include <fstream>
#include <sstream>

//Added by qt3to4:
#include <QLabel>
#include <QPixmap>
#include <QCloseEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>
#include <QWhatsThis>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>
#include <QCheckBox>
#include <QSplitter>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QTimer>

#include "clientwin.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "board.h"
#include "gametree.h"
#include "sgf.h"
#include "setting.h"
#include "icons.h"
#include "komispinbox.h"
#include "config.h"
#include "qgo_interface.h"
#include "ui_helpers.h"

//#ifdef USE_XPM
#include ICON_PREFS
#include ICON_GAMEINFO
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
#include ICON_AUTOPLAY
#include ICON_DELETE
#include ICON_FULLSCREEN
#include ICON_MANUAL
#include ICON_NAV_INTERSECTION
#include ICON_COORDS
#include ICON_SOUND_ON
#include ICON_SOUND_OFF
//#endif

std::list<MainWindow *> main_window_list;

/* Return a string to identify the screen.  We use its dimensions.  */
QString screen_key ()
{
	QScreen *scr = QApplication::primaryScreen ();
	QSize sz = scr->size ();
	return QString::number (sz.width ()) + "x" + QString::number (sz.height ());
}

MainWindow::MainWindow(QWidget* parent, std::shared_ptr<game_record> gr, GameMode mode)
	: QMainWindow(parent), m_game (gr), m_ascii_dlg (this), m_svg_dlg (this)
{
	setProperty("icon", setting->image0);
	setAttribute (Qt::WA_DeleteOnClose);

	isFullScreen = 0;
	setFocusPolicy(Qt::StrongFocus);

	setWindowIcon (setting->image0);

	local_stone_sound = setting->readBoolEntry(mode == modeMatch ? "SOUND_MATCH_BOARD"
						   : mode == modeObserve ? "SOUND_OBSERVE"
						   : mode == modeComputer ? "SOUND_COMPUTER"
						   : "SOUND_NORMAL");

	int game_style = m_game->style ();
	m_sgf_var_style = false;
	if (game_style != -1) {
		int allow = setting->readIntEntry ("VAR_SGF_STYLE");
		int our_style = 0;
		if (setting->readIntEntry ("VAR_GHOSTS") == 0)
			our_style |= 2;
		if (!setting->readBoolEntry ("VAR_CHILDREN"))
			our_style |= 1;
		if (our_style == game_style && allow != 1) {
			/* Hard to say what the best behaviour would be.  For now, if
			   the style matched exactly but we're not unconditionally
			   allowing the SGF to override our settings, we choose to not
			   do anything here, leaving m_sgf_var_style false so that
			   settings changes take effect on this board.  */
		} if (allow == 2 && our_style != game_style) {
			QMessageBox mb(QMessageBox::Question, tr("Choose variation display"),
				       QString(tr("The SGF file that is being opened uses a different style\n"
						  "of variation display.  Use the style found in the file?\n\n"
						  "You can customize this behaviour (and disable this dialog)\n"
						  "in the preferences.")),
				       QMessageBox::Yes | QMessageBox::No);
			if (mb.exec() == QMessageBox::Yes)
				m_sgf_var_style = true;
		} else if (allow)
			m_sgf_var_style = true;
	}

	initActions();
	initMenuBar(mode);
	initToolBar();
	initStatusBar();

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

	if (viewVertComment->isChecked ()) {
		// show vertical comment
		splitter = new QSplitter(Qt::Horizontal, this);
		mainWidget = new MainWidget(this, splitter);
		splitter_comment = new QSplitter(Qt::Vertical, splitter);
	} else {
		splitter = new QSplitter(Qt::Vertical, this);
		mainWidget = new MainWidget(this, splitter);
		splitter_comment = new QSplitter(Qt::Horizontal, splitter);
	}
	splitter->setOpaqueResize(false);
	splitter_comment->setVisible (viewComment->isChecked ());

	mainWidgetGuiLayout = new QGridLayout(mainWidget);
#if 0
	mainWidget->removeWidget (mainWidget->toolsFrame);
	mainWidget->removeWidget (mainWidget->boardFrame);
#endif
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

	//commentEdit = new QTextEdit(splitter_comment, "comments");
	comments_widget = new QWidget (splitter_comment);
	comments_layout = new QVBoxLayout (comments_widget);
	comments_layout->setContentsMargins (0, 0, 0, 0);
	commentEdit = new QTextEdit;
	commentEdit->setWordWrapMode(QTextOption::WordWrap);
	comments_layout->addWidget (commentEdit);
	commentEdit2 = new QLineEdit;
	comments_layout->addWidget (commentEdit2);

	commentEdit->addAction (escapeFocus);
	commentEdit2->addAction (escapeFocus);

	ListView_observers = new QTreeView (splitter_comment);
	ListView_observers->setFocusPolicy (Qt::NoFocus);
	ListView_observers->setSortingEnabled (true);

	gameTreeView = new GameTree (splitter_comment);

	splitter->setStretchFactor (splitter->indexOf (mainWidget), 0);
	splitter_comment->setStretchFactor(splitter_comment->indexOf (ListView_observers), 0);
	splitter_comment->setStretchFactor(splitter_comment->indexOf (gameTreeView), 0);

	if (mode == modeMatch || mode == modeObserve || mode == modeTeach)
		gameTreeView->setParent (nullptr);
	else
		ListView_observers->setParent (nullptr);

	gfx_board = mainWidget->gfx_board;

	CHECK_PTR(board);

	connect(mainWidget->goLastButton, &QToolButton::clicked, [=] () { gfx_board->goto_last_move (); });
	connect(mainWidget->goFirstButton, &QToolButton::clicked, [=] () { gfx_board->goto_first_move (); });
	connect(mainWidget->goNextButton, &QToolButton::clicked, [=] () { gfx_board->next_move (); });
	connect(mainWidget->goPrevButton, &QToolButton::clicked, [=] () { gfx_board->previous_move (); });
	connect(mainWidget->prevNumberButton, &QToolButton::clicked, [=] () { gfx_board->previous_count (); });
	connect(mainWidget->nextNumberButton, &QToolButton::clicked, [=] () { gfx_board->next_count (); });
	connect(mainWidget->prevCommentButton, &QToolButton::clicked, [=] () { gfx_board->previous_comment (); });
	connect(mainWidget->nextCommentButton, &QToolButton::clicked, [=] () { gfx_board->next_comment (); });

	connect(mainWidget->normalTools->anStartButton, &QToolButton::clicked,
		[=] (bool on) { if (on) gfx_board->start_analysis (); else gfx_board->stop_analysis (); });
	connect(mainWidget->normalTools->anPauseButton, &QToolButton::clicked,
		[=] (bool on) { gfx_board->pause_analysis (on); });

	connect(m_ascii_dlg.cb_coords, &QCheckBox::toggled, this, &MainWindow::slotFileExportASCII);
	connect(m_ascii_dlg.cb_numbering, &QCheckBox::toggled, this, &MainWindow::slotFileExportASCII);
	connect(m_svg_dlg.cb_coords, &QCheckBox::toggled, this, &MainWindow::slotFileExportSVG);
	connect(m_svg_dlg.cb_numbering, &QCheckBox::toggled, this, &MainWindow::slotFileExportSVG);
	/* These don't do anything that toggling the checkboxes wouldn't also do, but it's slightly more
	   intuitive to have a button for it.  */
	connect(m_ascii_dlg.buttonRefresh, &QPushButton::clicked, this, &MainWindow::slotFileExportASCII);
	connect(m_svg_dlg.buttonRefresh, &QPushButton::clicked, this, &MainWindow::slotFileExportSVG);

	setCentralWidget(splitter);

	// Create a timer instance
	// timerInterval = 2;  // 1000 msec
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &MainWindow::slotTimerForward);
	timerIntervals[0] = (float) 0.1;
	timerIntervals[1] = 0.5;
	timerIntervals[2] = 1.0;
	timerIntervals[3] = 2.0;
	timerIntervals[4] = 3.0;
	timerIntervals[5] = 5.0;

	toolBar->setFocus();
	updateFont();

	init_game_record (gr);
	setGameMode (mode);

	splitter_comment->resize (100, 100);
	restoreWindowSize ();

	updateBoard();

	connect(commentEdit, SIGNAL(textChanged()), this, SLOT(slotUpdateComment()));
	connect(commentEdit2, SIGNAL(returnPressed()), this, SLOT(slotUpdateComment2()));
	m_allow_text_update_signal = true;

	update_analysis (analyzer::disconnected);
	main_window_list.push_back (this);
}

void MainWindow::init_game_record (std::shared_ptr<game_record> gr)
{
	m_svg_dlg.hide ();
	m_ascii_dlg.hide ();

	m_game = gr;
	mainWidget->init_game_record (gr);
	updateCaption (false);
	const go_board &b = gr->get_root ()->get_board ();
	bool disable_rect = (b.torus_h () || b.torus_v ()) && setting->readIntEntry ("TOROID_DUPS") > 0;
	editRectSelect->setEnabled (!disable_rect);
}

void MainWindow::update_game_record ()
{
	updateCaption (gfx_board->modified ());
}

MainWindow::~MainWindow()
{
	main_window_list.remove (this);

	delete timer;
	delete commentEdit;
	delete commentEdit2;
	delete mainWidget;
	delete ListView_observers;
	delete comments_layout;
	delete comments_widget;
	delete splitter_comment;
	delete splitter;

	// status bar
	delete statusMode;
	delete statusNav;
	delete statusTurn;
	delete statusCoords;

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
	delete fileNewBoard;
	delete fileNew;
	delete fileNewVariant;
	delete fileOpen;
	delete fileSave;
	delete fileSaveAs;
	delete fileClose;
	delete fileExportASCII;
	delete fileExportSVG;
	delete fileImportSgfClipB;
	delete fileExportSgfClipB;
	delete fileExportPic;
	delete fileExportPicClipB;
	delete fileQuit;
	delete editDelete;
	delete editStone;
	delete editCircle;
	delete editSquare;
	delete editTriangle;
	delete editCross;
	delete editNumber;
	delete editLetter;
	delete editGroup;
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
	delete helpAboutApp;
	delete helpAboutQt;
	delete whatsThis;
}

void MainWindow::initActions()
{
	// Load the pixmaps
	QPixmap fileNewboardIcon, fileNewIcon, fileOpenIcon, fileSaveIcon, fileSaveAsIcon,
		transformIcon, charIcon, deleteIcon,
		nextCommentIcon, previousCommentIcon, navIntersectionIcon,
		rightArrowIcon, leftArrowIcon, two_rightArrowIcon, two_leftArrowIcon, autoplayIcon,
		prefsIcon, infoIcon, fullscreenIcon, manualIcon, coordsIcon, sound_onIcon, sound_offIcon;

	prefsIcon = QPixmap((package_settings_xpm));
	infoIcon = QPixmap((idea_xpm));
	fileNewboardIcon = QPixmap((newboard_xpm));
	fileNewIcon = QPixmap((filenew_xpm));
	fileOpenIcon = QPixmap((fileopen_xpm));
	fileSaveIcon = QPixmap((filesave_xpm));
	fileSaveAsIcon = QPixmap((filesaveas_xpm));
	transformIcon = QPixmap((transform_xpm));
	charIcon = QPixmap((charset_xpm));
	deleteIcon = QPixmap((editdelete_xpm));
	rightArrowIcon = QPixmap((rightarrow_xpm));
	leftArrowIcon = QPixmap((leftarrow_xpm));
	nextCommentIcon = QPixmap((rightcomment_xpm));
	previousCommentIcon = QPixmap((leftcomment_xpm));
	two_rightArrowIcon = QPixmap((two_rightarrow_xpm));
	two_leftArrowIcon = QPixmap((two_leftarrow_xpm));
	fullscreenIcon = QPixmap((window_fullscreen_xpm));
	manualIcon = QPixmap((help_xpm));
	autoplayIcon = QPixmap((player_pause_xpm));
	navIntersectionIcon  = QPixmap((navIntersection_xpm));  //SL added eb 11
	coordsIcon = QPixmap((coords_xpm));
	sound_onIcon= QPixmap((sound_on_xpm));
	sound_offIcon= QPixmap((sound_off_xpm));

	/*
	* Global actions
	*/
	// Escape focus: Escape key to get the focus from comment field to main window.
 	escapeFocus = new QAction(this);
	escapeFocus->setShortcut(Qt::Key_Escape);
	connect(escapeFocus, &QAction::triggered, this, &MainWindow::slotFocus);

	/*
	* Menu File
	*/
	// File New Board
	fileNewBoard = new QAction(fileNewboardIcon, tr("New &Board"), this);
	fileNewBoard->setShortcut (Qt::CTRL + Qt::Key_B);
	fileNewBoard->setStatusTip(tr("Creates a new board"));
	fileNewBoard->setWhatsThis(tr("New\n\nCreates a new board."));
	connect(fileNewBoard, &QAction::triggered, this, &MainWindow::slotFileNewBoard);

	// File New Game
	fileNew = new QAction(fileNewIcon, tr("&New game"), this);
	fileNew->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_N));
	fileNew->setStatusTip(tr("Creates a new game on this board"));
	fileNew->setWhatsThis(tr("New\n\nCreates a new game on this board."));
	connect(fileNew, &QAction::triggered, this, &MainWindow::slotFileNewGame);

	// File New Variant Game
	fileNewVariant = new QAction(QIcon (":/ClientWindowGui/images/clientwindow/torus.png"),
				     tr("New &variant game"), this);
	fileNewVariant->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_V));
	fileNewVariant->setStatusTip(tr("Creates a new game on this board"));
	fileNewVariant->setWhatsThis(tr("New\n\nCreates a new variant game on this board."));
	connect(fileNewVariant, &QAction::triggered, this, &MainWindow::slotFileNewVariantGame);

	// File Open
	fileOpen = new QAction(fileOpenIcon, tr("&Open"), this);
	fileOpen->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_O));
	fileOpen->setStatusTip(tr("Open a sgf file"));
	fileOpen->setWhatsThis(tr("Open\n\nOpen a sgf file."));
	connect(fileOpen, &QAction::triggered, this, &MainWindow::slotFileOpen);

	// File Save
	fileSave = new QAction(fileSaveIcon, tr("&Save"), this);
	fileSave->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_S));
	fileSave->setStatusTip(tr("Save a sgf file"));
	fileSave->setWhatsThis(tr("Save\n\nSave a sgf file."));
	connect(fileSave, &QAction::triggered, this, &MainWindow::slotFileSave);

	// File SaveAs
	fileSaveAs = new QAction(fileSaveAsIcon, tr("Save &As"), this);
	fileSaveAs->setStatusTip(tr("Save a sgf file under a new name"));
	fileSaveAs->setWhatsThis(tr("Save As\n\nSave a sgf file under a new name."));
	connect(fileSaveAs, &QAction::triggered, this, &MainWindow::slotFileSaveAs);

	// File Close
	fileClose = new QAction(tr("&Close"), this);
	fileClose->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_W));
	fileClose->setStatusTip(tr("Close this board"));
	fileClose->setWhatsThis(tr("Exit\n\nClose this board."));
	connect(fileClose, &QAction::triggered, this, &MainWindow::slotFileClose);

	// File ExportASCII
	fileExportASCII = new QAction(charIcon, tr("&Export ASCII"), this);
	fileExportASCII->setStatusTip(tr("Export current board to ASCII"));
	fileExportASCII->setWhatsThis(tr("Export ASCII\n\nExport current board to ASCII."));
	connect(fileExportASCII, &QAction::triggered, this, &MainWindow::slotFileExportASCII);

	// File ExportSVG
	fileExportSVG = new QAction(charIcon, tr("Export S&VG"), this);
	fileExportSVG->setStatusTip(tr("Export current board to SVG"));
	fileExportSVG->setWhatsThis(tr("Export SVG\n\nExport current board to SVG."));
	connect(fileExportSVG, &QAction::triggered, this, &MainWindow::slotFileExportSVG);

	// File ImportSgfClipB
	fileImportSgfClipB = new QAction(fileOpenIcon, tr("Import SGF &from clipboard"), this);
	fileImportSgfClipB->setStatusTip(tr("Import a complete game in SGF format from clipboard"));
	fileImportSgfClipB->setWhatsThis(tr("Import SGF from clipboard\n\n"
		"Import a complete game in SGF format from clipboard."));
	connect(fileImportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileImportSgfClipB);

	// File ExportSgfClipB
	fileExportSgfClipB = new QAction(fileSaveIcon, tr("Export SGF &to clipboard"), this);
	fileExportSgfClipB->setStatusTip(tr("Export a complete game in SGF format to clipboard"));
	fileExportSgfClipB->setWhatsThis(tr("Export SGF to clipboard\n\n"
		"Export a complete game in SGF format to clipboard."));
	connect(fileExportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileExportSgfClipB);

	// File ExportPic
	fileExportPic = new QAction(transformIcon, tr("Export &Image"), this);
	fileExportPic->setStatusTip(tr("Export current board to an image"));
	fileExportPic->setWhatsThis(tr("Export Image\n\nExport current board to an image."));
	connect(fileExportPic, &QAction::triggered, this, &MainWindow::slotFileExportPic);

	// File ExportPic
	fileExportPicClipB = new QAction(transformIcon, tr("E&xport Image to clipboard"), this);
	fileExportPicClipB->setStatusTip(tr("Export current board to the clipboard as image"));
	fileExportPicClipB->setWhatsThis(tr("Export Image to clipboard\n\nExport current board to the clipboard as image."));
	connect(fileExportPicClipB, &QAction::triggered, this, &MainWindow::slotFileExportPicClipB);

	// File Quit
	fileQuit = new QAction(QIcon (":/images/exit.png"), tr("E&xit"), this);
	fileQuit->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_Q));
	fileQuit->setStatusTip(tr("Quits the application"));
	fileQuit->setWhatsThis(tr("Exit\n\nQuits the application."));
	connect(fileQuit, &QAction::triggered, this, &MainWindow::slotFileClose);//(qGo*)qApp, SLOT(quit);

	/*
	* Menu Edit
	*/

	// Edit delete
	editDelete = new QAction (deleteIcon, tr("&Delete"), this);
	editDelete->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_D));
	editDelete->setStatusTip(tr("Delete this and all following positions"));
	editDelete->setWhatsThis(tr("Delete\n\nDelete this and all following positions."));
	connect(editDelete, &QAction::triggered, this, &MainWindow::slotEditDelete);

	setGameInfo = new QAction(infoIcon, tr("&Game Info"), this);
	setGameInfo->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_I));
	setGameInfo->setStatusTip(tr("Display game information"));
	setGameInfo->setWhatsThis(tr("Game Info\n\nDisplay game information."));
	connect(setGameInfo, &QAction::triggered, this, &MainWindow::slotSetGameInfo);

	editStone = new QAction(QIcon (":/BoardWindow/images/boardwindow/editstone.png"), tr("Play stone"), this);
	editStone->setCheckable (true);
	editStone->setStatusTip(tr("Normal mode, click to place a stone."));
	editStone->setWhatsThis(tr("Select normal behaviour in play or edit mode, click to place a stone normally."));
	connect(editStone, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editTriangle = new QAction(QIcon (":/BoardWindow/images/boardwindow/edittriangle.png"), tr("Set triangle mark"), this);
	editTriangle->setCheckable (true);
	editTriangle->setStatusTip(tr("Click places a triangle mark."));
	editTriangle->setWhatsThis(tr("In play or edit mode, click to place a triangle mark."));
	connect(editTriangle, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editCircle = new QAction(QIcon (":/BoardWindow/images/boardwindow/editcircle.png"), tr("Set circle mark"), this);
	editCircle->setCheckable (true);
	editCircle->setStatusTip(tr("Click places a circle mark."));
	editCircle->setWhatsThis(tr("In play or edit mode, click to place a circle mark."));
	connect(editCircle, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editCross = new QAction(QIcon (":/BoardWindow/images/boardwindow/editcross.png"), tr("Set cross mark"), this);
	editCross->setCheckable (true);
	editCross->setStatusTip(tr("Click places a cross mark."));
	editCross->setWhatsThis(tr("In play or edit mode, click to place a cross mark."));
	connect(editCross, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editSquare = new QAction(QIcon (":/BoardWindow/images/boardwindow/editsquare.png"), tr("Set square mark"), this);
	editSquare->setCheckable (true);
	editSquare->setStatusTip(tr("Click places a square mark."));
	editSquare->setWhatsThis(tr("In play or edit mode, click to place a square mark."));
	connect(editSquare, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editNumber = new QAction(QIcon (":/BoardWindow/images/boardwindow/editnumber.png"), tr("Set number mark"), this);
	editNumber->setCheckable (true);
	editNumber->setStatusTip(tr("Click places a number mark."));
	editNumber->setWhatsThis(tr("In play or edit mode, click to place a number mark."));
	connect(editNumber, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editLetter = new QAction(QIcon (":/BoardWindow/images/boardwindow/editletter.png"), tr("Set letter mark"), this);
	editLetter->setCheckable (true);
	editLetter->setStatusTip(tr("Click places a letter mark."));
	editLetter->setWhatsThis(tr("In play or edit mode, click to place a letter mark."));
	connect(editLetter, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editGroup = new QActionGroup (this);
	editGroup->addAction (editStone);
	editGroup->addAction (editCircle);
	editGroup->addAction (editTriangle);
	editGroup->addAction (editSquare);
	editGroup->addAction (editCross);
	editGroup->addAction (editNumber);
	editGroup->addAction (editLetter);
	editStone->setChecked (true);

	edit123 = new QAction(QIcon (":/BoardWindow/images/boardwindow/123.png"), tr("Number moves from here"), this);
	edit123->setCheckable (true);
	edit123->setStatusTip(tr("Select to start numbering moves from this position."));
	edit123->setWhatsThis(tr("Causes the board to display move numbers starting at this position."));
	connect(edit123, &QAction::triggered, this, &MainWindow::slotEdit123);

	editRectSelect = new QAction(QIcon (":/BoardWindow/images/boardwindow/rect_select.png"), tr("Select rectangle"), this);
	editRectSelect->setCheckable (true);
	editRectSelect->setStatusTip(tr("Choose a rectangle to export."));
	editRectSelect->setWhatsThis(tr("In play or edit mode, click to begin choosing a rectangle to be used when exporting."));
	editRectSelect->setShortcut(Qt::Key_R);
	connect(editRectSelect, &QAction::toggled, this, &MainWindow::slotEditRectSelect);

	editClearSelect = new QAction(QIcon (":/BoardWindow/images/boardwindow/clear_select.png"), tr("Clear selection"), this);
	editClearSelect->setStatusTip(tr("Clear selection."));
	editClearSelect->setWhatsThis(tr("Click to clear the selected rectangle and select the whole board again."));
	connect(editClearSelect, &QAction::triggered, this, &MainWindow::slotEditClearSelect);

	/*
	* Menu Navigation
	*/
	navBackward = new QAction(leftArrowIcon, tr("&Previous move") + "\t" + tr("Left"), this);
	navBackward->setStatusTip(tr("To previous move"));
	navBackward->setWhatsThis(tr("Previous move\n\nMove one move backward."));
	connect (navBackward, &QAction::triggered, [=] () { gfx_board->previous_move (); });

	navForward = new QAction(rightArrowIcon, tr("&Next move") + "\t" + tr("Right"), this);
	navForward->setStatusTip(tr("To next move"));
	navForward->setWhatsThis(tr("Next move\n\nMove one move forward."));
	connect (navForward, &QAction::triggered, [=] () { gfx_board->next_move (); });

	navFirst = new QAction(two_leftArrowIcon, tr("&First move") + "\t" + tr("Home"), this);
	navFirst->setStatusTip(tr("To first move"));
	navFirst->setWhatsThis(tr("First move\n\nMove to first move."));
	connect (navFirst, &QAction::triggered, [=] () { gfx_board->goto_first_move (); });

	navLast = new QAction(two_rightArrowIcon, tr("&Last move") + "\t" + tr("End"), this);
	navLast->setStatusTip(tr("To last move"));
	navLast->setWhatsThis(tr("Last move\n\nMove to last move."));
	connect (navLast, &QAction::triggered, [=] () { gfx_board->goto_last_move (); });

	navPrevVar = new QAction(QIcon (":/BoardWindow/images/boardwindow/bluearrow-up.png"),
				 tr("P&revious variation") + "\t" + tr("Up"), this);
	navPrevVar->setStatusTip(tr("To previous variation"));
	navPrevVar->setWhatsThis(tr("Previous variation\n\nMove to the previous variation of this move."));
	connect (navPrevVar, &QAction::triggered, [=] () { gfx_board->previous_variation (); });

	navNextVar = new QAction(QIcon (":/BoardWindow/images/boardwindow/bluearrow-down.png"),
				 tr("N&ext variation") + "\t" + tr("Down"), this);
	navNextVar->setStatusTip(tr("To next variation"));
	navNextVar->setWhatsThis(tr("Next variation\n\nMove to the next variation of this move."));
	connect (navNextVar, &QAction::triggered, [=] () { gfx_board->next_variation (); });

	navMainBranch = new QAction(QIcon (":/BoardWindow/images/boardwindow/bluearrow-end-up.png"),
				    tr("&Main branch"), this);
	navMainBranch->setShortcut (Qt::Key_Insert);
	navMainBranch->setStatusTip(tr("To main branch"));
	navMainBranch->setWhatsThis(tr("Main Branch\n\nMove to the main branch where variation started."));
	connect (navMainBranch, &QAction::triggered, [=] () { gfx_board->goto_main_branch (); });

	navStartVar = new QAction(QIcon (":/BoardWindow/images/boardwindow/bluearrow-end-left.png"),
				  tr("Variation &start") + "\t" + tr("PgUp"), this);
	navStartVar->setShortcut (Qt::Key_PageUp);
	navStartVar->setStatusTip(tr("To top of variation"));
	navStartVar->setWhatsThis(tr("Variation start\n\nMove to the top variation of this branch."));
	connect (navStartVar, &QAction::triggered, [=] () { gfx_board->goto_var_start (); });

	navNextBranch = new QAction(QIcon (":/BoardWindow/images/boardwindow/bluearrow-end-right.png"),
				    tr("Next &branch") + "\t" + tr("PgDn"), this);
	navNextBranch->setShortcut (Qt::Key_PageDown);
	navNextBranch->setStatusTip(tr("To next branch starting a variation"));
	navNextBranch->setWhatsThis(tr("Next branch\n\nMove to the next branch starting a variation."));
	connect (navNextBranch, &QAction::triggered, [=] () { gfx_board->goto_next_branch (); });

	navNthMove = new QAction(tr("&Goto Move"), this);
	navNthMove->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_G));
	navNthMove->setStatusTip(tr("Goto a move of main branch by number"));
	navNthMove->setWhatsThis(tr("Goto move\n\nGoto a move of main branch by number."));
	connect(navNthMove, &QAction::triggered, this, &MainWindow::slotNavNthMove);

	navAutoplay = new QAction(autoplayIcon, tr("&Autoplay"), this);
	navAutoplay->setCheckable (true);
	navAutoplay->setShortcut (QKeySequence (Qt::CTRL + Qt::Key_A));
	navAutoplay->setChecked(false);
	navAutoplay->setStatusTip(tr("Start/Stop autoplaying current game"));
	navAutoplay->setWhatsThis(tr("Autoplay\n\nStart/Stop autoplaying current game."));
	connect(navAutoplay, &QAction::toggled, this, &MainWindow::slotNavAutoplay);

	navSwapVariations = new QAction(tr("S&wap variations"), this);
	navSwapVariations->setStatusTip(tr("Swap current move with previous variation"));
	navSwapVariations->setWhatsThis(tr("Swap variations\n\nSwap current move with previous variation."));
	connect(navSwapVariations, &QAction::triggered, this, &MainWindow::slotNavSwapVariations);

	navPrevComment = new QAction(previousCommentIcon, tr("Previous &commented move"), this);
	navPrevComment->setStatusTip(tr("To previous comment"));
	navPrevComment->setWhatsThis(tr("Previous comment\n\nMove to the previous move that has a comment"));
	connect (navPrevComment, &QAction::triggered, [=] () { gfx_board->previous_comment (); });

	navNextComment = new QAction(nextCommentIcon, tr("Next c&ommented move"), this);
	navNextComment->setStatusTip(tr("To next comment"));
	navNextComment->setWhatsThis(tr("Next comment\n\nMove to the next move that has a comment"));
	connect (navNextComment, &QAction::triggered, [=] () { gfx_board->next_comment (); });

	navIntersection = new QAction(navIntersectionIcon, tr("Goto clic&ked move"), this);
	navIntersection->setStatusTip(tr("To clicked move"));
	navIntersection->setWhatsThis(tr("Click on a board intersection\n\nMove to the stone played at this intersection (if any)"));
	connect (navIntersection, &QAction::triggered, this, &MainWindow::slotNavIntersection);

	/*
	* Menu Settings
	*/
	// Settings Preferences
	setPreferences = new QAction(prefsIcon, tr("&Preferences"), this);
	setPreferences->setShortcut (Qt::ALT + Qt::Key_P);
	setPreferences->setStatusTip(tr("Edit the preferences"));
	setPreferences->setWhatsThis(tr("Preferences\n\nEdit the applications preferences."));
	connect(setPreferences, &QAction::triggered, client_window, &ClientWindow::slot_preferences);

	//Toggling sound
	QIcon  OIC;
	OIC.addPixmap ( sound_offIcon, QIcon::Normal, QIcon::On);
	OIC.addPixmap ( sound_onIcon, QIcon::Normal, QIcon::Off );
	soundToggle = new QAction(OIC, tr("&Mute stones sound"), this);
	soundToggle->setCheckable (true);
	soundToggle->setChecked(!local_stone_sound);
	soundToggle->setStatusTip(tr("Toggle stones sound on/off"));
	soundToggle->setWhatsThis(tr("Stones sound\n\nToggle stones sound on/off\nthis toggles only the stones sounds"));
	connect(soundToggle, &QAction::toggled, this, &MainWindow::slotSoundToggle);

	/*
	* Menu View
	*/
	// View Filebar toggle
	viewFileBar = new QAction(tr("&File toolbar"), this);
	viewFileBar->setCheckable (true);
	viewFileBar->setChecked(true);
	viewFileBar->setStatusTip(tr("Enables/disables the file toolbar"));
	viewFileBar->setWhatsThis(tr("File toolbar\n\nEnables/disables the file toolbar."));
	connect(viewFileBar, &QAction::toggled, this, &MainWindow::slotViewFileBar);

	// View Toolbar toggle
	viewToolBar = new QAction(tr("Navigation &toolbar"), this);
	viewToolBar->setCheckable (true);
	viewToolBar->setChecked(true);
	viewToolBar->setStatusTip(tr("Enables/disables the navigation toolbar"));
	viewToolBar->setWhatsThis(tr("Navigation toolbar\n\nEnables/disables the navigation toolbar."));
	connect(viewToolBar, &QAction::toggled, this, &MainWindow::slotViewToolBar);

	// View Editbar toggle
	viewEditBar = new QAction(tr("&Edit toolbar"), this);
	viewEditBar->setCheckable (true);
	viewEditBar->setChecked(true);
	viewEditBar->setStatusTip(tr("Enables/disables the edit toolbar"));
	viewEditBar->setWhatsThis(tr("Edit toolbar\n\nEnables/disables the edit toolbar."));
	connect(viewEditBar, &QAction::toggled, this, &MainWindow::slotViewEditBar);

	viewMenuBar = new QAction(tr("&Menubar"), this);
#if 0 /* After porting from Q3Action, these no longer trigger when the menu is hidden, so it can't be unhidden.  */
	// View Menubar toggle
	viewMenuBar->setCheckable (true);
	viewMenuBar->setShortcut (Qt::Key_F7);
	viewMenuBar->setChecked(true);
	viewMenuBar->setStatusTip(tr("Enables/disables the menubar"));
	viewMenuBar->setWhatsThis(tr("Menubar\n\nEnables/disables the menubar."));
	connect(viewMenuBar, &QAction::toggled, this, &MainWindow::slotViewMenuBar);
#endif

	// View Statusbar toggle
	viewStatusBar = new QAction(tr("&Statusbar"), this);
	viewStatusBar->setCheckable (true);
	viewStatusBar->setChecked(true);
	viewStatusBar->setStatusTip(tr("Enables/disables the statusbar"));
	viewStatusBar->setWhatsThis(tr("Statusbar\n\nEnables/disables the statusbar."));
	connect(viewStatusBar, &QAction::toggled, this, &MainWindow::slotViewStatusBar);

	// View Coordinates toggle
	viewCoords = new QAction(coordsIcon, tr("C&oordinates"), this);
	viewCoords->setCheckable (true);
	viewCoords->setShortcut (Qt::Key_F8);
	viewCoords->setChecked(false);
	viewCoords->setStatusTip(tr("Enables/disables the coordinates"));
	viewCoords->setWhatsThis(tr("Coordinates\n\nEnables/disables the coordinates."));
	connect(viewCoords, &QAction::toggled, this, &MainWindow::slotViewCoords);

	// View Slider toggle
	viewSlider = new QAction(tr("Sli&der"), this);
	viewSlider->setCheckable (true);
	viewSlider->setShortcut (Qt::CTRL + Qt::Key_F8);
	viewSlider->setChecked(false);
	viewSlider->setStatusTip(tr("Enables/disables the slider"));
	viewSlider->setWhatsThis(tr("Slider\n\nEnables/disables the slider."));
	connect(viewSlider, &QAction::toggled, this, &MainWindow::slotViewSlider);

	// View Sidebar toggle
	viewSidebar = new QAction(tr("Side&bar"), this);
	viewSidebar->setCheckable (true);
	viewSidebar->setShortcut (Qt::Key_F9);
	viewSidebar->setChecked(true);
	viewSidebar->setStatusTip(tr("Enables/disables the sidebar"));
	viewSidebar->setWhatsThis(tr("Sidebar\n\nEnables/disables the sidebar."));
	connect(viewSidebar, &QAction::toggled, this, &MainWindow::slotViewSidebar);

	QString scrkey = screen_key ();
	/* Inverted meaning, so that if the option does not exist, we choose the correct default
	   (visible comment).  */
	int inv_view_comment = setting->readIntEntry("INV_VIEW_COMMENT_" + scrkey);
	int vertical = setting->readIntEntry("BOARDVERTCOMMENT_" + scrkey);

	// View Comment toggle
	viewComment = new QAction(tr("&Comment"), this);
	viewComment->setCheckable (true);
	viewComment->setShortcut (Qt::Key_F10);
	viewComment->setChecked (!inv_view_comment);
	viewComment->setStatusTip(tr("Enables/disables the comment field"));
	viewComment->setWhatsThis(tr("Comment field\n\nEnables/disables the comment field."));
	connect(viewComment, &QAction::toggled, this, &MainWindow::slotViewComment);

	// View Vertical Comment toggle

	viewVertComment = new QAction(tr("&Vertical comment"), this);
	viewVertComment->setCheckable (true);
	viewVertComment->setShortcut (Qt::SHIFT + Qt::Key_F10);
	viewVertComment->setChecked (vertical);
	viewVertComment->setStatusTip(tr("Enables/disables a vertical direction of the comment field"));
	viewVertComment->setWhatsThis(tr("Vertical comment field\n\n"
					 "Enables/disables a vertical direction of the comment field."));
	connect(viewVertComment, &QAction::toggled, this, &MainWindow::slotViewVertComment);

	// View Save Size
	viewSaveSize = new QAction(tr("Save si&ze"), this);
	viewSaveSize->setStatusTip(tr("Save the current window size"));
	viewSaveSize->setWhatsThis(tr("Save size\n\n"
				      "Saves the current window size and restores it on the next program start.\n"));
	connect(viewSaveSize, &QAction::triggered, this, [=] () { saveWindowSize (); });

	// View Fullscreen
	viewFullscreen = new QAction(fullscreenIcon, tr("&Fullscreen"), this);
	viewFullscreen->setCheckable (true);
	viewFullscreen->setShortcut (Qt::Key_F11);
	viewFullscreen->setChecked(false);
	viewFullscreen->setStatusTip(tr("Enable/disable fullscreen mode"));
	viewFullscreen->setWhatsThis(tr("Fullscreen\n\nEnable/disable fullscreen mode."));
	connect(viewFullscreen, &QAction::toggled, this, &MainWindow::slotViewFullscreen);

	/* Analyze menu.  */

	anConnect = new QAction(tr("&Connect analysis engine"), this);
	anConnect->setStatusTip(tr("Start up a configured analysis engine"));
	anConnect->setWhatsThis(tr("Try to find an engine configured as an analysis tool in the engine list,\n"
				      "and connect to it.\n"));
	connect(anConnect, &QAction::triggered, this, [=] () { gfx_board->start_analysis (); });

	anPause = new QAction(tr("&Pause analysis engine"), this);
	anPause->setCheckable (true);
	anPause->setStatusTip(tr("Stop the analysis temporarily"));
	anPause->setWhatsThis(tr("Click to pause or unpause the analysis engine.\n"));
	connect(anPause, &QAction::toggled, this, [=] (bool on) { if (on) { mainWidget->grey_eval_bar (); } gfx_board->pause_analysis (on); });

	anDisconnect = new QAction(tr("&Disconnect analysis engine"), this);
	anDisconnect->setStatusTip(tr("Detach the running analysis engine"));
	anDisconnect->setWhatsThis(tr("Detach the currently running analysis engine.\n"));
	connect(anDisconnect, &QAction::triggered, this, [=] () { gfx_board->stop_analysis (); });

	/*
	 * Menu Help
	 */
	// Help Manual
	helpManual = new QAction(manualIcon, tr("&Manual"), this);
	helpManual->setShortcut (Qt::Key_F1);
	helpManual->setStatusTip(tr("Opens the manual"));
	helpManual->setWhatsThis(tr("Help\n\nOpens the manual of the application."));
	connect(helpManual, &QAction::triggered, this, [=] (bool) { qgo->openManual (); });

	// Help About
	helpAboutApp = new QAction(tr("&About..."), this);
	helpAboutApp->setStatusTip(tr("About the application"));
	helpAboutApp->setWhatsThis(tr("About\n\nAbout the application."));
	connect(helpAboutApp, &QAction::triggered, [=] (bool) { help_about (); });

	// Help AboutQt
	helpAboutQt = new QAction(tr("About &Qt..."), this);
	helpAboutQt->setStatusTip(tr("About Qt"));
	helpAboutQt->setWhatsThis(tr("About Qt\n\nAbout Qt."));
	connect(helpAboutQt, &QAction::triggered, [=] (bool) { QMessageBox::aboutQt (this); });

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
	navIntersection->setEnabled(false);

	whatsThis = QWhatsThis::createAction (this);
}

void MainWindow::initMenuBar(GameMode mode)
{
	// submenu Import/Export
	importExportMenu = new QMenu(tr("&Import/Export"));
	importExportMenu->addAction (fileExportASCII);
	importExportMenu->addAction (fileExportSVG);
	importExportMenu->addAction (fileImportSgfClipB);
	importExportMenu->addAction (fileExportSgfClipB);
	importExportMenu->addAction (fileExportPic);
	importExportMenu->addAction (fileExportPicClipB);

	importExportMenu->insertSeparator(fileImportSgfClipB);
	importExportMenu->insertSeparator(fileExportPic);

	// menuBar entry fileMenu
	fileMenu = new QMenu(tr("&File"));
	fileMenu->addAction (fileNewBoard);
	fileMenu->addAction (fileNew);
	fileMenu->addAction (fileNewVariant);
	fileMenu->addAction (fileOpen);
	fileMenu->addAction (fileSave);
	fileMenu->addAction (fileSaveAs);
	fileMenu->addAction (fileClose);
	fileMenu->addSeparator ();
	fileMenu->addMenu (importExportMenu);
	fileMenu->addSeparator ();
	fileMenu->addAction (fileQuit);

	// menuBar entry editMenu
	editMenu = new QMenu(tr("&Edit"));
	editMenu->addAction (setGameInfo);

	editMenu->addSeparator ();

	editMenu->addAction (editDelete);

	editMenu->addSeparator ();

	editMenu->addAction (editStone);
	editMenu->addAction (editTriangle);
	editMenu->addAction (editSquare);
	editMenu->addAction (editCircle);
	editMenu->addAction (editCross);
	editMenu->addAction (editNumber);
	editMenu->addAction (editLetter);

	editMenu->addSeparator ();

	editMenu->addAction (editRectSelect);
	editMenu->addAction (editClearSelect);

	// menuBar entry navMenu
	navMenu = new QMenu(tr("&Navigation"));
	navMenu->addAction (navFirst);
	navMenu->addAction (navBackward);
	navMenu->addAction (navForward);
	navMenu->addAction (navLast);

	navMenu->addSeparator ();
	navMenu->addAction (navMainBranch);
	navMenu->addAction (navPrevVar);
	navMenu->addAction (navNextVar);
	navMenu->addAction (navStartVar);
	navMenu->addAction (navNextBranch);

	navMenu->addSeparator ();
	navMenu->addAction (navNthMove);
	navMenu->addAction (navPrevComment);
	navMenu->addAction (navNextComment);		// end add

	// menuBar entry settingsMenu
	settingsMenu = new QMenu(tr("&Settings"));
	settingsMenu->addAction (setPreferences);
	settingsMenu->addAction (soundToggle);

	settingsMenu->insertSeparator(soundToggle);

	// menuBar entry viewMenu
	viewMenu = new QMenu(tr("&View"));
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

	anMenu = new QMenu (tr ("&Analysis"));
	anMenu->addAction (anConnect);
	anMenu->addAction (anDisconnect);
	anMenu->addAction (anPause);

	// menuBar entry helpMenu
	helpMenu = new QMenu(tr("&Help"));
	helpMenu->addAction (helpManual);
	helpMenu->addAction (whatsThis);
	helpMenu->addAction (helpAboutApp);
	helpMenu->addAction (helpAboutQt);
	helpMenu->insertSeparator(helpAboutApp);

	// menubar configuration
	menuBar()->addMenu(fileMenu);
	menuBar()->addMenu(editMenu);
	menuBar()->addMenu(navMenu);
	menuBar()->addMenu(settingsMenu);
	menuBar()->addMenu(viewMenu);
	if (mode == modeNormal || mode == modeObserve)
		menuBar()->addMenu(anMenu);
	menuBar()->addMenu(helpMenu);
}

void MainWindow::initToolBar()
{
	// File toolbar
	fileBar = addToolBar ("filebar");

	fileBar->addAction (fileNew);
	// fileBar->addAction (fileNewVariant);
	fileBar->addAction (fileOpen);
	fileBar->addAction (fileSave);
	fileBar->addAction (fileSaveAs);

	// Navigation toolbar
	toolBar = addToolBar ("toolbar");

	toolBar->addAction (navMainBranch);
	toolBar->addAction (navPrevVar);
	toolBar->addAction (navNextVar);
	toolBar->addAction (navStartVar);
	toolBar->addAction (navNextBranch);

	toolBar->addSeparator();
	toolBar->addAction (navIntersection);
	toolBar->addSeparator();

	toolBar->addSeparator();

	toolBar->addAction (soundToggle);
	toolBar->addAction (viewCoords);

	toolBar->addSeparator();

	toolBar->addAction (whatsThis);
	toolBar->addSeparator();

	// Edit toolbar
	editBar = addToolBar ("editbar");

	editBar->addAction (setGameInfo);

	editBar->addSeparator ();
	editBar->addAction (editDelete);
	editBar->addAction (editStone);
	editBar->addAction (editTriangle);
	editBar->addAction (editSquare);
	editBar->addAction (editCircle);
	editBar->addAction (editCross);
	editBar->addAction (editNumber);
	editBar->addAction (editLetter);
	editBar->addSeparator ();
	editBar->addAction (edit123);
	editBar->addSeparator ();
	editBar->addAction (editRectSelect);
	editBar->addAction (editClearSelect);
}

void MainWindow::initStatusBar()
{
	// The coords widget
	statusCoords = new QLabel (statusBar());
	statusBar()->addWidget(statusCoords);
	//statusBar()->show();
	statusBar()->setSizeGripEnabled(true);
	statusBar()->showMessage(tr("Ready."));  // Normal indicator

	// The turn widget
	statusTurn = new QLabel(statusBar());
	statusTurn->setAlignment(Qt::AlignCenter);
	statusTurn->setText(" 0 ");
	statusBar()->addPermanentWidget(statusTurn);
	statusTurn->setToolTip (tr("Current move"));
	statusTurn->setWhatsThis (tr("Move\nDisplays the number of the current turn and the last move played."));

	// The nav widget
	statusNav = new QLabel(statusBar());
	statusNav->setAlignment(Qt::AlignCenter);
	statusNav->setText(" 0/0 ");
	statusBar()->addPermanentWidget(statusNav);
	statusNav->setToolTip (tr("Brothers / sons"));
	statusNav->setWhatsThis (tr("Navigation\nShows the brothers and sons of the current move."));

	// The mode widget
	statusMode = new QLabel(statusBar());
	statusMode->setAlignment(Qt::AlignCenter);
	statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
	statusBar()->addPermanentWidget(statusMode);
	statusMode->setToolTip (tr("Current mode"));
	statusMode->setWhatsThis (tr("Mode\nShows the current mode. 'N' for normal mode, 'E' for edit mode."));
}

void MainWindow::updateCaption (bool modified)
{
	// Print caption
	// example: qGo 0.0.5 - Zotan 8k vs. tgmouse 10k
	// or if game name is given: qGo 0.0.5 - Kogo's Joseki Dictionary
	int game_number = 0;
	QString s;
	if (modified)
		s += "* ";
	if (game_number != 0)
		s += "(" + QString::number(game_number) + ") ";
	QString title = QString::fromStdString (m_game->title ());
	QString player_w = QString::fromStdString (m_game->name_white ());
	QString player_b = QString::fromStdString (m_game->name_black ());
	QString rank_w = QString::fromStdString (m_game->rank_white ());
	QString rank_b = QString::fromStdString (m_game->rank_black ());

	if (title.length () > 0) {
		s += title;
	} else {
		s += player_w;
		if (rank_w.length () > 0)
			s += " " + rank_w;
		s += " " + tr ("vs.") + " ";
		s += player_b;
		if (rank_b.length () > 0)
			s += " " + rank_b;
	}
	s += "   " + QString (PACKAGE " " VERSION);
	setWindowTitle (s);

	rank_w.truncate (5);
	int rwlen = rank_w.length ();
	player_w.truncate(15 - rwlen);
	if (rwlen > 0)
		player_w += " " + rank_w;
	mainWidget->normalTools->whiteLabel->setText(player_w);
	mainWidget->scoreTools->whiteLabel->setText(player_w);

	rank_b.truncate (5);
	int rblen = rank_b.length ();
	player_b.truncate(15 - rblen);
	if (rblen > 0)
		player_b += " " + rank_b;
	mainWidget->normalTools->blackLabel->setText(player_b);
	mainWidget->scoreTools->blackLabel->setText(player_b);
}

void MainWindow::update_game_tree (game_state *active)
{
	gameTreeView->update (m_game, active);
}

void MainWindow::slotFileNewBoard (bool)
{
	open_local_board (client_window, game_dialog_type::none);
}

void MainWindow::slotFileNewGame (bool)
{
	if (!checkModified())
		return;

	std::shared_ptr<game_record> gr = new_game_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);

	statusBar()->showMessage(tr("New board prepared."));
}

void MainWindow::slotFileNewVariantGame (bool)
{
	if (!checkModified())
		return;

	std::shared_ptr<game_record> gr = new_variant_game_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);

	statusBar()->showMessage(tr("New board prepared."));
}

void MainWindow::slotFileOpen (bool)
{
	if (!checkModified())
		return;
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Open SGF file"),
						      setting->readEntry("LAST_DIR"),
						      tr("SGF Files (*.sgf *.SGF);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)")));
	if (fileName.isEmpty())
		return;

	if (setting->readBoolEntry("REM_DIR"))
		rememberLastDir(fileName);

	QByteArray qba = fileName.toUtf8();
	if (doOpen(qba.constData ()))
	  	statusBar()->showMessage(fileName + " " + tr("loaded."));
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
		filter = fileName.mid(oldpos+1, fileName.length()-pos).toUpper();

	return filter;
}

bool MainWindow::doOpen(const char *fileName)
{
	std::ifstream isgf (fileName);
	std::shared_ptr<game_record> gr = record_from_stream (isgf);
	if (gr == nullptr)
		/* Assume alerts were shown in record_from_stream.  */
		return false;

	init_game_record (gr);
	setGameMode (modeNormal);

	return true;
}

bool MainWindow::slotFileSave (bool)
{
	QString fileName = QString::fromStdString (m_game->filename ());
	if (fileName.isEmpty())
	{
		if (setting->readBoolEntry("REM_DIR"))
			fileName = setting->readEntry("LAST_DIR");
		else
			fileName = QString::null;
		return doSave(fileName, false);
	}
	else
		return doSave(fileName, true);
}

bool MainWindow::slotFileSaveAs (bool)
{
	std::string saved_name = m_game->filename ();
	QString fileName = saved_name == "" ? QString () : QString::fromStdString (saved_name);
	return doSave (fileName, false);
}

bool MainWindow::doSave(QString fileName, bool force)
{
	if (fileName.isNull () || fileName.isEmpty () || !force)
  	{
		if (QDir(fileName).exists())
			fileName = QString::fromStdString (get_candidate_filename (fileName.toStdString (), *m_game));
		else if (fileName.isNull() || fileName.isEmpty()) {
			std::string dir = "";
			if (setting->readBoolEntry("REM_DIR"))
				dir = setting->readEntry("LAST_DIR").toStdString ();

			fileName = QString::fromStdString (get_candidate_filename (dir, *m_game));
		}
		if (!force)
			fileName = QFileDialog::getSaveFileName(this, tr ("Save SGF file"),
								fileName, tr("SGF Files (*.sgf);;All Files (*)"));
	}

	if (fileName.isEmpty())
		return false;

	if (getFileExtension(fileName, false).isEmpty())
		fileName.append(".sgf");

	std::string sfn = fileName.toStdString ();
	try {
		std::ofstream of (sfn);
		std::string sgf = m_game->to_sgf ();
		of << sgf;
		m_game->set_filename (fileName.toStdString ());
		if (setting->readBoolEntry("REM_DIR"))
			rememberLastDir(fileName);
	} catch (...) {
		QMessageBox::warning(this, PACKAGE, tr("Cannot save SGF file."));
		return false;
	}

	statusBar()->showMessage (fileName + " " + tr("saved."));
	gfx_board->setModified (false);
	return true;
}

void MainWindow::slotFileClose (bool)
{
	if (checkModified() == 1)
	{
		gfx_board->setModified(false);  // Prevent to ask a second time in qGo::quit()
		close();
	}
}

void MainWindow::slotFileImportSgfClipB(bool)
{
	if (!checkModified ())
		return;

        QString sgfString = QApplication::clipboard()->text();
	std::string sgf_str = sgfString.toStdString ();
	std::stringstream isgf (sgf_str);
	std::shared_ptr<game_record> gr = record_from_stream (isgf);
	if (gr == nullptr)
		/* Assume alerts were shown in record_from_stream.  */
		return;

	init_game_record (gr);
	setGameMode (modeNormal);

	statusBar()->showMessage(tr("SGF imported."));
}

void MainWindow::slotFileExportSgfClipB(bool)
{
	std::string sgf = m_game->to_sgf ();
	QApplication::clipboard()->setText(QString::fromStdString (sgf));
	statusBar()->showMessage(tr("SGF exported."));
}

void MainWindow::slotFileExportASCII(bool)
{
	m_ascii_dlg.show ();
	QString s = gfx_board->render_ascii (m_ascii_dlg.cb_numbering->isChecked (),
					     m_ascii_dlg.cb_coords->isChecked ());
	m_ascii_dlg.textEdit->setText (s);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotFileExportSVG(bool)
{
	m_svg_dlg.show ();
	QByteArray s = gfx_board->render_svg (m_svg_dlg.cb_numbering->isChecked (),
					      m_svg_dlg.cb_coords->isChecked ());
	m_svg_dlg.set (s);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotFileExportPic(bool)
{
	QString filter;
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export image as"), setting->readEntry("LAST_DIR"),
							"PNG (*.png);;BMP (*.bmp);;XPM (*.xpm);;XBM (*.xbm);;PNM (*.pnm);;GIF (*.gif);;JPG (*.jpg);;MNG (*.mng)",
							&filter);


	if (fileName.isEmpty())
		return;

	filter.truncate (3);
	qDebug () << "filter: " << filter << "\n";
	QPixmap pm = gfx_board->grabPicture ();
	if (!pm.save (fileName, filter.toLatin1 ()))
		QMessageBox::warning (this, PACKAGE, tr("Failed to save image!"));
}

void MainWindow::slotFileExportPicClipB(bool)
{
	QApplication::clipboard()->setPixmap (gfx_board->grabPicture ());
}

void MainWindow::slotEditDelete(bool)
{
	gfx_board->deleteNode();
}

void MainWindow::slotEditRectSelect(bool on)
{
	qDebug () << "rectSelect " << on << "\n";
	gfx_board->set_rect_select (on);
}

void MainWindow::slotEditClearSelect(bool)
{
	editRectSelect->setChecked (false);
	gfx_board->clear_selection ();
}

void MainWindow::done_rect_select (int, int, int, int)
{
	editRectSelect->setChecked (false);
}

void MainWindow::slotEditGroup (bool)
{
	mark m = mark::none;
	if (editTriangle->isChecked ())
		m = mark::triangle;
	else if (editSquare->isChecked ())
		m = mark::square;
	else if (editCircle->isChecked ())
		m = mark::circle;
	else if (editCross->isChecked ())
		m = mark::cross;
	else if (editNumber->isChecked ())
		m = mark::num;
	else if (editLetter->isChecked ())
		m = mark::letter;
	gfx_board->setMarkType (m);
}

void MainWindow::slotEdit123 (bool on)
{
	gfx_board->set_start_count (on);
}

void MainWindow::slotNavIntersection(bool)
{
	gfx_board->navIntersection();
}

void MainWindow::slotNavNthMove(bool)
{
#if 0
	NthMoveDialog dlg(this);
	dlg.moveSpinBox->setValue(gfx_board->getCurrentMoveNumber());
	dlg.moveSpinBox->setFocus();

	if (dlg.exec() == QDialog::Accepted)
		gfx_board->gotoNthMove(dlg.moveSpinBox->value());
#endif
}

void MainWindow::slotNavAutoplay(bool toggle)
{
#if 0
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
		if (setting->readBoolEntry("SGF_TIME_TAGS") && gfx_board->getGameData()->timeSystem != time_none)
			// set time to 1 sec
			timer->start(1000);
		else
			// set time interval as selected
			timer->start(int(timerIntervals[setting->readIntEntry("TIMER_INTERVAL")] * 1000));
		statusBar()->showMessage(tr("Autoplay started."));
	}
#endif
}

void MainWindow::slotNavSwapVariations(bool)
{
#if 0
	if (gfx_board->swapVariations())
		statusBar()->showMessage(tr("Variations swapped."));
	else
		statusBar()->showMessage(tr("No previous variation available."));
#endif
}

void MainWindow::updateBoard()
{
	viewSlider->setChecked (setting->readBoolEntry ("SLIDER"));
	viewSidebar->setChecked (setting->readBoolEntry ("SIDEBAR"));
	viewCoords->setChecked (setting->readBoolEntry ("BOARD_COORDS"));
	gfx_board->set_sgf_coords (setting->readBoolEntry ("SGF_BOARD_COORDS"));
	gfx_board->set_antiClicko (setting->readBoolEntry ("ANTICLICKO"));

	int ghosts = setting->readIntEntry ("VAR_GHOSTS");
	bool children = setting->readBoolEntry ("VAR_CHILDREN");
	int style = m_game->style ();

	/* Should never have the first condition and not the second, but it doesn't
	   hurt to be careful.  */
	if (m_sgf_var_style && style != -1) {
		children = (style & 1) == 0;
		if (style & 2)
			ghosts = 0;
		/* The SGF file says nothing about how to render the markup.  Letters
		   are somewhat implied, but maybe it's better to leave that final
		   choice to the user.  */
		else if (ghosts == 0)
			ghosts = 2;
	}
	gfx_board->set_vardisplay (children, ghosts);

	QString scrkey = screen_key ();
	int inv_view_comment = setting->readIntEntry("INV_VIEW_COMMENT_" + scrkey);
	bool vertical = setting->readIntEntry("BOARDVERTCOMMENT_" + scrkey);
	viewComment->setChecked(!inv_view_comment);
	viewVertComment->setChecked (vertical);

#if 0 // @@@
	QToolTip::setEnabled(setting->readBoolEntry("TOOLTIPS"));
	if (timer->isActive())
	{
		if (setting->readBoolEntry("SGF_TIME_TAGS") && gfx_board->getGameData()->timeSystem != time_none)
			timer->changeInterval(1000);
		else
			timer->changeInterval(int(timerIntervals[setting->readIntEntry("TIMER_INVERVAL")] * 1000));
	}
#endif

	gfx_board->update_prefs ();
	slotViewLeftSidebar();

	const go_board &b = m_game->get_root ()->get_board ();
	bool disable_rect = (b.torus_h () || b.torus_v ()) && setting->readIntEntry ("TOROID_DUPS") > 0;
	editRectSelect->setEnabled (!disable_rect);
}

void MainWindow::slotSetGameInfo(bool)
{
	GameInfoDialog dlg(this);

	dlg.gameNameEdit->setText(QString::fromStdString (m_game->title ()));
	dlg.playerWhiteEdit->setText(QString::fromStdString (m_game->name_white ()));
	dlg.playerBlackEdit->setText(QString::fromStdString (m_game->name_black ()));
	dlg.whiteRankEdit->setText(QString::fromStdString (m_game->rank_white ()));
	dlg.blackRankEdit->setText(QString::fromStdString (m_game->rank_black ()));
	dlg.resultEdit->setText(QString::fromStdString (m_game->result ()));
	dlg.dateEdit->setText(QString::fromStdString (m_game->date ()));
	dlg.placeEdit->setText(QString::fromStdString (m_game->place ()));
	dlg.copyrightEdit->setText(QString::fromStdString (m_game->copyright ()));
 	dlg.komiSpin->setValue(m_game->komi ());
	dlg.handicapSpin->setValue(m_game->handicap ());

	if (dlg.exec() == QDialog::Accepted)
	{
		m_game->set_name_black (dlg.playerBlackEdit->text().toStdString ());
		m_game->set_name_white (dlg.playerWhiteEdit->text().toStdString ());
		m_game->set_rank_black (dlg.blackRankEdit->text().toStdString ());
		m_game->set_rank_white (dlg.whiteRankEdit->text().toStdString ());
		m_game->set_komi (dlg.komiSpin->value());
		m_game->set_handicap (dlg.handicapSpin->value());
		m_game->set_title (dlg.gameNameEdit->text().toStdString ());
		m_game->set_result (dlg.resultEdit->text().toStdString ());
		m_game->set_date (dlg.dateEdit->text().toStdString ());
		m_game->set_place (dlg.placeEdit->text().toStdString ());
		m_game->set_copyright (dlg.copyrightEdit->text().toStdString ());

		/* The board may call it if it wasn't modified yet, but otherwise
		   we must call it ourselves to update the information.  */
		updateCaption (true);
		mainWidget->update_game_record (m_game);
		gfx_board->setModified (true);
	}
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
	statusBar()->setVisible (toggle);
	setting->writeBoolEntry("STATUSBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewCoords(bool toggle)
{
	if (!toggle)
		gfx_board->set_show_coords(false);
	else
		gfx_board->set_show_coords(true);

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
	setting->writeIntEntry("INV_VIEW_COMMENT_" + screen_key (), toggle ? 0 : 1);
	splitter_comment->setVisible (toggle);
	setFocus();

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewVertComment(bool toggle)
{
	setting->writeIntEntry("BOARDVERTCOMMENT_" + screen_key (), toggle ? 1 : 0);
	splitter->setOrientation(toggle ? Qt::Horizontal : Qt::Vertical);
	splitter->setStretchFactor(0, 0);
	splitter_comment->setOrientation(!toggle ? Qt::Horizontal : Qt::Vertical);
	if (ListView_observers->parent () != nullptr)
		splitter_comment->setStretchFactor(splitter_comment->indexOf (ListView_observers), 0);
	if (gameTreeView->parent () != nullptr)
		splitter_comment->setStretchFactor(splitter_comment->indexOf (gameTreeView), 0);
}

// set sidbar left or right
void MainWindow::slotViewLeftSidebar()
{
	mainWidgetGuiLayout->removeWidget(mainWidget->boardFrame);
	mainWidgetGuiLayout->removeWidget(mainWidget->toolsFrame);

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
	mainWidget->toggleSidebar(toggle);
	setting->writeBoolEntry("SIDEBAR", toggle);

	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewFullscreen(bool toggle)
{
	if (!toggle)
		showNormal();
	else
		showFullScreen();

	isFullScreen = toggle;
}

void MainWindow::slotTimerForward()
{
#if 0
	static int eventCounter = 0;
	static int moveHasTimeInfo = 0;
	if (timer->isActive() && setting->readBoolEntry("SGF_TIME_TAGS") && gfx_board->getGameData()->timeSystem != time_none)
	{
		bool isBlacksTurn = gfx_board->to_move () == black;

		// decrease time info
		QString tmp;
		int seconds;

		if (isBlacksTurn)
			tmp = mainWidget->normalTools->pb_timeBlack->text();
		else
			tmp = mainWidget->normalTools->pb_timeWhite->text();

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

#if 0 /* @@@ */
		// set stones using sgf's time info
		Move *m = gfx_board->getBoardHandler()->getTree()->getCurrent();

		interfaceHandler->setTimes(isBlacksTurn, seconds, openMoves);

		if (m->getMoveNumber() == 0) {
			gfx_board->nextMove();
			if (setting->readBoolEntry("SOUND_AUTOPLAY"))
				setting->qgo->playAutoPlayClick();
		} else if (m->son == 0)
		{
			timer->stop();
			navAutoplay->setChecked(false);
			statusBar()->showMessage(tr("Autoplay stopped."));
		}
		else if (!m->son->getTimeinfo())
		{
			// no time info at this node; use settings
			int time = int(timerIntervals[setting->readIntEntry("TIMER_INTERVAL")]);
			if (time > 1 && (++eventCounter%time == 0) || time <= 1) {
				gfx_board->nextMove();
				if (setting->readBoolEntry("SOUND_AUTOPLAY"))
					setting->qgo->playAutoPlayClick();
				/* @@@ if end reached.  */
				{
					timer->stop();
					navAutoplay->setChecked(false);
					statusBar()->showMessage(tr("Autoplay stopped."));
				}
			}

			// indicate move to have time Info
			moveHasTimeInfo = 2;
		}
		else if (m->son->getTimeLeft() >= seconds || moveHasTimeInfo > 0)
		{
			// check if byoyomi period changed
			if (gfx_board->getGameData()->timeSystem == canadian &&
				m->son->getOpenMoves() > openMoves+1 && moveHasTimeInfo == 0)
			{
				if (seconds > (m->son->getTimeLeft() - gfx_board->getGameData()->byoTime))
					return;
			}
			gfx_board->nextMove();
			if (setting->readBoolEntry("SOUND_AUTOPLAY"))
				setting->qgo->playAutoPlayClick();

			/* @@@ if end reached.  */
			{
				timer->stop();
				navAutoplay->setChecked(false);
				statusBar()->showMessage(tr("Autoplay stopped."));
			}

			if (moveHasTimeInfo > 0)
				moveHasTimeInfo--;
		}
#endif
	}
	else if (!isActiveWindow() && timer->isActive())
	{
		timer->stop();
		navAutoplay->setChecked(false);
		statusBar()->showMessage(tr("Autoplay stopped."));
	} else {
		/* Autoplay here? @@@ */
		gfx_board->nextMove();
		if (setting->readBoolEntry("SOUND_AUTOPLAY"))
			setting->qgo->playAutoPlayClick();
	}
#endif
}

void MainWindow::coords_changed (const QString &t1, const QString &t2)
{
	statusBar ()->clearMessage ();
	statusCoords->setText (" " + t1 + " " + t2 + " ");
}

void MainWindow::saveWindowSize()
{
	QString strKey = screen_key ();

	// store window size, format, comment format
	setting->writeBoolEntry("BOARDFULLSCREEN_" + strKey, isFullScreen);

	setting->writeEntry("BOARDWINDOW_" + strKey,
			    QString::number(pos().x()) + DELIMITER +
			    QString::number(pos().y()) + DELIMITER +
			    QString::number(size().width()) + DELIMITER +
			    QString::number(size().height()));

	if (viewComment->isChecked ()) {
		QString key = "BOARDSPLITTER_";
		if (gameTreeView->parent () != nullptr)
			key = "BOARDSPLITTER_GT_";
		setting->writeEntry(key + strKey,
				    QString::number(splitter->sizes().first()) + DELIMITER +
				    QString::number(splitter->sizes().last()) + DELIMITER +
				    QString::number(splitter_comment->sizes().first()) + DELIMITER +
				    QString::number(splitter_comment->sizes().last()));
	}
	statusBar()->showMessage(tr("Window size saved.") + " (" + strKey + ")");
}

bool MainWindow::restoreWindowSize ()
{
	QString strKey = screen_key ();

	// restore board window
	QString s = setting->readEntry("BOARDWINDOW_" + strKey);
	if (s.length() <= 5)
		return false;

	// do not resize until end of this procedure
	gfx_board->lockResize = true;

	if (setting->readBoolEntry("BOARDFULLSCREEN_" + strKey))
		viewFullscreen->setChecked(true);
	else {
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

	if (viewComment->isChecked ()) {
		// restore splitter in board window
		QString split1 = setting->readEntry("BOARDSPLITTER_" + strKey);
		QString split2 = setting->readEntry("BOARDSPLITTER_GT_" + strKey);
		if (gameTreeView->parent () == nullptr)
			s = split1;
		else
			s = split2;
		if (s.length() > 5) {
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
	slotViewCoords (viewCoords->isChecked ());

	// ok, resize
	gfx_board->lockResize = false;
	gfx_board->changeSize();

	statusBar()->showMessage(tr("Window size restored.") + " (" + strKey + ")");

	// update current move
#if 0 /* @@@ */
	gfx_board->refreshDisplay();
#endif

	return true;
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key())
	{
/*
		// TODO: DEBUG
#ifndef NO_DEBUG
	case Key_W:
		gfx_board->debug();
		break;
		
	case Key_L:
		gfx_board->openSGF("foo.sgf");
		break;
		
	case Key_S:
		gfx_board->saveBoard("foo.sgf");
		break;
		
	case Key_X:
		gfx_board->openSGF("foo.xml", "XML");
		break;
		// /DEBUG
#endif
*/
	case Qt::Key_Left:
		if (navBackward->isEnabled ())
			navBackward->trigger ();
		break;

	case Qt::Key_Right:
		if (navForward->isEnabled ())
			navForward->trigger ();
		break;

	case Qt::Key_Up:
		if (navPrevVar->isEnabled ())
			navPrevVar->trigger ();
		break;

	case Qt::Key_Down:
		if (navNextVar->isEnabled ())
			navNextVar->trigger ();
		break;

	case Qt::Key_Home:
		if (navFirst->isEnabled ())
			navFirst->trigger ();
		break;

	case Qt::Key_End:
		if (navLast->isEnabled ())
			navLast->trigger ();
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
		e->accept();
	}
	else
		e->ignore();
}

int MainWindow::checkModified(bool interactive)
{
	if (!gfx_board->modified ())
		return 1;

	if (!interactive)
		return 0;

	switch (QMessageBox::warning(this, PACKAGE,
		tr("You modified the game.\nDo you want to save your changes?"),
		tr("Yes"), tr("No"), tr("Cancel"),
		0, 2))
	{
	case 0:
		return slotFileSave() && !gfx_board->modified ();

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

void MainWindow::slotUpdateComment()
{
	if (!m_allow_text_update_signal)
		return;
	gfx_board->update_comment (commentEdit->toPlainText());
}

/* Called from an external source to append to the comments window.  */
void MainWindow::append_comment(const QString &t)
{
	bool old = m_allow_text_update_signal;
	m_allow_text_update_signal = false;
	commentEdit->append (t);
	m_allow_text_update_signal = old;
}

void MainWindow::slotUpdateComment2()
{
	QString text = commentEdit2->text();

	// clear entry
	commentEdit2->setText("");

	// don't show short text
	if (text.length() < 1)
		return;

	// emit signal to opponent in online game
	emit signal_sendcomment (text);
}

void MainWindow::updateFont()
{
	// editable fields
	setFont(setting->fontComments);

	// observer
	ListView_observers->setFont(setting->fontLists);

	// rest: standard font
	mainWidget->updateFont ();
	gameTreeView->update_item_size ();
}

// used in slot_editBoardInNewWindow()
void MainWindow::slot_animateClick(bool)
{
#if 0
	getInterfaceHandler()->refreshButton->animateClick();
#endif
}

void MainWindow::slot_editBoardInNewWindow(bool)
{
	std::shared_ptr<game_record> newgr = std::make_shared<game_record> (*m_game);
	// online mode -> don't score, open new Window instead
	MainWindow *w = new MainWindow (0, newgr);

#if 0 /* @@@ This has never worked.  */
	// create update button
	w->mainWidget->refreshButton->setText(tr("Update"));
#if 0
	QToolTip::add(w->getInterfaceHandler()->refreshButton, tr("Update from online game"));
	QWhatsThis::add(w->getInterfaceHandler()->refreshButton, tr("Update from online game to local board and supersede own changes."));
#endif
	w->mainWidget->refreshButton->setEnabled(true);
	connect(w->mainWidget->refreshButton, SIGNAL(clicked()), this, SLOT(slotFileExportSgfClipB()));
	connect(w->mainWidget->refreshButton, SIGNAL(clicked()), w, SLOT(slotFileImportSgfClipB()));
	connect(w->mainWidget->refreshButton, SIGNAL(clicked()), w, SLOT(slotNavLastByTime()));
	QTimer::singleShot(100, w, SLOT(slot_animateClick()));
#endif

	w->navLast->trigger ();
	w->show ();
}

void MainWindow::slotSoundToggle(bool toggle)
{
	local_stone_sound = !toggle;
}

void MainWindow::setGameMode(GameMode mode)
{
	if (mode == modeEdit || mode == modeNormal || mode == modeObserve) {
		editGroup->setEnabled (true);
	} else {
		editStone->setChecked (true);
		gfx_board->setMarkType (mark::none);
		editGroup->setEnabled (false);
	}

	mainWidget->normalTools->anGroup->setVisible (mode == modeNormal || mode == modeObserve);
	mainWidget->normalTools->anStartButton->setVisible (mode == modeNormal || mode == modeObserve);
	mainWidget->normalTools->anPauseButton->setVisible (mode == modeNormal || mode == modeObserve);

	bool enable_nav = mode == modeNormal; /* @@@ teach perhaps? */

	navPrevVar->setEnabled (enable_nav);
	navNextVar->setEnabled (enable_nav);
	navBackward->setEnabled (enable_nav);
	navForward->setEnabled (enable_nav);
	navFirst->setEnabled (enable_nav);
	navStartVar->setEnabled (enable_nav);
	navMainBranch->setEnabled (enable_nav);
	navLast->setEnabled (enable_nav);
	navNextBranch->setEnabled (enable_nav);
	navPrevComment->setEnabled (enable_nav);
	navNextComment->setEnabled (enable_nav);
	navIntersection->setEnabled (enable_nav);
	navNthMove->setEnabled (enable_nav);
	editDelete->setEnabled (enable_nav);
	navSwapVariations->setEnabled (enable_nav);

	fileImportSgfClipB->setEnabled (enable_nav);

	bool editable_comments = mode == modeNormal || mode == modeEdit || mode == modeScore || mode == modeComputer;
	commentEdit->setReadOnly (!editable_comments || mode == modeEdit);
	// commentEdit->setDisabled (editable_comments);
	commentEdit2->setEnabled (!editable_comments);
	commentEdit2->setVisible (!editable_comments);

	if (mode == modeMatch || mode == modeObserve || mode == modeTeach) {
		if (gameTreeView->parent () != nullptr) {
			splitter_comment->addWidget (ListView_observers);
			gameTreeView->setParent (nullptr);
		}
	} else {
		if (ListView_observers->parent () != nullptr) {
			ListView_observers->setParent (nullptr);
			splitter_comment->addWidget (gameTreeView);
		}
	}

	fileNew->setEnabled (mode == modeNormal || mode == modeEdit);
	fileNewVariant->setEnabled (mode == modeNormal || mode == modeEdit);
	fileNewBoard->setEnabled (mode == modeNormal || mode == modeEdit);
	fileOpen->setEnabled (mode == modeNormal || mode == modeEdit);

	switch (mode)
	{
	case modeEdit:
		statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
		break;

	case modeNormal:
		statusMode->setText(" " + QObject::tr("E", "Board status line: edit mode") + " ");
		break;

	case modeObserve:
		statusMode->setText(" " + QObject::tr("O", "Board status line: observe mode") + " ");
		break;

	case modeMatch:
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modeComputer:
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modeTeach:
		statusMode->setText(" " + QObject::tr("T", "Board status line: teach mode") + " ");
		break;

	case modeScore:
		commentEdit->setReadOnly(true);
		commentEdit2->setDisabled(true);
		/* fall through */
	case modeScoreRemote:
		statusMode->setText(" " + QObject::tr("S", "Board status line: score mode") + " ");
		break;
	}
	mainWidget->setGameMode (mode);
}

void MainWindow::setMoveData (game_state &gs, const go_board &b, GameMode mode)
{
	int sons = gs.n_children ();

	bool is_root_node = gs.root_node_p ();

	switch (mode)
	{
	case modeNormal:
		navPrevVar->setEnabled(gs.has_prev_sibling ());
		navNextVar->setEnabled(gs.has_next_sibling ());
		navStartVar->setEnabled(!is_root_node);
		navMainBranch->setEnabled(!is_root_node);
		navNextBranch->setEnabled(sons > 0);
		navSwapVariations->setEnabled(!is_root_node);

		/* fall through */
	case modeObserve:
		navBackward->setEnabled(!is_root_node);
		navForward->setEnabled(sons > 0);
		navFirst->setEnabled(!is_root_node);
		navLast->setEnabled(sons > 0);
		navPrevComment->setEnabled(!is_root_node);
		navNextComment->setEnabled(sons > 0);
		navIntersection->setEnabled(true);
		break;

	case modeScore:
	case modeEdit:
		navPrevVar->setEnabled(false);
		navNextVar->setEnabled(false);
		navBackward->setEnabled(false);
		navForward->setEnabled(false);
		navFirst->setEnabled(false);
		navStartVar->setEnabled(false);
		navMainBranch->setEnabled(false);
		navLast->setEnabled(false);
		navNextBranch->setEnabled(false);
		navSwapVariations->setEnabled(false);
		navPrevComment->setEnabled(false);
		navNextComment->setEnabled(false);
		navIntersection->setEnabled(false);
		break;
	default:
		break;
	}

	/* Refresh comment from move unless we are in a game mode that just keeps
	   appending to the comment.  */
	if (!commentEdit->isReadOnly ()) {
		bool old = m_allow_text_update_signal;
		m_allow_text_update_signal = false;
		std::string c = gs.comment ();
		if (c.size () == 0)
			commentEdit->clear();
		else
			commentEdit->setText(QString::fromStdString (c));
		m_allow_text_update_signal = old;
	}

	edit123->setChecked (gs.get_start_count ());
	mainWidget->setMoveData (gs, b, mode);
}

void MainWindow::player_move (stone_color, int, int)
{
	if (local_stone_sound)
		qgo->playStoneSound ();
}

void MainWindow::doPass()
{
	gfx_board->doPass();
	emit signal_pass();
}

void MainWindow::doCountDone()
{
	if (gfx_board->getGameMode () == modeScoreRemote) {
		emit signal_done ();
		return;
	}

	float komi = m_game->komi ();
	int capW = mainWidget->scoreTools->capturesWhite->text().toInt();
	int capB = mainWidget->scoreTools->capturesBlack->text().toInt();
	int terrW = mainWidget->scoreTools->terrWhite->text().toInt();
	int terrB = mainWidget->scoreTools->terrBlack->text().toInt();

	float totalWhite = capW + terrW + komi;
	int totalBlack = capB + terrB;
	float result = 0;
	QString rs;

	QString s;
	QTextStream (&s) << tr("White") << "\n" << terrW << " + " << capW << " + " << komi << " = " << totalWhite << "\n";
	QTextStream (&s) << tr("Black") << "\n" << terrB << " + " << capB << " = " << totalBlack << "\n\n";

	if (totalBlack > totalWhite) {
		result = totalBlack - totalWhite;
		s.append(tr("Black wins with %1").arg(result));
		rs = "B+" + QString::number(result);
	} else if (totalWhite > totalBlack) {
		result = totalWhite - totalBlack;
		s.append(tr("White wins with %1").arg(result));
		rs = "W+" + QString::number(result);
	} else {
		rs = tr("Jigo");
		s.append(rs);
	}

	std::string old_result = m_game->result ();
	std::string new_result = rs.toStdString ();
	if (old_result != "" && old_result != new_result) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning(this, PACKAGE,
					      tr("Game result differs from the one stored.\nOverwrite stored game result?"),
					      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (choice == QMessageBox::Yes)
			m_game->set_result (rs.toStdString ());
	}
	//if (QMessageBox::information(this, PACKAGE " - " + tr("Game Over"), s, tr("Ok"), tr("Update gameinfo")) == 1)

	gfx_board->doCountDone();
	mainWidget->doRealScore (false);
}

void MainWindow::doResign()
{
	emit signal_resign();
}

void MainWindow::doUndo()
{
	emit signal_undo();
}

void MainWindow::doAdjourn()
{
	emit signal_adjourn();
}

void MainWindow::set_observer_model (QStandardItemModel *m)
{
	ListView_observers->setModel (m);
	ListView_observers->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ListView_observers->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

MainWindow_GTP::MainWindow_GTP (QWidget *parent, std::shared_ptr<game_record> gr, const Engine &program,
				bool b_is_comp, bool w_is_comp)
	: MainWindow (parent, gr, modeComputer), Gtp_Controller (this)
{
	gfx_board->set_player_colors (!w_is_comp, !b_is_comp);
	m_gtp = create_gtp (program, m_game->boardsize (), m_game->komi (), m_game->handicap ());
}

MainWindow_GTP::~MainWindow_GTP ()
{
	delete m_gtp;
}

void MainWindow_GTP::gtp_startup_success ()
{
	show ();
	if (!gfx_board->player_to_move_p ())
		m_gtp->request_move (gfx_board->to_move ());
}

void MainWindow_GTP::gtp_played_move (int x, int y)
{
	if (local_stone_sound)
		qgo->playStoneSound();
	gfx_board->play_external_move (x, y);
}

void MainWindow_GTP::gtp_played_pass ()
{
	if (local_stone_sound)
		qgo->playPassSound();
	const game_state *st = gfx_board->get_state ();
	gfx_board->play_external_pass ();
	if (st->was_pass_p ()) {
		m_gtp->quit ();
		/* As of now, we're disconnected, and the scoring function should
		   remember normal mode.  */
		setGameMode (modeNormal);
		mainWidget->doRealScore (true);
	}
}

void MainWindow_GTP::gtp_played_resign ()
{
	if (gfx_board->player_is (black)) {
		m_game->set_result ("B+R");
	} else {
		m_game->set_result ("W+R");
	}

	QMessageBox mb(QMessageBox::Information, tr("Game end"),
		       QString(tr("The computer has resigned the game.")),
		       QMessageBox::Ok | QMessageBox::Default);
	mb.exec ();

	setGameMode (modeNormal);
	m_gtp->quit();
	gfx_board->set_player_colors (true, true);
}

void MainWindow_GTP::gtp_failure (const QString &err)
{
	if (gfx_board->getGameMode () != modeComputer)
		return;
	show ();
	setGameMode (modeNormal);
	gfx_board->set_player_colors (true, true);
	QMessageBox msg(QString (QObject::tr("Error")), err,
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default,
			Qt::NoButton, Qt::NoButton);
	msg.activateWindow();
	msg.raise();
	msg.exec();
}

void MainWindow_GTP::gtp_exited ()
{
	if (gfx_board->getGameMode () == modeComputer) {
		setGameMode (modeNormal);
		gfx_board->set_player_colors (true, true);
		show ();
		QMessageBox::warning (this, PACKAGE, QObject::tr ("GTP process exited unexpectedly."));
	}
}

void MainWindow_GTP::player_move (stone_color col, int x, int y)
{
	MainWindow::player_move (col, x, y);

	if (gfx_board->getGameMode () != modeComputer)
		return;
	m_gtp->played_move (col, x, y);
	m_gtp->request_move (col == black ? white : black);
}


void MainWindow_GTP::doPass ()
{
	if (gfx_board->getGameMode () != modeComputer) {
		MainWindow::doPass ();
		return;
	}
	if (!gfx_board->player_to_move_p ())
		return;
	const game_state *st = gfx_board->get_state ();
	stone_color col = gfx_board->to_move ();
	MainWindow::doPass ();
	m_gtp->played_move_pass (col);
	if (st->was_pass_p ()) {
		m_gtp->quit ();
		/* As of now, we're disconnected, and the scoring function should
		   remember normal mode.  */
		setGameMode (modeNormal);
		mainWidget->doRealScore (true);
	} else
		m_gtp->request_move (col == black ? white : black);
}

void MainWindow_GTP::doResign ()
{
	MainWindow::doResign();
	setGameMode (modeNormal);
	gfx_board->set_player_colors (true, true);
	m_gtp->quit();
	if (gfx_board->player_is (black))
		m_game->set_result ("W+R");
	else
		m_game->set_result ("B+R");
}

void MainWindow::update_analysis (analyzer state)
{
	mainWidget->evalView->setVisible (state == analyzer::running || state == analyzer::paused);
	anConnect->setEnabled (state == analyzer::disconnected);
	anConnect->setChecked (state != analyzer::disconnected);
	anDisconnect->setEnabled (state != analyzer::disconnected);
	mainWidget->normalTools->anStartButton->setChecked (state != analyzer::disconnected);
	if (state == analyzer::disconnected)
		mainWidget->normalTools->anStartButton->setIcon (QIcon (":/images/exit.png"));
	else if (state == analyzer::starting)
		mainWidget->normalTools->anStartButton->setIcon (QIcon (":/images/power-standby.png"));
	else
		mainWidget->normalTools->anStartButton->setIcon (QIcon (":/images/power-on.png"));
	mainWidget->normalTools->anPrimaryBox->setEnabled (state == analyzer::running);
	mainWidget->normalTools->anShownBox->setEnabled (state == analyzer::running);
	anPause->setEnabled (state != analyzer::disconnected);
	mainWidget->normalTools->anPauseButton->setEnabled (state != analyzer::disconnected);
	mainWidget->normalTools->anPauseButton->setChecked (state == analyzer::paused);
	anPause->setChecked (state == analyzer::paused);

	if (state == analyzer::disconnected) {
		mainWidget->normalTools->primaryCoords->setText ("");
		mainWidget->normalTools->primaryWR->setText ("");
		mainWidget->normalTools->primaryVisits->setText ("");
		mainWidget->normalTools->shownCoords->setText ("");
		mainWidget->normalTools->shownWR->setText ("");
		mainWidget->normalTools->shownVisits->setText ("");
	}
}
