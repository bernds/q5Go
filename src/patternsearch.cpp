#include <memory>

#include <QActionGroup>
#include <QFontMetrics>
#include <QMessageBox>
#include <QDir>
#include <QRegExp>

#include "clientwin.h"
#include "patternsearch.h"
#include "ui_helpers.h"

#include "ui_patternsearch_gui.h"

QGraphicsScene *create_scene_for (QGraphicsView *v)
{
	int w = v->width ();
	int h = v->height ();
	QGraphicsScene *s = new QGraphicsScene (0, 0, w, h, v);
	v->setScene (s);
	return s;
}

PatternSearchWindow::PatternSearchWindow ()
	: ui (new Ui::PatternSearchWindow), m_model (true), m_previewer (new FigureView (nullptr, true, true))
{
	ui->setupUi (this);
	go_board b (19);
	game_info info;
	info.name_w = QObject::tr ("White").toStdString ();
	info.name_b = QObject::tr ("Black").toStdString ();
	m_orig_game = m_game = std::make_shared<game_record> (b, black, info);
	ui->boardView->reset_game (m_game);
	ui->dbListView->setModel (&m_model);
	connect (&m_model, &gamedb_model::signal_changed, [this] ()
		 {
			 QString s = tr ("Game list: ");
			 ui->listDock->setWindowTitle (s + m_model.status_string ());
		 });
	connect (ui->dbListView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [this] (const QItemSelection &, const QItemSelection &) { update_selection (); });

	connect (ui->fileNewBoard, &QAction::triggered,
		 [=] (bool) { open_local_board (this, game_dialog_type::none, screen_key (this)); });
	connect (ui->fileClose, &QAction::triggered, this, &MainWindow::close);

	connect (ui->searchPattern, &QAction::triggered, [this] () { pattern_search (false); });
	connect (ui->searchAll, &QAction::triggered, [this] () { pattern_search (true); });
	connect (ui->resetButton, &QPushButton::clicked, [this] (bool) { m_model.reset_filters (); });
	connect (ui->actionReset, &QAction::triggered, [this] () { m_model.reset_filters (); });
	connect (ui->resetAll, &QAction::triggered, [this] ()
		 {
			 m_model.reset_filters ();
			 m_game = m_orig_game;
			 ui->boardView->reset_game (m_game);
			 clear_preview_cursor ();
		 });
	connect (ui->forgetAction, &QAction::triggered, [this] ()
		 {
			 clear_preview_cursor ();
			 ui->boardView->set_cont_data (nullptr);
			 m_previews.clear ();
			 m_search_cont = nullptr;
			 m_preview_scene->clear ();
			 m_stats_scene->clear ();
			 m_result_scene->clear ();
			 choices_resized ();
		 });
	connect (ui->dbListView, &ClickableListView::doubleclicked, this, &PatternSearchWindow::handle_doubleclick);

	connect (ui->openButton, &QPushButton::clicked, this, &PatternSearchWindow::do_open);

	connect (ui->navBackward, &QAction::triggered, this, &PatternSearchWindow::nav_previous_move);
	connect (ui->navForward, &QAction::triggered, this, &PatternSearchWindow::nav_next_move);
	connect (ui->navFirst, &QAction::triggered, this, &PatternSearchWindow::nav_goto_first_move);
	connect (ui->navLast, &QAction::triggered, this, &PatternSearchWindow::nav_goto_last_move);
	connect (ui->editDelete, &QAction::triggered, this, &PatternSearchWindow::editDelete);
	connect (ui->navGotoCont, &QAction::triggered, this, &PatternSearchWindow::nav_goto_cont);
	connect (ui->boardView, &SimpleBoard::signal_nav_forward, this, &PatternSearchWindow::nav_next_move);
	connect (ui->boardView, &SimpleBoard::signal_nav_backward, this, &PatternSearchWindow::nav_previous_move);

	connect (ui->helpUsing, &QAction::triggered, this, &PatternSearchWindow::slot_using);

	connect (ui->setPreferences, &QAction::triggered, client_window, &ClientWindow::slot_preferences);
	connect (ui->setDBPrefs, &QAction::triggered, [] () { client_window->dlgSetPreferences (6); });

	m_previewer->set_never_sync (true);
	m_previewer->set_show_hoshis (false);

	m_preview_scene = create_scene_for (ui->gameChoiceView);
	m_info_scene = create_scene_for (ui->gameInfoView);
	m_stats_scene = create_scene_for (ui->statsView);
	m_result_scene = create_scene_for (ui->resultView);

	connect (ui->gameChoiceView, &SizeGraphicsView::resized, this, &PatternSearchWindow::choices_resized);
	connect (ui->statsView, &SizeGraphicsView::resized, this, &PatternSearchWindow::redraw_stats);

	connect (ui->boardView, &SimpleBoard::signal_move_made, [this] () { update_actions (); });
	connect (ui->editClearSelect, &QAction::triggered, [this] () { ui->boardView->clear_selection (); });

	connect (ui->editStone, &QAction::triggered, this, &PatternSearchWindow::slot_choose_color);
	connect (ui->editWhite, &QAction::triggered, this, &PatternSearchWindow::slot_choose_color);
	connect (ui->editBlack, &QAction::triggered, this, &PatternSearchWindow::slot_choose_color);

	m_edit_group = new QActionGroup (this);
	m_edit_group->addAction (ui->editStone);
	m_edit_group->addAction (ui->editBlack);
	m_edit_group->addAction (ui->editWhite);
	ui->editStone->setChecked (true);

	connect (ui->viewPercent, &QAction::triggered, this, &PatternSearchWindow::slot_choose_view);
	connect (ui->viewNumbers, &QAction::triggered, this, &PatternSearchWindow::slot_choose_view);
	connect (ui->viewLetters, &QAction::triggered, this, &PatternSearchWindow::slot_choose_view);

	connect (ui->viewCoords, &QAction::toggled, [this] (bool toggle) { ui->boardView->set_show_coords (toggle); });
	ui->viewCoords->setChecked (setting->readBoolEntry ("BOARD_COORDS"));

	m_view_group = new QActionGroup (this);
	m_view_group->addAction (ui->viewPercent);
	m_view_group->addAction (ui->viewNumbers);
	m_view_group->addAction (ui->viewLetters);
	int val = setting->readIntEntry ("SEARCH_VIEW");
	if (val == 0) {
		ui->viewLetters->setChecked (true);
		ui->boardView->set_cont_view (pattern_cont_view::letters);
	} else if (val == 1) {
		ui->viewNumbers->setChecked (true);
		ui->boardView->set_cont_view (pattern_cont_view::numbers);
	} else {
		ui->viewPercent->setChecked (true);
		ui->boardView->set_cont_view (pattern_cont_view::percent);
	}

	addActions ({ ui->navForward, ui->navBackward, ui->navFirst, ui->navLast });
	addActions ({ ui->searchPattern, ui->searchAll });

	ui->anchorAction->setChecked (true);

	/* Later, once we have more ways of representing the data.  */
	ui->statsButtonFrame->setVisible (false);

	search_thread.start ();

	update_actions ();
	update_selection ();
	update_caption ();

	show ();
	restore_layout ();
}

