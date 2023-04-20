#include <fstream>
#include <QFileDialog>

#include "analyzedlg.h"
#include "ui_analyze_gui.h"

#include "clientwin.h"
#include "ui_helpers.h"
#include "gotools.h"

AnalyzeDialog *analyze_dialog;

AnalyzeDialog::AnalyzeDialog (QWidget *parent, const QString &filename)
	: QMainWindow (parent), GTP_Eval_Controller (parent), ui (std::make_unique<Ui::AnalyzeDialog> ())
{
	ui->setupUi (this);

	ui->filenameEdit->setText (filename);
	if (!filename.isEmpty ()) {
		QFileInfo fi (filename);
		m_last_dir = fi.dir ().absolutePath ();
	} else
		m_last_dir = setting->readEntry ("LAST_DIR");
	ui->secondsEdit->setValidator (&m_seconds_vald);
	ui->maxlinesEdit->setValidator (&m_lines_vald);

	const QStyle *style = qgo_app->style ();
	int iconsz = style->pixelMetric (QStyle::PixelMetric::PM_ToolBarIconSize);

	QSize sz (iconsz, iconsz);
	ui->openButton->setIconSize (sz);
	ui->trashButton->setIconSize (sz);
	ui->openDoneButton->setIconSize (sz);
	ui->trashDoneButton->setIconSize (sz);

	ui->openButton->setEnabled (false);

	/* "Swap komi if better" seems like a reasonable default.  It only affects games with negative
	   komi.  */
	ui->komiComboBox->setCurrentIndex (1);

	connect (ui->enqueueButton, &QPushButton::clicked, [=] (bool) { start_job (); });
	connect (ui->fileselButton, &QPushButton::clicked, [=] (bool) { select_file (); });
	connect (ui->enqueueDirButton, &QPushButton::clicked, [=] (bool) { enqueue_dir (); });
	connect (ui->dbselButton, &QPushButton::clicked, [=] (bool) { select_file_db (); });
	connect (ui->openButton, &QPushButton::clicked, [=] (bool) { open_in_progress_window (false); });
	connect (ui->openDoneButton, &QPushButton::clicked, [=] (bool) { open_in_progress_window (true); });
	connect (ui->trashButton, &QPushButton::clicked, [=] (bool) { discard_job (false); });
	connect (ui->trashDoneButton, &QPushButton::clicked, [=] (bool) { discard_job (true); });
	ui->jobView->setModel (&m_jobs.model);
	ui->doneView->setModel (&m_done.model);
	connect (ui->jobView, &QListView::clicked, [=] (QModelIndex) { update_progress (); });
	connect (ui->jobView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [=] (const QItemSelection &, const QItemSelection &) { update_progress (); });

	void (QSpinBox::*changed) (int) = &QSpinBox::valueChanged;
	connect (ui->boardsizeSpinBox, changed, [this] (int) { update_engines (); });
	void (QComboBox::*cic) (int) = &QComboBox::currentIndexChanged;
	connect (ui->engineComboBox, cic, [this] (int v) { ui->engineStartButton->setEnabled (v != -1 && analyzer_state () == analyzer::disconnected ); });
	connect (ui->configureButton, &QPushButton::clicked, [=] (bool) { client_window->dlgSetPreferences (3); });
	connect (ui->engineStartButton, &QPushButton::clicked, [=] (bool) { start_engine (); });
	connect (ui->engineStopButton, &QPushButton::clicked, [=] (bool) { stop_engine (); });
	connect (ui->enginePauseButton, &QPushButton::toggled, [=] (bool on)
		 {
			 if (on)
				 pause_analyzer ();
			 else
				 queue_next ();
		 });
	connect (ui->engineLogButton, &QPushButton::clicked, [=] (bool) { m_analyzer->dialog ()->show (); });

	connect (ui->closeButton, &QPushButton::clicked, [=] (bool) { close (); });

	update_engines ();

	analyzer_state_changed ();
}

AnalyzeDialog::~AnalyzeDialog ()
{
	stop_analyzer ();
}

