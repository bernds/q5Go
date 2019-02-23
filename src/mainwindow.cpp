/*
* mainwindow.cpp - qGo's main window
*/

#include "qgo.h"

#include <fstream>
#include <sstream>

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
#include <QMessageBox>
#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QTimer>

#include "clientwin.h"
#include "mainwindow.h"
#include "board.h"
#include "gametree.h"
#include "sgf.h"
#include "setting.h"
#include "komispinbox.h"
#include "config.h"
#include "qgo_interface.h"
#include "autodiagsdlg.h"
#include "ui_helpers.h"

std::list<MainWindow *> main_window_list;

/* Return a string to identify the screen.  We use its dimensions.  */
QString screen_key (QWidget *w)
{
	QScreen *scr = nullptr;
	QWindow *win = w->windowHandle ();
	if (win != nullptr)
		scr = win->screen ();
	if (scr == nullptr)
		scr = QApplication::primaryScreen ();
	QSize sz = scr->size ();
	return QString::number (sz.width ()) + "x" + QString::number (sz.height ());
}

MainWindow::MainWindow(QWidget* parent, std::shared_ptr<game_record> gr, GameMode mode)
	: QMainWindow(parent), m_game (gr), m_ascii_dlg (this), m_svg_dlg (this)
{
	setupUi (this);

	gameTreeView->set_board_win (this);
	evalGraph->set_board_win (this);
	gfx_board->set_board_win (this);
	diagView->set_board_win (this);

	/* This needs to be set early, before calling setGameMode.  It is used in two places:
	   when setting the window caption through init_game_record, and when restoring the
	   initial default layout (the results of that aren't used, but we want to avoid
	   uninitialized reads).  */
	m_gamemode = mode;

	setProperty("icon", setting->image0);
	setAttribute (Qt::WA_DeleteOnClose);
	setDockNestingEnabled (true);

	isFullScreen = 0;
	setFocusPolicy(Qt::StrongFocus);

	setWindowIcon (setting->image0);

	if (!gr->filename ().empty ()) {
		QFileInfo fi (QString::fromStdString (gr->filename ()));
		setting->writeEntry ("LAST_DIR", fi.dir ().absolutePath ());
	}

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

	initActions ();
	initMenuBar(mode);
	initToolBar();
	initStatusBar();

	viewStatusBar->setChecked (setting->readBoolEntry("STATUSBAR"));
	viewMenuBar->setChecked (setting->readBoolEntry("MENUBAR"));

	if (setting->readBoolEntry("SIDEBAR_LEFT"))
		slotViewLeftSidebar ();

	commentEdit->setWordWrapMode(QTextOption::WordWrap);
	diagCommentView->setWordWrapMode(QTextOption::WordWrap);
	diagCommentView->setVisible (false);

	commentEdit->addAction (escapeFocus);
	commentEdit2->addAction (escapeFocus);

	// gameTreeView = new GameTree (this, treeDockContents);

	normalTools->show();
	scoreTools->hide();

	showSlider = true;
	toggleSlider(setting->readBoolEntry("SLIDER"));
	slider->setMaximum(SLIDER_INIT);
	sliderRightLabel->setText(QString::number(SLIDER_INIT));
	sliderSignalToggle = true;

	int w = evalView->width ();
	int h = evalView->height ();

	m_eval_canvas = new QGraphicsScene (0, 0, w, h, evalView);
	evalView->setScene (m_eval_canvas);
	m_eval_bar = m_eval_canvas->addRect (QRectF (0, 0, w, h / 2), Qt::NoPen, QBrush (Qt::black));

	m_eval = 0.5;
	connect (evalView, &SizeGraphicsView::resized, this, [=] () { set_eval (m_eval); });

	connect(passButton, &QPushButton::clicked, this, &MainWindow::doPass);
        connect(undoButton, &QPushButton::clicked, this, &MainWindow::doUndo);
        connect(adjournButton, &QPushButton::clicked, this, &MainWindow::doAdjourn);
        connect(resignButton, &QPushButton::clicked, this, &MainWindow::doResign);
        connect(doneButton, &QPushButton::clicked, this, &MainWindow::doCountDone);

	goLastButton->setDefaultAction (navLast);
	goFirstButton->setDefaultAction (navFirst);
	goNextButton->setDefaultAction (navForward);
	goPrevButton->setDefaultAction (navBackward);
	prevNumberButton->setDefaultAction (navPrevFigure);
	nextNumberButton->setDefaultAction (navNextFigure);
	prevCommentButton->setDefaultAction (navPrevComment);
	nextCommentButton->setDefaultAction (navNextComment);

	connect(diagEditButton, &QToolButton::clicked, this, &MainWindow::slotDiagEdit);
	connect(diagASCIIButton, &QToolButton::clicked, this, &MainWindow::slotDiagASCII);
	connect(diagSVGButton, &QToolButton::clicked, this, &MainWindow::slotDiagSVG);
	void (QComboBox::*cact) (int) = &QComboBox::activated;
	connect(diagComboBox, cact, this, &MainWindow::slotDiagChosen);

	connect(normalTools->anStartButton, &QToolButton::clicked,
		[=] (bool on) { if (on) gfx_board->start_analysis (); else gfx_board->stop_analysis (); });
	connect(normalTools->anPauseButton, &QToolButton::clicked,
		[=] (bool on) { gfx_board->pause_analysis (on); });

	connect(m_ascii_dlg.cb_coords, &QCheckBox::toggled, [=] (bool) { update_ascii_dialog (); });
	connect(m_ascii_dlg.cb_numbering, &QCheckBox::toggled, [=] (bool) { update_ascii_dialog (); });
	connect(m_svg_dlg.cb_coords, &QCheckBox::toggled, [=] (bool) { update_svg_dialog (); });
	connect(m_svg_dlg.cb_numbering, &QCheckBox::toggled, [=] (bool) { update_svg_dialog (); });
	/* These don't do anything that toggling the checkboxes wouldn't also do, but it's slightly more
	   intuitive to have a button for it.  */
	connect(m_ascii_dlg.buttonRefresh, &QPushButton::clicked, [=] (bool) { update_ascii_dialog (); });
	connect(m_svg_dlg.buttonRefresh, &QPushButton::clicked, [=] (bool) { update_svg_dialog (); });

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

#if 1
	/* This is a hack.  We appear to be hitting Qt bugs 70571/65592, where dock sizes snap back
	   violently as soon as the user moves the mouse over the board window.
	   There is a thread at
	     https://forum.qt.io/topic/94473/qdockwidget-resize-issue/16
	   where someone seems to have the same issue, also involving a QGraphicsView, and the
	   following is an adpatation of the suggested workaround.
	   This should be removed once we can assume Qt 5.12 and the bug is indeed fixed.  */
	resizeDocks ({diagsDock, treeDock, commentsDock, observersDock, graphDock}, { 0, 0, 0, 0, 0 }, Qt::Horizontal);
	resizeDocks ({diagsDock, treeDock, commentsDock, observersDock, graphDock}, { 0, 0, 0, 0, 0 }, Qt::Vertical);
#endif
	/* Order of operations here: restore a default layout if the user saved one.  We
	   need to have the game mode variable set for this.
	   Then, choose visibility defaults for the docks.
	   Then, do a proper setGameMode, to hide all panes that should not be visible.
	   Finally, restore the specific layout if one was saved.  */
	restoreWindowLayout (true);

	int figuremode = setting->readIntEntry ("BOARD_DIAGMODE");
	if (figuremode == 1 && m_game->get_root ()->has_figure_recursive ())
		figuremode = 2;
	diagsDock->setVisible (figuremode == 2 || mode == modeBatch);
	graphDock->setVisible (figuremode == 2 || mode == modeBatch);
	if (mode == modeMatch || mode == modeTeach || mode == modeObserve)
		observersDock->setVisible (true);
	setGameMode (mode);
	updateBoard();
	restoreWindowLayout (false);

	connect(commentEdit, &QTextEdit::textChanged, this, &MainWindow::slotUpdateComment);
	connect(commentEdit2, &QLineEdit::returnPressed, this, &MainWindow::slotUpdateComment2);
	m_allow_text_update_signal = true;

	update_analysis (analyzer::disconnected);
	main_window_list.push_back (this);
}