PatternSearchWindow::~PatternSearchWindow ()
{
	search_thread.exit ();
	search_thread.wait ();
	delete last_pattern;

	delete m_edit_group;
	delete m_view_group;
	delete m_previewer;
}

void PatternSearchWindow::closeEvent (QCloseEvent *e)
{
	QString strKey = screen_key (this);

	QByteArray v1 = saveState ().toHex ();
	QByteArray v2 = saveGeometry ().toHex ();
	setting->writeEntry("SEARCHLAYOUT1_" + strKey, QString::fromLatin1 (v1));
	setting->writeEntry("SEARCHLAYOUT2_" + strKey, QString::fromLatin1 (v2));

	QMainWindow::closeEvent (e);
}

void PatternSearchWindow::restore_layout ()
{
	QString strKey = screen_key (this);
	QString s1 = setting->readEntry ("SEARCHLAYOUT1_" + strKey);
	QString s2 = setting->readEntry ("SEARCHLAYOUT2_" + strKey);

	if (s1.isEmpty () || s2.isEmpty ())
		return;
	QRegExp verify ("^[0-9A-Fa-f]*$");
	if (!verify.exactMatch (s1) || !verify.exactMatch (s2))
		return;

	restoreGeometry (QByteArray::fromHex (s2.toLatin1 ()));
	restoreState (QByteArray::fromHex (s1.toLatin1 ()));
}