void AnalyzeDialog::closeEvent (QCloseEvent *e)
{
	/* We could allow the user to hide this dialog and let things run in the background.
	   The downside of that is that this may be the only window and we'd have to check
	   for that case.  Also, I think it's good to have the window open as a reminder
	   that an engine is running.  */
	if (m_jobs.jobs.size () != 0) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning (this, PACKAGE,
					       tr ("Jobs are still running.  Do you wish to terminate the engine and discard the jobs?"),
					       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

		if (choice == QMessageBox::Cancel) {
			e->ignore ();
			return;
		}
	} else if (m_done.jobs.size () != 0) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning (this, PACKAGE,
					       tr ("Completed jobs contain unsaved data.  Do you wish to discard the jobs?"),
					       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

		if (choice == QMessageBox::Cancel) {
			e->ignore ();
			return;
		}
	}
	stop_analyzer ();
	m_jobs.model.clear ();
	m_jobs.jobs.clear ();
	m_jobs.map.clear ();
	update_progress ();
	e->accept ();
}

void AnalyzeDialog::analyzer_state_changed ()
{
	analyzer s = analyzer_state ();
	switch (s) {
	case analyzer::disconnected:
		ui->engineStatusLabel->setText (tr ("not running"));
		break;
	case analyzer::starting:
		ui->engineStatusLabel->setText (tr ("starting up"));
		break;
	case analyzer::paused:
		ui->engineStatusLabel->setText (tr ("idle"));
		break;
	case analyzer::running:
		ui->engineStatusLabel->setText (tr ("working"));
		break;
	}
	ui->engineStopButton->setEnabled (s != analyzer::disconnected);
	ui->enginePauseButton->setEnabled (s == analyzer::running || s == analyzer::paused);
	if (s == analyzer::disconnected)
		ui->enginePauseButton->setChecked (false);
	ui->engineStartButton->setEnabled (ui->engineComboBox->currentIndex () != -1 && s == analyzer::disconnected);
	ui->engineComboBox->setEnabled (s == analyzer::disconnected);
	ui->engineLogButton->setEnabled (m_analyzer != nullptr && !m_analyzer->dialog ()->isVisible ());

	bool any_jobs = m_jobs.model.rowCount () != 0 || m_done.model.rowCount () != 0;
	ui->boardsizeSpinBox->setEnabled (!any_jobs && s == analyzer::disconnected);
}

AnalyzeDialog::job::job (AnalyzeDialog *dlg, const QString &title, go_game_ptr gr, int n_seconds, int n_lines,
			 engine_komi k, bool comments, go_rules rules)
	: m_dlg (dlg), m_title (title), m_game (gr), m_n_seconds (n_seconds), m_n_lines (n_lines), m_komi_type (k),
	  m_comments (comments), m_rules (rules)
{
	std::vector<game_state *> *q = &m_queue;
	std::function<bool (game_state *)> f = [&q] (game_state *st) -> bool
		{
			eval ev = st->best_eval ();
			if (st->has_figure () && ev.visits > 0)
				return false;
			/* Ignore score and pass nodes on the grounds that they should be identical to the
			   preceding one.  */
			if (st->was_score_p () || st->was_pass_p ())
				return true;
			q->push_back (st);
			return true;
		};
	/* This produces the nodes in the reverse order of the game.  We rely on this
	   when calculating win rate changes.  */
	m_game->get_root ()->walk_tree (f);
	if (k == engine_komi::both) {
		q = &m_queue_flipped;
		m_game->get_root ()->walk_tree (f);
	}
	m_initial_size = m_queue.size () + m_queue_flipped.size ();
}

AnalyzeDialog::job::~job ()
{
	if (m_win)
		disconnect (m_connection);
}