void MainWindow::init_game_record (std::shared_ptr<game_record> gr)
{
	m_svg_dlg.hide ();
	m_ascii_dlg.hide ();

	m_game = gr;
	game_state *root = gr->get_root ();
	const go_board b (root->get_board (), none);
	delete m_empty_state;
	m_empty_state = new game_state (b, black);

	/* The order actually matters here.  The gfx_board will call back into MainWindow
	   to update figures, which means we need to have diagView set up.  */
	diagView->reset_game (gr);
	gfx_board->reset_game (gr);
	updateCaption ();
	update_game_record ();

	bool disable_rect = (b.torus_h () || b.torus_v ()) && setting->readIntEntry ("TOROID_DUPS") > 0;
	editRectSelect->setEnabled (!disable_rect);

	int figuremode = setting->readIntEntry ("BOARD_DIAGMODE");
	if (!diagsDock->isVisible () && figuremode == 1 && root->has_figure_recursive ()) {
		diagsDock->show ();
	}
}

void MainWindow::update_game_record ()
{
	if (m_game->ranked_type () == ranked::free)
		normalTools->TextLabel_free->setText(QApplication::tr("free"));
	else if (m_game->ranked_type () == ranked::ranked)
		normalTools->TextLabel_free->setText(QApplication::tr("rated"));
	else
		normalTools->TextLabel_free->setText(QApplication::tr("teach"));
	normalTools->komi->setText(QString::number(m_game->komi ()));
	scoreTools->komi->setText(QString::number(m_game->komi ()));
	normalTools->handicap->setText(QString::number(m_game->handicap ()));
	updateCaption ();
}

MainWindow::~MainWindow()
{
	main_window_list.remove (this);

	delete m_empty_state;

	delete timer;

	// status bar
	delete statusMode;
	delete statusNav;
	delete statusTurn;
	delete statusCoords;

	// Actions
	delete escapeFocus;
	delete editGroup;
	delete navAutoplay;
	delete navSwapVariations;

	delete whatsThis;
}