void PatternSearchWindow::update_preview (preview &gmp)
{
	auto info = gmp.game->info ();
	const go_board &b = gmp.game->get_root ()->get_board ();

	int margin = 6;
	int img_size = m_preview_w - margin;

	m_previewer->reset_game (gmp.game);
	m_previewer->set_show_coords (false);
	m_previewer->set_margin (0);
	m_previewer->resizeBoard (img_size, img_size);
	m_previewer->set_displayed (gmp.state);

	board_rect crop_rect = gmp.selection;
	int min_width = std::min (std::max (8, crop_rect.height ()), b.size_x ());
	int min_height = std::min (std::max (8, crop_rect.width ()), b.size_y ());
	while (crop_rect.width () < min_width) {
		if (crop_rect.x1 > 0)
			crop_rect.x1--;
		if (crop_rect.width () >= min_width)
			break;
		if (crop_rect.x2 + 1 < b.size_x ())
			crop_rect.x2++;
	}
	while (crop_rect.height () < min_height) {
		if (crop_rect.y1 > 0)
			crop_rect.y1--;
		if (crop_rect.height () >= min_height)
			break;
		if (crop_rect.y2 + 1 < b.size_y ())
			crop_rect.y2++;
	}
	m_previewer->set_drawn_selection (gmp.selection);
	m_previewer->set_crop (crop_rect);
	QPixmap board_pm = m_previewer->draw_position (0);
	m_previewer->clear_selection ();
	m_previewer->clear_crop ();

	QPixmap pm (img_size, img_size);
	pm.fill (Qt::transparent);
	QPainter p;
	p.begin (&pm);
	p.drawImage (QPoint (0, 0), m_previewer->background_image (), m_previewer->wood_rect ());
	p.drawPixmap (0, 0, board_pm);
	p.end ();
	delete gmp.pixmap;

	auto callback = [this, &gmp] () { preview_clicked (gmp); };
	auto menu_callback = [this, &gmp] (QGraphicsSceneContextMenuEvent *e) -> bool
		{
			preview_menu (e, gmp);
			return true;
		};

	gmp.pixmap = new ClickablePixmap (pm, callback, menu_callback);
	m_preview_scene->addItem (gmp.pixmap);
	int szdiff_x = m_preview_w - img_size;
	int szdiff_y = m_preview_h - img_size;
	gmp.pixmap->setPos (gmp.x + szdiff_x / 2, gmp.y + szdiff_y / 2);
}

void PatternSearchWindow::clear_preview_cursor ()
{
	delete m_cursor;
	m_cursor = nullptr;
	m_cursor_preview = nullptr;
}

void PatternSearchWindow::set_preview_cursor (const preview &p)
{
	delete m_cursor;
	m_cursor = m_preview_scene->addRect (0, 0, m_preview_w, m_preview_h,
					     Qt::NoPen, QBrush (Qt::blue));
	m_cursor->setPos (p.x, p.y);
	m_cursor->setZValue (-1);
	ui->gameChoiceView->ensureVisible (m_cursor);
	m_cursor_preview = &p;
}

