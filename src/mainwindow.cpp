/*
* mainwindow.cpp - qGo's main window
*/

#include "qgo.h"
#include "common.h"

#include <fstream>
#include <sstream>
#include <cmath>

#include <QRegExp>
#include <QLabel>
#include <QPixmap>
#include <QWindow>
#include <QScreen>
#include <QCloseEvent>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>
#include <QWhatsThis>
#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QToolBar>
#include <QMessageBox>
#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QLineEdit>
#include <QTimer>
#include <QFontMetrics>
#include <QBuffer>
#include <QClipboard>

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
#include "newaigamedlg.h"
#include "ui_helpers.h"
#include "slideview.h"
#include "multisave.h"
#include "edit_analysis.h"
#include "patternsearch.h"
#include "gotools.h"

#include "ui_boardwindow_gui.h"

std::list<MainWindow *> main_window_list;

/* Return a string to identify the screen.  We use its dimensions.  */
QString screen_key (QWidget *w)
{
	QScreen *scr = nullptr;
	QWindow *win = w->windowHandle ();
	if (win != nullptr)
		scr = win->screen ();
	if (scr == nullptr) {
		scr = QApplication::primaryScreen ();
	}
	QSize sz = scr->size ();
	return QString::number (sz.width ()) + "x" + QString::number (sz.height ());
}

void an_id_model::populate_list (go_game_ptr game)
{
	beginResetModel ();
	m_entries.clear ();
	std::function<void (const analyzer_id &, bool)> f2 = [this] (const analyzer_id &id, bool score) -> void
		{
			bool found = false;
			for (auto &e: m_entries)
				if (e.first == id) {
					found = true;
					e.second |= score;
					break;
				}
			if (!found)
				m_entries.emplace_back (id, score);
		};
	std::function<bool (game_state *)> f = [&f2] (game_state *st) -> bool
		{
			st->collect_analyzers (f2);
			return true;
		};
	game_state *r = game->get_root ();
	r->walk_tree (f);
	endResetModel ();
}

/* This is called to notify us that an evaluation from NEW_ID came in.  */
void an_id_model::notice_analyzer_id (const analyzer_id &new_id, bool have_score)
{
	bool found = false;
	for (auto &e: m_entries) {
		if (e.first == new_id) {
			found = true;
			e.second |= have_score;
			break;
		}
	}
	if (!found) {
		beginInsertRows (QModelIndex (), m_entries.size (), m_entries.size ());
		m_entries.emplace_back (new_id, have_score);
		endInsertRows ();
	}
}

QVariant an_id_model::data (const QModelIndex &index, int role) const
{
	static Qt::GlobalColor colors[] = { Qt::blue, Qt::red, Qt::green, Qt::cyan, Qt::yellow, Qt::darkBlue, Qt::darkRed, Qt::darkGreen };
	int row = index.row ();
	int col = index.column ();
	if (row < 0 || (size_t)row >= m_entries.size () || col != 0)
		return QVariant ();
	if (role == Qt::DecorationRole) {
		return QColor (colors[row % 8]);
	}
	if (role != Qt::DisplayRole)
		return QVariant ();

	const analyzer_id &id = m_entries[row].first;
	if (!id.komi_set)
		return QString::fromStdString (id.engine);
	return QString::fromStdString (id.engine) + " @ " + QString::number (id.komi);
}

QModelIndex an_id_model::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

QModelIndex an_id_model::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

int an_id_model::rowCount (const QModelIndex &) const
{
	return m_entries.size ();
}

int an_id_model::columnCount (const QModelIndex &) const
{
	return 1;
}

QVariant an_id_model::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	switch (section) {
	case 0:
		return tr ("Engine");
	}
	return QVariant ();
}

bool an_id_model::removeRows (int row, int count, const QModelIndex &)
{
	if (row < 0 || count < 1)
		return false;
	size_t last = row + count;
	if (last > m_entries.size ())
		return false;
	beginRemoveRows (QModelIndex (), row, row + count - 1);
	auto beg = m_entries.begin ();
	m_entries.erase (beg + row, beg + row + count);
	endRemoveRows ();
	return true;
}