void MainWindow::initActions ()
{
	// Escape focus: Escape key to get the focus from comment field to main window.
 	escapeFocus = new QAction(this);
	escapeFocus->setShortcut(Qt::Key_Escape);
	connect(escapeFocus, &QAction::triggered, this, &MainWindow::slotFocus);

	/* File menu.  */
	connect(fileNewBoard, &QAction::triggered, this, &MainWindow::slotFileNewBoard);
	connect(fileNew, &QAction::triggered, this, &MainWindow::slotFileNewGame);
	connect(fileNewVariant, &QAction::triggered, this, &MainWindow::slotFileNewVariantGame);
	connect(fileOpen, &QAction::triggered, this, &MainWindow::slotFileOpen);
	connect(fileSave, &QAction::triggered, this, &MainWindow::slotFileSave);
	connect(fileSaveAs, &QAction::triggered, this, &MainWindow::slotFileSaveAs);
	connect(fileClose, &QAction::triggered, this, &MainWindow::slotFileClose);
	connect(fileQuit, &QAction::triggered, this, &MainWindow::slotFileClose);//(qGo*)qApp, SLOT(quit);

	connect(fileExportASCII, &QAction::triggered, this, &MainWindow::slotFileExportASCII);
	connect(fileExportSVG, &QAction::triggered, this, &MainWindow::slotFileExportSVG);
	connect(fileImportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileImportSgfClipB);
	connect(fileExportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileExportSgfClipB);
	connect(fileExportPic, &QAction::triggered, this, &MainWindow::slotFileExportPic);
	connect(fileExportPicClipB, &QAction::triggered, this, &MainWindow::slotFileExportPicClipB);

	/* Edit menu.  */
	connect(editDelete, &QAction::triggered, this, &MainWindow::slotEditDelete);
	connect(setGameInfo, &QAction::triggered, this, &MainWindow::slotSetGameInfo);

	connect(editStone, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect(editTriangle, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect(editCircle, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect(editCross, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect(editSquare, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect(editNumber, &QAction::triggered, this, &MainWindow::slotEditGroup);
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

	connect (editFigure, &QAction::triggered, this, &MainWindow::slotEditFigure);
	connect (editRectSelect, &QAction::toggled, this, &MainWindow::slotEditRectSelect);
	connect (editClearSelect, &QAction::triggered, this, &MainWindow::slotEditClearSelect);

	connect (editAutoDiags, &QAction::triggered,
		 [this] (bool)
		 {
			 AutoDiagsDialog dlg (m_game, this);
			 dlg.exec ();
			 update_game_tree ();
			 update_figures ();
		 });

	/* Navigation menu.  */
	connect (navBackward, &QAction::triggered, [=] () { gfx_board->previous_move (); });
	connect (navForward, &QAction::triggered, [=] () { gfx_board->next_move (); });
	connect (navFirst, &QAction::triggered, [=] () { gfx_board->goto_first_move (); });
	connect (navLast, &QAction::triggered, [=] () { gfx_board->goto_last_move (); });
	connect (navPrevVar, &QAction::triggered, [=] () { gfx_board->previous_variation (); });
	connect (navNextVar, &QAction::triggered, [=] () { gfx_board->next_variation (); });
	connect (navMainBranch, &QAction::triggered, [=] () { gfx_board->goto_main_branch (); });
	connect (navStartVar, &QAction::triggered, [=] () { gfx_board->goto_var_start (); });
	connect (navNextBranch, &QAction::triggered, [=] () { gfx_board->goto_next_branch (); });
	connect (navPrevComment, &QAction::triggered, [=] () { gfx_board->previous_comment (); });
	connect (navNextComment, &QAction::triggered, [=] () { gfx_board->next_comment (); });
	connect (navPrevFigure, &QAction::triggered, [=] () { gfx_board->previous_figure (); });
	connect (navNextFigure, &QAction::triggered, [=] () { gfx_board->next_figure (); });

	connect (navNthMove, &QAction::triggered, this, &MainWindow::slotNavNthMove);
	connect (navIntersection, &QAction::triggered, this, &MainWindow::slotNavIntersection);

	navAutoplay = new QAction(tr("&Autoplay"), this);
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

	/* Settings menu.  */
	connect(setPreferences, &QAction::triggered, client_window, &ClientWindow::slot_preferences);
	connect(soundToggle, &QAction::toggled, this, &MainWindow::slotSoundToggle);
	soundToggle->setChecked(!local_stone_sound);

	/* View menu.  */
	connect(viewMenuBar, &QAction::toggled, this, &MainWindow::slotViewMenuBar);
	connect(viewStatusBar, &QAction::toggled, this, &MainWindow::slotViewStatusBar);
	connect(viewCoords, &QAction::toggled, this, &MainWindow::slotViewCoords);
	connect(viewSlider, &QAction::toggled, this, &MainWindow::slotViewSlider);
	connect(viewSidebar, &QAction::toggled, this, &MainWindow::slotViewSidebar);
	connect(layoutSaveDefault, &QAction::triggered, this, [=] () { saveWindowLayout (true); });
	connect(layoutRestoreDefault, &QAction::triggered, this, [=] () { restoreWindowLayout (true); });
	connect(layoutSaveCurrent, &QAction::triggered, this, [=] () { saveWindowLayout (false); });
	connect(layoutRestoreCurrent, &QAction::triggered, this, [=] () { restoreWindowLayout (false); });
	connect(layoutPortrait, &QAction::triggered, this, [=] () { defaultPortraitLayout (); });
	connect(layoutLandscape, &QAction::triggered, this, [=] () { defaultLandscapeLayout (); });
	connect(viewFullscreen, &QAction::toggled, this, &MainWindow::slotViewFullscreen);
	connect(viewNumbers, &QAction::toggled, this, &MainWindow::slotViewMoveNumbers);
	connect(viewDiagComments, &QAction::toggled, this, &MainWindow::slotViewDiagComments);

	/* Analyze menu.  */
	connect(anConnect, &QAction::triggered, this, [=] () { gfx_board->start_analysis (); });
	connect(anPause, &QAction::toggled, this, [=] (bool on) { if (on) { grey_eval_bar (); } gfx_board->pause_analysis (on); });
	connect(anDisconnect, &QAction::triggered, this, [=] () { gfx_board->stop_analysis (); });
	connect(anBatch, &QAction::triggered, [] (bool) { show_batch_analysis (); });

	/* Help menu.  */
	connect(helpManual, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("index.html")); });
	connect(helpReadme, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("readme.html")); });
	/* There isn't actually a manual.  Well, there is, but it's outdated and we don't ship it.  */
	helpManual->setVisible (false);
	helpManual->setEnabled (false);
	connect(helpAbout, &QAction::triggered, [=] (bool) { help_about (); });
	connect(helpAboutQt, &QAction::triggered, [=] (bool) { QMessageBox::aboutQt (this); });
	whatsThis = QWhatsThis::createAction (this);

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
	navNextFigure->setEnabled(false);
	navPrevFigure->setEnabled(false);
	navIntersection->setEnabled(false);

	/* Need actions with shortcuts added to the window, so that the shortcut still works
	   if the menubar is hidden.  */
	addActions ({ fileNewBoard, fileNew, fileOpen, fileSave, fileSaveAs, fileClose, fileQuit });
	addActions ({ navForward, navBackward, navFirst, navLast, navPrevVar, navNextVar });
	addActions ({ setGameInfo, editDelete, editFigure, editRectSelect, editClearSelect });
	addActions ({ navMainBranch, navStartVar, navNextBranch, navNthMove });
	addActions ({ setPreferences, soundToggle });
	addActions ({ viewMenuBar, viewSidebar, viewCoords, viewFullscreen });
	addActions ({ helpManual, whatsThis });
}

void MainWindow::initMenuBar (GameMode mode)
{
	QAction *view_first = viewMenu->actions().at(0);

	viewMenu->insertAction (view_first, fileBar->toggleViewAction ());
	viewMenu->insertAction (view_first, toolBar->toggleViewAction ());
	viewMenu->insertAction (view_first, editBar->toggleViewAction ());
	viewMenu->insertAction (view_first, miscBar->toggleViewAction ());
	viewMenu->insertSeparator(view_first);
	viewMenu->insertAction (view_first, commentsDock->toggleViewAction ());
	viewMenu->insertAction (view_first, observersDock->toggleViewAction ());
	viewMenu->insertAction (view_first, diagsDock->toggleViewAction ());
	viewMenu->insertAction (view_first, graphDock->toggleViewAction ());
	viewMenu->insertAction (view_first, treeDock->toggleViewAction ());

	helpMenu->addSeparator ();
	helpMenu->addAction (whatsThis);

	anMenu->setVisible (mode == modeNormal || mode == modeObserve);
}

void MainWindow::initToolBar()
{
	miscBar->addSeparator();
	miscBar->addAction (soundToggle);
	miscBar->addAction (viewCoords);

	miscBar->addSeparator();
	miscBar->addAction (whatsThis);
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

void MainWindow::updateCaption ()
{
	bool modified = m_game->modified ();

	// Print caption
	// example: qGo 0.0.5 - Zotan 8k vs. tgmouse 10k
	// or if game name is given: qGo 0.0.5 - Kogo's Joseki Dictionary
	int game_number = 0;
	QString s;
	if (modified)
		s += "* ";
	if (m_gamemode == modeBatch)
		s += tr ("Analysis in progress: ");
	else if (refreshButton->isVisibleTo (this))
		s += tr ("Off-line copy: ");

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
	normalTools->whiteLabel->setText(player_w);
	scoreTools->whiteLabel->setText(player_w);

	rank_b.truncate (5);
	int rblen = rank_b.length ();
	player_b.truncate(15 - rblen);
	if (rblen > 0)
		player_b += " " + rank_b;
	normalTools->blackLabel->setText(player_b);
	scoreTools->blackLabel->setText(player_b);
}

void MainWindow::update_game_tree ()
{
	game_state *st = gfx_board->displayed ();
	gameTreeView->update (m_game, st);
	evalGraph->update (m_game, st);
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

	std::shared_ptr<game_record> gr = open_file_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);
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

bool MainWindow::slotFileSave (bool)
{
	QString fileName = QString::fromStdString (m_game->filename ());
	if (fileName.isEmpty())
	{
		fileName = setting->readEntry("LAST_DIR");
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

bool MainWindow::doSave (QString fileName, bool force)
{
	if (m_game->errors ().any_set ()) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning (this, PACKAGE,
					       tr ("This file had errors during loading and may be corrupt.\nDo you still want to save it?"),
					       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (choice != QMessageBox::Yes)
			return false;
	}
	if (fileName.isNull () || fileName.isEmpty () || !force)
  	{
		if (QDir(fileName).exists())
			fileName = get_candidate_filename (fileName, *m_game);
		else if (fileName.isNull() || fileName.isEmpty()) {
			QString dir = setting->readEntry("LAST_DIR");

			fileName = get_candidate_filename (dir, *m_game);
		}
		if (!force)
			fileName = QFileDialog::getSaveFileName(this, tr ("Save SGF file"),
								fileName, tr("SGF Files (*.sgf);;All Files (*)"));
	}

	if (fileName.isEmpty())
		return false;

	if (getFileExtension(fileName, false).isEmpty())
		fileName.append(".sgf");

	QFile of (fileName);
	if (!of.open (QIODevice::WriteOnly)) {
		QMessageBox::warning (this, PACKAGE, tr("Cannot open SGF file for saving."));
		return false;
	}
	std::string sgf = m_game->to_sgf ();
	QByteArray bytes = QByteArray::fromStdString (sgf);
	qint64 written = of.write (bytes);
	if (written != bytes.length ()) {
		QMessageBox::warning (this, PACKAGE, tr("Failed to save SGF file."));
		return false;
	}
	of.close ();

	statusBar()->showMessage (fileName + " " + tr("saved."));
	m_game->set_modified (false);
	m_game->clear_errors ();
	update_game_record ();
	return true;
}

void MainWindow::slotFileClose (bool)
{
	close ();
}

void MainWindow::slotFileImportSgfClipB(bool)
{
	if (!checkModified ())
		return;

        QString sgfString = QApplication::clipboard()->text();
	QByteArray bytes = sgfString.toUtf8 ();
	QBuffer buf (&bytes);
	buf.open (QBuffer::ReadOnly);
	std::shared_ptr<game_record> gr = record_from_stream (buf, nullptr);
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

void MainWindow::update_ascii_dialog ()
{
	QString s = m_ascii_update_source->render_ascii (m_ascii_dlg.cb_numbering->isChecked (),
							 m_ascii_dlg.cb_coords->isChecked ());
	m_ascii_dlg.textEdit->setText (s);
}

void MainWindow::update_svg_dialog ()
{
	QByteArray s = m_svg_update_source->render_svg (m_svg_dlg.cb_numbering->isChecked (),
							m_svg_dlg.cb_coords->isChecked ());
	m_svg_dlg.set (s);
}

void MainWindow::slotFileExportASCII(bool)
{
	m_ascii_update_source = gfx_board;
	if (!m_ascii_dlg.isVisible ()) {
		/* Set defaults if the dialog is not open.  */
		m_ascii_dlg.cb_numbering->setChecked (viewNumbers->isChecked ());
	}
	update_ascii_dialog ();
	m_ascii_dlg.show ();
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotFileExportSVG(bool)
{
	m_svg_update_source = gfx_board;
	if (!m_svg_dlg.isVisible ()) {
		/* Set defaults if the dialog is not open.  */
		m_svg_dlg.cb_numbering->setChecked (viewNumbers->isChecked ());
	}
	update_svg_dialog ();
	m_svg_dlg.show ();
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

void MainWindow::slotEditFigure (bool on)
{
	game_state *st = gfx_board->displayed ();
	if (on)
		st->set_figure (256, "");
	else
		st->clear_figure ();
	update_figures ();
	update_game_tree ();
}

void MainWindow::slotDiagChosen (int idx)
{
	game_state *st = m_figures.at (idx);
	diagView->set_displayed (st);
	diagCommentView->setText (QString::fromStdString (st->comment ()));
}

void MainWindow::slotDiagEdit (bool)
{
	game_state *st = diagView->displayed ();
	FigureDialog dlg (st, this);
	dlg.exec ();
	update_figures ();
}

void MainWindow::slotDiagASCII (bool)
{
	m_ascii_update_source = diagView;
	int print_num = m_ascii_update_source->displayed ()->print_numbering_inherited ();
	m_ascii_dlg.cb_numbering->setChecked (print_num != 0);
	update_ascii_dialog ();
	m_ascii_dlg.buttonRefresh->hide ();
	m_ascii_dlg.exec ();
	m_ascii_dlg.buttonRefresh->show ();
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotDiagSVG (bool)
{
	m_svg_update_source = diagView;
	int print_num = m_svg_update_source->displayed ()->print_numbering_inherited ();
	m_svg_dlg.cb_numbering->setChecked (print_num != 0);
	update_svg_dialog ();
	m_svg_dlg.buttonRefresh->hide ();
	m_svg_dlg.exec ();
	m_svg_dlg.buttonRefresh->show ();
	statusBar()->showMessage(tr("Ready."));
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

/* Some of the docks shouldn't be available in certain modes: offline boards don't need
   observers, and match games shouldn't get an evaluation and don't need figures.
   We call this when setting up the window, and also when restoring the layout, since
   restoreState may do things we don't want.  */

void MainWindow::hide_panes_for_mode ()
{
	bool is_online = m_gamemode == modeMatch || m_gamemode == modeObserve || m_gamemode == modeTeach;
	if (is_online) {
		treeDock->setVisible (false);
		treeDock->toggleViewAction ()->setVisible (false);
		if (m_gamemode != modeObserve) {
			graphDock->setVisible (false);
			graphDock->toggleViewAction ()->setVisible (false);
		}
		diagsDock->setVisible (false);
		diagsDock->toggleViewAction ()->setVisible (false);
	} else {
		observersDock->setVisible (false);
		observersDock->toggleViewAction ()->setVisible (false);
		treeDock->toggleViewAction ()->setVisible (true);
		diagsDock->toggleViewAction ()->setVisible (true);
		graphDock->toggleViewAction ()->setVisible (true);
	}
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

	gameTreeView->update_prefs ();
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
	dlg.eventEdit->setText(QString::fromStdString (m_game->event ()));
	dlg.roundEdit->setText(QString::fromStdString (m_game->round ()));
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
		m_game->set_event (dlg.eventEdit->text().toStdString ());
		m_game->set_round (dlg.roundEdit->text().toStdString ());
		m_game->set_copyright (dlg.copyrightEdit->text().toStdString ());

		m_game->set_modified (true);
		update_game_record ();
	}
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
	gfx_board->set_show_coords(toggle);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewMoveNumbers(bool toggle)
{
	gfx_board->set_show_move_numbers (toggle);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewDiagComments(bool toggle)
{
	diagCommentView->setVisible (toggle);
	comments_widget->setVisible (!toggle);
	if (toggle)
		commentsDock->setWindowTitle (tr ("Diag. comments"));
	else
		commentsDock->setWindowTitle (tr ("Comments"));
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewSlider(bool toggle)
{
	if (!toggle)
		toggleSlider(false);
	else
		toggleSlider(true);

	statusBar()->showMessage(tr("Ready."));
}

// set sidbar left or right
void MainWindow::slotViewLeftSidebar()
{
	mainGridLayout->removeWidget(boardFrame);
	mainGridLayout->removeWidget(toolsFrame);

	if (setting->readBoolEntry("SIDEBAR_LEFT"))
	{
		mainGridLayout->addWidget(toolsFrame, 0, 0, 2, 1);
		mainGridLayout->addWidget(boardFrame, 0, 1, 2, 1);
	}
	else
	{
		mainGridLayout->addWidget(toolsFrame, 0, 1, 2, 1);
		mainGridLayout->addWidget(boardFrame, 0, 0, 2, 1);
	}
}

void MainWindow::slotViewSidebar(bool toggle)
{
	toggleSidebar(toggle);
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
			tmp = normalTools->pb_timeBlack->text();
		else
			tmp = normalTools->pb_timeWhite->text();

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

QString MainWindow::visible_panes_key ()
{
	QString v;
	v += diagsDock->isVisibleTo (this) ? "1" : "0";
	v += treeDock->isVisibleTo (this) ? "1" : "0";
	v += graphDock->isVisibleTo (this) ? "1" : "0";
	v += commentsDock->isVisibleTo (this) ? "1" : "0";
	v += observersDock->isVisibleTo (this) ? "1" : "0";
	return v;
}

void MainWindow::restore_visibility_from_key (const QString &v)
{
	diagsDock->setVisible (v[0] == '1');
	treeDock->setVisible (v[1] == '1');
	graphDock->setVisible (v[2] == '1');
	commentsDock->setVisible (v[3] == '1');
	observersDock->setVisible (v[4] == '1');
}

void MainWindow::saveWindowLayout (bool dflt)
{
	QString strKey = screen_key (this);
	QString panesKey = visible_panes_key ();

	if (!dflt)
		strKey += "_" + panesKey;

	// store window size, format, comment format
	setting->writeBoolEntry("BOARDFULLSCREEN_" + strKey, isFullScreen);

	QByteArray v1 = saveState ().toHex ();
	QByteArray v2 = saveGeometry ().toHex ();
	setting->writeEntry("BOARDLAYOUT1_" + strKey, QString::fromLatin1 (v1));
	setting->writeEntry("BOARDLAYOUT2_" + strKey, QString::fromLatin1 (v2));

	statusBar()->showMessage(tr("Window size saved.") + " (" + strKey + ")");
}

bool MainWindow::restoreWindowLayout (bool dflt)
{
	QString strKey = screen_key (this);
	QString panesKey = visible_panes_key ();

	if (!dflt)
		strKey += "_" + panesKey;

	// restore board window
	QString s1 = setting->readEntry("BOARDLAYOUT1_" + strKey);
	QString s2 = setting->readEntry("BOARDLAYOUT2_" + strKey);
	if (s1.isEmpty () || s2.isEmpty ())
		return false;
	QRegExp verify ("^[0-9A-Fa-f]*$");
	if (!verify.exactMatch (s1) || !verify.exactMatch (s2))
		return false;

	// do not resize until end of this procedure
	gfx_board->lockResize = true;

	restoreGeometry (QByteArray::fromHex (s2.toLatin1 ()));
	restoreState (QByteArray::fromHex (s1.toLatin1 ()));

	if (!dflt)
		restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();

	// ok, resize
	gfx_board->lockResize = false;
	gfx_board->changeSize();

	statusBar()->showMessage(tr("Window size restored.") + " (" + strKey + ")");

	return true;
}

void MainWindow::defaultPortraitLayout ()
{
	commentsDock->setVisible (true);
	if (m_gamemode == modeMatch || m_gamemode == modeObserve || m_gamemode == modeTeach)
		observersDock->setVisible (true);

	QString panesKey = visible_panes_key ();
	removeDockWidget (diagsDock);
	removeDockWidget (treeDock);
	removeDockWidget (graphDock);
	removeDockWidget (commentsDock);
	removeDockWidget (observersDock);
	addDockWidget (Qt::BottomDockWidgetArea, graphDock);
	splitDockWidget (graphDock, diagsDock, Qt::Vertical);
	splitDockWidget (diagsDock, commentsDock, Qt::Horizontal);
	splitDockWidget (commentsDock, observersDock, Qt::Horizontal);
	splitDockWidget (commentsDock, treeDock, Qt::Horizontal);
	restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();
	setFocus ();
}

void MainWindow::defaultLandscapeLayout ()
{
	commentsDock->setVisible (true);
	if (m_gamemode == modeMatch || m_gamemode == modeObserve || m_gamemode == modeTeach)
		observersDock->setVisible (true);

	QString panesKey = visible_panes_key ();
	removeDockWidget (diagsDock);
	removeDockWidget (treeDock);
	removeDockWidget (graphDock);
	removeDockWidget (commentsDock);
	removeDockWidget (observersDock);
	addDockWidget (Qt::BottomDockWidgetArea, treeDock);
	addDockWidget (Qt::RightDockWidgetArea, diagsDock);
	splitDockWidget (diagsDock, commentsDock, Qt::Horizontal);
	splitDockWidget (diagsDock, graphDock, Qt::Vertical);
	splitDockWidget (commentsDock, observersDock, Qt::Vertical);
	restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();
	setFocus ();
}

void MainWindow::closeEvent (QCloseEvent *e)
{
	// qDebug("MainWindow::closeEvent(QCloseEvent *e)");
	if (getBoard()->getGameMode() == modeObserve || checkModified ())
	{
		emit signal_closeevent();
		e->accept();
	}
	else
		e->ignore();
}

int MainWindow::checkModified (bool interactive)
{
	if (!m_game->modified ())
		return 1;

	if (!interactive)
		return 0;

	QMessageBox::StandardButton choice;
	choice = QMessageBox::warning(this, PACKAGE,
				      tr("You modified the game.\nDo you want to save your changes?"),
				      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

	switch (choice)
	{
	case QMessageBox::Yes:
		return slotFileSave() && !m_game->modified ();

	case QMessageBox::No:
		return 2;

	case QMessageBox::Cancel:
		return 0;

	default:
		qWarning("Unknown messagebox input.");
		return 0;
	}
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
	setFont (setting->fontComments);

	// observer
	ListView_observers->setFont (setting->fontLists);

	QFont f (setting->fontStandard);
	f.setBold (true);
	scoreTools->totalBlack->setFont (f);
	scoreTools->totalWhite->setFont (f);

	setFont (setting->fontStandard);
	normalTools->wtimeView->update_font (setting->fontClocks);
	normalTools->btimeView->update_font (setting->fontClocks);
	commentEdit->setCurrentFont(setting->fontComments);

	QFontMetrics fm (setting->fontStandard);
	QRect r = fm.boundingRect ("Variation 12 of 15");
	QRect r2 = fm.boundingRect ("Move 359 (W M19)");
	int strings_width = std::max (r.width (), r2.width ());
	int min_width = std::max (125, strings_width) + 25;
	toolsFrame->setMaximumWidth (min_width);
	toolsFrame->setMinimumWidth (min_width);
	toolsFrame->resize (min_width, toolsFrame->height ());

	int h = fm.height ();
	QString wimg = ":/images/stone_w16.png";
	QString bimg = ":/images/stone_b16.png";
	if (h >= 32) {
		 wimg = ":/images/stone_w32.png";
		 bimg = ":/images/stone_b32.png";
	} else if (h >= 22) {
		wimg = ":/images/stone_w22.png";
		bimg = ":/images/stone_b22.png";
	}
	QIcon icon (bimg);
	icon.addPixmap (wimg, QIcon::Normal, QIcon::On);

	colorButton->setIcon (icon);
	colorButton->setIconSize (QSize (h, h));
	normalTools->whiteStoneLabel->setPixmap (QPixmap (wimg));
	normalTools->blackStoneLabel->setPixmap (QPixmap (bimg));
	scoreTools->whiteStoneLabel->setPixmap (wimg);
	scoreTools->blackStoneLabel->setPixmap (bimg);
}

void MainWindow::slot_editBoardInNewWindow(bool)
{
	std::shared_ptr<game_record> newgr = std::make_shared<game_record> (*m_game);
	// online mode -> don't score, open new Window instead
	MainWindow *w = new MainWindow (nullptr, newgr);

	connect (w->refreshButton, &QPushButton::clicked,
		 [gr = m_game, w] (bool)
		 {
			 w->init_game_record (std::make_shared<game_record> (*gr));
			 w->navLast->trigger ();
		 });
	w->refreshButton->setEnabled (true);
	w->refreshButton->setVisible (true);
	w->updateCaption ();

	w->navLast->trigger ();
	w->show ();
}

void MainWindow::slotSoundToggle(bool toggle)
{
	local_stone_sound = !toggle;
}

// set a tab on toolsTabWidget
void MainWindow::setToolsTabWidget(enum tabType p, enum tabState s)
{
	QWidget *w = nullptr;

	switch (p)
	{
		case tabNormalScore:
			w = tab_ns;
			break;

		case tabTeachGameTree:
			w = tab_tg;
			break;

		default:
			return;
	}

	if (s == tabSet)
	{
		// check whether the page to switch to is enabled
		if (!toolsTabWidget->isEnabled())
			toolsTabWidget->setTabEnabled(toolsTabWidget->indexOf (w), true);

		toolsTabWidget->setCurrentIndex(p);
	}
	else
	{
		// check whether the current page is to disable; then set to 'normal'
		if (s == tabDisable && toolsTabWidget->currentIndex() == p)
			toolsTabWidget->setCurrentIndex(tabNormalScore);

		toolsTabWidget->setTabEnabled(toolsTabWidget->indexOf (w), s == tabEnable);
	}
}

void MainWindow::setGameMode(GameMode mode)
{
	m_gamemode = mode;
	if (mode == modeEdit || mode == modeNormal || mode == modeObserve) {
		editGroup->setEnabled (true);
	} else {
		editStone->setChecked (true);
		gfx_board->setMarkType (mark::none);
		editGroup->setEnabled (false);
	}

	normalTools->anGroup->setVisible (mode == modeNormal || mode == modeObserve);
	normalTools->anStartButton->setVisible (mode == modeNormal || mode == modeObserve);
	normalTools->anPauseButton->setVisible (mode == modeNormal || mode == modeObserve);

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

	bool editable_comments = mode == modeNormal || mode == modeEdit || mode == modeScore || mode == modeComputer || mode == modeBatch;
	commentEdit->setReadOnly (!editable_comments || mode == modeEdit);
	// commentEdit->setDisabled (editable_comments);
	commentEdit2->setEnabled (!editable_comments);
	commentEdit2->setVisible (!editable_comments);

	hide_panes_for_mode ();

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

	case modeBatch:
		statusMode->setText(" " + QObject::tr("A", "Board status line: batch analysis") + " ");
		break;

	}

	setToolsTabWidget(tabTeachGameTree, mode == modeTeach ? tabEnable : tabDisable);
	if (mode != modeTeach && mode != modeScoreRemote && mode != modeScore && tab_tg->parent () != nullptr)
		tab_tg->setParent (nullptr);

	passButton->setVisible (mode != modeObserve);
	doneButton->setVisible (mode == modeScore || mode == modeScoreRemote);
	scoreButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	undoButton->setVisible (mode == modeScoreRemote || mode == modeMatch || mode == modeComputer || mode == modeTeach);
	adjournButton->setVisible (mode == modeMatch || mode == modeTeach || mode == modeScoreRemote);
	resignButton->setVisible (mode == modeMatch || mode == modeComputer || mode == modeTeach || mode == modeScoreRemote);
	resignButton->setEnabled (mode != modeScoreRemote);
	refreshButton->setVisible (false);
	editButton->setVisible (mode == modeObserve);
	editPosButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	editPosButton->setEnabled (mode == modeNormal || mode == modeEdit);
	colorButton->setEnabled (mode == modeEdit || mode == modeNormal);

	slider->setEnabled (mode == modeNormal || mode == modeObserve || mode == modeBatch);

	passButton->setEnabled (mode != modeScore && mode != modeScoreRemote);
	editButton->setEnabled (mode != modeScore);
	scoreButton->setEnabled (mode != modeEdit);

	gfx_board->setMode (mode);

	scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote);
	normalTools->setVisible (mode != modeScore && mode != modeScoreRemote);

	if (mode == modeMatch || mode == modeTeach) {
		qDebug () << gfx_board->player_is (white) << " : " << gfx_board->player_is (black);
		QWidget *timeSelf = gfx_board->player_is (black) ? normalTools->btimeView : normalTools->wtimeView;
		QWidget *timeOther = gfx_board->player_is (black) ? normalTools->wtimeView : normalTools->btimeView;
#if 0
		switch (gsName)
		{
		case IGS:
			timeSelf->setToolTip(tr("remaining time / stones"));
			break;

		default:
			timeSelf->setToolTip(tr("click to pause/unpause the game"));
			break;
		}
#else
		timeSelf->setToolTip (tr("click to pause/unpause the game"));
#endif
		timeOther->setToolTip (tr("click to add 1 minute to your opponent's clock"));
	} else {
		normalTools->btimeView->setToolTip (tr ("Time remaining for this move"));
		normalTools->wtimeView->setToolTip (tr ("Time remaining for this move"));
	}
	updateCaption ();
}

void MainWindow::update_figure_display ()
{
	game_state *fig = diagView->displayed ();
	if (fig != nullptr) {
		int flags = fig->figure_flags ();
		diagView->set_show_coords (!(flags & 1));
		diagView->set_show_figure_caps (!(flags & 256));
		diagView->set_show_hoshis (!(flags & 512));
		diagView->changeSize ();
	}
}

void MainWindow::update_figures ()
{
	game_state *gs = gfx_board->displayed ();
	game_state *old_fig = diagView->displayed ();
	int keep_old_fig = -1;
	m_figures.clear ();
	diagComboBox->clear ();
	bool main_fig = gs->has_figure ();
	editFigure->setChecked (main_fig);
	if (main_fig) {
		const std::string &title = gs->figure_title ();
		m_figures.push_back (gs);
		if (title == "")
			diagComboBox->addItem ("Current position (untitled)");
		else
			diagComboBox->addItem (QString::fromStdString (title));
		if (gs == old_fig)
			keep_old_fig = 0;
	}
	int count = 1;
	auto children = gs->children ();
	for (auto it: children) {
		/* Skip the first child - it's the main variation, and should only appear
		   in the list as "Current position", once it is reached.  */
		if (it == children[0])
			continue;
		if (it->has_figure ()) {
			if (count == 1 && m_figures.size () != 0) {
				m_figures.push_back (nullptr);
				diagComboBox->insertSeparator (1);
			}
			const std::string &title = it->figure_title ();
			if (title == "")
				diagComboBox->addItem ("Subposition " + QString::number (count) + " (untitled)");
			else
				diagComboBox->addItem (QString::fromStdString (title));
			m_figures.push_back (it);
			if (it == old_fig)
				keep_old_fig = diagComboBox->count () - 1;
			count++;
		}
	}
	bool have_any = m_figures.size () != 0;
	diagComboBox->setEnabled (have_any);
	diagEditButton->setEnabled (have_any);
	diagASCIIButton->setEnabled (have_any);
	diagSVGButton->setEnabled (have_any);
	if (setting->readBoolEntry ("BOARD_DIAGCLEAR")) {
		diagView->setEnabled (have_any);
		if (!have_any)
			diagView->set_displayed (m_empty_state);
	}
	if (!have_any) {
		diagComboBox->addItem ("No diagrams available");
		diagCommentView->clear ();
	} else if (main_fig) {
		diagView->set_displayed (gs);
		diagCommentView->setText (QString::fromStdString (gs->comment ()));
	} else if (keep_old_fig == -1) {
		diagView->set_displayed (m_figures[0]);
		diagCommentView->setText (QString::fromStdString (m_figures[0]->comment ()));
	} else {
		diagComboBox->setCurrentIndex (keep_old_fig);
	}
	update_figure_display ();
}

void MainWindow::set_game_position (game_state *gs)
{
	/* Have to call this first so we trace the correct path in the game tree
	   once move_state calls back into there.  */
	gs->make_active ();
	gfx_board->move_state (gs);
}

void MainWindow::setMoveData (game_state &gs, const go_board &b, GameMode mode)
{
	bool is_root_node = gs.root_node_p ();
	size_t brothers = gs.n_siblings ();
	size_t sons = gs.n_children ();
	stone_color to_move = gs.to_move ();
	int move_nr = gs.move_number ();
	int var_nr = gs.var_number ();

	bool good_mode = mode == modeNormal || mode == modeObserve || mode == modeBatch;

	navBackward->setEnabled (good_mode && !is_root_node);
	navForward->setEnabled (good_mode && sons > 0);
	navFirst->setEnabled (good_mode && !is_root_node);
	navLast->setEnabled (good_mode && sons > 0);
	navPrevComment->setEnabled (good_mode && !is_root_node);
	navNextComment->setEnabled (good_mode && sons > 0);
	navPrevFigure->setEnabled (good_mode && !is_root_node);
	navNextFigure->setEnabled (good_mode && sons > 0);
	navIntersection->setEnabled (good_mode);

	switch (mode)
	{
	case modeNormal:
		navSwapVariations->setEnabled(!is_root_node);

		/* fall through */
	case modeBatch:
		navPrevVar->setEnabled(gs.has_prev_sibling ());
		navNextVar->setEnabled(gs.has_next_sibling ());
		navStartVar->setEnabled(!is_root_node);
		navMainBranch->setEnabled(!is_root_node);
		navNextBranch->setEnabled(sons > 0);

		/* fall through */
	case modeObserve:
		break;

	case modeScore:
	case modeEdit:
		navPrevVar->setEnabled(false);
		navNextVar->setEnabled(false);
		navStartVar->setEnabled(false);
		navMainBranch->setEnabled(false);
		navNextBranch->setEnabled(false);
		navSwapVariations->setEnabled(false);
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

	update_figures ();

	QString w_str = QObject::tr("W");
	QString b_str = QObject::tr("B");

	QString s (QObject::tr ("Move") + " ");
	s.append (QString::number (move_nr));
	if (move_nr == 0) {
		/* Nothing to add for the root.  */
	} else if (gs.was_pass_p ()) {
		s.append(" (" + (to_move == black ? w_str : b_str) + " ");
		s.append(" " + QObject::tr("Pass") + ")");
	} else if (gs.was_score_p ()) {
		s.append(tr (" (Scoring)"));
	} else if (!gs.was_edit_p ()) {
		int x = gs.get_move_x ();
		int y = gs.get_move_y ();
		s.append(" (");
		s.append((to_move == black ? w_str : b_str) + " ");
		s.append(QString(QChar(static_cast<const char>('A' + (x < 9 ? x : x + 1)))));
		s.append(QString::number (b.size_y () - y) + ")");
	}
	s.append (QObject::tr ("\nVariation ") + QString::number (var_nr)
		  + QObject::tr (" of ") + QString::number (1 + brothers) + "\n");
	s.append (QString::number (sons) + " ");
	if (sons == 1)
		s.append (QObject::tr("child position"));
	else
		s.append (QObject::tr("child positions"));
	moveNumLabel->setText(s);

	bool warn = false;
	if (gs.was_move_p () && gs.get_move_color () == to_move)
		warn = true;
	if (gs.root_node_p ()) {
		bool empty = b.position_empty_p ();
		bool b_to_move = to_move == black;
		warn |= empty != b_to_move;
	}
	turnWarning->setVisible (warn);
	colorButton->setChecked (to_move == white);
#if 0
	statusTurn->setText(" " + s.right (s.length() - 5) + " ");  // Without 'Move '
	statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
#endif

	if (to_move == black)
		turnLabel->setText(QObject::tr ("Black to play"));
	else
		turnLabel->setText(QObject::tr ("White to play"));

	const QStyle *style = qgo_app->style ();
	int iconsz = style->pixelMetric (QStyle::PixelMetric::PM_ToolBarIconSize);

	QSize sz (iconsz, iconsz);
	goPrevButton->setIconSize (sz);
	goNextButton->setIconSize (sz);
	goFirstButton->setIconSize (sz);
	goLastButton->setIconSize (sz);
	prevCommentButton->setIconSize (sz);
	nextCommentButton->setIconSize (sz);
	prevNumberButton->setIconSize (sz);
	nextNumberButton->setIconSize (sz);

	scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote || gs.was_score_p ());
	normalTools->setVisible (mode != modeScore && mode != modeScoreRemote && !gs.was_score_p ());

	if (mode == modeNormal) {
		normalTools->wtimeView->set_time (&gs, white);
		normalTools->btimeView->set_time (&gs, black);
	}
	// Update slider
	toggleSliderSignal (false);

	int mv = gs.active_var_max ();
	setSliderMax (mv);
	slider->setValue (move_nr);
	toggleSliderSignal (true);
}

void MainWindow::on_colorButton_clicked (bool)
{
	stone_color col = gfx_board->swap_edit_to_move ();
	colorButton->setChecked (col == white);
}

/* The "Edit Position" button: Switch to edit mode.  */
void MainWindow::doEditPos (bool toggle)
{
	qDebug("MainWindow::doEditPos()");
	if (toggle)
		setGameMode (modeEdit);
	else
		setGameMode (modeNormal);
}

/* The "Edit Game" button: Open a new window to edit observed game.  */
void MainWindow::doEdit()
{
	qDebug("MainWindow::doEdit()");
	slot_editBoardInNewWindow(false);
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

void MainWindow::doRealScore (bool toggle)
{
	qDebug("MainWindow::doRealScore()");
	/* Can be called when the user toggles the button, but also when a GTP board
	   enters scoring after two passes.  */
	scoreButton->setChecked (toggle);
	if (toggle)
	{
		m_remember_mode = gfx_board->getGameMode();
		m_remember_tab = toolsTabWidget->currentIndex();

		setToolsTabWidget(tabTeachGameTree, tabDisable);

		setGameMode (modeScore);
	}
	else
	{
		setToolsTabWidget(tabTeachGameTree, tabEnable);

		setGameMode (m_remember_mode);
		setToolsTabWidget(static_cast<tabType>(m_remember_tab));
	}
}

void MainWindow::doCountDone()
{
	if (gfx_board->getGameMode () == modeScoreRemote) {
		emit signal_done ();
		return;
	}

	double komi = m_game->komi ();
	int capW = scoreTools->capturesWhite->text().toInt();
	int capB = scoreTools->capturesBlack->text().toInt();
	int terrW = scoreTools->terrWhite->text().toInt();
	int terrB = scoreTools->terrBlack->text().toInt();

	double totalWhite = capW + terrW + komi;
	int totalBlack = capB + terrB;
	double result = 0;
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
	doRealScore (false);
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
	: MainWindow (parent, gr, modeComputer), GTP_Controller (this)
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
	const game_state *st = gfx_board->displayed ();
	gfx_board->play_external_pass ();
	if (st->was_pass_p ()) {
		m_gtp->quit ();
		/* As of now, we're disconnected, and the scoring function should
		   remember normal mode.  */
		setGameMode (modeNormal);
		doRealScore (true);
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
	const game_state *st = gfx_board->displayed ();
	stone_color col = gfx_board->to_move ();
	MainWindow::doPass ();
	m_gtp->played_move_pass (col);
	if (st->was_pass_p ()) {
		m_gtp->quit ();
		/* As of now, we're disconnected, and the scoring function should
		   remember normal mode.  */
		setGameMode (modeNormal);
		doRealScore (true);
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
	evalView->setVisible (state == analyzer::running || state == analyzer::paused);
	anConnect->setEnabled (state == analyzer::disconnected);
	anConnect->setChecked (state != analyzer::disconnected);
	anDisconnect->setEnabled (state != analyzer::disconnected);
	normalTools->anStartButton->setChecked (state != analyzer::disconnected);
	if (state == analyzer::disconnected)
		normalTools->anStartButton->setIcon (QIcon (":/images/exit.png"));
	else if (state == analyzer::starting)
		normalTools->anStartButton->setIcon (QIcon (":/images/power-standby.png"));
	else
		normalTools->anStartButton->setIcon (QIcon (":/images/power-on.png"));
	normalTools->anPrimaryBox->setEnabled (state == analyzer::running);
	normalTools->anShownBox->setEnabled (state == analyzer::running);
	anPause->setEnabled (state != analyzer::disconnected);
	normalTools->anPauseButton->setEnabled (state != analyzer::disconnected);
	normalTools->anPauseButton->setChecked (state == analyzer::paused);
	anPause->setChecked (state == analyzer::paused);

	if (state == analyzer::disconnected) {
		normalTools->primaryCoords->setText ("");
		normalTools->primaryWR->setText ("");
		normalTools->primaryVisits->setText ("");
		normalTools->shownCoords->setText ("");
		normalTools->shownWR->setText ("");
		normalTools->shownVisits->setText ("");
	}
}

void MainWindow::recalc_scores(const go_board &b)
{
	int caps_b, caps_w, score_b, score_w;
	b.get_scores (caps_b, caps_w, score_b, score_w);

	scoreTools->capturesWhite->setText(QString::number(caps_w));
	scoreTools->capturesBlack->setText(QString::number(caps_b));
	normalTools->capturesWhite->setText(QString::number(caps_w));
	normalTools->capturesBlack->setText(QString::number(caps_b));

	scoreTools->terrWhite->setText(QString::number(score_w));
	scoreTools->totalWhite->setText(QString::number(score_w + caps_w + m_game->komi ()));
	scoreTools->terrBlack->setText(QString::number(score_b));
	scoreTools->totalBlack->setText(QString::number(score_b + caps_b));

	double res = score_w + caps_w + m_game->komi () - score_b - caps_b;
	if (res < 0)
		scoreTools->result->setText ("B+" + QString::number (-res));
	else if (res == 0)
		scoreTools->result->setText ("Jigo");
	else
		scoreTools->result->setText ("W+" + QString::number (res));
}

void MainWindow::setSliderMax(int n)
{
	if (n < 0)
		n = 0;

	slider->setMaximum(n);
	sliderRightLabel->setText(QString::number(n));
}

void MainWindow::sliderChanged(int n)
{
	if (sliderSignalToggle)
		gfx_board->goto_nth_move_in_var (n);
}

void MainWindow::toggleSlider(bool b)
{
	if (showSlider == b)
		return;

	showSlider = b;

	if (b)
	{
		slider->show();
		sliderLeftLabel->show();
		sliderRightLabel->show();
	}
	else
	{
		slider->hide();
		sliderLeftLabel->hide();
		sliderRightLabel->hide();
	}
}

void MainWindow::toggleSidebar(bool toggle)
{
	if (!toggle)
		toolsFrame->hide();
	else
		toolsFrame->show();
}

void MainWindow::setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones,
			  bool warn_black, bool warn_white, int timer_cnt)
{
	if (!btime.isEmpty())
	{
		if (bstones != QString("-1"))
			normalTools->btimeView->set_text(btime + " / " + bstones);
		else
			normalTools->btimeView->set_text(btime);
	}

	if (!wtime.isEmpty())
	{
		if (wstones != QString("-1"))
			normalTools->wtimeView->set_text(wtime + " / " + wstones);
		else
			normalTools->wtimeView->set_text(wtime);
	}

	// warn if I am within the last 10 seconds
	if (gfx_board->getGameMode() == modeMatch)
	{
		if (gfx_board->player_is (black))
		{
			normalTools->wtimeView->flash (false);
			normalTools->btimeView->flash (timer_cnt % 2 != 0 && warn_black);
			if (warn_black)
				qgo->playTimeSound();
		}
		else if (gfx_board->player_is (white) && warn_white)
		{
			normalTools->btimeView->flash (false);
			normalTools->wtimeView->flash (timer_cnt % 2 != 0 && warn_white);
			if (warn_white)
				qgo->playTimeSound();
		}
	} else {
		normalTools->btimeView->flash (false);
		normalTools->wtimeView->flash (false);
	}
}

void MainWindow::set_eval (double eval)
{
	m_eval = eval;
	int w = evalView->width ();
	int h = evalView->height ();
	m_eval_canvas->setSceneRect (0, 0, w, h);
	m_eval_bar->setRect (0, 0, w, h * eval);
	m_eval_bar->setPos (0, 0);
	m_eval_canvas->update ();
}

void MainWindow::set_eval (const QString &move, double eval, stone_color to_move, int visits)
{
	evalView->setBackgroundBrush (QBrush (Qt::white));
	m_eval_bar->setBrush (QBrush (Qt::black));
	set_eval (to_move == black ? eval : 1 - eval);

	int winrate_for = setting->readIntEntry ("ANALYSIS_WINRATE");
	stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;
	if (to_move == wr_swap_col)
		eval = 1 - eval;

	normalTools->primaryCoords->setText (move);
	normalTools->primaryWR->setText (QString::number (100 * eval, 'f', 1));
	normalTools->primaryVisits->setText (QString::number (visits));
	if (winrate_for == 0 || (winrate_for == 2 && to_move == black)) {
		normalTools->pWRLabel->setText (tr ("B Win %"));
		normalTools->sWRLabel->setText (tr ("B Win %"));
	} else {
		normalTools->pWRLabel->setText (tr ("W Win %"));
		normalTools->sWRLabel->setText (tr ("W Win %"));
	}
}

void MainWindow::set_2nd_eval (const QString &move, double eval, stone_color to_move, int visits)
{
	if (move.isEmpty ()) {
		normalTools->shownCoords->setText ("");
		normalTools->shownWR->setText ("");
		normalTools->shownVisits->setText ("");
	} else {
		int winrate_for = setting->readIntEntry ("ANALYSIS_WINRATE");
		stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;
		if (to_move == wr_swap_col)
			eval = 1 - eval;
		normalTools->shownCoords->setText (move);
		normalTools->shownWR->setText (QString::number (100 * eval, 'f', 1));
		normalTools->shownVisits->setText (QString::number (visits));
	}
}

void MainWindow::grey_eval_bar ()
{
	evalView->setBackgroundBrush (QBrush (Qt::lightGray));
	m_eval_bar->setBrush (QBrush (Qt::darkGray));
}