void PatternSearchWindow::choices_resized ()
{
	size_t n_elts = m_previews.size ();

	if (n_elts == 0)
		return;

	std::vector<board_preview *> preview_ptrs (n_elts);
	for (size_t i = 0; i < n_elts; i++)
		preview_ptrs[i] = &m_previews[i];
	std::tie (m_preview_w, m_preview_h) = layout_previews (ui->gameChoiceView, preview_ptrs, 0);
	for (auto &p: m_previews) {
		update_preview (p);
	}
	// ui->gameChoiceView->setSceneRect (m_preview_scene->itemsBoundingRect ());
	if (m_cursor_preview != nullptr)
		set_preview_cursor (*m_cursor_preview);
}


void PatternSearchWindow::update_caption ()
{
	QString s = tr ("Pattern search");
	if (m_game != m_orig_game) {
		QString title = QString::fromStdString (m_game->info ().title);
		QString player_w = QString::fromStdString (m_game->info ().name_w);
		QString player_b = QString::fromStdString (m_game->info ().name_b);

		if (title.length () > 0) {
			s += " - " + title;
		} else {
			s += " - " + player_w;
			s += " " + tr ("vs.") + " ";
			s += player_b;
		}
	}
	setWindowTitle (s);
}

void PatternSearchWindow::preview_clicked (const preview &p)
{
	apply_game_result (p.games_result);
	m_game = p.game;
	ui->boardView->reset_game (p.game);
	ui->boardView->set_displayed (p.state);
	ui->boardView->set_cont_data (&p.cont);
	slot_choose_view ();
	ui->boardView->set_selection (p.selection);

	delete last_pattern;
	last_pattern = new go_pattern (p.pat);

	set_preview_cursor (p);
	update_caption ();

	m_result_same = p.n_hits_same;
	m_result_inv = p.n_hits_inverted;
	m_result_n_games = p.games_result.popcnt ();
	redraw_result ();

	m_search_cont = &p.cont;
	redraw_stats ();
}

void PatternSearchWindow::preview_menu (QGraphicsSceneContextMenuEvent *e, const preview &p)
{
	QMenu menu;
	menu.addAction (QIcon (":/images/trash.png"), QObject::tr ("&Forget this search"),
			[this, &p] ()
			{
				delete p.pixmap;
				clear_preview_cursor ();
				m_previews.erase (std::remove_if (std::begin (m_previews), std::end (m_previews),
								  [&p] (const preview &other)
								  {
									  return &p == &other;
								  }),
						  std::end (m_previews));
				m_search_cont = nullptr;
				m_stats_scene->clear ();
				m_result_scene->clear ();
				choices_resized ();
			});
	menu.exec (e->screenPos ());
}

void PatternSearchWindow::update_actions ()
{
	game_state *st = ui->boardView->displayed ();
	ui->navForward->setEnabled (st->n_children () > 0);
	ui->navLast->setEnabled (st->n_children () > 0);
	ui->navFirst->setEnabled (!st->root_node_p ());
	ui->navBackward->setEnabled (!st->root_node_p ());
}

void PatternSearchWindow::slot_choose_view (bool)
{
	pattern_cont_view v = (ui->viewPercent->isChecked () ? pattern_cont_view::percent
			       : ui->viewLetters->isChecked () ? pattern_cont_view::letters
			       : pattern_cont_view::numbers);
	ui->boardView->set_cont_view (v);
	setting->writeIntEntry ("SEARCH_VIEW",
				v == pattern_cont_view::letters ? 0 : v == pattern_cont_view::numbers ? 1 : 2);
}

void PatternSearchWindow::slot_choose_color (bool)
{
	stone_color c = ui->editStone->isChecked () ? none : ui->editBlack->isChecked () ? black : white;
	ui->boardView->set_stone_color (c);
}