class undo_move_entry : public undo_entry
{
	game_state *m_parent;
	game_state *m_move;
	size_t m_idx;

public:
	undo_move_entry (const QString &op, game_state *parent, game_state *move, size_t idx)
		: undo_entry (op), m_parent (parent), m_move (move), m_idx (idx)
	{
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_insert_entry : public undo_entry
{
	game_state *m_parent;
	game_state *m_move;
	size_t m_idx;

public:
	undo_insert_entry (const QString &op, game_state *parent, game_state *move, size_t idx)
		: undo_entry (op), m_parent (parent), m_move (move), m_idx (idx)
	{
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_delete_entry : public undo_entry
{
	game_state *m_parent;
	game_state *m_move;
	size_t m_idx;

public:
	undo_delete_entry (const QString &op, game_state *parent, game_state *move, size_t idx)
		: undo_entry (op), m_parent (parent), m_move (move), m_idx (idx)
	{
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_info_entry : public undo_entry
{
	go_game_ptr m_game;
	game_info m_before, m_after;

public:
	undo_info_entry (const QString &op, go_game_ptr game, game_info oldi, game_info newi)
		: undo_entry (op), m_game (game), m_before (std::move (oldi)), m_after (std::move (newi))
	{
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_comment_entry : public undo_entry
{
	game_state *m_move;
	std::string m_before, m_after;

public:
	undo_comment_entry (const QString &op, game_state *move, std::string oldc, std::string newc)
		: undo_entry (op), m_move (move), m_before (std::move (oldc)), m_after (std::move (newc))
	{
	}
	void update (std::string new_newc)
	{
		m_after = std::move (new_newc);
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_edit_entry : public undo_entry
{
	game_state *m_move;
	go_board m_before, m_after;
	stone_color m_before_to_move, m_after_to_move;
	/* True if this edit was applied through the sidebar "Modify" button,
	   as opposed to smaller edits which just place marks.  */
	bool m_was_edit_mode;

public:
	undo_edit_entry (const QString &op, game_state *move,
			 go_board oldb, stone_color oldc, go_board newb, stone_color newc,
			 bool was_edit_mode)
		: undo_entry (op), m_move (move), m_before (std::move (oldb)), m_after (std::move (newb)),
		  m_before_to_move (oldc), m_after_to_move (newc), m_was_edit_mode (was_edit_mode)
	{
	}
	void update (go_board new_newb)
	{
		m_after = std::move (new_newb);
	}
	bool was_edit_mode () { return m_was_edit_mode; }
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

class undo_tomove_entry : public undo_entry
{
	game_state *m_move;
	stone_color m_before_to_move, m_after_to_move;

public:
	undo_tomove_entry (const QString &op, game_state *move, stone_color oldc, stone_color newc)
		: undo_entry (op), m_move (move), m_before_to_move (oldc), m_after_to_move (newc)
	{
	}
	virtual game_state *apply_undo () override;
	virtual game_state *apply_redo () override;
};

game_state *undo_tomove_entry::apply_undo ()
{
	m_move->set_to_move (m_before_to_move);
	return m_move;
}

game_state *undo_tomove_entry::apply_redo ()
{
	m_move->set_to_move (m_after_to_move);
	return m_move;
}

game_state *undo_move_entry::apply_undo ()
{
	m_move->disconnect ();
	return m_parent;
}

game_state *undo_move_entry::apply_redo ()
{
	m_parent->add_child_tree_at (m_move, m_idx);
	return m_move;
}

game_state *undo_insert_entry::apply_undo ()
{
#ifdef CHECKING
	assert (m_move->n_children () == 1);
#endif
	game_state *old_child = m_move->children ()[0];
	old_child->disconnect ();
	size_t idx = m_move->disconnect ();
#ifdef CHECKING
	assert (idx == m_idx);
#endif
	m_parent->add_child_tree_at (old_child, idx);
	return m_parent;
}

game_state *undo_insert_entry::apply_redo ()
{
	game_state *curr_at_pos = m_parent->children ()[m_idx];
	curr_at_pos->disconnect ();
	m_move->add_child_tree (curr_at_pos);
#ifdef CHECKING
	/* The inserted position should not be present in the game tree at this point.  */
	assert (m_move->prev_move () == nullptr);
#endif
	m_parent->add_child_tree_at (m_move, m_idx);
	return m_move;
}

game_state *undo_delete_entry::apply_undo ()
{
	m_parent->add_child_tree_at (m_move, m_idx);
	return m_move;
}

game_state *undo_delete_entry::apply_redo ()
{
	m_move->disconnect ();
	return m_parent;
}

game_state *undo_info_entry::apply_undo ()
{
	m_game->set_info (m_before);
	return nullptr;
}

game_state *undo_info_entry::apply_redo ()
{
	m_game->set_info (m_after);
	return nullptr;
}

game_state *undo_comment_entry::apply_undo ()
{
	m_move->set_comment (m_before);
	return m_move;
}

game_state *undo_comment_entry::apply_redo ()
{
	m_move->set_comment (m_after);
	return m_move;
}

game_state *undo_edit_entry::apply_undo ()
{
	m_move->replace (m_before, m_before_to_move);
	return m_move;
}

game_state *undo_edit_entry::apply_redo ()
{
	m_move->replace (m_after, m_after_to_move);
	return m_move;
}

MainWindow::MainWindow (QWidget* parent, go_game_ptr gr, const QString opener_scrkey, GameMode mode, time_settings ts)
	: QMainWindow (parent), ui (std::make_unique<Ui::BoardWindow> ()),
	  m_timer_white (ts), m_timer_black (ts),
	  m_game (gr), m_ascii_dlg (this), m_svg_dlg (this)
{
	ui->setupUi (this);

	ui->anIdListView->setModel (&m_an_id_model);
	ui->gameTreeView->set_board_win (this, ui->gtHeaderView);
	ui->evalGraph->set_board_win (this, &m_an_id_model);
	ui->gfx_board->set_board_win (this);
	ui->diagView->set_board_win (this);

	/* This needs to be set early, before calling setGameMode.  It is used in two places:
	   when setting the window caption through init_game_record, and when restoring the
	   initial default layout (the results of that aren't used, but we want to avoid
	   uninitialized reads).  */
	m_gamemode = mode;

	setAttribute (Qt::WA_DeleteOnClose);
	setDockNestingEnabled (true);

	isFullScreen = 0;
	setFocusPolicy(Qt::StrongFocus);

	const std::string &f = gr->filename ();
	if (!f.empty ()) {
		QFileInfo fi (QString::fromStdString (f));
		setting->writeEntry ("LAST_DIR", fi.dir ().absolutePath ());
	}

	local_stone_sound = setting->readBoolEntry(mode == modeMatch ? "SOUND_MATCH_BOARD"
						   : mode == modeObserve || mode == modeObserveGTP || mode == modeObserveMulti ? "SOUND_OBSERVE"
						   : mode == modeComputer ? "SOUND_COMPUTER"
						   : "SOUND_NORMAL");

	int game_style = m_game->info ().style;
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

	ui->autoBox->setVisible (false);
	ui->autoplayToolButton->setDefaultAction (ui->navAutoplay);
	ui->navAutoplay->setVisible (mode == modeNormal);
	connect (ui->autoSlider, &QSlider::valueChanged, this, &MainWindow::autoSliderChanged);
	autoSliderChanged (ui->autoSlider->value ());

	initActions ();
	initMenuBar(mode);
	initToolBar();
	initStatusBar();

	/* Only ever shown if this is opened through "Edit Game" in an observed online game.  */
	ui->refreshButton->setVisible (false);

	ui->viewStatusBar->setChecked (setting->readBoolEntry("STATUSBAR"));
	ui->viewMenuBar->setChecked (setting->readBoolEntry("MENUBAR"));

	if (setting->readBoolEntry("SIDEBAR_LEFT"))
		slotViewLeftSidebar ();

	ui->commentEdit->setWordWrapMode(QTextOption::WordWrap);
	ui->diagCommentView->setWordWrapMode(QTextOption::WordWrap);
	ui->diagCommentView->setVisible (false);

	ui->commentEdit->addAction (escapeFocus);
	ui->commentEdit2->addAction (escapeFocus);

	ui->normalTools->show();
	ui->scoreTools->hide();
	ui->againButton->hide ();
	ui->unobserveButton->hide ();

	showSlider = setting->readBoolEntry ("SLIDER");
	ui->sliderWidget->setVisible (showSlider);
	ui->slider->setMaximum (SLIDER_INIT);
	ui->sliderRightLabel->setText (QString::number (SLIDER_INIT));
	sliderSignalToggle = true;

	int w = ui->evalView->width ();
	int h = ui->evalView->height ();

	m_eval_canvas = new QGraphicsScene (0, 0, w, h, ui->evalView);
	ui->evalView->setScene (m_eval_canvas);
	m_eval_bar = m_eval_canvas->addRect (QRectF (0, 0, w, h / 2), Qt::NoPen, QBrush (Qt::black));
	m_eval_mid = m_eval_canvas->addRect (QRectF (0, (h - 1) / 2, w, 2), Qt::NoPen, QBrush (Qt::red));
	m_eval_mid->setZValue (10);

	m_eval = 0.5;

	connect (ui->evalView, &SizeGraphicsView::resized, this, [=] () { set_eval_bar (m_eval); });
	connect (ui->anIdListView, &ClickableListView::current_changed, [this] () { update_game_tree (); });

	connect (ui->passButton, &QPushButton::clicked, this, &MainWindow::doPass);
	connect (ui->doneButton, &QPushButton::clicked, this, &MainWindow::doCountDone);
	connect (ui->leaveMatchButton, &QPushButton::clicked,
		 [this] (bool) {
			 m_remember_mode = modeNormal;
			 if (m_gamemode == modePostMatch)
				 setGameMode (modeNormal);
			 remove_connector ();
		 });
	connect (ui->editAppendButton, &QPushButton::clicked, [this] (bool) { leave_edit_append (); setGameMode (m_remember_mode); });
	connect (ui->editReplaceButton, &QPushButton::clicked, [this] (bool) { leave_edit_modify (); setGameMode (m_remember_mode); });
	connect (ui->editInsertButton, &QPushButton::clicked, [this] (bool) { leave_edit_prepend (); setGameMode (m_remember_mode); });

	connect (ui->editButton, &QPushButton::clicked, this, &MainWindow::doEdit);
	connect (ui->editPosButton, &QPushButton::clicked, this, &MainWindow::doEditPos);
	connect (ui->slider, &QSlider::valueChanged, this, &MainWindow::sliderChanged);
	connect (ui->scoreButton, &QPushButton::clicked, this, &MainWindow::doRealScore);

	ui->goLastButton->setDefaultAction (ui->navLast);
	ui->goFirstButton->setDefaultAction (ui->navFirst);
	ui->goNextButton->setDefaultAction (ui->navForward);
	ui->goPrevButton->setDefaultAction (ui->navBackward);
	ui->prevNumberButton->setDefaultAction (ui->navPrevFigure);
	ui->nextNumberButton->setDefaultAction (ui->navNextFigure);
	ui->prevCommentButton->setDefaultAction (ui->navPrevComment);
	ui->nextCommentButton->setDefaultAction (ui->navNextComment);

	scoreGroup = new QButtonGroup (this);
	scoreGroup->addButton (ui->scoreTools->scoreAreaButton);
	scoreGroup->addButton (ui->scoreTools->scoreTerrButton);
	connect (ui->scoreTools->scoreAreaButton, &QRadioButton::toggled, [this] (bool) { update_score_type (); });
	connect (ui->scoreTools->scoreTerrButton, &QRadioButton::toggled, [this] (bool) { update_score_type (); });

	connect(ui->diagEditButton, &QToolButton::clicked, this, &MainWindow::slotDiagEdit);
	connect(ui->diagASCIIButton, &QToolButton::clicked, this, &MainWindow::slotDiagASCII);
	connect(ui->diagSVGButton, &QToolButton::clicked, this, &MainWindow::slotDiagSVG);
	connect(ui->diagComboBox, combo_activated_int, this, &MainWindow::slotDiagChosen);

	connect(ui->normalTools->anStartButton, &QToolButton::clicked,
		[=] (bool on) { if (on) start_analysis (); else ui->gfx_board->stop_analysis (); });
	connect(ui->normalTools->anPauseButton, &QToolButton::clicked,
		[=] (bool on) { ui->anPause->setChecked (on); });
	connect(ui->normalTools->anHideButton, &QToolButton::toggled, this, &MainWindow::slotToggleHideAnalysis);

	connect(m_ascii_dlg.cb_coords, &QCheckBox::toggled, [=] (bool) { update_ascii_dialog (); });
	connect(m_ascii_dlg.cb_numbering, &QCheckBox::toggled, [=] (bool) { update_ascii_dialog (); });
	connect(m_ascii_dlg.targetComboBox, combo_activated_int, [=] (int) { update_ascii_dialog (); });
	connect(m_svg_dlg.cb_coords, &QCheckBox::toggled, [=] (bool) { update_svg_dialog (); });
	connect(m_svg_dlg.cb_numbering, &QCheckBox::toggled, [=] (bool) { update_svg_dialog (); });
	/* These don't do anything that toggling the checkboxes wouldn't also do, but it's slightly more
	   intuitive to have a button for it.  */
	connect(m_ascii_dlg.buttonRefresh, &QPushButton::clicked, [=] (bool) { update_ascii_dialog (); });
	connect(m_svg_dlg.buttonRefresh, &QPushButton::clicked, [=] (bool) { update_svg_dialog (); });

	timer = new QTimer (this);
	connect (timer, &QTimer::timeout, this, &MainWindow::slotTimerForward);
	if (mode == modeNormal) {
		autoplay_timer = new QTimer (this);
		connect (autoplay_timer, &QTimer::timeout, this, &MainWindow::slotAutoplayTimer);
	}
	if (ts.system != time_system::none)
		timer->start (100);

	ui->toolBar->setFocus ();

	init_game_record (gr);

#if 1
	/* This is a hack.  We appear to be hitting Qt bugs 70571/65592, where dock sizes snap back
	   violently as soon as the user moves the mouse over the board window.
	   There is a thread at
	     https://forum.qt.io/topic/94473/qdockwidget-resize-issue/16
	   where someone seems to have the same issue, also involving a QGraphicsView, and the
	   following is an adpatation of the suggested workaround.
	   This should be removed once we can assume Qt 5.12 and the bug is indeed fixed.  */
	resizeDocks ({ui->diagsDock, ui->treeDock, ui->commentsDock, ui->observersDock, ui->graphDock}, { 0, 0, 0, 0, 0 }, Qt::Horizontal);
	resizeDocks ({ui->diagsDock, ui->treeDock, ui->commentsDock, ui->observersDock, ui->graphDock}, { 0, 0, 0, 0, 0 }, Qt::Vertical);
#endif
	/* Order of operations here: restore a default layout if the user saved one.  We
	   need to have the game mode variable set for this.
	   Then, choose visibility defaults for the docks.
	   Then, do a proper setGameMode, to hide all panes that should not be visible.
	   Finally, restore the specific layout if one was saved.  */
	restoreWindowLayout (true, opener_scrkey);

	int figuremode = setting->readIntEntry ("BOARD_DIAGMODE");
	if (figuremode == 1 && m_game->get_root ()->has_figure_recursive ())
		figuremode = 2;
	ui->diagsDock->setVisible (figuremode == 2 || mode == modeBatch);
	ui->graphDock->setVisible (figuremode == 2 || mode == modeBatch || mode == modeComputer || mode == modeObserveGTP);
	if (mode == modeMatch || mode == modeTeach || mode == modeObserve)
		ui->observersDock->setVisible (true);
	ui->chooseDock->setVisible (mode == modeObserveMulti);
	setGameMode (mode);

	update_settings ();

	restoreWindowLayout (false, opener_scrkey);

	update_view_menu ();

	connect(ui->commentEdit, &QTextEdit::textChanged, this, &MainWindow::slotUpdateComment);
	connect(ui->commentEdit2, &QLineEdit::returnPressed, this, &MainWindow::slotUpdateComment2);
	m_allow_text_update_signal = true;

	m_default_text_color = ui->commentEdit->textColor ();

	update_analysis (analyzer::disconnected);
	main_window_list.push_back (this);

	connect (db_data_controller, &GameDB_Data_Controller::signal_prepare_load,
		 this, &MainWindow::disable_search_pattern);
	connect (db_data, &GameDB_Data::signal_load_complete,
		 this, &MainWindow::enable_search_pattern);
	ui->searchPattern->setEnabled (db_data->load_complete);
	ui->searchPattern->setVisible (m_gamemode != modeMatch && !setting->m_dbpaths.isEmpty ());
	connect (ui->searchPattern, &QAction::triggered, this, &MainWindow::perform_search);
}

Board *MainWindow::getBoard () const
{
	return ui->gfx_board;
}

void MainWindow::disable_search_pattern ()
{
	ui->searchPattern->setEnabled (false);
}

void MainWindow::enable_search_pattern ()
{
	ui->searchPattern->setEnabled (true);
}

void MainWindow::enable_pb_controls (bool on)
{
	ui->pb_controls->setEnabled (on);
}

void MainWindow::check_pb_controls (bool on)
{
	ui->pb_controls->setChecked (on);
}

void MainWindow::update_rules (go_game_ptr gr)
{
	go_rules r = go_rules::tt;
	go_rules guess = guess_rules (gr->info ());
	QString guess_str = rules_name (guess);
	ui->rulesGuess->setText (tr ("Best guess from SGF: %1").arg (guess_str));
	if (ui->rulesChinese->isChecked ())
		r = go_rules::chinese;
	else if (ui->rulesJapanese->isChecked ())
		r = go_rules::japanese;
	else
		r = guess;
	ui->gfx_board->set_rules (r);
}

void MainWindow::init_game_record (go_game_ptr gr)
{
	if (m_game != nullptr && !gr->same_size (*m_game))
		ui->gfx_board->stop_analysis ();

	m_undo_stack.clear ();
	m_undo_stack_pos = 0;
	update_undo_menus ();

	m_svg_dlg.hide ();
	m_ascii_dlg.hide ();

	m_game = gr;
	game_state *root = gr->get_root ();
	const go_board b (root->get_board (), none);
	m_empty_state = gr->create_game_state (b, black);

	/* The order actually matters here.  The gfx_board will call back into MainWindow
	   to update figures, which means we need to have ui->diagView set up.  */
	ui->diagView->reset_game (gr);
	ui->gfx_board->reset_game (gr);
	if (slideView != nullptr)
		slideView->set_game (gr);
	m_an_id_model.populate_list (gr);
	if (m_an_id_model.rowCount () > 0)
		ui->anIdListView->setCurrentIndex (m_an_id_model.index (0, 0));
	ui->anIdListView->setVisible (m_an_id_model.rowCount () > 1);

	updateCaption ();
	update_game_record ();

	bool disable_rect = (b.torus_h () || b.torus_v ()) && setting->readIntEntry ("TOROID_DUPS") > 0;
	ui->editRectSelect->setEnabled (!disable_rect);

	int figuremode = setting->readIntEntry ("BOARD_DIAGMODE");
	if (!ui->diagsDock->isVisible () && figuremode == 1 && root->has_figure_recursive ()) {
		ui->diagsDock->show ();
	}
}

void MainWindow::update_game_record ()
{
	update_rules (m_game);
	ranked r = m_game->info ().rated;
	if (r == ranked::unknown) {
		ui->normalTools->byoWidget->hide ();
		ui->normalTools->TextLabel_free->hide ();
	} else {
		ui->normalTools->byoWidget->show ();
		ui->normalTools->TextLabel_free->show ();
		if (r == ranked::free)
			ui->normalTools->TextLabel_free->setText (tr ("free"));
		else if (r == ranked::ranked)
			ui->normalTools->TextLabel_free->setText (tr ("rated"));
		else
			ui->normalTools->TextLabel_free->setText (tr ("teach"));
	}
	ui->normalTools->komi->setText (QString::number (m_game->info ().komi));
	ui->scoreTools->komi->setText (QString::number (m_game->info ().komi));
	ui->normalTools->handicap->setText (QString::number (m_game->info ().handicap));
	updateCaption ();
}

MainWindow::~MainWindow()
{
	main_window_list.remove (this);

	delete slideView;

	for (auto it: engine_actions)
		delete it;

	delete timer;
	delete scoreGroup;

	// status bar
	delete statusMode;
	delete statusNav;
	delete statusTurn;
	delete statusCoords;

	// Actions
	delete escapeFocus;
	delete editGroup;
	delete rulesGroup;
	delete viewNumsGroup;
	delete navSwapVariations;

	delete whatsThis;
}

void MainWindow::set_displayed (game_state *st)
{
	ui->gfx_board->set_displayed (st);
}

void MainWindow::nav_next_move ()
{
	game_state *st = ui->gfx_board->displayed ();
	game_state *next = st->next_move ();
	if (next == nullptr)
		return;
	set_displayed (next);
}

void MainWindow::nav_previous_move ()
{
	game_state *st = ui->gfx_board->displayed ();
	game_state *next = st->prev_move ();
	if (next == nullptr)
		return;
	set_displayed (next);
}

void MainWindow::nav_previous_comment ()
{
	game_state *st = ui->gfx_board->displayed ();
	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->comment () != "") {
			set_displayed (st);
			break;
		}
	}
}

void MainWindow::nav_next_comment ()
{
	game_state *st = ui->gfx_board->displayed ();
	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->comment () != "") {
			set_displayed (st);
			break;
		}
	}
}

void MainWindow::nav_previous_figure ()
{
	game_state *st = ui->gfx_board->displayed ();
	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->has_figure ()) {
			set_displayed (st);
			break;
		}
	}
}

void MainWindow::nav_next_figure ()
{
	game_state *st = ui->gfx_board->displayed ();
	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->has_figure ()) {
			set_displayed (st);
			break;
		}
	}
}

void MainWindow::nav_next_variation ()
{
	game_state *st = ui->gfx_board->displayed ();
	game_state *next = st->next_sibling (true);
	if (next == nullptr)
		return;
	if (next != st)
		set_displayed (next);
}

void MainWindow::nav_previous_variation ()
{
	game_state *st = ui->gfx_board->displayed ();
	game_state *next = st->prev_sibling (true);
	if (next == nullptr)
		return;
	if (next != st)
		set_displayed (next);
}

void MainWindow::nav_goto_first_move ()
{
	set_displayed (m_game->get_root ());
}

void MainWindow::nav_goto_last_move ()
{
	game_state *st_orig = ui->gfx_board->displayed ();
	game_state *st = st_orig;
	for (;;) {
		game_state *next = st->next_move ();
		if (next == nullptr) {
			if (st != st_orig)
				set_displayed (st);
			break;
		}
		st = next;
	}
}

void MainWindow::nav_find_move (int x, int y)
{
	game_state *st = ui->gfx_board->displayed ();
	if (!st->was_move_p () || st->get_move_x () != x || st->get_move_y () != y)
		st = m_game->get_root ();

	for (;;) {
		st = st->next_move ();
		if (st == nullptr)
			return;
		if (st->was_move_p () && st->get_move_x () == x && st->get_move_y () == y)
			break;
	}
	set_displayed (st);
}

void MainWindow::nav_goto_nth_move (int n)
{
	game_state *st = m_game->get_root ();
	while (st->move_number () < n && st->n_children () > 0) {
		st = st->next_move (true);
	}
	set_displayed (st);
}

void MainWindow::nav_goto_nth_move_in_var (int n)
{
	game_state *st = ui->gfx_board->displayed ();

	while (st->move_number () != n) {
		game_state *next = st->move_number () < n ? st->next_move () : st->prev_move ();
		/* Some added safety... this should not happen.  */
		if (next == nullptr)
			break;
		st = next;
	}
	set_displayed (st);
}

void MainWindow::nav_goto_main_branch ()
{
	game_state *st = ui->gfx_board->displayed ();
	game_state *go_to = st;
	while (st != nullptr) {
		while (st->has_prev_sibling ())
			st = go_to = st->prev_sibling (true);
		st = st->prev_move ();
	}
	if (go_to != st)
		set_displayed (go_to);
}

void MainWindow::nav_goto_var_start ()
{
	game_state *st = ui->gfx_board->displayed ();
	st = st->prev_move ();
	if (st == nullptr)
		return;

	/* Walk back, ending either at the root or at a node which has
	   more than one sibling.  */
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *prev = st->prev_move ();
		if (prev == nullptr)
			break;
		st = prev;
	}
	set_displayed (st);
}

void MainWindow::nav_goto_next_branch ()
{
	game_state *st_orig = ui->gfx_board->displayed ();
	game_state *st = st_orig;
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *next = st->next_move ();
		if (next == nullptr)
			break;
		st = next;
	}
	if (st != st_orig)
		set_displayed (st);
}

void MainWindow::initActions ()
{
	// Escape focus: Escape key to get the focus from comment field to main window.
 	escapeFocus = new QAction(this);
	escapeFocus->setShortcut(Qt::Key_Escape);
	connect (escapeFocus, &QAction::triggered, this, &MainWindow::slotFocus);

	/* File menu.  */
	connect (ui->fileNewBoard, &QAction::triggered, this, &MainWindow::slotFileNewBoard);
	connect (ui->fileNew, &QAction::triggered, this, &MainWindow::slotFileNewGame);
	connect (ui->fileNewVariant, &QAction::triggered, this, &MainWindow::slotFileNewVariantGame);
	connect (ui->fileOpen, &QAction::triggered, this, &MainWindow::slotFileOpen);
	connect (ui->fileOpenDB, &QAction::triggered, this, &MainWindow::slotFileOpenDB);
	connect (ui->fileSave, &QAction::triggered, this, &MainWindow::slotFileSave);
	connect (ui->fileSaveAs, &QAction::triggered, this, &MainWindow::slotFileSaveAs);
	connect (ui->fileSaveVariations, &QAction::triggered, this, &MainWindow::slotFileSaveVariations);
	connect (ui->fileClose, &QAction::triggered, this, &MainWindow::slotFileClose);
	connect (ui->fileQuit, &QAction::triggered, this, &MainWindow::slotFileClose);//(qGo*)qApp, SLOT(quit);

	connect (ui->fileExportASCII, &QAction::triggered, this, &MainWindow::slotFileExportASCII);
	connect (ui->fileExportSVG, &QAction::triggered, this, &MainWindow::slotFileExportSVG);
	connect (ui->fileImportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileImportSgfClipB);
	connect (ui->fileExportSgfClipB, &QAction::triggered, this, &MainWindow::slotFileExportSgfClipB);
	connect (ui->fileExportPic, &QAction::triggered, this, &MainWindow::slotFileExportPic);
	connect (ui->fileExportPicClipB, &QAction::triggered, this, &MainWindow::slotFileExportPicClipB);
	connect (ui->fileExportSlide, &QAction::triggered, [this] (bool)
		{
			if (slideView == nullptr) {
				slideView = new SlideView (this);
				slideView->set_game (m_game);
				slideView->set_active (ui->gfx_board->displayed ());
			}
			slideView->show ();
		});

	/* Edit menu.  */
	connect (ui->editCopyPos, &QAction::triggered, this, &MainWindow::slotEditCopyPos);
	connect (ui->editPastePos, &QAction::triggered, this, &MainWindow::slotEditPastePos);
	connect (ui->editDelete, &QAction::triggered, this, &MainWindow::slotEditDelete);
	connect (ui->setGameInfo, &QAction::triggered, this, &MainWindow::slotSetGameInfo);

	connect (ui->editStone, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editTriangle, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editCircle, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editCross, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editSquare, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editNumber, &QAction::triggered, this, &MainWindow::slotEditGroup);
	connect (ui->editLetter, &QAction::triggered, this, &MainWindow::slotEditGroup);

	editGroup = new QActionGroup (this);
	editGroup->addAction (ui->editStone);
	editGroup->addAction (ui->editCircle);
	editGroup->addAction (ui->editTriangle);
	editGroup->addAction (ui->editSquare);
	editGroup->addAction (ui->editCross);
	editGroup->addAction (ui->editNumber);
	editGroup->addAction (ui->editLetter);
	ui->editStone->setChecked (true);

	connect (ui->rulesGuess, &QAction::triggered, [this] () { update_rules (m_game); });
	connect (ui->rulesJapanese, &QAction::triggered, [this] () { update_rules (m_game); });
	connect (ui->rulesChinese, &QAction::triggered, [this] () { update_rules (m_game); });

	rulesGroup = new QActionGroup (this);
	rulesGroup->addAction (ui->rulesGuess);
	rulesGroup->addAction (ui->rulesChinese);
	rulesGroup->addAction (ui->rulesJapanese);
	ui->rulesGuess->setChecked (true);

	engineGroup = new QActionGroup (this);

	connect (ui->editUndo, &QAction::triggered, [=] () { perform_undo (); });
	connect (ui->editRedo, &QAction::triggered, [=] () { perform_redo (); });
	connect (ui->editFigure, &QAction::triggered, this, &MainWindow::slotEditFigure);
	connect (ui->editRectSelect, &QAction::toggled, this, &MainWindow::slotEditRectSelect);
	connect (ui->editClearSelect, &QAction::triggered, this, &MainWindow::slotEditClearSelect);

	connect (ui->editAutoDiags, &QAction::triggered,
		 [this] (bool)
		 {
			 AutoDiagsDialog dlg (m_game, this);
			 if (dlg.exec ()) {
				 if (!ui->diagsDock->isVisible ()) {
					 ui->diagsDock->setVisible (true);
					 restoreWindowLayout (false);
				 }
				 update_game_tree ();
				 update_figures ();
			 }
		 });

	/* Navigation menu.  */
	connect (ui->navBackward, &QAction::triggered, [=] () { nav_previous_move (); });
	connect (ui->navForward, &QAction::triggered, [=] () { nav_next_move (); });
	connect (ui->navFirst, &QAction::triggered, [=] () { nav_goto_first_move (); });
	connect (ui->navLast, &QAction::triggered, [=] () { nav_goto_last_move (); });
	connect (ui->navPrevVar, &QAction::triggered, [=] () { nav_previous_variation (); });
	connect (ui->navNextVar, &QAction::triggered, [=] () { nav_next_variation (); });
	connect (ui->navMainBranch, &QAction::triggered, [=] () { nav_goto_main_branch (); });
	connect (ui->navStartVar, &QAction::triggered, [=] () { nav_goto_var_start (); });
	connect (ui->navNextBranch, &QAction::triggered, [=] () { nav_goto_next_branch (); });
	connect (ui->navPrevComment, &QAction::triggered, [=] () { nav_previous_comment (); });
	connect (ui->navNextComment, &QAction::triggered, [=] () { nav_next_comment (); });
	connect (ui->navPrevFigure, &QAction::triggered, [=] () { nav_previous_figure (); });
	connect (ui->navNextFigure, &QAction::triggered, [=] () { nav_next_figure (); });

	connect (ui->navNthMove, &QAction::triggered, this, &MainWindow::slotNavNthMove);
	connect (ui->navIntersection, &QAction::triggered, this, &MainWindow::slotNavIntersection);

	connect (ui->navAutoplay, &QAction::toggled, this, &MainWindow::slotNavAutoplay);

	navSwapVariations = new QAction(tr("S&wap variations"), this);
	navSwapVariations->setStatusTip(tr("Swap current move with previous variation"));
	navSwapVariations->setWhatsThis(tr("Swap variations\n\nSwap current move with previous variation."));
	connect (navSwapVariations, &QAction::triggered, this, &MainWindow::slotNavSwapVariations);

	/* Settings menu.  */
	connect (ui->setPreferences, &QAction::triggered, client_window, &ClientWindow::slot_preferences);
	connect (ui->soundToggle, &QAction::toggled, this, &MainWindow::slotSoundToggle);
	ui->soundToggle->setChecked(!local_stone_sound);

	/* View menu.  */
	viewNumsGroup = new QActionGroup (this);
	viewNumsGroup->addAction (ui->viewNumbersOff);
	viewNumsGroup->addAction (ui->viewNumbersVarsLong);
	viewNumsGroup->addAction (ui->viewNumbersVarsShort);
	viewNumsGroup->addAction (ui->viewNumbersFull);
	ui->viewNumbersOff->setChecked (true);

	connect (ui->viewMenuBar, &QAction::toggled, this, &MainWindow::slotViewMenuBar);
	connect (ui->viewStatusBar, &QAction::toggled, this, &MainWindow::slotViewStatusBar);
	connect (ui->viewCoords, &QAction::toggled, this, &MainWindow::slotViewCoords);
	connect (ui->viewSlider, &QAction::toggled, this, &MainWindow::slotViewSlider);
	connect (ui->viewSidebar, &QAction::toggled, this, &MainWindow::slotViewSidebar);
	connect (ui->layoutSaveDefault, &QAction::triggered, this, [=] () { saveWindowLayout (true); });
	connect (ui->layoutRestoreDefault, &QAction::triggered, this, [=] () { restoreWindowLayout (true); });
	connect (ui->layoutSaveCurrent, &QAction::triggered, this, [=] () { saveWindowLayout (false); });
	connect (ui->layoutRestoreCurrent, &QAction::triggered, this, [=] () { restoreWindowLayout (false); });
	connect (ui->layoutPortrait, &QAction::triggered, this, [=] () { defaultPortraitLayout (); });
	connect (ui->layoutLandscape, &QAction::triggered, this, [=] () { defaultLandscapeLayout (); });
	connect (ui->viewFullscreen, &QAction::toggled, this, &MainWindow::slotViewFullscreen);
	connect (ui->viewNumbersFull, &QAction::toggled, this, &MainWindow::slotViewMoveNumbers);
	connect (ui->viewNumbersVarsLong, &QAction::toggled, this, &MainWindow::slotViewMoveNumbers);
	connect (ui->viewNumbersVarsShort, &QAction::toggled, this, &MainWindow::slotViewMoveNumbers);
	connect (ui->viewNumbersOff, &QAction::toggled, this, &MainWindow::slotViewMoveNumbers);
	connect (ui->viewDiagComments, &QAction::toggled, this, &MainWindow::slotViewDiagComments);
	connect (ui->viewConnections, &QAction::toggled, this, &MainWindow::slotViewConnections);

	/* Analyze menu.  */
	connect (ui->anConnect, &QAction::triggered, this, [=] () { start_analysis (); });
	connect (ui->anPause, &QAction::toggled, this, [=] (bool on) { if (on) { grey_eval_bar (); } ui->gfx_board->pause_analysis (on); });
	connect (ui->anDisconnect, &QAction::triggered, this, [=] () { ui->gfx_board->stop_analysis (); });
	connect (ui->anBatch, &QAction::triggered, [] (bool) { show_batch_analysis (); });
	connect (ui->anMessages, &QAction::triggered, [this] (bool) { ui->gfx_board->show_analyzer_dialog (); });
	connect (ui->anPlay, &QAction::triggered, this, &MainWindow::slotPlayFromHere);
	connect (ui->anEdit, &QAction::triggered, this, &MainWindow::slotEditAnalysis);

	/* Help menu.  */
	connect (ui->helpManual, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("index.html")); });
	connect (ui->helpReadme, &QAction::triggered, [=] (bool) { qgo->openManual (QUrl ("readme.html")); });
	/* There isn't actually a manual.  Well, there is, but it's outdated and we don't ship it.  */
	ui->helpManual->setVisible (false);
	ui->helpManual->setEnabled (false);
	connect (ui->helpAbout, &QAction::triggered, [=] (bool) { help_about (); });
	connect (ui->helpAboutQt, &QAction::triggered, [=] (bool) { QMessageBox::aboutQt (this); });
	whatsThis = QWhatsThis::createAction (this);

	connect (ui->actionTutorials, &QAction::triggered, [] () { show_tutorials (); });

	// Disable some toolbuttons at startup
	ui->navAutoplay->setEnabled (false);
	ui->navForward->setEnabled (false);
	ui->navBackward->setEnabled (false);
	ui->navFirst->setEnabled (false);
	ui->navLast->setEnabled (false);
	ui->navPrevVar->setEnabled (false);
	ui->navNextVar->setEnabled (false);
	ui->navMainBranch->setEnabled (false);
	ui->navStartVar->setEnabled (false);
	ui->navNextBranch->setEnabled (false);
	navSwapVariations->setEnabled (false);
	ui->navPrevComment->setEnabled (false);
	ui->navNextComment->setEnabled (false);
	ui->navNextFigure->setEnabled (false);
	ui->navPrevFigure->setEnabled (false);
	ui->navIntersection->setEnabled (false);

	ui->anMessages->setEnabled (false);

	/* Need actions with shortcuts added to the window, so that the shortcut still works
	   if the menubar is hidden.  */
	addActions ({ ui->fileNewBoard, ui->fileNew, ui->fileOpen, ui->fileSave, ui->fileSaveAs, ui->fileClose, ui->fileQuit });
	addActions ({ ui->navForward, ui->navBackward, ui->navFirst, ui->navLast, ui->navPrevVar, ui->navNextVar });
	addActions ({ ui->setGameInfo, ui->editDelete, ui->editFigure, ui->editRectSelect, ui->editClearSelect });
	addActions ({ ui->navMainBranch, ui->navStartVar, ui->navNextBranch, ui->navNthMove });
	addActions ({ ui->setPreferences, ui->soundToggle });
	addActions ({ ui->viewMenuBar, ui->viewSidebar, ui->viewCoords, ui->viewFullscreen });
	addActions ({ ui->helpManual, whatsThis });
}