game_state *AnalyzeDialog::job::select_request (bool pop)
{
	if (m_queue_flipped.size () > 0 && m_dlg->m_current_komi.isEmpty ()) {
		m_initial_size -= m_queue_flipped.size ();
		m_queue_flipped.clear ();
	}
	if (m_queue.size () == 0) {
		m_komi_type = engine_komi::do_swap;
		if (m_queue_flipped.size () == 0)
			return nullptr;
		game_state *st = m_queue_flipped.back ();
		if (pop)
			m_queue_flipped.pop_back ();
		return st;
	}

	game_state *st = m_queue.back ();
	if (pop)
		m_queue.pop_back ();
	return st;
}

void AnalyzeDialog::job::show_window (bool done)
{
	if (m_win == nullptr) {
		m_win = new MainWindow (nullptr, m_game, screen_key (m_dlg), done ? modeNormal : modeBatch);
		m_connection = connect (m_win, &MainWindow::signal_closeevent,
					[this] ()
					{
						m_win = nullptr;
						m_dlg->update_progress ();
						if (m_display == &m_dlg->m_done && !m_game->modified ()) {
							m_dlg->remove_job (*m_display, this);
							m_dlg->update_progress ();
						}
					});
	}
	m_win->show ();
	m_win->activateWindow ();
}

void AnalyzeDialog::insert_job (display &q, QListView *view, job *j)
{
	int idx = m_job_count++;

	QStandardItem *item = new QStandardItem (j->m_title);
	item->setDropEnabled (false);
	item->setEditable (false);
	item->setData (idx, Qt::UserRole + 1);

	j->m_display = &q;
	q.jobs.push_back (j);
	q.map.insert (idx, j);
	q.model.appendRow (item);
	view->setCurrentIndex (item->index ());
}

void AnalyzeDialog::remove_job (display &q, job *j)
{
	q.jobs.erase (std::remove (std::begin (q.jobs), std::end (q.jobs), j), std::end (q.jobs));
	j->m_display = nullptr;
	for (int i = 0; i < q.model.rowCount (); i++) {
		QStandardItem *item = q.model.item (i);
		int jidx = item->data (Qt::UserRole + 1).toInt ();
		if (q.map[jidx] == j) {
			q.map.remove (jidx);
			q.model.removeRow (i);
			return;
		}
	}
	throw std::logic_error ("did not find job");
}

AnalyzeDialog::job *AnalyzeDialog::selected_job (bool done)
{
	QListView *view = done ? ui->doneView : ui->jobView;
	display &q = done ? m_done : m_jobs;
	const QModelIndexList &selected = view->selectionModel ()->selectedIndexes ();
	if (selected.length () == 0)
		return nullptr;

	QModelIndex i = selected.first ();
	QStandardItem *item = q.model.itemFromIndex (i);
	int jidx = item->data (Qt::UserRole + 1).toInt ();
	return q.map[jidx];
}