void PatternSearchWindow::editDelete ()
{
	game_state *st = ui->boardView->displayed ();
	if (st->root_node_p ())
		return;

	game_state *parent = st->prev_move ();
	ui->boardView->set_displayed (parent);
	st->disconnect ();
	update_actions ();
}

void PatternSearchWindow::nav_next_move ()
{
	game_state *st = ui->boardView->displayed ();
	game_state *next = st->next_move ();
	if (next == nullptr)
		return;
	ui->boardView->set_displayed (next);
	clear_preview_cursor ();
	update_actions ();
}

void PatternSearchWindow::nav_previous_move ()
{
	game_state *st = ui->boardView->displayed ();
	game_state *next = st->prev_move ();
	if (next == nullptr)
		return;
	ui->boardView->set_displayed (next);
	clear_preview_cursor ();
	update_actions ();
}

void PatternSearchWindow::nav_goto_first_move ()
{
	ui->boardView->set_displayed (m_game->get_root ());
	clear_preview_cursor ();
	update_actions ();
}

void PatternSearchWindow::nav_goto_last_move ()
{
	game_state *st_orig = ui->boardView->displayed ();
	game_state *st = st_orig;
	for (;;) {
		game_state *next = st->next_move ();
		if (next == nullptr)
			break;
		st = next;
	}
	ui->boardView->set_displayed (st);
	clear_preview_cursor ();
	update_actions ();
}

void PatternSearchWindow::nav_goto_cont ()
{
	game_state *st = ui->boardView->displayed ();
	board_rect sel = ui->boardView->get_selection ();
	for (;;) {
		game_state *next = st->next_move ();
		if (next == nullptr)
			break;
		st = next;
		if (!st->was_move_p ())
			continue;
		int x = st->get_move_x ();
		int y = st->get_move_y ();
		if (x >= sel.x1 && x <= sel.x2 && y >= sel.y1 && y <= sel.y2)
			break;
	}
	ui->boardView->set_displayed (st);
	update_actions ();
}

void PatternSearchWindow::update_selection ()
{
	QFontMetrics m (setting->fontStandard);
	int h = m.lineSpacing () * 2;
	ui->infoContents->setMaximumHeight (h + ui->infoContents->minimumHeight () + ui->infoFrame->frameWidth () * 2 + 2);

	QItemSelectionModel *sel = ui->dbListView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	ui->openButton->setEnabled (selection);

	m_info_scene->clear ();
	if (!selection)
		return;

	QModelIndex i = selected.first ();
	const gamedb_entry &e = m_model.find (i.row ());
	QString s1 = e.pw + " - " + e.pb;
	if (!e.result.isEmpty ())
		s1 += ", " + e.result;
	QString s2 = e.event;
	if (!s2.isEmpty ())
		s2 += ", ";
	s2 += e.date;
	/* Using labels seems like the natural way to do this, but they scale according to their
	   contents, changing the layout of the entire window, which is undesirable.  */
	m_info_scene->addSimpleText (s1, setting->fontStandard);
	auto item2 = m_info_scene->addSimpleText (s2, setting->fontStandard);
	item2->moveBy (0, m.lineSpacing ());
}