void MainWindow::initMenuBar (GameMode mode)
{
	QAction *view_first = ui->viewMenu->actions().at(0);

	ui->viewMenu->insertAction (view_first, ui->fileBar->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->toolBar->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->editBar->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->miscBar->toggleViewAction ());
	ui->viewMenu->insertSeparator(view_first);
	ui->viewMenu->insertAction (view_first, ui->commentsDock->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->observersDock->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->diagsDock->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->graphDock->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->treeDock->toggleViewAction ());
	ui->viewMenu->insertAction (view_first, ui->chooseDock->toggleViewAction ());

	ui->helpMenu->addSeparator ();
	ui->helpMenu->addAction (whatsThis);

	ui->anMenu->setVisible (mode == modeNormal || mode == modeObserve);

	populate_engines_menu ();
}

void MainWindow::initToolBar()
{
	QToolButton *menu_button = new QToolButton (this);
	menu_button->setPopupMode(QToolButton::InstantPopup);
	menu_button->setIcon (QIcon (":/BoardWindow/images/boardwindow/eye.png"));
	menu_button->setMenu (ui->viewMenu);
	menu_button->setToolTip ("View menu");
	ui->miscBar->addWidget (menu_button);

	ui->miscBar->addSeparator();
	ui->miscBar->addAction (whatsThis);
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
	statusMode->setWhatsThis(tr("Mode\nShows the current mode. 'N' for normal mode, 'E' for edit mode."));
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
	else if (m_gamemode == modeObserveMulti || m_gamemode == modeObserve)
		s += tr ("Observe: ");
	else if (ui->refreshButton->isVisibleTo (this))
		s += tr ("Off-line copy: ");

	if (game_number != 0)
		s += "(" + QString::number(game_number) + ") ";
	QString title = QString::fromStdString (m_game->info ().title);
	QString player_w = QString::fromStdString (m_game->info ().name_w);
	QString player_b = QString::fromStdString (m_game->info ().name_b);
	QString rank_w = QString::fromStdString (m_game->info ().rank_w);
	QString rank_b = QString::fromStdString (m_game->info ().rank_b);

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

	QString override;
	/* Override with fixed titles for streaming.  */
	if (m_gamemode == modeMatch || m_gamemode == modeTeach)
		override = setting->readEntry ("WTITLE_MATCH");
	else if (m_gamemode == modeComputer || m_gamemode == modeObserveGTP)
		override = setting->readEntry ("WTITLE_ENGINE");
	else if (m_gamemode == modeObserve || m_gamemode == modeObserveMulti)
		override = setting->readEntry ("WTITLE_OBSERVE");

	if (!override.isEmpty ())
		setWindowTitle (override);
	else
		setWindowTitle (s);

	rank_w.truncate (5);
	int rwlen = rank_w.length ();
	player_w.truncate(15 - rwlen);
	if (rwlen > 0)
		player_w += " " + rank_w;
	ui->normalTools->whiteLabel->setText(player_w);
	ui->scoreTools->whiteLabel->setText(player_w);

	rank_b.truncate (5);
	int rblen = rank_b.length ();
	player_b.truncate(15 - rblen);
	if (rblen > 0)
		player_b += " " + rank_b;
	ui->normalTools->blackLabel->setText(player_b);
	ui->scoreTools->blackLabel->setText(player_b);
}

void MainWindow::update_game_tree ()
{
	game_state *st = ui->gfx_board->displayed ();
	ui->gameTreeView->update (m_game, st);
	if (slideView != nullptr)
		slideView->set_active (st);
	QModelIndex idx = ui->anIdListView->currentIndex ();
	if (idx.isValid ()) {
		ui->gfx_board->set_analyzer_id (m_an_id_model.entries ()[idx.row ()].first);
	}
	ui->evalGraph->update (m_game, st, idx.isValid () ? idx.row () : 0);
}

void MainWindow::slotFileNewBoard (bool)
{
	open_local_board (client_window, game_dialog_type::none, screen_key (this));
}

void MainWindow::slotFileNewGame (bool)
{
	if (!checkModified())
		return;

	go_game_ptr gr = new_game_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);
	populate_engines_menu ();

	statusBar()->showMessage(tr("New board prepared."));
}

void MainWindow::slotFileNewVariantGame (bool)
{
	if (!checkModified())
		return;

	go_game_ptr gr = new_variant_game_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);
	populate_engines_menu ();

	statusBar()->showMessage(tr("New board prepared."));
}

void MainWindow::slotFileOpen (bool)
{
	if (!checkModified())
		return;

	go_game_ptr gr = open_file_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);
	populate_engines_menu ();
}