void AnalyzeDialog::discard_job (bool done)
{
	job *j = selected_job (done);
	if (j == nullptr)
		return;
	QMessageBox::StandardButton choice;
	choice = QMessageBox::warning (this, PACKAGE, tr ("Really discard selected job?"),
				       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
	if (choice != QMessageBox::Yes)
		return;
	/* Not sure if the job could have moved while the message box was open.
	   In any case, using m_display should always give the correct answer.
	   Checking for nullptr is ultra-paranoid.  */
	if (j->m_display == nullptr)
		return;
	if (j == m_requester)
		m_requester = nullptr;
	remove_job (*j->m_display, j);
	update_progress ();
}

void AnalyzeDialog::open_in_progress_window (bool done)
{
	job *j = selected_job (done);
	if (j == nullptr)
		return;
	j->show_window (done);
	update_progress ();
}

void AnalyzeDialog::select_file ()
{
	QString filename = open_filename_dialog (this);
	if (filename.isEmpty())
		return;
	QFileInfo fi (filename);
	m_last_dir = fi.dir ().absolutePath ();
	ui->filenameEdit->setText (filename);
}

void AnalyzeDialog::enqueue_dir ()
{
	QString dirname = QFileDialog::getExistingDirectory (this, tr ("Choose directory to add to the queue"), m_last_dir);
	if (dirname.isEmpty())
		return;
	QDir dir (dirname);
	QStringList filters;
	filters << "*.sgf" << "*.SGF";
	dir.setNameFilters (filters);
	int skipped = 0;
	int failed = 0;
	for (auto f: dir.entryList (QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name)) {
		start_job (dir.absoluteFilePath (f));
	}
}

void AnalyzeDialog::select_file_db ()
{
	QString filename = open_db_filename_dialog (this);
	if (filename.isEmpty())
		return;
	QFileInfo fi (filename);
	m_last_dir = fi.dir ().absolutePath ();
	ui->filenameEdit->setText (filename);
}

/* Called only in situations when we know the engine is running.  */
void AnalyzeDialog::queue_next ()
{
	if (m_jobs.jobs.size () != 0 && !ui->enginePauseButton->isChecked ()) {
		QStandardItem *item = m_jobs.model.item (0);
		int jidx = item->data (Qt::UserRole + 1).toInt ();
		job *j = m_jobs.map[jidx];
		game_state *st = j->select_request (false);
		if (st != nullptr && st->get_board ().size_x () == ui->boardsizeSpinBox->value ()) {
			m_seconds_count = 0;
			m_requester = j;
			bool flip = j->m_komi_type == engine_komi::do_swap;
			if (j->m_komi_type == engine_komi::maybe_swap && !m_current_komi.isEmpty ()) {
				bool ok;
				double k = m_current_komi.toFloat (&ok);
				double gm_k = j->m_game->info ().komi;
				if (ok && std::abs (k - gm_k) > std::abs (k + gm_k))
					flip = true;
			}
			set_rules (j->m_rules);
			request_analysis (j->m_game, st, flip);
			return;
		}
	}
	pause_analyzer ();
}

void AnalyzeDialog::notice_analyzer_id (const analyzer_id &id, bool have_score)
{
	job *j = m_requester;
	if (j == nullptr)
		return;
	if (j->m_win != nullptr)
		j->m_win->update_analyzer_ids (id, have_score);
}

inline std::string s_tr (const char *s)
{
	return QObject::tr (s).toStdString ();
}

void AnalyzeDialog::eval_received (const analyzer_id &id, const QString &, int, bool have_score)
{
	job *j = m_requester;
	if (j == nullptr) {
		/* This occurs when the currently processed job is manually deleted by the user.  */
		queue_next ();
		return;
	}
	if (++m_seconds_count < j->m_n_seconds)
		return;
	if (j->m_done == 0) {
		game_state *r = j->m_game->get_root ();
		std::string comm = r->comment ();
		QString report = tr ("Analysis by %1, ruleset %2\n").arg (QString::fromStdString (id.engine)).arg (rules_name (j->m_rules));
		comm += report.toStdString ();
		r->set_comment (comm);
	}
	j->m_done++;
	update_progress ();

	game_state *st = j->select_request (true);
	st->update_eval (*m_eval_state);
	auto variations = m_eval_state->take_children ();
	if (j->m_comments && variations.size () > 0) {
		game_state *best = variations[0];
		eval e = best->best_eval ();
		if (e.visits > 0) {
			std::string comm = st->comment ();
			if (comm.length () > 0) {
				if (comm.back () != '\n')
					comm.push_back ('\n');
				comm += "----------------\n";
			}
			auto cname = st->get_board ().coords_name (best->get_move_x (), best->get_move_y (), false);
			comm += s_tr ("Engine top choice: ") + cname.first + cname.second;
			comm += s_tr (", ") + std::to_string (e.visits) + s_tr (" visits") + s_tr (", winrate B: ") + komi_str (e.wr_black * 100) + "%";
			if (have_score) {
				double sval = e.score_mean;
				if (sval < 0)
					sval = -sval, comm += s_tr ("\nScore: W+");
				else
					comm += s_tr ("\nScore: B+");
				comm += komi_str ((long)(sval * 100) / 100.);
				comm += s_tr (" (stddev ") + komi_str ((long)(e.score_stddev * 100) / 100.) + s_tr (")");
			}
			comm += "\n";
			game_state *next = st->next_primary_move ();
			if (next && next->was_move_p ()) {
				auto nextcname = st->get_board ().coords_name (next->get_move_x (), next->get_move_y (), false);
				comm += s_tr ("Game move: ") + nextcname.first + nextcname.second;
				if (cname != nextcname) {
					comm += s_tr (", ");
					for (size_t cnt = 1; cnt < variations.size (); cnt++) {
						if (variations[cnt]->get_move_x () == next->get_move_x ()
						    && variations[cnt]->get_move_y () == next->get_move_y ()) {
							comm += s_tr ("choice #") + std::to_string (cnt + 1);
							goto found;
						}
					}
					comm += "not considered";
					found:

					eval e2 = next->eval_from (e.id, true);
					if (e2.visits > 0) {
						double diff = e2.wr_black - e.wr_black;
						std::string diffstr = komi_str (diff * 100);
						if (diff > 0)
							diffstr = "+" + diffstr;
						comm += s_tr (", winrate B: ") + komi_str (e2.wr_black * 100) + "% (" + diffstr + ")";
					}
				}
				comm += "\n";
			}
			comm += s_tr ("Analysis: ") + e.id.engine;
			if (e.id.komi_set)
				comm += s_tr (" @") + komi_str (e.id.komi) + s_tr (" komi");
			comm += "\n";
			st->set_comment (comm);
		}
	}
	int count = 0;
	for (auto it: variations) {
		eval ev = it->best_eval ();
		double wr = ev.wr_black;
		QString cnt = QString::number (count + 1);
		QString wrb = QString::number (wr * 100);
		QString wrw = QString::number ((1 - wr) * 100);
		QString vis = QString::number (ev.visits);
		QString title = tr ("PV ") + cnt + ": " + tr ("W Win ") + wrw + "%, " + tr ("B Win ") + wrb + "% " + tr ("at ") + vis + tr (" visits");
		title += QString (" (%1)").arg (QString::fromStdString (ev.id.engine));
		it->set_figure (257, title.toStdString ());
		st->add_child_tree (it);
		j->m_game->set_modified ();
		if (j->m_win) {
			j->m_win->update_game_tree ();
			j->m_win->update_figures ();
			j->m_win->update_game_record ();
		}
		count++;
		if (count >= j->m_n_lines)
			break;
	}

	if (j->select_request (false) == nullptr) {
		if (j->m_win != nullptr)
			j->m_win->setGameMode (modeNormal);

		remove_job (m_jobs, j);
		insert_job (m_done, ui->doneView, j);
		update_progress ();
	}

	queue_next ();
}

void AnalyzeDialog::update_buttons (display &d, QListView *view, QProgressBar *bar, QToolButton *trash, QToolButton *open)
{
	QItemSelectionModel *sel = view->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool entries = d.model.rowCount () != 0;
	bool selection = selected.length () != 0;

	view->setEnabled (entries);
	if (bar != nullptr)
		bar->setEnabled (selection);
	if (!selection) {
		if (bar != nullptr)
			bar->setValue (0);
		open->setEnabled (false);
		open->setChecked (false);
		trash->setEnabled (false);
		return;
	}
	QModelIndex i = selected.first ();
	QStandardItem *item = d.model.itemFromIndex (i);
	int jidx = item->data (Qt::UserRole + 1).toInt ();
	job *j = d.map[jidx];
	if (j == nullptr) {
		if (bar != nullptr)
			bar->setValue (0);
		open->setEnabled (false);
		open->setChecked (false);
		trash->setEnabled (false);
		return;
	}
	trash->setEnabled (true);
	open->setChecked (j->m_win != nullptr);
	open->setEnabled (j->m_win == nullptr);
	if (bar != nullptr) {
		bar->setRange (0, j->m_initial_size);
		bar->setValue (j->m_done);
	}
}

void AnalyzeDialog::update_engines ()
{
	/* Keep old entry showing if the engine is running.  */
	if (!ui->engineComboBox->isEnabled ())
		return;

	auto new_list = client_window->analysis_engines (ui->boardsizeSpinBox->value ());
	ui->engineComboBox->clear ();
	for (auto &it: new_list) {
		ui->engineComboBox->addItem (it.title);
	}
	m_engines = new_list;
}

void AnalyzeDialog::update_progress ()
{
	update_buttons (m_jobs, ui->jobView, ui->progressBar, ui->openButton, ui->trashButton);
	update_buttons (m_done, ui->doneView, nullptr, ui->openDoneButton, ui->trashDoneButton);

	bool any_jobs = m_jobs.model.rowCount () != 0 || m_done.model.rowCount () != 0;
	ui->boardsizeSpinBox->setEnabled (!any_jobs && analyzer_state () == analyzer::disconnected);

	/* Garbage collect.  */
	if (!any_jobs)
		m_all_jobs.clear ();
}

void AnalyzeDialog::gtp_startup_success (GTP_Process *)
{
	analyzer_state_changed ();
	queue_next ();
}

void AnalyzeDialog::gtp_failure (GTP_Process *, const QString &err)
{
	clear_eval_data ();
	QMessageBox msg(QString (QObject::tr("Error")), err,
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default,
			Qt::NoButton, Qt::NoButton);
	msg.exec();
}

void AnalyzeDialog::gtp_exited (GTP_Process *)
{
	clear_eval_data ();
	QMessageBox::warning (this, PACKAGE, QObject::tr ("GTP process exited unexpectedly."));
	analyzer_state_changed ();
}

void AnalyzeDialog::stop_engine ()
{
	if (analyzer_state () == analyzer::disconnected)
		return;
	m_seconds_count = 0;
	stop_analyzer ();
}

void AnalyzeDialog::start_engine ()
{
	if (analyzer_state () != analyzer::disconnected)
		return;
	int idx = ui->engineComboBox->currentIndex ();
	if (idx < 0 || idx >= m_engines.count ())
		return;

	ui->boardsizeSpinBox->setEnabled (false);

	const Engine &e = m_engines.at (idx);
	m_current_komi = e.komi;
	start_analyzer (e, ui->boardsizeSpinBox->text ().toInt (), 7.5, false);
}

void AnalyzeDialog::start_job (const QString &f)
{
	go_game_ptr gr = record_from_file (f, nullptr);
	if (gr == nullptr)
		return;

	QFileInfo fi (f);
	m_last_dir = fi.dir ().absolutePath ();

	game_state *root = gr->get_root ();
	const go_board &b = root->get_board ();
	if (b.size_x () != b.size_y ()) {
		QMessageBox::warning (this, PACKAGE, tr ("Analysis is supported only for square boards!"));
		return;
	}
	if (b.size_x () != ui->boardsizeSpinBox->value ()) {
		QMessageBox::warning (this, PACKAGE,
				      tr ("File has a different boardsize than selected!"));
		return;
	}
	ui->filenameEdit->setText ("");
	int komi_val = ui->komiComboBox->currentIndex ();
	int rules_val = ui->rulesComboBox->currentIndex ();
	go_rules r = guess_rules (gr->info ());
	if (rules_val == 1)
		r = go_rules::japanese;
	else if (rules_val == 2)
		r = go_rules::chinese;
	engine_komi k = komi_val == 2 ? engine_komi::both : komi_val == 1 ? engine_komi::maybe_swap : engine_komi::dflt;
	m_all_jobs.emplace_front (this, f, gr, ui->secondsEdit->text ().toInt (), ui->maxlinesEdit->text ().toInt (), k,
				  ui->commentsCheckBox->isChecked (), r);
	job *j = &m_all_jobs.front ();
	insert_job (m_jobs, ui->jobView, j);

	update_progress ();
	if (analyzer_state () == analyzer::paused) {
		queue_next ();
	}
}

void AnalyzeDialog::start_job ()
{
	start_job (ui->filenameEdit->text ());
}