void PatternSearchWindow::redraw_stats ()
{
	m_stats_scene->clear ();
	if (m_search_cont == nullptr)
		return;

	int w1 = ui->statsView->width ();
	int h1 = ui->statsView->height ();
	QFontMetrics m (setting->fontStandard);

	if (w1 < 20 || h1 < 10 + m.lineSpacing ())
		return;
	int margin_x = std::max (4, w1 / 80);
	int margin_y = std::max (4, h1 / 80);
	int margin = std::min (margin_x, margin_y);
	int w = w1 - 2 * margin;
	int h = h1 - 2 * margin;

	size_t sz = m_search_cont->size ();
	std::vector<int> rank (sz);
	size_t count = 0;
	for (size_t i = 0; i < sz; i++) {
		auto &entry = (*m_search_cont)[i].get (none);
		if (entry.count > 0) {
			rank[entry.rank] = i;
			++count;
		}
	}
	if (count == 0)
		return;

	count = std::min (count, (size_t)52);
	int rk0_count = (*m_search_cont)[rank[0]].get (none).count;

	std::vector<QGraphicsSimpleTextItem *> stis;
	double max_w = 0;
	for (size_t i = 0; i < count; i++) {
		char c = (i < 26 ? 'A' : 'a' - 26) + i;
		int this_count = (*m_search_cont)[rank[i]].get (none).count;
#ifdef CHECKING
		assert (this_count > 0);
#endif
		QString s = QString ("%1: %2").arg (QChar (c)).arg (this_count);
		QGraphicsSimpleTextItem *item = m_stats_scene->addSimpleText (s, setting->fontStandard);
		double iw = item->boundingRect ().width ();
		double new_max_w = std::max (iw, max_w);
		if ((i + 1) * new_max_w + i * 10 > w && i > 0) {
			count = i;
			delete item;
			break;
		}
		max_w = new_max_w;
		stis.push_back (item);
	}
	double spacing = count > 1 ? (w - max_w * count) / (count - 1) : 0;
	double real_w = max_w < 10 ? max_w : max_w * 8. / 10;
	spacing = std::min (spacing, max_w * 0.75);

	for (size_t i = 0; i < stis.size (); i++) {
		auto it = stis[i];
		int x = margin + (max_w + spacing) * i;
		double iw = it->boundingRect ().width ();
		it->moveBy (x + (max_w - iw) / 2, margin + h - m.lineSpacing ());

		int cw = (*m_search_cont)[rank[i]].get (white).count;
		int cb = (*m_search_cont)[rank[i]].get (black).count;
		if (cw + cb == 0)
			/* This can't happen.  But let's be safe.  */
			cw = 1;
		int bar_height = h - m.lineSpacing () - 4;
		int thisbar_height = (double)bar_height * (cw + cb) / rk0_count;
		int thisbar_off = margin + bar_height - thisbar_height;
		int w_height = thisbar_height * cw / (cw + cb);
		int b_height = thisbar_height - w_height;
		if (cw > 0 && w_height == 0)
			w_height = 1;
		if (cb > 0 && b_height == 0)
			b_height = 1;
		if (b_height > 0)
			m_stats_scene->addRect (x + (max_w - real_w) / 2, thisbar_off, real_w, b_height, Qt::NoPen, Qt::black);
		if (w_height > 0)
			m_stats_scene->addRect (x + (max_w - real_w) / 2, thisbar_off + b_height, real_w, w_height, Qt::NoPen, Qt::white);
	}
}

void PatternSearchWindow::redraw_result ()
{
	QFontMetrics m (setting->fontStandard);
	int h = m.lineSpacing () * 3;
	ui->resultContents->setMaximumHeight (h + ui->resultContents->minimumHeight () + ui->resultFrame->frameWidth () * 2 + 2);

	m_result_scene->clear ();
	QString s1 = tr ("%1 matches in %2 games.").arg (m_result_same + m_result_inv).arg (m_result_n_games);
	QString s2 = tr ("%1 matches with same colors.").arg (m_result_same);
	QString s3 = tr ("%1 matches with inverted colors.").arg (m_result_inv);
	m_result_scene->addSimpleText (s1, setting->fontStandard);
	auto item2 = m_result_scene->addSimpleText (s2, setting->fontStandard);
	item2->moveBy (0, m.lineSpacing ());
	auto item3 = m_result_scene->addSimpleText (s3, setting->fontStandard);
	item3->moveBy (0, m.lineSpacing () * 2);
}

const gamedb_entry *last_loaded;

go_game_ptr PatternSearchWindow::load_selected ()
{
	QItemSelectionModel *sel = ui->dbListView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	if (!selection)
		return nullptr;
	QModelIndex i = selected.first ();
	const gamedb_entry &e = m_model.find (i.row ());
	last_loaded = &e;
	QDir d (e.dirname);
	QString filename = d.filePath (e.filename);
	qDebug () << filename;
	return record_from_file (filename, nullptr);
}