void MainWindow::slotFileOpenDB (bool)
{
	if (!checkModified())
		return;

	go_game_ptr gr = open_db_dialog (this);
	if (gr == nullptr)
		return;

	init_game_record (gr);
	setGameMode (modeNormal);
	populate_engines_menu ();
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

void MainWindow::slotFileSaveVariations (bool)
{
	MultiSaveDialog msd (this, m_game, ui->gfx_board->displayed ());
	msd.exec ();
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
	bool isdir = !fileName.isEmpty () && QDir(fileName).exists();
	if (isdir)
		fileName = get_candidate_filename (fileName, m_game->info ());
	else if (fileName.isEmpty()) {
		QString dir = setting->readEntry("LAST_DIR");
		fileName = get_candidate_filename (dir, m_game->info ());
	}

	if (!force) {
		fileName = QFileDialog::getSaveFileName (this, tr ("Save SGF file"),
							 fileName, tr ("SGF Files (*.sgf);;All Files (*)"));
	}
	if (fileName.isEmpty())
		return false;

	if (getFileExtension (fileName, false).isEmpty ())
		fileName.append (".sgf");

	if (!save_to_file (this, m_game, fileName))
		return false;

	statusBar()->showMessage (fileName + " " + tr("saved."));
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
	go_game_ptr gr = record_from_stream (buf, nullptr);
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
							 m_ascii_dlg.cb_coords->isChecked (),
							 m_ascii_dlg.targetComboBox->currentIndex () == 0);
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
	m_ascii_update_source = ui->gfx_board;
	if (!m_ascii_dlg.isVisible ()) {
		/* Set defaults if the dialog is not open.  */
		m_ascii_dlg.cb_numbering->setChecked (!ui->viewNumbersOff->isChecked ());
	}
	update_ascii_dialog ();
	m_ascii_dlg.show ();
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotFileExportSVG(bool)
{
	m_svg_update_source = ui->gfx_board;
	if (!m_svg_dlg.isVisible ()) {
		/* Set defaults if the dialog is not open.  */
		m_svg_dlg.cb_numbering->setChecked (!ui->viewNumbersOff->isChecked ());
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
	QPixmap pm = ui->gfx_board->grabPicture ();
	if (!pm.save (fileName, filter.toLatin1 ()))
		QMessageBox::warning (this, PACKAGE, tr("Failed to save image!"));
}

void MainWindow::slotFileExportPicClipB(bool)
{
	QApplication::clipboard()->setPixmap (ui->gfx_board->grabPicture ());
}

void MainWindow::slotEditCopyPos (bool)
{
	game_state *st = ui->gfx_board->displayed ();
	const go_board &b = st->get_board ();
	QString s;
	for (int y = 0; y < b.size_y (); y++) {
		for (int x = 0; x < b.size_x (); x++) {
			stone_color c = b.stone_at (x, y);
			s += c == none ? ' ' : c == black ? 'X' : 'O';
		}
		s += '\n';
	}
	QApplication::clipboard ()->setText (s);
}

void MainWindow::slotEditPastePos (bool)
{
	QString s = QApplication::clipboard ()->text ();
	game_state *st = ui->gfx_board->displayed ();
	go_board b = st->get_board ();
	try {
		int sx = b.size_x ();
		int sy = b.size_y ();
		if (s.length () != (sx + 1) * sy)
			throw false;
		int pos = 0;
		for (int y = 0; y < sy; y++) {
			for (int x = 0; x < sx; x++) {
				QChar c = s[pos++];
				if (c != ' ' && c != 'X' && c != 'O')
					throw false;
				b.set_stone (x, y, c == ' ' ? none : c == 'X' ? black : white);
			}
			if (s[pos++] != '\n')
				throw false;
		}
		ui->gfx_board->replace_displayed (b);
	} catch (...) {
		QMessageBox::warning (this, PACKAGE, tr ("No valid position to paste found in the clipboard."));
	}
}

void MainWindow::slotEditDelete (bool)
{
	game_state *st = ui->gfx_board->displayed ();
	if (st->root_node_p ())
		return;

	game_state *parent = st->prev_move ();
	st->walk_tree ([this, parent] (game_state *s)->bool { ui->gfx_board->transfer_displayed (s, parent); return true; });
	size_t idx = st->disconnect ();
	push_undo (std::make_unique<undo_delete_entry> ("Delete move", parent, st, idx));
	update_game_tree ();
	setMoveData (parent);
	ui->gfx_board->setModified ();
}

void MainWindow::slotEditRectSelect(bool on)
{
	qDebug () << "rectSelect " << on << "\n";
	ui->gfx_board->set_rect_select (on);
}

void MainWindow::slotEditClearSelect(bool)
{
	ui->editRectSelect->setChecked (false);
	ui->gfx_board->clear_selection ();
}

void MainWindow::done_rect_select ()
{
	ui->editRectSelect->setChecked (false);
}

void MainWindow::slotEditGroup (bool)
{
	mark m = mark::none;
	if (ui->editTriangle->isChecked ())
		m = mark::triangle;
	else if (ui->editSquare->isChecked ())
		m = mark::square;
	else if (ui->editCircle->isChecked ())
		m = mark::circle;
	else if (ui->editCross->isChecked ())
		m = mark::cross;
	else if (ui->editNumber->isChecked ())
		m = mark::num;
	else if (ui->editLetter->isChecked ())
		m = mark::letter;
	ui->gfx_board->setMarkType (m);
}

void MainWindow::slotEditFigure (bool on)
{
	game_state *st = ui->gfx_board->displayed ();
	if (on) {
		st->set_figure (256, "");
		if (!ui->diagsDock->isVisible ()) {
			ui->diagsDock->setVisible (true);
			restoreWindowLayout (false);
		}
	} else
		st->clear_figure ();
	update_figures ();
	update_game_tree ();
}

void MainWindow::slotEditAnalysis (bool)
{
	EditAnalysisDialog dlg (this, m_game, &m_an_id_model);
	dlg.exec ();
}

void MainWindow::slotPlayFromHere (bool)
{
	NewAIGameDlg dlg (this, true);
	if (dlg.exec () != QDialog::Accepted)
		return;

	int eidx = dlg.engine_index ();
	const Engine &engine = setting->m_engines[eidx];
	std::shared_ptr<game_record> gr = std::make_shared<game_record> (*m_game);
	game_state *curr_pos = ui->gfx_board->displayed ();
	game_state *st = gr->get_root ()->follow_path (curr_pos->path_from_root ());

	bool computer_white = dlg.computer_white_p ();
	time_settings ts = dlg.timing ();
	new MainWindow_GTP (0, gr, st, screen_key (this), engine, ts, !computer_white, computer_white);
}

void MainWindow::slotDiagChosen (int idx)
{
	game_state *st = m_figures.at (idx);
	ui->diagView->set_displayed (st);
	ui->diagCommentView->setText (QString::fromStdString (st->comment ()));
}

void MainWindow::slotDiagEdit (bool)
{
	game_state *st = ui->diagView->displayed ();
	FigureDialog dlg (st, this);
	dlg.exec ();
	update_figures ();
}

void MainWindow::slotDiagASCII (bool)
{
	m_ascii_update_source = ui->diagView;
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
	m_svg_update_source = ui->diagView;
	int print_num = m_svg_update_source->displayed ()->print_numbering_inherited ();
	m_svg_dlg.cb_numbering->setChecked (print_num != 0);
	update_svg_dialog ();
	m_svg_dlg.buttonRefresh->hide ();
	m_svg_dlg.exec ();
	m_svg_dlg.buttonRefresh->show ();
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotAutoplayTimer ()
{
	if (!ui->navForward->isEnabled ()) {
		ui->navAutoplay->setChecked (false);
		return;
	}
	if (ui->autoPauseButton->isChecked ())
		return;

	if (m_autoplay_count-- > 0)
		return;
	m_autoplay_count = m_autoplay_init = ui->autoSlider->value ();
	nav_next_move ();
	if (!ui->navForward->isEnabled ()) {
		ui->navAutoplay->setChecked (false);
		return;
	}
}

void MainWindow::slotNavAutoplay (bool on)
{
	ui->autoBox->setVisible (on);
	if (!on) {
		autoplay_timer->stop ();
		ui->autoPauseButton->setChecked (false);
		return;
	}
	m_autoplay_count = m_autoplay_init;
	autoplay_timer->start (500);
}

void MainWindow::autoSliderChanged (int v)
{
	m_autoplay_count = m_autoplay_init = v;
	ui->autoSpeedLabel->setText (tr ("%1s").arg (v * 0.5));
}

void MainWindow::slotNavIntersection(bool)
{
	ui->gfx_board->navIntersection();
}

void MainWindow::slotNavNthMove(bool)
{
#if 0
	NthMoveDialog dlg(this);
	dlg.moveSpinBox->setValue(ui->gfx_board->getCurrentMoveNumber());
	dlg.moveSpinBox->setFocus();

	if (dlg.exec() == QDialog::Accepted)
		ui->gfx_board->gotoNthMove(dlg.moveSpinBox->value());
#endif
}

void MainWindow::slotNavSwapVariations(bool)
{
#if 0
	if (ui->gfx_board->swapVariations())
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
	bool is_online = (m_gamemode == modeMatch || m_gamemode == modeObserve
			  || m_gamemode == modeObserveMulti || m_gamemode == modeTeach);
	if (is_online) {
		ui->treeDock->setVisible (false);
		ui->treeDock->toggleViewAction ()->setVisible (false);
		if (m_gamemode != modeObserve && m_gamemode != modeObserveMulti) {
			ui->graphDock->setVisible (false);
			ui->graphDock->toggleViewAction ()->setVisible (false);
		}
		ui->diagsDock->setVisible (false);
		ui->diagsDock->toggleViewAction ()->setVisible (false);
	} else {
		ui->observersDock->setVisible (false);
		ui->observersDock->toggleViewAction ()->setVisible (false);
		ui->treeDock->toggleViewAction ()->setVisible (true);
		ui->diagsDock->toggleViewAction ()->setVisible (true);
		ui->graphDock->toggleViewAction ()->setVisible (true);
	}
	ui->chooseDock->toggleViewAction ()->setVisible (m_gamemode == modeObserveMulti);
}

void MainWindow::slotEngineGroup (bool)
{
	QAction *checked_engine = engineGroup->checkedAction ();
	ui->anConnect->setEnabled (!ui->anDisconnect->isEnabled () && checked_engine != nullptr);
	ui->normalTools->anStartButton->setEnabled (ui->normalTools->anStartButton->isChecked () || checked_engine != nullptr);
}

void MainWindow::slotToggleHideAnalysis (bool on)
{
	ui->gfx_board->set_hide_analysis (on);
	if (on)
		ui->normalTools->anHideButton->setStatusTip (tr ("Evaluations are not shown on the board."));
	else
		ui->normalTools->anHideButton->setStatusTip (tr ("Evaluations are shown on the board."));
}

void MainWindow::start_analysis ()
{
	QAction *checked = engineGroup->checkedAction ();
	if (checked == nullptr) {
		/* We should not get here - the actions/buttons should be disabled.  */
		QMessageBox::warning (this, PACKAGE, tr ("You did not configure any analysis engine for this boardsize!"));
		return;
	}
	auto it = engine_map.find (checked);
	if (it == engine_map.end ()) {
		/* Should not get here either.  */
		QMessageBox::warning (this, PACKAGE, tr ("Internal error - engine not found."));
		return;
	}
	ui->gfx_board->start_analysis (it.value ());
	ui->anMessages->setEnabled (true);
}

void MainWindow::populate_engines_menu ()
{
	QAction *old_checked = engineGroup->checkedAction ();
	QString old_title;
	if (old_checked)
		old_title = old_checked->text ();
	for (auto it: engine_actions)
		delete it;
	engine_actions.clear ();
	engine_map.clear ();

	const go_board &b = m_game->get_root ()->get_board ();
	if (b.size_x () != b.size_y ())
		return;

	qDebug () << "finding engines: " << b.size_x ();

	QList<Engine> available = client_window->analysis_engines (b.size_x ());
	for (const auto &it: available) {
		qDebug () << "Engine: " << it.title;
		QAction *a = new QAction (it.title);
		engine_map.insert (a, it);
		connect (a, &QAction::triggered, this, &MainWindow::slotEngineGroup);
		engine_actions.append (a);
		engineGroup->addAction (a);
		a->setCheckable (true);
		if (it.title == old_title)
			a->setChecked (true);
	}
	if (engineGroup->checkedAction () == nullptr && engine_actions.length () > 0)
		engine_actions.first ()->setChecked (true);
	ui->anChooseMenu->addActions (engine_actions);
}

void MainWindow::update_settings ()
{
	update_font ();

	ui->searchPattern->setVisible (m_gamemode != modeMatch && !setting->m_dbpaths.isEmpty ());

	ui->viewCoords->setChecked (setting->readBoolEntry ("BOARD_COORDS"));
	ui->gfx_board->set_sgf_coords (setting->readBoolEntry ("SGF_BOARD_COORDS"));

	int ghosts = setting->readIntEntry ("VAR_GHOSTS");
	bool children = setting->readBoolEntry ("VAR_CHILDREN");
	int style = m_game->info ().style;

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
	ui->gfx_board->set_vardisplay (children, ghosts);

#if 0 // @@@
	QToolTip::setEnabled(setting->readBoolEntry("TOOLTIPS"));
#endif

	ui->gfx_board->update_prefs ();
	ui->diagView->update_prefs ();

	slotViewLeftSidebar();

	const go_board &b = m_game->get_root ()->get_board ();
	bool disable_rect = (b.torus_h () || b.torus_v ()) && setting->readIntEntry ("TOROID_DUPS") > 0;
	ui->editRectSelect->setEnabled (!disable_rect);

	ui->gameTreeView->update_prefs ();

	if (slideView != nullptr)
		slideView->update_prefs ();

	if (setting->engines_changed)
		populate_engines_menu ();

	updateCaption ();
}

void MainWindow::slotSetGameInfo (bool)
{
	GameInfoDialog dlg (this);
	const game_info &i = m_game->info ();
	dlg.gameNameEdit->setText (QString::fromStdString (i.title));
	dlg.playerWhiteEdit->setText (QString::fromStdString (i.name_w));
	dlg.playerBlackEdit->setText (QString::fromStdString (i.name_b));
	dlg.whiteRankEdit->setText (QString::fromStdString (i.rank_w));
	dlg.blackRankEdit->setText (QString::fromStdString (i.rank_b));
	dlg.resultEdit->setText (QString::fromStdString (i.result));
	dlg.dateEdit->setText (QString::fromStdString (i.date));
	dlg.rulesEdit->setText (QString::fromStdString (i.rules));
	dlg.placeEdit->setText (QString::fromStdString (i.place));
	dlg.eventEdit->setText (QString::fromStdString (i.event));
	dlg.roundEdit->setText (QString::fromStdString (i.round));
	dlg.copyrightEdit->setText (QString::fromStdString (i.copyright));
	dlg.komiSpin->setValue (i.komi);
	dlg.handicapSpin->setValue (i.handicap);

	if (dlg.exec() == QDialog::Accepted)
	{
		game_info newi = i;
		newi.name_b = dlg.playerBlackEdit->text().toStdString ();
		newi.name_w = dlg.playerWhiteEdit->text().toStdString ();
		newi.rank_b = dlg.blackRankEdit->text().toStdString ();
		newi.rank_w = dlg.whiteRankEdit->text().toStdString ();
		newi.komi = dlg.komiSpin->value();
		newi.handicap = dlg.handicapSpin->value();
		newi.title = dlg.gameNameEdit->text().toStdString ();
		newi.result = dlg.resultEdit->text().toStdString ();
		newi.date = dlg.dateEdit->text().toStdString ();
		newi.rules = dlg.rulesEdit->text().toStdString ();
		newi.place = dlg.placeEdit->text().toStdString ();
		newi.event = dlg.eventEdit->text().toStdString ();
		newi.round = dlg.roundEdit->text().toStdString ();
		newi.copyright = dlg.copyrightEdit->text().toStdString ();
		push_undo (std::make_unique<undo_info_entry> ("Edit game information", m_game, i, newi));
		m_game->set_info (newi);
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
	ui->gfx_board->set_show_coords(toggle);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewMoveNumbers(bool)
{
	movenums mn = (ui->viewNumbersFull->isChecked () ? movenums::full
		       : ui->viewNumbersVarsLong->isChecked () ? movenums::vars_long
		       : ui->viewNumbersVarsShort->isChecked () ? movenums::vars
		       : movenums::off);
	ui->gfx_board->set_show_move_numbers (mn);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewConnections(bool toggle)
{
	ui->gfx_board->set_visualize_connections (toggle);
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewDiagComments(bool toggle)
{
	ui->diagCommentView->setVisible (toggle);
	ui->comments_widget->setVisible (!toggle);
	if (toggle)
		ui->commentsDock->setWindowTitle (tr ("Diag. comments"));
	else
		ui->commentsDock->setWindowTitle (tr ("Comments"));
	statusBar()->showMessage(tr("Ready."));
}

void MainWindow::slotViewSlider (bool toggle)
{
	ui->sliderWidget->setVisible (toggle);

	statusBar ()->showMessage (tr ("Ready."));
}

// set sidbar left or right
void MainWindow::slotViewLeftSidebar()
{
	ui->mainGridLayout->removeWidget(ui->boardFrame);
	ui->mainGridLayout->removeWidget(ui->toolsFrame);

	if (setting->readBoolEntry("SIDEBAR_LEFT"))
	{
		ui->mainGridLayout->addWidget(ui->toolsFrame, 0, 0, 2, 1);
		ui->mainGridLayout->addWidget(ui->boardFrame, 0, 1, 2, 1);
	}
	else
	{
		ui->mainGridLayout->addWidget(ui->toolsFrame, 0, 1, 2, 1);
		ui->mainGridLayout->addWidget(ui->boardFrame, 0, 0, 2, 1);
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

void MainWindow::time_loss (stone_color)
{
}

void MainWindow::slotTimerForward ()
{
	auto f = [this] (move_timer &t, stone_color col, ClockView *clock, std::string &last)
		{
			if (!t.update (false))
				time_loss (col);
			else {
				std::string r = t.report ();
				if (r != last) {
					clock->set_text (QString::fromStdString (r));
					last = r;
				}
			}
		};
	f (m_timer_white, white, ui->normalTools->wtimeView, m_tstr_white);
	f (m_timer_black, black, ui->normalTools->btimeView, m_tstr_black);
}

void MainWindow::coords_changed (const QString &t1, const QString &t2)
{
	statusBar ()->clearMessage ();
	statusCoords->setText (" " + t1 + " " + t2 + " ");
}

QString MainWindow::visible_panes_key ()
{
	QString v;
	v += ui->diagsDock->isVisibleTo (this) ? "1" : "0";
	v += ui->treeDock->isVisibleTo (this) ? "1" : "0";
	v += ui->graphDock->isVisibleTo (this) ? "1" : "0";
	v += ui->commentsDock->isVisibleTo (this) ? "1" : "0";
	v += ui->observersDock->isVisibleTo (this) ? "1" : "0";
	return v;
}

void MainWindow::restore_visibility_from_key (const QString &v)
{
	ui->diagsDock->setVisible (v[0] == '1');
	ui->treeDock->setVisible (v[1] == '1');
	ui->graphDock->setVisible (v[2] == '1');
	ui->commentsDock->setVisible (v[3] == '1');
	ui->observersDock->setVisible (v[4] == '1');
}

void MainWindow::saveWindowLayout (bool dflt)
{
	QString strKey = screen_key (this);
	QString panesKey = visible_panes_key ();

	if (!dflt)
		strKey += "_" + panesKey;

	/* ObserveMulti has an extra pane, which we could encode in the panes key.
	   Unfortunately there exist variants of q5go already which have additional
	   panes (e.g. for games in a zip archive).  So there is a risk of incompatible
	   config files.
	   So maybe this method (using a unique identifier for a visible pane) is the
	   model to go with for any future extension.  */
	if (m_gamemode == modeObserveMulti && !dflt)
		strKey += "_multi";

	// store window size, format, comment format
	setting->writeBoolEntry("BOARDFULLSCREEN_" + strKey, isFullScreen);

	QByteArray v1 = saveState ().toHex ();
	QByteArray v2 = saveGeometry ().toHex ();
	setting->writeEntry("BOARDLAYOUT1_" + strKey, QString::fromLatin1 (v1));
	setting->writeEntry("BOARDLAYOUT2_" + strKey, QString::fromLatin1 (v2));

	QString v3 = ui->viewStatusBar->isChecked () ? "1" : "0";
	v3 += ui->viewMenuBar->isChecked () ? "1" : "0";
	v3 += ui->viewSlider->isChecked () ? "1" : "0";
	v3 += ui->viewSidebar->isChecked () ? "1" : "0";
	setting->writeEntry("BOARDLAYOUT3_" + strKey, v3);

	statusBar ()->showMessage(tr("Window size saved.") + " (" + strKey + ")");
}

void MainWindow::update_view_menu ()
{
	ui->viewSlider->setChecked (ui->sliderWidget->isVisibleTo (this));
	ui->viewMenuBar->setChecked (menuBar ()->isVisibleTo (this));
	ui->viewStatusBar->setChecked (statusBar ()->isVisibleTo (this));
	ui->viewSidebar->setChecked (ui->toolsFrame->isVisibleTo (this));
}

bool MainWindow::restoreWindowLayout (bool dflt, const QString &scrkey)
{
	QString strKey = scrkey.isEmpty () ? screen_key (this) : scrkey;
	QString panesKey = visible_panes_key ();

	if (!dflt)
		strKey += "_" + panesKey;

	if (m_gamemode == modeObserveMulti && !dflt)
		strKey += "_multi";

	// restore board window
	QString s1 = setting->readEntry("BOARDLAYOUT1_" + strKey);
	QString s2 = setting->readEntry("BOARDLAYOUT2_" + strKey);
	QString s3 = setting->readEntry("BOARDLAYOUT3_" + strKey);
	if (s1.isEmpty () || s2.isEmpty ())
		return false;
	QRegExp verify ("^[0-9A-Fa-f]*$");
	if (!verify.exactMatch (s1) || !verify.exactMatch (s2))
		return false;

	// do not resize until end of this procedure
	ui->gfx_board->lockResize = true;

	restoreGeometry (QByteArray::fromHex (s2.toLatin1 ()));
	restoreState (QByteArray::fromHex (s1.toLatin1 ()));

	if (!dflt)
		restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();

	if (s3.length () >= 4) {
		statusBar ()->setVisible (s3[0] != '0');
		menuBar ()->setVisible (s3[1] != '0');
		ui->sliderWidget->setVisible (s3[2] != '0');
		ui->toolsFrame->setVisible (s3[3] != '0');
	}
	update_view_menu ();

	// ok, resize
	ui->gfx_board->lockResize = false;
	ui->gfx_board->changeSize();

	QString extra = menuBar ()->isVisibleTo (this) ? "" : tr (" - Press F7 to show menu bar");
	statusBar()->showMessage(tr("Window size restored.") + " (" + strKey + ")" + extra);

	return true;
}

void MainWindow::defaultPortraitLayout ()
{
	ui->commentsDock->setVisible (true);
	if (m_gamemode == modeMatch || m_gamemode == modeObserve || m_gamemode == modeTeach)
		ui->observersDock->setVisible (true);

	QString panesKey = visible_panes_key ();
	removeDockWidget (ui->diagsDock);
	removeDockWidget (ui->treeDock);
	removeDockWidget (ui->graphDock);
	removeDockWidget (ui->commentsDock);
	removeDockWidget (ui->observersDock);
	addDockWidget (Qt::BottomDockWidgetArea, ui->graphDock);
	splitDockWidget (ui->graphDock, ui->diagsDock, Qt::Vertical);
	splitDockWidget (ui->diagsDock, ui->commentsDock, Qt::Horizontal);
	splitDockWidget (ui->commentsDock, ui->observersDock, Qt::Horizontal);
	splitDockWidget (ui->commentsDock, ui->treeDock, Qt::Horizontal);
	restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();
	setFocus ();
}

void MainWindow::defaultLandscapeLayout ()
{
	ui->commentsDock->setVisible (true);
	if (m_gamemode == modeMatch || m_gamemode == modeObserve || m_gamemode == modeTeach)
		ui->observersDock->setVisible (true);

	QString panesKey = visible_panes_key ();
	removeDockWidget (ui->diagsDock);
	removeDockWidget (ui->treeDock);
	removeDockWidget (ui->graphDock);
	removeDockWidget (ui->commentsDock);
	removeDockWidget (ui->observersDock);
	addDockWidget (Qt::BottomDockWidgetArea, ui->treeDock);
	addDockWidget (Qt::RightDockWidgetArea, ui->diagsDock);
	splitDockWidget (ui->diagsDock, ui->commentsDock, Qt::Horizontal);
	splitDockWidget (ui->diagsDock, ui->graphDock, Qt::Vertical);
	splitDockWidget (ui->commentsDock, ui->observersDock, Qt::Vertical);
	restore_visibility_from_key (panesKey);
	hide_panes_for_mode ();
	setFocus ();
}

void MainWindow::keyPressEvent (QKeyEvent *e)
{
	switch (e->key ())
	{
		case Qt::Key_Alt:
			menuBar ()->show ();
			break;

		default:
			break;
	}
	QMainWindow::keyPressEvent (e);
}

void MainWindow::keyReleaseEvent (QKeyEvent *e)
{
	switch (e->key ())
	{
		case Qt::Key_Alt:
			if (!ui->viewMenuBar->isChecked ())
				menuBar ()->hide ();
			break;

		default:
			break;
	}
	QMainWindow::keyPressEvent (e);
}

void MainWindow::closeEvent (QCloseEvent *e)
{
	if (checkModified ())
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

void MainWindow::slotUpdateComment ()
{
	if (!m_allow_text_update_signal)
		return;
	undo_comment_entry *uce = top_undo_entry<undo_comment_entry> ();
	game_state *st = ui->gfx_board->displayed ();
	std::string s = ui->commentEdit->toPlainText ().toStdString ();
	if (uce)
		uce->update (s);
	else {
		push_undo (std::make_unique<undo_comment_entry> ("Edit comment", st, st->comment (), s));
	}
	st->set_comment (s);
	ui->gfx_board->setModified ();
	if (slideView != nullptr)
		slideView->set_active (st);
}

/* Used by the IGS observer window when switching between games.  */
void MainWindow::set_comment (const QString &t)
{
	bool old = m_allow_text_update_signal;
	m_allow_text_update_signal = false;
	ui->commentEdit->setText (t);
	m_allow_text_update_signal = old;
}

/* Called from an external source to append to the comments window.  */
void MainWindow::append_comment (const QString &t, QColor col)
{
	bool old = m_allow_text_update_signal;
	m_allow_text_update_signal = false;
	/* The append method inserts paragraphs, which means we get an extra unwanted newline.  */
	ui->commentEdit->moveCursor (QTextCursor::End);
	if (col.isValid ())
		ui->commentEdit->setTextColor (col);
	else
		ui->commentEdit->setTextColor (m_default_text_color);
	ui->commentEdit->insertPlainText (t);
	ui->commentEdit->moveCursor (QTextCursor::End);
	m_allow_text_update_signal = old;
}

/* Called if something may have changed the comment text in the game record.  */
void MainWindow::refresh_comment ()
{
	bool old = m_allow_text_update_signal;
	m_allow_text_update_signal = false;
	game_state *gs = ui->gfx_board->displayed ();
	std::string c = gs->comment ();
	if (c.size () == 0)
		ui->commentEdit->clear ();
	else
		ui->commentEdit->setText (QString::fromStdString (c));

	m_allow_text_update_signal = old;
}

void MainWindow::slotUpdateComment2 ()
{
	QString text = ui->commentEdit2->text();

	// clear entry
	ui->commentEdit2->setText("");

	// don't show short text
	if (text.length() < 1)
		return;

	// emit signal to opponent in online game
	emit signal_sendcomment (text);
}

void MainWindow::update_font ()
{
	// editable fields
	setFont (setting->fontComments);

	// observer
	ui->ListView_observers->setFont (setting->fontLists);

	QFont f (setting->fontStandard);
	f.setBold (true);
	ui->scoreTools->totalBlack->setFont (f);
	ui->scoreTools->totalWhite->setFont (f);

	setFont (setting->fontStandard);
	ui->normalTools->wtimeView->update_font (setting->fontClocks);
	ui->normalTools->btimeView->update_font (setting->fontClocks);

	bool old = m_allow_text_update_signal;
	m_allow_text_update_signal = false;

	QTextCursor c = ui->commentEdit->textCursor ();
	ui->commentEdit->selectAll ();
	ui->commentEdit->setCurrentFont (setting->fontComments);
	ui->commentEdit->setTextCursor (c);
	ui->commentEdit->setCurrentFont (setting->fontComments);
	ui->commentEdit->setFont (setting->fontComments);

	ui->commentEdit2->setFont (setting->fontComments);

	m_allow_text_update_signal = old;

	QFontMetrics fm (setting->fontStandard);
	QRect r = fm.boundingRect ("Variation 12 of 15");
	QRect r2 = fm.boundingRect ("Move 359 (W M19)");
	int strings_width = std::max (r.width (), r2.width ());
	int min_width = std::max (125, strings_width) + 25;
	ui->toolsFrame->setMaximumWidth (min_width);
	ui->toolsFrame->setMinimumWidth (min_width);
	ui->toolsFrame->resize (min_width, ui->toolsFrame->height ());

	int h = fm.height ();
	QString wimg = ":/images/stone_w16.png";
	QString bimg = ":/images/stone_b16.png";
	QString timg = ":/images/warn-tri16.png";
	if (h >= 32) {
		 wimg = ":/images/stone_w32.png";
		 bimg = ":/images/stone_b32.png";
		 timg = ":/images/warn-tri32.png";
	} else if (h >= 22) {
		wimg = ":/images/stone_w22.png";
		bimg = ":/images/stone_b22.png";
		timg = ":/images/warn-tri22.png";
	}
	QIcon icon (bimg);
	icon.addPixmap (wimg, QIcon::Normal, QIcon::On);

	ui->colorButton->setIcon (icon);
	ui->colorButton->setIconSize (QSize (h, h));
	ui->normalTools->whiteStoneLabel->setPixmap (QPixmap (wimg));
	ui->normalTools->blackStoneLabel->setPixmap (QPixmap (bimg));
	ui->scoreTools->whiteStoneLabel->setPixmap (wimg);
	ui->scoreTools->blackStoneLabel->setPixmap (bimg);
	ui->turnWarning->setPixmap (timg);
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
			w = ui->tab_ns;
			break;

		case tabTeachGameTree:
			w = ui->tab_tg;
			break;

		default:
			return;
	}

	if (s == tabSet)
	{
		// check whether the page to switch to is enabled
		if (!ui->toolsTabWidget->isEnabled())
			ui->toolsTabWidget->setTabEnabled(ui->toolsTabWidget->indexOf (w), true);

		ui->toolsTabWidget->setCurrentIndex(p);
	}
	else
	{
		// check whether the current page is to disable; then set to 'normal'
		if (s == tabDisable && ui->toolsTabWidget->currentIndex() == p)
			ui->toolsTabWidget->setCurrentIndex(tabNormalScore);

		ui->toolsTabWidget->setTabEnabled(ui->toolsTabWidget->indexOf (w), s == tabEnable);
	}
}

void MainWindow::update_pass_button ()
{
	bool to_move = ui->gfx_board->player_to_move_p ();
	bool enabled_p = m_gamemode != modeScore && m_gamemode != modeScoreRemote && to_move;
	/* Undo is invisible if it shouldn't be enabled, except when playing the computer.  */
	bool enabled_u = true;

	if (m_gamemode == modeComputer) {
		if (!to_move)
			enabled_u = false;
		else {
			game_state *st = ui->gfx_board->displayed ();
			if (st->root_node_p () || st->prev_move ()->root_node_p ())
				enabled_u = false;
		}
	}

	ui->passButton->setEnabled (enabled_p);
	ui->undoButton->setEnabled (enabled_u);
}

void MainWindow::setGameMode (GameMode mode)
{
	GameMode old_mode = m_gamemode;
#ifdef CHECKING
	if (old_mode == modeObserveMulti && mode != modeObserveMulti) {
		fprintf (stderr, "Serious issue with multi-observer mode\n");
	}
#endif
	m_gamemode = mode;
	if (mode == modePostMatch || mode == modeNormal) {
		m_remember_mode = mode;
		/* Disconnect any special functionality from the Pass and Done button.
		   This handles a number of game end conditions for IGS or GTP windows.  */
		disconnect (ui->passButton, &QPushButton::clicked, nullptr, nullptr);
		connect (ui->passButton, &QPushButton::clicked, this, &MainWindow::doPass);
		disconnect (ui->doneButton, &QPushButton::clicked, nullptr, nullptr);
		connect (ui->doneButton, &QPushButton::clicked, this, &MainWindow::doCountDone);

		/* Also make sure we can edit the game freely in these modes.  */
		ui->gfx_board->set_player_colors (true, true);
		ui->gfx_board->set_game_position (nullptr);
	}

	bool editable_comments = (mode != modeMatch && mode != modePostMatch && mode != modeObserve && mode != modeObserveMulti
				  && mode != modeScoreRemote && m_remember_mode != modePostMatch);
	ui->leaveMatchButton->setVisible (mode == modePostMatch);
	ui->viewConnections->setVisible (mode != modeMatch && mode != modeTeach && mode != modeComputer);

	/* For almost everything below this, modePostMatch behaves just like modeNormal.  */
	GameMode mode_in = mode;
	if (mode == modePostMatch)
		mode = modeNormal;
	bool is_observe = mode == modeObserve || mode == modeObserveMulti;

	if (mode == modeNormal) {
		m_timer_white.stop (false);
		m_timer_black.stop (false);
	}
	if (mode == modeEdit || mode == modeNormal || mode == modePostMatch || is_observe || mode == modeObserveGTP) {
		editGroup->setEnabled (true);
	} else {
		ui->editStone->setChecked (true);
		ui->gfx_board->setMarkType (mark::none);
		editGroup->setEnabled (false);
	}

	ui->scoreTools->scoreTypeLine->setVisible (mode == modeScore);
	ui->scoreTools->scoreTerrButton->setVisible (mode == modeScore);
	ui->scoreTools->scoreAreaButton->setVisible (mode == modeScore);
	if (mode == modeScoreRemote)
		ui->scoreTools->scoreTerrButton->setChecked (true);

	ui->normalTools->anGroup->setVisible (mode == modeNormal || is_observe);
	ui->normalTools->anStartButton->setVisible (mode == modeNormal || is_observe);
	ui->normalTools->anPauseButton->setVisible (mode == modeNormal || is_observe);
	ui->normalTools->anHideButton->setVisible (mode == modeNormal || is_observe);

	/* Don't allow navigation through these back doors when in edit or score mode.  */
	ui->evalGraph->setEnabled (mode != modeEdit && mode != modeScore && mode != modeScoreRemote);
	ui->gameTreeView->setEnabled (mode != modeEdit && mode != modeScore && mode != modeScoreRemote);

	bool enable_nav = mode == modeNormal; /* @@@ teach perhaps? */

	ui->navPrevVar->setEnabled (enable_nav);
	ui->navNextVar->setEnabled (enable_nav);
	ui->navBackward->setEnabled (enable_nav);
	ui->navForward->setEnabled (enable_nav);
	ui->navFirst->setEnabled (enable_nav);
	ui->navStartVar->setEnabled (enable_nav);
	ui->navMainBranch->setEnabled (enable_nav);
	ui->navLast->setEnabled (enable_nav);
	ui->navNextBranch->setEnabled (enable_nav);
	ui->navPrevComment->setEnabled (enable_nav);
	ui->navNextComment->setEnabled (enable_nav);
	ui->navIntersection->setEnabled (enable_nav);
	ui->navNthMove->setEnabled (enable_nav);
	ui->editPastePos->setEnabled (mode == modeEdit);
	ui->editDelete->setEnabled (enable_nav && mode_in != modePostMatch);
	navSwapVariations->setEnabled (enable_nav);

	ui->fileImportSgfClipB->setEnabled (enable_nav);

	ui->anPlay->setEnabled (mode == modeNormal || is_observe);

	ui->commentEdit->setReadOnly (!editable_comments || mode == modeEdit);
	// ui->commentEdit->setDisabled (editable_comments);
	ui->commentEdit2->setEnabled (!editable_comments);
	ui->commentEdit2->setVisible (!editable_comments);

	hide_panes_for_mode ();

	ui->fileNew->setEnabled (mode == modeNormal || mode == modeEdit);
	ui->fileNewVariant->setEnabled (mode == modeNormal || mode == modeEdit);
	ui->fileOpen->setEnabled (mode == modeNormal || mode == modeEdit);
	ui->fileOpenDB->setEnabled (mode == modeNormal || mode == modeEdit);

	switch (mode)
	{
	case modeEdit:
		statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
		break;

	case modeNormal:
		statusMode->setText(" " + QObject::tr("E", "Board status line: edit mode") + " ");
		break;

	case modeObserve:
	case modeObserveMulti:
		statusMode->setText(" " + QObject::tr("O", "Board status line: observe mode") + " ");
		break;

	case modeObserveGTP:
		statusMode->setText(" " + QObject::tr("O", "Board status line: observe GTP mode") + " ");
		break;

	case modeMatch:
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modePostMatch:
		statusMode->setText(" " + QObject::tr("N", "Board status line: post-match normal mode") + " ");
		break;

	case modeComputer:
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		break;

	case modeTeach:
		statusMode->setText(" " + QObject::tr("T", "Board status line: teach mode") + " ");
		break;

	case modeScore:
		ui->commentEdit->setReadOnly(true);
		ui->commentEdit2->setDisabled(true);
		/* fall through */
	case modeScoreRemote:
		statusMode->setText(" " + QObject::tr("S", "Board status line: score mode") + " ");
		break;

	case modeBatch:
		statusMode->setText(" " + QObject::tr("A", "Board status line: batch analysis") + " ");
		break;

	}

	setToolsTabWidget(tabTeachGameTree, mode == modeTeach ? tabEnable : tabDisable);
	if (mode != modeTeach && mode != modeScoreRemote && mode != modeScore && ui->tab_tg->parent () != nullptr)
		ui->tab_tg->setParent (nullptr);

	ui->followButton->setVisible (mode == modeObserveGTP);
	ui->passButton->setVisible (!is_observe && mode != modeObserveGTP && mode != modeBatch);
	ui->doneButton->setVisible (mode == modeScore || mode == modeScoreRemote);
	ui->scoreButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	ui->undoButton->setVisible (mode == modeScoreRemote || mode == modeMatch || mode == modeComputer || mode == modeTeach);
	ui->adjournButton->setVisible (mode == modeMatch || mode == modeTeach || mode == modeScoreRemote);
	ui->resignButton->setVisible (mode == modeMatch || mode == modeComputer || mode == modeTeach || mode == modeScoreRemote);
	ui->resignButton->setEnabled (mode != modeScoreRemote);
	ui->refreshButton->setEnabled (mode == modeNormal);
	ui->editButton->setVisible (is_observe);
	ui->editPosButton->setVisible (mode == modeNormal || mode == modeEdit || mode == modeScore);
	ui->editPosButton->setEnabled (mode == modeNormal || mode == modeEdit);
	ui->editAppendButton->setVisible (mode == modeEdit);
	ui->againButton->setEnabled (mode == modeNormal || mode == modeComputer);

	if (mode == modeEdit) {
		game_state *st = ui->gfx_board->displayed ();
		/* We require ST to be an edit/score, because if we inserted an edit before a move, that move
		   would probably no longer make sense in the edited board position.  */
		ui->editInsertButton->setVisible (!st->root_node_p () && !st->was_move_p ());
		/* Likewise for modifying an edit node - it must not have any non-edit children.  */
		bool vis = st->was_edit_p () || st->root_node_p ();
		for (const auto &c: st->children ())
			if (!c->was_edit_p ()) {
				vis = false;
				break;
			}
		ui->editReplaceButton->setVisible (vis);
	} else {
		ui->editPosButton->setChecked (false);
		ui->editReplaceButton->setVisible (false);
		ui->editInsertButton->setVisible (false);
	}
	ui->colorButton->setEnabled (mode == modeEdit || mode == modeNormal);

	ui->slider->setEnabled (mode == modeNormal || is_observe || mode == modeObserveGTP || mode == modeBatch);

	ui->editButton->setEnabled (mode != modeScore);
	ui->scoreButton->setEnabled (mode != modeEdit);

	if (mode == modeScore || mode == modeScoreRemote || mode == modeEdit) {
		game_state *st = ui->gfx_board->displayed ();
		m_pos_before_edit = st;

		go_board b = st->get_board ();
		if (mode == modeScore || mode == modeScoreRemote) {
			if (mode == modeScore && st->was_score_p ())
				b.territory_from_markers ();
			else
				b.calc_scoring_markers_complex ();
		}
		game_state *edit_st = m_game->create_game_state (b, st->to_move ());
		set_displayed (edit_st);
	} else if (old_mode == modeScore || old_mode == modeScoreRemote || old_mode == modeEdit) {
		/* The only way the scored or edited board is added to the game tree is through
		   doCountDone and leave_edit.  These perform the necessary insertions and clear
		   m_pos_before_edit.  */
		if (m_pos_before_edit != nullptr) {
			game_state *edit_st = ui->gfx_board->displayed ();
			set_displayed (m_pos_before_edit);
			m_game->release_game_state (edit_st);
			m_pos_before_edit = nullptr;
		}
	}

	ui->gfx_board->setMode (mode);

	update_pass_button ();

	ui->scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote);
	ui->normalTools->setVisible (mode != modeScore && mode != modeScoreRemote);

	if (mode == modeMatch || mode == modeTeach) {
		qDebug () << ui->gfx_board->player_is (white) << " : " << ui->gfx_board->player_is (black);
		QWidget *timeSelf = ui->gfx_board->player_is (black) ? ui->normalTools->btimeView : ui->normalTools->wtimeView;
		QWidget *timeOther = ui->gfx_board->player_is (black) ? ui->normalTools->wtimeView : ui->normalTools->btimeView;
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
		ui->normalTools->btimeView->setToolTip (tr ("Time remaining for this move"));
		ui->normalTools->wtimeView->setToolTip (tr ("Time remaining for this move"));
	}
	updateCaption ();
}

void MainWindow::update_figure_display ()
{
	game_state *fig = ui->diagView->displayed ();
	if (fig != nullptr) {
		int flags = fig->figure_flags ();
		ui->diagView->set_show_coords (!(flags & 1));
		ui->diagView->set_show_figure_caps (!(flags & 256));
		ui->diagView->set_show_hoshis (!(flags & 512));
		ui->diagView->changeSize ();
	}
}

void MainWindow::update_figures ()
{
	game_state *gs = ui->gfx_board->displayed ();
	game_state *old_fig = ui->diagView->displayed ();
	int keep_old_fig = -1;
	m_figures.clear ();
	ui->diagComboBox->clear ();
	bool main_fig = gs->has_figure ();
	ui->editFigure->setChecked (main_fig);
	if (main_fig) {
		const std::string &title = gs->figure_title ();
		m_figures.push_back (gs);
		if (title == "")
			ui->diagComboBox->addItem ("Current position (untitled)");
		else
			ui->diagComboBox->addItem (QString::fromStdString (title));
		if (gs == old_fig)
			keep_old_fig = 0;
	}
	int count = 1;
	auto children = gs->children ();
	for (const auto it: children) {
		/* Skip the first child - it's the main variation, and should only appear
		   in the list as "Current position", once it is reached.  */
		if (it == children[0])
			continue;
		if (it->has_figure ()) {
			if (count == 1 && m_figures.size () != 0) {
				m_figures.push_back (nullptr);
				ui->diagComboBox->insertSeparator (1);
			}
			const std::string &title = it->figure_title ();
			if (title == "")
				ui->diagComboBox->addItem ("Subposition " + QString::number (count) + " (untitled)");
			else
				ui->diagComboBox->addItem (QString::fromStdString (title));
			m_figures.push_back (it);
			if (it == old_fig)
				keep_old_fig = ui->diagComboBox->count () - 1;
			count++;
		}
	}
	bool have_any = m_figures.size () != 0;
	ui->diagComboBox->setEnabled (have_any);
	ui->diagEditButton->setEnabled (have_any);
	ui->diagASCIIButton->setEnabled (have_any);
	ui->diagSVGButton->setEnabled (have_any);
	if (setting->readBoolEntry ("BOARD_DIAGCLEAR")) {
		ui->diagView->setEnabled (have_any);
		if (!have_any)
			ui->diagView->set_displayed (m_empty_state);
	}
	if (!have_any) {
		ui->diagComboBox->addItem ("No diagrams available");
		ui->diagCommentView->clear ();
	} else if (main_fig) {
		ui->diagView->set_displayed (gs);
		ui->diagCommentView->setText (QString::fromStdString (gs->comment ()));
	} else if (keep_old_fig == -1) {
		ui->diagView->set_displayed (m_figures[0]);
		ui->diagCommentView->setText (QString::fromStdString (m_figures[0]->comment ()));
	} else {
		ui->diagComboBox->setCurrentIndex (keep_old_fig);
	}
	update_figure_display ();
}

/* Called from the Remove Analysis dialog.  TO_DELETE may be empty and we still may have made
   changes (removing analysis).  */
void MainWindow::remove_nodes (const std::vector<game_state *> &to_delete)
{
	game_state *root = m_game->get_root ();
	for (auto st: to_delete) {
		st->walk_tree ([board = ui->gfx_board, root] (game_state *st)
			       {
				       board->transfer_displayed (st, root); return true;
			       });
		st->disconnect ();
	}

	ui->gfx_board->setModified ();
	m_undo_stack.clear ();
	m_undo_stack_pos = 0;
	update_undo_menus ();

	update_figures ();
	update_game_tree ();
	/* Force redraw.  */
	set_displayed (ui->gfx_board->displayed ());
}

void MainWindow::set_game_position (game_state *gs)
{
	/* Have to call this first so we trace the correct path in the game tree
	   once set_displayed calls back into there.  */
	gs->make_active ();
	set_displayed (gs);
}

void MainWindow::mark_dead_external (int x, int y)
{
	ui->gfx_board->mark_dead_external (x, y);
}

/* Determine if we should update the comment edit field with the data from the game
   record when navigating to a new move.  */
bool MainWindow::comments_from_game_p ()
{
	/* In online matches, comments are only appended to.  When setting up the window,
	   the main comment edit is set as readonly, and that's a good way to test.  */
	return !ui->commentEdit->isReadOnly ();
}

void MainWindow::setMoveData (const game_state *gs)
{
	const go_board &b = gs->get_board ();
	bool is_root_node = gs->root_node_p ();
	size_t brothers = gs->n_siblings ();
	size_t sons = gs->n_children ();
	stone_color to_move = gs->to_move ();
	int move_nr = gs->move_number ();
	int var_nr = gs->var_number ();

	GameMode mode = m_gamemode;
	bool good_mode = mode == modeNormal || mode == modeObserve || mode == modeObserveMulti || mode == modeObserveGTP || mode == modeBatch || mode == modePostMatch;

	ui->navBackward->setEnabled (good_mode && !is_root_node);
	ui->navForward->setEnabled (good_mode && sons > 0);
	ui->navAutoplay->setEnabled (good_mode && sons > 0);
	ui->navFirst->setEnabled (good_mode && !is_root_node);
	ui->navLast->setEnabled (good_mode && sons > 0);
	ui->navPrevComment->setEnabled (good_mode && !is_root_node);
	ui->navNextComment->setEnabled (good_mode && sons > 0);
	ui->navPrevFigure->setEnabled (good_mode && !is_root_node);
	ui->navNextFigure->setEnabled (good_mode && sons > 0);
	ui->navIntersection->setEnabled (good_mode);

	switch (mode)
	{
	case modeNormal:
	case modePostMatch:
		navSwapVariations->setEnabled(!is_root_node);

		/* fall through */
	case modeBatch:
		ui->navPrevVar->setEnabled (gs->has_prev_sibling ());
		ui->navNextVar->setEnabled (gs->has_next_sibling ());
		ui->navStartVar->setEnabled (!is_root_node);
		ui->navMainBranch->setEnabled (!is_root_node);
		ui->navNextBranch->setEnabled (sons > 0);

		/* fall through */
	case modeObserve:
	case modeObserveGTP:
		break;

	case modeScore:
	case modeEdit:
		ui->navPrevVar->setEnabled (false);
		ui->navNextVar->setEnabled (false);
		ui->navStartVar->setEnabled (false);
		ui->navMainBranch->setEnabled (false);
		ui->navNextBranch->setEnabled (false);
		navSwapVariations->setEnabled (false);
		break;
	default:
		break;
	}

	/* Refresh comment from move unless we are in a game mode that just keeps
	   appending to the comment.  */
	if (comments_from_game_p ())
		refresh_comment ();

	update_figures ();

	QString w_str = QObject::tr("W");
	QString b_str = QObject::tr("B");

	QString s (QObject::tr ("Move") + " ");
	s.append (QString::number (move_nr));
	if (move_nr == 0) {
		/* Nothing to add for the root.  */
	} else if (gs->was_pass_p ()) {
		s.append(" (" + (to_move == black ? w_str : b_str) + " ");
		s.append(" " + QObject::tr("Pass") + ")");
	} else if (gs->was_score_p ()) {
		s.append(tr (" (Scoring)"));
	} else if (!gs->was_edit_p ()) {
		int x = gs->get_move_x ();
		int y = gs->get_move_y ();
		s.append(" (");
		auto pair = b.coords_name (x, y, ui->gfx_board->sgf_coords ());
		s.append((to_move == black ? w_str : b_str) + " ");
		s.append(QString::fromStdString (pair.first));
		s.append(QString::fromStdString (pair.second));
		s.append(")");
	}
	s.append (QObject::tr ("\nVariation ") + QString::number (var_nr)
		  + QObject::tr (" of ") + QString::number (1 + brothers) + "\n");
	s.append (QString::number (sons) + " ");
	if (sons == 1)
		s.append (QObject::tr("child position"));
	else
		s.append (QObject::tr("child positions"));
	ui->moveNumLabel->setText (s);

	bool warn = false;
	if (gs->was_move_p () && gs->get_move_color () == to_move)
		warn = true;
	if (gs->root_node_p ()) {
		bool empty = b.position_empty_p ();
		bool b_to_move = to_move == black;
		warn |= empty != b_to_move;
	}
	ui->turnWarning->setVisible (warn);
	ui->colorButton->setChecked (to_move == white);
#if 0
	statusTurn->setText(" " + s.right (s.length() - 5) + " ");  // Without 'Move '
	statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
#endif

	if (to_move == black)
		ui->turnLabel->setText(QObject::tr ("Black to play"));
	else
		ui->turnLabel->setText(QObject::tr ("White to play"));

	const QStyle *style = qgo_app->style ();
	int iconsz = style->pixelMetric (QStyle::PixelMetric::PM_ToolBarIconSize);

	QSize sz (iconsz, iconsz);
	ui->goPrevButton->setIconSize (sz);
	ui->goNextButton->setIconSize (sz);
	ui->goFirstButton->setIconSize (sz);
	ui->goLastButton->setIconSize (sz);
	ui->prevCommentButton->setIconSize (sz);
	ui->nextCommentButton->setIconSize (sz);
	ui->prevNumberButton->setIconSize (sz);
	ui->nextNumberButton->setIconSize (sz);

	update_pass_button ();

	ui->scoreTools->setVisible (mode == modeScore || mode == modeScoreRemote || gs->was_score_p ());
	ui->normalTools->setVisible (mode != modeScore && mode != modeScoreRemote && !gs->was_score_p ());

	if (mode == modeNormal) {
		ui->normalTools->wtimeView->set_time (gs, white);
		ui->normalTools->btimeView->set_time (gs, black);
	}
	// Update slider
	toggleSliderSignal (false);

	int mv = gs->active_var_max ();
	setSliderMax (mv);
	ui->slider->setValue (move_nr);
	toggleSliderSignal (true);
}

void MainWindow::on_colorButton_clicked (bool)
{
	game_state *st = ui->gfx_board->displayed ();
	stone_color col = st->to_move ();
	stone_color newcol = col == black ? white : black;
	push_undo (std::make_unique<undo_tomove_entry> ("Switch side to move", st, col, newcol));
	st->set_to_move (newcol);
	ui->colorButton->setChecked (col == white);
	setMoveData (st);
}

/* The "Edit Position" button: Switch to edit mode.  */
void MainWindow::doEditPos (bool toggle)
{
	if (toggle) {
		ui->editPosButton->setText (tr ("Cancel edit"));
		setGameMode (modeEdit);
	} else {
		ui->editPosButton->setText (tr ("Edit position"));
		setGameMode (m_remember_mode);
	}
}

/* The "Edit Game" button: Open a new window to edit observed game.  */
void MainWindow::doEdit ()
{
	go_game_ptr newgr = std::make_shared<game_record> (*m_game);
	// online mode -> don't score, open new Window instead
	MainWindow *w = new MainWindow (nullptr, newgr);

	connect (w->ui->refreshButton, &QPushButton::clicked,
		 [gr = m_game, w] (bool)
		 {
			 w->init_game_record (std::make_shared<game_record> (*gr));
			 w->ui->navLast->trigger ();
		 });
	w->ui->refreshButton->setEnabled (true);
	w->ui->refreshButton->setVisible (true);
	w->updateCaption ();

	w->ui->navLast->trigger ();
	w->show ();
}

/* Try to make a move and return status: the game state for the position after the
   move, and a boolean to indicate if the position was added (true) or if it existed
   before (false).  */
MainWindow::move_result MainWindow::make_move (game_state *from, int x, int y)
{
	stone_color col = from->to_move ();
	if (!from->valid_move_p (x, y, col))
		return std::make_pair (nullptr, false);
	for (game_state *c: from->children ())
		if (c->was_move_p () && c->get_move_x () == x && c->get_move_y () == y) {
			ui->gfx_board->transfer_displayed (from, c);
			return std::make_pair (c, false);
		}

	/* Construct the new position and check for ko.  */
	go_board new_board (from->get_board (), mark::none);
	new_board.add_stone (x, y, col);
	game_state *from_parent = from->prev_move ();
	if (from_parent != nullptr && from_parent->get_board ().position_equal_p (new_board))
		return std::make_pair (nullptr, false);

	ui->gfx_board->setModified ();
	game_state *st_new = from->add_child_move (new_board, col, x, y);
	if (m_gamemode == modeNormal) {
		auto name = new_board.coords_name (x, y, ui->gfx_board->sgf_coords ());
		QString qn = QString::fromStdString (name.first) + QString::fromStdString (name.second);
		push_undo (std::make_unique<undo_move_entry> ("Play " + qn, from, st_new,
							      from->find_child_idx (st_new)));
	}
	ui->gfx_board->transfer_displayed (from, st_new);
	return std::make_pair (st_new, true);
}

game_state *MainWindow::player_move (int x, int y)
{
	ui->gfx_board->set_time_warning (0);
	game_state *st = ui->gfx_board->displayed ();
	bool existed;
	std::tie (st, existed) = make_move (st, x, y);
	if (st == nullptr)
		return st;
	if (local_stone_sound)
		qgo->playStoneSound ();
	return st;
}

void MainWindow::doPass ()
{
	if (!ui->gfx_board->player_to_move_p ())
		return;
	ui->gfx_board->set_time_warning (0);
	if (m_gamemode == modeNormal || m_gamemode == modeComputer) {
		game_state *st = ui->gfx_board->displayed ();
		game_state *st_new = st->add_child_pass ();
		push_undo (std::make_unique<undo_move_entry> ("Pass", st, st_new,
							      st->find_child_idx (st_new)));
		ui->gfx_board->transfer_displayed (st, st_new);
	}
}

void MainWindow::doRealScore (bool toggle)
{
	qDebug("MainWindow::doRealScore()");
	/* Can be called when the user toggles the button, but also when a GTP board
	   enters scoring after two passes.  */
	ui->scoreButton->setChecked (toggle);
	if (toggle)
	{
		m_remember_tab = ui->toolsTabWidget->currentIndex();

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

void MainWindow::doCountDone ()
{
	QString rs = ui->scoreTools->result->text ();
	QString s = m_result_text;

	if (m_result < 0) {
		s.append (tr ("Black wins with %1").arg (-m_result));
	} else if (m_result > 0) {
		s.append (tr ("White wins with %1").arg (m_result));
	} else {
		s.append (rs);
	}

	std::string old_result = m_game->info ().result;
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

	game_state *st = ui->gfx_board->displayed ();
	m_pos_before_edit->add_child_tree (st);
	push_undo (std::make_unique<undo_move_entry> ("Score the game", m_pos_before_edit, st,
						      m_pos_before_edit->find_child_idx (st)));
	m_pos_before_edit = nullptr;
	doRealScore (false);
}

/* Called when setting marks outside of edit/score mode.  */
void MainWindow::notice_mark_change (const go_board &newpos)
{
	undo_edit_entry *uce = top_undo_entry<undo_edit_entry> ();
	if (uce != nullptr && !uce->was_edit_mode ()) {
		uce->update (newpos);
		return;
	}
	game_state *st = ui->gfx_board->displayed ();
	const go_board &b = st->get_board ();
	push_undo (std::make_unique<undo_edit_entry> ("Edit marks", st, b, st->to_move (),
						      newpos, st->to_move (), false));
}

void MainWindow::leave_edit_modify ()
{
	game_state *st = ui->gfx_board->displayed ();
	const go_board &b = st->get_board ();
	push_undo (std::make_unique<undo_edit_entry> ("Edit position", m_pos_before_edit,
						      m_pos_before_edit->get_board (), m_pos_before_edit->to_move (),
						      b, st->to_move (), true));
	m_pos_before_edit->replace (b, st->to_move ());
	set_displayed (m_pos_before_edit);
	m_pos_before_edit = nullptr;
	m_game->release_game_state (st);
	ui->gfx_board->setModified ();
	ui->editPosButton->setText (tr ("Edit position"));
}

void MainWindow::leave_edit_append ()
{
	game_state *st = ui->gfx_board->displayed ();
	const go_board &b = st->get_board ();
	game_state *new_st = m_pos_before_edit->add_child_edit (b, st->to_move ());
	push_undo (std::make_unique<undo_move_entry> ("Append edited position", m_pos_before_edit,
						      new_st, m_pos_before_edit->find_child_idx (new_st)));
	set_displayed (new_st);
	m_pos_before_edit = nullptr;
	ui->gfx_board->setModified ();
	ui->editPosButton->setText (tr ("Edit position"));
}

void MainWindow::leave_edit_prepend ()
{
	game_state *st = ui->gfx_board->displayed ();
	const go_board &b = st->get_board ();
	game_state *parent = m_pos_before_edit->prev_move ();
	if (parent == nullptr)
		return;
	game_state *new_st = parent->replace_child_edit (m_pos_before_edit, b, st->to_move ());
	push_undo (std::make_unique<undo_insert_entry> ("Insert edited position", parent,
							new_st, parent->find_child_idx (new_st)));
	set_displayed (new_st);
	m_pos_before_edit = nullptr;
	ui->gfx_board->setModified ();
	ui->editPosButton->setText (tr ("Edit position"));
}

void MainWindow::add_engine_pv (game_state *st, game_state *pv)
{
	st->add_child_tree (pv);

	push_undo (std::make_unique<undo_move_entry> ("Append engine PV", st,
						      pv, st->find_child_idx (pv)));

	ui->gfx_board->setModified ();
	update_game_tree ();
	update_figures ();
}

void MainWindow::set_observer_model (QStandardItemModel *m)
{
	ui->ListView_observers->setModel (m);
	ui->ListView_observers->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->ListView_observers->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

void MainWindow_GTP::start_game (const Engine &program, bool b_is_comp, bool w_is_comp, const go_board &b)
{
	ui->gfx_board->set_player_colors (!w_is_comp, !b_is_comp);
	m_starting_up = 1;
	auto g = create_gtp (program, b.size_x (), b.size_y (), m_game->info ().komi);
	if (b_is_comp)
		m_gtp_b = g;
	else
		m_gtp_w = g;
	disconnect (ui->passButton, &QPushButton::clicked, nullptr, nullptr);
	connect (ui->passButton, &QPushButton::clicked, this, &MainWindow_GTP::player_pass);
	connect (ui->undoButton, &QPushButton::clicked, this, &MainWindow_GTP::player_undo);
	connect (ui->resignButton, &QPushButton::clicked, this, &MainWindow_GTP::player_resign);
	connect (ui->againButton, &QPushButton::clicked, this, &MainWindow_GTP::play_again);
	ui->againButton->show ();
}

MainWindow_GTP::MainWindow_GTP (QWidget *parent, go_game_ptr gr, QString opener_scrkey, const Engine &program,
				const time_settings &ts, bool b_is_comp, bool w_is_comp)
	: MainWindow (parent, gr, opener_scrkey, modeComputer, ts), GTP_Eval_Controller (this)
{
	game_state *root = gr->get_root ();
	game_state *st = root;
	m_game_position = root;

	while (st->n_children () > 0)
		st = st->next_primary_move ();
	m_start_positions.push_back (st);

	const go_board &b = root->get_board ();
	start_game (program, b_is_comp, w_is_comp, b);
}

MainWindow_GTP::MainWindow_GTP (QWidget *parent, go_game_ptr gr, game_state *st, QString opener_scrkey, const Engine &program,
				const time_settings &ts, bool b_is_comp, bool w_is_comp)
	: MainWindow (parent, gr, opener_scrkey, modeComputer, ts), GTP_Eval_Controller (this)
{
	game_state *root = gr->get_root ();
	m_start_positions.push_back (st);
	m_game_position = root;
	ui->gfx_board->set_game_position (m_game_position);
	update_pass_button ();

	const go_board &b = root->get_board ();
	start_game (program, b_is_comp, w_is_comp, b);
}

MainWindow_GTP::MainWindow_GTP (QWidget *parent, go_game_ptr gr, QString opener_scrkey,
				const Engine &program_w, const Engine &program_b,
				const time_settings &ts, int n_games, bool book)
	: MainWindow (parent, gr, opener_scrkey, modeObserveGTP, ts), GTP_Eval_Controller (this)
{
	game_state *root = gr->get_root ();
	game_state *primary = root;
	while (primary->n_children () > 0)
		primary = primary->next_primary_move ();

	m_game_position = gr->get_root ();
	ui->gfx_board->set_game_position (m_game_position);
	update_pass_button ();
	for (int i = 0; i < n_games; i++)
		if (book) {
			std::function<bool (game_state *)> f = [this] (game_state *st) -> bool
				{
					if (st->n_children () == 0)
						m_start_positions.push_back (st);
					return true;
				};
			gr->get_root ()->walk_tree (f);
		} else
			m_start_positions.push_back (primary);

	ui->gfx_board->set_player_colors (false, false);
	m_starting_up = 2;
	const go_board &b = root->get_board ();
	m_gtp_w = create_gtp (program_w, b.size_x (), b.size_y (), m_game->info ().komi);
	m_gtp_b = create_gtp (program_b, b.size_x (), b.size_y (), m_game->info ().komi);
	connect (ui->followButton, &QPushButton::clicked, [this] (bool) { set_displayed (m_game_position); });
}

MainWindow_GTP::~MainWindow_GTP ()
{
	delete m_gtp_w;
	delete m_gtp_b;
}

void MainWindow_GTP::play_again ()
{
	setGameMode (modeComputer);
	disconnect (ui->passButton, &QPushButton::clicked, nullptr, nullptr);
	connect (ui->passButton, &QPushButton::clicked, this, &MainWindow_GTP::player_pass);
	ui->gfx_board->set_player_colors (m_gtp_w == nullptr, m_gtp_b == nullptr);
	m_game_position = m_game->get_root ();
	set_displayed (m_game_position);
	m_start_positions.push_back (m_game_position);
	setup_game ();
}

void MainWindow_GTP::request_next_move ()
{
	if (m_game_position->was_move_p ()) {
		stone_color prev_c = m_game_position->get_move_color ();
		move_timer *mt = prev_c == white ? &m_timer_white : &m_timer_black;
		mt->report (m_game_position, prev_c);
	}
	stone_color c = m_game_position->to_move ();
	GTP_Process *p = c == white ? m_gtp_w : m_gtp_b;
	if (p != nullptr) {
		move_timer *mt = c == white ? &m_timer_white : &m_timer_black;
		if (mt->settings ().system != time_system::none)
			p->send_remaining_time (c, QString::fromStdString (mt->report_gtp ()));
		set_analysis_state (m_game, m_game_position);
		p->request_move (c, true);
		mt->start ();
		ui->againButton->setEnabled (false);
	}
}

void MainWindow_GTP::gtp_setup_success (GTP_Process *)
{
	if (--m_setting_up > 0)
		return;
	show ();
	m_timer_white.reset ();
	m_timer_black.reset ();
	request_next_move ();
}

void MainWindow_GTP::setup_game ()
{
	game_state *st = m_start_positions.back ();
	if (m_game_position != st)
		ui->gfx_board->transfer_displayed (m_game_position, st);
	m_game_position = st;
	ui->gfx_board->set_game_position (m_game_position);
	update_pass_button ();

	m_setting_up = 0;
	if (m_gtp_w) {
		m_setting_up++;
		m_gtp_w->setup_initial_position (st);
	}
	if (m_gtp_b) {
		m_setting_up++;
		m_gtp_b->setup_initial_position (st);
	}
}

void MainWindow_GTP::gtp_startup_success (GTP_Process *p)
{
	p->setup_timing (m_timer_white.settings ());
	if (--m_starting_up > 0)
		return;
	setup_game ();
}

void MainWindow_GTP::time_loss (stone_color col)
{
	QString result = col == white ? "B+T" : "W+T";
	if (two_engines ()) {
		if (col == white)
			game_end (result, black);
		else
			game_end (result, white);
		return;
	}

	QString report;
	if (col == white) {
		report = tr ("Black wins on time.");
	} else {
		report = tr ("White wins on time.");
	}

	m_game->set_result (result.toStdString ());
	setGameMode (modeNormal);
	QMessageBox mb (QMessageBox::Information, tr ("Clock has run out"), report,
			QMessageBox::Ok | QMessageBox::Default);
	mb.exec ();
}

/* Return true if the game should continue.  */
bool MainWindow_GTP::stop_move_timer ()
{
	if (game_mode () != modeComputer && game_mode () != modeObserveGTP)
		return false;

	stone_color col = m_game_position->to_move ();
	move_timer *mt = col == white ? &m_timer_white : &m_timer_black;
	bool has_time = mt->stop (true);
	if (!has_time) {
		time_loss (col);
		return false;
	}
	return true;
}

void MainWindow_GTP::gtp_played_move (GTP_Process *p, int x, int y)
{
	ui->againButton->setEnabled (true);

	if (!stop_move_timer ())
		return;

	if (local_stone_sound)
		qgo->playStoneSound ();
	stone_color col = m_game_position->to_move ();

	game_state *st = m_game_position;
	game_state *st_new;
	bool existed;
	std::tie (st_new, existed) = make_move (st, x, y);
	if (st_new == nullptr) {
		QMessageBox mb (QMessageBox::Information, tr ("Invalid move by the engine"),
				tr ("An invalid move was played by the engine, game terminated."),
				QMessageBox::Ok | QMessageBox::Default);
		mb.exec ();
		if (m_gtp_w)
			m_gtp_w->quit ();
		if (m_gtp_b)
			m_gtp_b->quit ();
		setGameMode (modeNormal);
		ui->againButton->hide ();
		return;
	}
	st->make_child_primary (st_new);
	if (m_eval_state != nullptr) {
		for (const game_state *c: m_eval_state->children ()) {
			if (!c->was_move_p () || c->get_move_x () != x || c->get_move_y () != y)
				continue;
			st_new->update_eval (*c);
		}
	}
	ui->gfx_board->setModified ();
	ui->gfx_board->transfer_displayed (st, st_new);
	m_game_position = st_new;
	ui->gfx_board->set_game_position (st_new);
	update_pass_button ();
	update_game_tree ();

	if (two_engines ()) {
		GTP_Process *other = m_gtp_w == p ? m_gtp_b : m_gtp_w;
		other->played_move (col, x, y);
		request_next_move ();
	} else {
		move_timer *mt = m_gtp_b == p ? &m_timer_white : &m_timer_black;
		mt->start ();
	}
}

void MainWindow_GTP::gtp_report_score (GTP_Process *p, const QString &s)
{
	QString txt;
	if (p == m_gtp_w) {
		if (!s.isEmpty ()) {
			m_score_report = tr ("Reported score by White: ") + s + "\n";
			m_winner_1 = s[0] == 'B' ? black : s[0] == 'W' ? white : none;
		}
		m_gtp_b->request_score ();
	} else {
		if (!s.isEmpty ()) {
			m_score_report += tr ("Reported score by Black: ") + s + "\n";
			m_winner_2 = s[0] == 'B' ? black : s[0] == 'W' ? white : none;
		}
		stone_color winner = unknown;
		if (m_winner_1 != unknown && m_winner_2 != unknown && m_winner_1 != m_winner_2)
			m_disagreements++;
		else if (m_winner_1 != unknown)
			winner = m_winner_1;
		else if (m_winner_2 != unknown)
			winner = m_winner_2;
		if (m_score_report.isEmpty ())
			m_score_report = tr ("Neither program reported a score.\n");
		game_end (m_score_report, winner);
	}
}

static void append_result (game_state *st, const QString &result)
{
	const std::string &c = st->comment ();
	bool ends_in_nl = c.length () == 0 || c.back () == '\n';
	st->set_comment (c + (!ends_in_nl ? "\n" : "") + result.toStdString ());
}

void MainWindow_GTP::game_end (const QString &result, stone_color winner)
{
	if (winner == white)
		m_wins_w++;
	else if (winner == black)
		m_wins_b++;
	else if (winner == none)
		m_jigo++;
	game_state *st = m_start_positions.back ();
	m_n_games++;
	QString res = tr ("Game #%1:\n").arg (m_n_games) + result;
	append_result (st, res);
	append_result (m_game_position, res);
	game_state *r = m_game->get_root ();
	if (r != st)
		append_result (r, res);

	refresh_comment ();
	ui->gfx_board->setModified ();
	m_start_positions.pop_back ();

	if (m_start_positions.size () == 0) {
		QString stats = tr ("Wins for White/Black: %1/%2").arg (m_wins_w).arg (m_wins_b);
		if (m_jigo > 0)
			stats += tr (" Jigo: %1").arg (m_jigo);
		if (m_disagreements > 0)
			stats += tr (" Disagreements: %1").arg (m_disagreements);
		stats += "\n";
		append_result (r, "======================\n" + stats);
		QMessageBox mb (QMessageBox::Information, tr ("Game end"),
				tr ("Engine play has completed."),
				QMessageBox::Ok | QMessageBox::Default);
		mb.exec ();
		setGameMode (modeNormal);
		return;
	}
	setup_game ();
}

void MainWindow_GTP::enter_scoring ()
{
	if (two_engines ()) {
		m_winner_1 = m_winner_2 = unknown;
		m_gtp_w->request_score ();
	} else {
		/* As of now, we're disconnected, and the scoring function should
		   remember normal mode.  */
		setGameMode (modeNormal);
		doRealScore (true);
	}
}

void MainWindow_GTP::gtp_played_pass (GTP_Process *p)
{
	ui->againButton->setEnabled (true);
	if (!stop_move_timer ())
		return;

	if (local_stone_sound)
		qgo->playPassSound();
	game_state *st = m_game_position;
	stone_color col = st->to_move ();

	game_state *st_new = st->add_child_pass ();
	ui->gfx_board->transfer_displayed (st, st_new);
	m_game_position = st_new;
	ui->gfx_board->set_game_position (st_new);
	update_pass_button ();
	update_game_tree ();

	if (st->was_pass_p ())
		enter_scoring ();
	else if (two_engines ()) {
		GTP_Process *other = m_gtp_w == p ? m_gtp_b : m_gtp_w;
		other->played_move_pass (col);
		request_next_move ();
	} else {
		move_timer *mt = m_gtp_b == p ? &m_timer_white : &m_timer_black;
		mt->start ();
	}
}

void MainWindow_GTP::gtp_played_resign (GTP_Process *p)
{
	m_timer_white.stop (false);
	m_timer_black.stop (false);

	QString result = p == m_gtp_w ? tr ("B+R") : tr ("W+R");

	if (two_engines ()) {
		game_end (tr ("Game result: ") + result + "\n", p == m_gtp_w ? black : white);
		return;
	}
	m_game->set_result (result.toStdString ());
	setGameMode (modeNormal);

	QMessageBox mb (QMessageBox::Information, tr ("Game end"),
		       tr ("The computer has resigned the game."),
		       QMessageBox::Ok | QMessageBox::Default);
	mb.exec ();
}

void MainWindow_GTP::gtp_failure (GTP_Process *, const QString &err)
{
	ui->againButton->hide ();

	if (game_mode () != modeComputer && game_mode () != modeObserveGTP)
		return;
	show ();
	setGameMode (modeNormal);
	QMessageBox msg(QString (QObject::tr("Error")), err,
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default,
			Qt::NoButton, Qt::NoButton);
	msg.activateWindow();
	msg.raise();
	msg.exec();
}

void MainWindow_GTP::gtp_exited (GTP_Process *)
{
	ui->againButton->hide ();

	if (game_mode () == modeComputer || game_mode () == modeObserveGTP) {
		setGameMode (modeNormal);
		show ();
		QMessageBox::warning (this, PACKAGE, QObject::tr ("GTP process exited unexpectedly."));
	}
}

game_state *MainWindow_GTP::player_move (int x, int y)
{
	if (game_mode () == modeComputer)
		stop_move_timer ();

	game_state *new_st = MainWindow::player_move (x, y);
	if (new_st == nullptr)
		return new_st;

	if (game_mode () != modeComputer)
		return new_st;

	m_game_position = new_st;
	ui->gfx_board->set_game_position (new_st);
	update_pass_button ();

	stone_color col = new_st->get_move_color ();

	auto *gtp = single_engine ();
	gtp->played_move (col, x, y);
	request_next_move ();
	return new_st;
}


void MainWindow_GTP::player_pass ()
{
	if (game_mode () == modeComputer)
		stop_move_timer ();

	const game_state *st = ui->gfx_board->displayed ();
	stone_color col = ui->gfx_board->to_move ();
	if (!ui->gfx_board->player_to_move_p ())
		return;

	doPass ();
	if (game_mode () != modeComputer)
		return;

	auto *gtp = single_engine ();
	m_game_position = ui->gfx_board->displayed ();
	ui->gfx_board->set_game_position (m_game_position);
	update_pass_button ();

	gtp->played_move_pass (col);
	if (st->was_pass_p ())
		enter_scoring ();
	else
		request_next_move ();
}

void MainWindow_GTP::player_undo ()
{
	/* Shouldn't happen (button will be invisible), this is purely defensive.  */
	if (game_mode () != modeComputer)
		return;

	if (m_game_position->root_node_p ())
		return;
	game_state *p = m_game_position->prev_move ();
	if (p->root_node_p ())
		return;
	auto g = single_engine ();
	g->undo_move ();
	g->undo_move ();
	m_game_position = p->prev_move ();
	update_pass_button ();
	ui->gfx_board->set_game_position (m_game_position);
	set_displayed (m_game_position);
}

void MainWindow_GTP::player_resign ()
{
	m_timer_white.stop (false);
	m_timer_black.stop (false);
	if (game_mode () != modeComputer)
		return;

	setGameMode (modeNormal);
	if (ui->gfx_board->player_is (black))
		m_game->set_result ("W+R");
	else
		m_game->set_result ("B+R");
}

void MainWindow_GTP::eval_received (const analyzer_id &id, const QString &move, int visits, bool have_score)
{
	update_analyzer_ids (id, have_score);
	game_state *st = m_game_position;
	st->update_eval (*m_eval_state);
	set_eval (move, m_primary_eval, m_primary_have_score, m_primary_score,
		  st->to_move (), visits);
	update_game_tree ();
}

void MainWindow::update_analysis (analyzer state)
{
	ui->evalView->setVisible (state == analyzer::running || state == analyzer::paused);

	QAction *checked_engine = engineGroup->checkedAction ();

	ui->anConnect->setEnabled (state == analyzer::disconnected && checked_engine != nullptr);
	ui->anConnect->setChecked (state != analyzer::disconnected);
	ui->anDisconnect->setEnabled (state != analyzer::disconnected);
	ui->anChooseMenu->setEnabled (!ui->anDisconnect->isEnabled ());
	ui->normalTools->anStartButton->setChecked (state != analyzer::disconnected);
	ui->normalTools->anStartButton->setEnabled (ui->normalTools->anStartButton->isChecked () || checked_engine != nullptr);
	if (state == analyzer::disconnected)
		ui->normalTools->anStartButton->setIcon (QIcon (":/images/exit.png"));
	else if (state == analyzer::starting)
		ui->normalTools->anStartButton->setIcon (QIcon (":/images/power-standby.png"));
	else
		ui->normalTools->anStartButton->setIcon (QIcon (":/images/power-on.png"));

	ui->anPause->setEnabled (state != analyzer::disconnected);
	ui->normalTools->anPauseButton->setEnabled (state != analyzer::disconnected);
	ui->normalTools->anPauseButton->setChecked (state == analyzer::paused);
	ui->anPause->setChecked (state == analyzer::paused);

	/* A precaution: let's not have two parts of the program get into fights whether to add or
	   remove analysis.  */
	ui->anEdit->setEnabled (state != analyzer::running);

	if (state == analyzer::disconnected) {
		clear_primary_eval ();
		clear_2nd_eval ();
	}
}

/* After m_score has been set, calculate a result based on the state of the area/territory radio buttons.
   The result we calculate is used later in doCountDone.  */
void MainWindow::update_score_type ()
{
	bool terr = ui->scoreTools->scoreTerrButton->isChecked ();
	ui->scoreTools->capturesWhiteLabel->setEnabled (terr);
	ui->scoreTools->capturesBlackLabel->setEnabled (terr);
	ui->scoreTools->capturesWhite->setEnabled (terr);
	ui->scoreTools->capturesBlack->setEnabled (terr);
	ui->scoreTools->stonesWhiteLabel->setEnabled (!terr);
	ui->scoreTools->stonesBlackLabel->setEnabled (!terr);
	ui->scoreTools->stonesWhite->setEnabled (!terr);
	ui->scoreTools->stonesBlack->setEnabled (!terr);
	double extra_w = terr ? m_score.caps_w : m_score.stones_w;
	double extra_b = terr ? m_score.caps_b : m_score.stones_b;
	double total_w = m_score.score_w + extra_w + m_game->info ().komi;
	double total_b = m_score.score_b + extra_b;

	ui->scoreTools->totalWhite->setText (QString::number (total_w));
	ui->scoreTools->totalBlack->setText (QString::number (total_b));
	m_result = m_score.score_w + extra_w + m_game->info ().komi - m_score.score_b - extra_b;
	if (m_result < 0)
		ui->scoreTools->result->setText ("B+" + QString::number (-m_result));
	else if (m_result == 0)
		ui->scoreTools->result->setText ("Jigo");
	else
		ui->scoreTools->result->setText ("W+" + QString::number (m_result));

	m_result_text = "";
	QTextStream ts (&m_result_text);
	ts << tr("White") << "\n" << m_score.score_w << " + " << extra_w << " + " << m_game->info ().komi << " = " << total_w << "\n";
	ts << tr("Black") << "\n" << m_score.score_b << " + " << extra_b << " = " << total_b << "\n\n";
}

void MainWindow::recalc_scores (const go_board &b)
{
	m_score = b.get_scores ();

	ui->scoreTools->capturesWhite->setText (QString::number (m_score.caps_w));
	ui->scoreTools->capturesBlack->setText (QString::number (m_score.caps_b));
	ui->scoreTools->stonesWhite->setText (QString::number (m_score.stones_w));
	ui->scoreTools->stonesBlack->setText (QString::number (m_score.stones_b));
	ui->normalTools->capturesWhite->setText (QString::number (m_score.caps_w));
	ui->normalTools->capturesBlack->setText (QString::number (m_score.caps_b));

	ui->scoreTools->terrWhite->setText (QString::number (m_score.score_w));
	ui->scoreTools->terrBlack->setText (QString::number (m_score.score_b));

	update_score_type ();
}

void MainWindow::setSliderMax (int n)
{
	if (n < 0)
		n = 0;

	ui->slider->setMaximum (n);
	ui->sliderRightLabel->setText (QString::number (n));
}

void MainWindow::sliderChanged (int n)
{
	if (sliderSignalToggle)
		nav_goto_nth_move_in_var (n);
}

void MainWindow::toggleSidebar(bool toggle)
{
	if (!toggle)
		ui->toolsFrame->hide();
	else
		ui->toolsFrame->show();
}

void MainWindow::setTimes (int bt, const QString &bstones, int wt, const QString &wstones,
			   bool warn_black, bool warn_white, int timer_cnt)
{
	QString wtime = sec_to_time (wt);
	QString btime = sec_to_time (bt);

	if (bstones != "-1")
		ui->normalTools->btimeView->set_text (btime + " / " + bstones);
	else
		ui->normalTools->btimeView->set_text (btime);

	if (wstones != "-1")
		ui->normalTools->wtimeView->set_text (wtime + " / " + wstones);
	else
		ui->normalTools->wtimeView->set_text (wtime);

	// warn if I am within the last 10 seconds
	if (m_gamemode == modeMatch)
	{
		if (ui->gfx_board->player_is (black))
		{
			ui->normalTools->wtimeView->flash (false);
			ui->normalTools->btimeView->flash (timer_cnt % 2 != 0 && warn_black);
			if (warn_black) {
				ui->gfx_board->set_time_warning (bt);
				qgo->playTimeSound();
			}
		}
		else if (ui->gfx_board->player_is (white) && warn_white)
		{
			ui->normalTools->btimeView->flash (false);
			ui->normalTools->wtimeView->flash (timer_cnt % 2 != 0 && warn_white);
			if (warn_white) {
				ui->gfx_board->set_time_warning (wt);
				qgo->playTimeSound();
			}
		}
	} else {
		ui->normalTools->btimeView->flash (false);
		ui->normalTools->wtimeView->flash (false);
	}
}

void MainWindow::update_undo_menus ()
{
	ui->editRedo->setEnabled (m_undo_stack.size () > m_undo_stack_pos);
	ui->editUndo->setEnabled (m_undo_stack_pos > 0);
	if (m_undo_stack_pos > 0) {
		ui->editUndo->setText (tr ("&Undo %1").arg (m_undo_stack[m_undo_stack_pos - 1]->op_str ()));
	} else
		ui->editUndo->setText (tr ("&Undo"));
	if (m_undo_stack_pos < m_undo_stack.size ()) {
		ui->editRedo->setText (tr ("&Redo %1").arg (m_undo_stack[m_undo_stack_pos]->op_str ()));
	} else
		ui->editRedo->setText (tr ("R&edo"));
}

void MainWindow::push_undo (std::unique_ptr<undo_entry> e)
{
	m_undo_stack.erase (std::begin (m_undo_stack) + m_undo_stack_pos, std::end (m_undo_stack));
	m_undo_stack.emplace_back (std::move (e));
	m_undo_stack_pos = m_undo_stack.size ();
	update_undo_menus ();
}

void MainWindow::perform_undo ()
{
	if (m_undo_stack_pos == 0)
		return;
	int pos = m_undo_stack_pos;
#if 0
	if (pos == m_undo_stack.size ()) {
		/* We need to save the current position for redo, if it isn't in the undo stack yet.  */
		push_undo ("Position before undos", false);
	}
#endif
	m_undo_stack_pos = pos - 1;
	auto &ur = m_undo_stack[m_undo_stack_pos];
	game_state *st = ur->apply_undo ();
	if (st != nullptr)
		set_displayed (st);

	update_game_record ();
	update_game_tree ();
	update_undo_menus ();
	refresh_comment ();
}

void MainWindow::perform_redo ()
{
	if (m_undo_stack_pos == m_undo_stack.size ())
		return;
	auto &ur = m_undo_stack[m_undo_stack_pos++];
	game_state *st = ur->apply_redo ();
	if (st != nullptr)
		set_displayed (st);

	update_game_record ();
	update_game_tree ();
	update_undo_menus ();
	refresh_comment ();
}

/* Called whenever a new evaluation comes in from NEW_ID.  We update the evaluation graph.  */
void MainWindow::update_analyzer_ids (const analyzer_id &new_id, bool have_score)
{
	int old_cnt = m_an_id_model.rowCount ();
	m_an_id_model.notice_analyzer_id (new_id, have_score);
	if (m_an_id_model.rowCount () > old_cnt) {
		if (old_cnt == 1)
			ui->anIdListView->setVisible (true);
	}
	QModelIndex idx = ui->anIdListView->currentIndex ();
	if (!idx.isValid () && m_an_id_model.rowCount () > 0) {
		ui->anIdListView->setCurrentIndex (m_an_id_model.index (0, 0));
		idx = ui->anIdListView->currentIndex ();
	}
	ui->evalGraph->update (m_game, ui->gfx_board->displayed (), idx.isValid () ? idx.row () : 0);
}

void MainWindow::set_eval_bar (double eval)
{
	m_eval = eval;
	int w = ui->evalView->width ();
	int h = ui->evalView->height ();
	m_eval_canvas->setSceneRect (0, 0, w, h);
	m_eval_bar->setRect (0, 0, w, h * eval);
	m_eval_bar->setPos (0, 0);
	m_eval_mid->setRect (0, (h - 1) / 2, w, 2);
	m_eval_canvas->update ();
}

void MainWindow::clear_primary_eval ()
{
	ui->normalTools->anPrimaryBox->setTitle (tr ("Primary move"));
	ui->normalTools->primaryWR->setText ("");
	ui->normalTools->primaryVisits->setText ("");
	ui->normalTools->anPrimaryBox->setEnabled (false);
	ui->normalTools->anShownBox->setEnabled (false);
}

void MainWindow::clear_2nd_eval ()
{
	ui->normalTools->anShownBox->setTitle (tr ("Highlighted move"));
	ui->normalTools->shownWR->setText ("");
	ui->normalTools->shownVisits->setText ("");
}

QString MainWindow::format_eval (double eval, bool have_score, double score, stone_color to_move)
{
	int winrate_for = setting->readIntEntry ("ANALYSIS_WINRATE");
	stone_color wr_swap_col = winrate_for == 0 ? white : winrate_for == 1 ? black : none;
	if (to_move == wr_swap_col)
		eval = 1 - eval;

	QString ev_text = (winrate_for == 0 || (winrate_for == 2 && to_move == black)) ? tr ("B %1%") : tr ("W %1%");
	ev_text = ev_text.arg (QString::number (100 * eval, 'f', 1));
	if (have_score) {
		ev_text += ", ";
		if (score > 0)
			ev_text += tr ("B+%1").arg (QString::number (score, 'f', 2));
		else
			ev_text += tr ("W+%1").arg (QString::number (-score, 'f', 2));
	}
	return ev_text;
}

void MainWindow::set_eval (const QString &move, double eval, bool have_score, double score,
			   stone_color to_move, int visits)
{
	if (move.isEmpty ()) {
		clear_primary_eval ();
		return;
	}
	ui->evalView->setBackgroundBrush (QBrush (Qt::white));
	m_eval_bar->setBrush (QBrush (Qt::black));
	set_eval_bar (to_move == black ? eval : 1 - eval);

	ui->normalTools->anPrimaryBox->setTitle (tr ("Primary move: %1").arg (move));
	ui->normalTools->primaryWR->setText (format_eval (eval, have_score, score, to_move));
	ui->normalTools->primaryVisits->setText (QString::number (visits));

	ui->normalTools->anPrimaryBox->setEnabled (true);
	ui->normalTools->anShownBox->setEnabled (true);
}

void MainWindow::set_2nd_eval (const QString &move, double eval, bool have_score, double score,
			       stone_color to_move, int visits)
{
	if (move.isEmpty ()) {
		clear_2nd_eval ();
		return;
	}

	ui->normalTools->anShownBox->setTitle (tr ("Highlighted: %1").arg (move));
	ui->normalTools->shownWR->setText (format_eval (eval, have_score, score, to_move));
	ui->normalTools->shownVisits->setText (visits == 0 ? tr ("Game move") : QString::number (visits));
}

void MainWindow::grey_eval_bar ()
{
	ui->evalView->setBackgroundBrush (QBrush (Qt::lightGray));
	m_eval_bar->setBrush (QBrush (Qt::darkGray));
}

void MainWindow::perform_search ()
{
        go_game_ptr gr = std::make_shared<game_record> (*m_game);
        game_state *curr_pos = ui->gfx_board->displayed ();
        game_state *st = gr->get_root ()->follow_path (curr_pos->path_from_root ());

	show_pattern_search ();

	patsearch_window->do_search (gr, st, ui->gfx_board->get_selection ());
}