void PatternSearchWindow::handle_doubleclick ()
{
	go_game_ptr new_gr = load_selected ();
	if (new_gr == nullptr)
		return;
	clear_preview_cursor ();
	m_game = new_gr;
	ui->boardView->reset_game (new_gr);
	update_caption ();
	if (last_pattern != nullptr) {
		board_rect sel_rect;
		game_state *st = find_first_match (new_gr, *last_pattern, sel_rect);
		if (st) {
			const go_board &b = st->get_board ();
			ui->boardView->set_displayed (st);
			ui->boardView->set_selection (sel_rect);
			m_game_cont.clear ();
			m_game_cont.resize (b.bitsize ());
			int count = 1;
			for (;;) {
				st = st->next_move ();
				if (st == nullptr || !st->was_move_p ())
					break;
				int x = st->get_move_x ();
				int y = st->get_move_y ();
				stone_color col = st->get_move_color ();
				if (!sel_rect.contained (x, y))
					continue;
				unsigned bp = b.bitpos (x, y);
				if (m_game_cont[bp].get (none).count != 0)
					break;
				m_game_cont[bp].get (none).count = count;
				m_game_cont[bp].get (col).count = count;
				count++;
				if (count > 5)
					break;
			}
			ui->boardView->set_cont_data (&m_game_cont);
			ui->boardView->set_cont_view (pattern_cont_view::numbers);
		}
	}
	update_actions ();
}

void PatternSearchWindow::do_open (bool)
{
	go_game_ptr new_gr = load_selected ();
	if (new_gr == nullptr)
		return;
	MainWindow *win = new MainWindow (0, new_gr);
	win->show ();
}

search_runnable::search_runnable (PatternSearchWindow *parent, gamedb_model *m, gamedb_model::search_result **r,
				  go_pattern &&p, std::atomic<long> *cur, std::atomic<long> *max)
	: m_model (m), m_result (r), m_cur (cur), m_max (max), m_pattern (p)
{
	connect (this, &search_runnable::signal_completed, parent, &PatternSearchWindow::slot_completed);
}

void search_runnable::start_search ()
{
	*m_result = new gamedb_model::search_result (m_model->find_pattern (m_pattern, m_cur, m_max));
	std::atomic_thread_fence (std::memory_order_seq_cst);
	emit signal_completed ();
	deleteLater ();
}

void PatternSearchWindow::apply_game_result (const bit_array &games)
{
	std::vector<unsigned> new_games (games.popcnt ());
	int idx = 0;
	for (unsigned i = 0; i < games.bitsize (); i++) {
		i = games.ffs (i);
		if (i == games.bitsize ())
			break;
#ifdef CHECKING
		if (idx == new_games.size ())
			abort ();
#endif
		new_games[idx++] = i;
	}
#ifdef CHECKING
		if (idx != new_games.size ())
			abort ();
#endif
	m_model.apply_game_list (std::move (new_games));
}

void PatternSearchWindow::slot_completed ()
{
	killTimer (m_progress_timer);
	ui->progressBar->setValue (m_progress_max);

	std::atomic_thread_fence (std::memory_order_seq_cst);
	db_data->db_mutex.unlock ();

	std::vector<std::array<int, 2>> &hits = m_result->first;
	std::vector<gamedb_model::cont_bw> &conts = m_result->second;

	bit_array games (hits.size ());
	int m1 = 0, m2 = 0;
	for (size_t i = 0; i < hits.size (); i++) {
		m1 += hits[i][0];
		m2 += hits[i][1];
		if (hits[i][0] + hits[i][1] > 0)
			games.set_bit (i);
	}
	apply_game_result (games);

	game_state *st = ui->boardView->displayed ();
	board_rect sel = ui->boardView->get_selection ();
	clear_preview_cursor ();
	m_previews.emplace_back (*last_pattern, m_game, st, sel, games, m1, m2);
	m_result_n_games = games.popcnt ();
	m_result_same = m1;
	m_result_inv = m2;
	choices_resized ();

	preview &p = m_previews.back ();
	set_preview_cursor (p);

	const go_board &b = st->get_board ();

	unsigned sz_x = last_pattern->sz_x ();
	unsigned sz_y = last_pattern->sz_y ();
	for (unsigned x = 0; x < sz_x; x++) {
		for (unsigned y = 0; y < sz_y; y++) {
			int pos = x + y * sz_x;
			int board_pos = b.bitpos (x + p.selection.x1, y + p.selection.y1);
			p.cont[board_pos].get (white).count += conts[pos].first;
			p.cont[board_pos].get (none).count += conts[pos].first;
			p.cont[board_pos].get (black).count += conts[pos].second;
			p.cont[board_pos].get (none).count += conts[pos].second;
		}
	}
	std::vector<int> rank (b.bitsize ());
	for (size_t i = 0; i < b.bitsize (); i++)
		rank[i] = i;
	std::sort (rank.begin (), rank.end (),
		   [&p] (int a, int b)
		   {
			   return p.cont[a].get (none).count > p.cont[b].get (none).count;
		   });
	for (size_t i = 0; i < b.bitsize (); i++)
		p.cont[rank[i]].get (none).rank = i;

	ui->boardView->set_displayed (st);
	ui->boardView->set_cont_data (&p.cont);
	slot_choose_view ();
	redraw_result ();
	m_search_cont = &p.cont;
	redraw_stats ();
	delete m_result;

	setEnabled (true);
}

void PatternSearchWindow::pattern_search (bool all_games)
{
	setEnabled (false);
	if (all_games)
		m_model.reset_filters ();
	go_pattern p = ui->boardView->selected_pattern ();
	if (!ui->anchorAction->isChecked ())
		p.clear_alignment ();
	search_runnable *r = new search_runnable (this, &m_model, &m_result, std::move (p),
						  &m_progress, &m_progress_max);
	r->moveToThread (&search_thread);
	connect (this, &PatternSearchWindow::signal_start_search, r, &search_runnable::start_search);
	connect (&search_thread, &QThread::finished, r, &QObject::deleteLater);
	m_progress.store (0);
	m_progress_max.store (0);
	emit signal_start_search ();

	delete last_pattern;
	last_pattern = new go_pattern (p);
	db_data->db_mutex.lock ();
	ui->progressBar->reset ();
	m_progress_timer = startTimer (200);
}

void PatternSearchWindow::timerEvent (QTimerEvent *)
{
	if (m_progress_max.load () == 0)
		return;
	ui->progressBar->setValue (m_progress);
	ui->progressBar->setMaximum (m_progress_max);
}

void PatternSearchWindow::do_search (go_game_ptr gr, game_state *st, const board_rect &r)
{
	m_game = gr;
	ui->boardView->reset_game (gr);
	ui->boardView->set_displayed (st);
	ui->boardView->set_selection (r);
	pattern_search (true);
}

void PatternSearchWindow::slot_using ()
{
	QString txt;
	txt = tr ("<p>Place stones normally, or use shift/control to place black and white stones specifically.</p>");
	txt += tr ("<p>Select a rectangle using the right mouse button. Search in the current game list by pressing S, or in all the games by pressing A (or use the menus and icons).</p>");
	txt += tr ("<p>Once a search is complete, click on one of the games to bring up the position where the pattern occurs. ");
	txt += tr ("The pattern will be highlighted.  You can press N to go to the next move within that region.</p>");
	QMessageBox mb;
	mb.setWindowTitle (PACKAGE " " VERSION);
	mb.setTextFormat (Qt::RichText);
	mb.setText (txt);
	mb.exec ();
}
