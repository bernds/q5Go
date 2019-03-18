#include <fstream>

#include "analyzedlg.h"

#include "clientwin.h"
#include "ui_helpers.h"

AnalyzeDialog *analyze_dialog;

AnalyzeDialog::AnalyzeDialog (QWidget *parent, const QString &filename)
	: QMainWindow (parent), GTP_Eval_Controller (parent)
{
	setupUi (this);
	filenameEdit->setText (filename);
	if (!filename.isEmpty ()) {
		QFileInfo fi (filename);
		m_last_dir = fi.dir ().absolutePath ();
	} else
		m_last_dir = setting->readEntry ("LAST_DIR");
	secondsEdit->setValidator (&m_seconds_vald);
	maxlinesEdit->setValidator (&m_lines_vald);

	const QStyle *style = qgo_app->style ();
	int iconsz = style->pixelMetric (QStyle::PixelMetric::PM_ToolBarIconSize);

	QSize sz (iconsz, iconsz);
	openButton->setIconSize (sz);
	trashButton->setIconSize (sz);
	openDoneButton->setIconSize (sz);
	trashDoneButton->setIconSize (sz);

	openButton->setEnabled (false);

	/* "Swap komi if better" seems like a reasonable default.  It only affects games with negative
	   komi.  */
	komiComboBox->setCurrentIndex (1);

	connect (enqueueButton, &QPushButton::clicked, [=] (bool) { start_job (); });
	connect (fileselButton, &QPushButton::clicked, [=] (bool) { select_file (); });
	connect (openButton, &QPushButton::clicked, [=] (bool) { open_in_progress_window (false); });
	connect (openDoneButton, &QPushButton::clicked, [=] (bool) { open_in_progress_window (true); });
	connect (trashButton, &QPushButton::clicked, [=] (bool) { discard_job (false); });
	connect (trashDoneButton, &QPushButton::clicked, [=] (bool) { discard_job (true); });
	jobView->setModel (&m_jobs.model);
	doneView->setModel (&m_done.model);
	connect (jobView, &QListView::clicked, [=] (QModelIndex) { update_progress (); });
	connect (jobView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [=] (const QItemSelection &, const QItemSelection &) { update_progress (); });

	void (QSpinBox::*changed) (int) = &QSpinBox::valueChanged;
	connect (boardsizeSpinBox, changed, [this] (int) { update_engines (); });
	void (QComboBox::*cic) (int) = &QComboBox::currentIndexChanged;
	connect (engineComboBox, cic, [this] (int v) { engineStartButton->setEnabled (v != -1 && analyzer_state () == analyzer::disconnected ); });
	connect (configureButton, &QPushButton::clicked, [=] (bool) { client_window->dlgSetPreferences (3); });
	connect (engineStartButton, &QPushButton::clicked, [=] (bool) { start_engine (); });
	connect (engineLogButton, &QPushButton::clicked, [=] (bool) { m_analyzer->dialog ()->show (); });

	connect (closeButton, &QPushButton::clicked, [=] (bool) { close (); });

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
		engineStatusLabel->setText (tr ("not running"));
		break;
	case analyzer::starting:
		engineStatusLabel->setText (tr ("starting up"));
		break;
	case analyzer::paused:
		engineStatusLabel->setText (tr ("idle"));
		break;
	case analyzer::running:
		engineStatusLabel->setText (tr ("working"));
		break;
	}
	engineStartButton->setEnabled (engineComboBox->currentIndex () != -1 && s == analyzer::disconnected);
	engineComboBox->setEnabled (s == analyzer::disconnected);
	engineLogButton->setEnabled (m_analyzer != nullptr && !m_analyzer->dialog ()->isVisible ());

	bool any_jobs = m_jobs.model.rowCount () != 0 || m_done.model.rowCount () != 0;
	boardsizeSpinBox->setEnabled (!any_jobs && s == analyzer::disconnected);
}

AnalyzeDialog::job::job (AnalyzeDialog *dlg, QString &title, std::shared_ptr<game_record> gr, int n_seconds, int n_lines,
			 engine_komi k)
	: m_dlg (dlg), m_title (title), m_game (gr), m_n_seconds (n_seconds), m_n_lines (n_lines), m_komi_type (k)
{
	std::vector<game_state *> *q = &m_queue;
	std::function<bool (game_state *)> f = [this, &q] (game_state *st) -> bool
		{
			eval ev = st->best_eval ();
			if (st->has_figure () && ev.visits > 0)
				return false;
			q->push_back (st);
			return true;
		};
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
	QListView *view = done ? doneView : jobView;
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
	filenameEdit->setText (filename);
}

/* Called only in situations when we know the engine is running.  */
void AnalyzeDialog::queue_next ()
{
	if (m_jobs.jobs.size () != 0) {
		QStandardItem *item = m_jobs.model.item (0);
		int jidx = item->data (Qt::UserRole + 1).toInt ();
		job *j = m_jobs.map[jidx];
		game_state *st = j->select_request (false);
		if (st != nullptr && st->get_board ().size_x () == boardsizeSpinBox->value ()) {
			m_seconds_count = 0;
			m_requester = j;
			if (analyzer_state () == analyzer::paused)
				pause_analyzer (false, st);
			else {
				bool flip = j->m_komi_type == engine_komi::do_swap;
				if (j->m_komi_type == engine_komi::maybe_swap && !m_current_komi.isEmpty ()) {
					bool ok;
					double k = m_current_komi.toFloat (&ok);
					double gm_k = j->m_game->komi ();
					if (ok && std::abs (k - gm_k) > std::abs (k + gm_k))
						flip = true;
				}
				request_analysis (st, flip);
			}
			return;
		}
	}
	pause_analyzer (true, nullptr);
}

void AnalyzeDialog::notice_analyzer_id (const analyzer_id &id)
{
	job *j = m_requester;
	if (j == nullptr)
		return;
	if (j->m_win != nullptr)
		j->m_win->update_analyzer_ids (id);
}

void AnalyzeDialog::eval_received (const QString &, int)
{
	job *j = m_requester;
	if (j == nullptr) {
		/* This occurs when the currently processed job is manually deleted by the user.  */
		queue_next ();
		return;
	}
	if (++m_seconds_count < j->m_n_seconds)
		return;
	j->m_done++;
	update_progress ();

	game_state *st = j->select_request (true);
	st->update_eval (*m_eval_state);
	auto variations = m_eval_state->take_children ();
	int count = 0;
	for (auto it: variations) {
		eval ev = it->best_eval ();
		double wr = ev.wr_black;
		QString cnt = QString::number (count + 1);
		QString wrb = QString::number (wr * 100);
		QString wrw = QString::number ((1 - wr) * 100);
		QString vis = QString::number (ev.visits);
		QString title = tr ("PV ") + cnt + ": " + tr ("W Win ") + wrw + "%, " + tr ("B Win ") + wrb + "% " + tr ("at ") + vis + tr (" visits.");
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
		insert_job (m_done, doneView, j);
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
	if (!engineComboBox->isEnabled ())
		return;

	auto new_list = client_window->analysis_engines (boardsizeSpinBox->value ());
	engineComboBox->clear ();
	for (auto &it: new_list) {
		engineComboBox->addItem (it.title);
	}
	m_engines = new_list;
}

void AnalyzeDialog::update_progress ()
{
	update_buttons (m_jobs, jobView, progressBar, openButton, trashButton);
	update_buttons (m_done, doneView, nullptr, openDoneButton, trashDoneButton);

	bool any_jobs = m_jobs.model.rowCount () != 0 || m_done.model.rowCount () != 0;
	boardsizeSpinBox->setEnabled (!any_jobs && analyzer_state () == analyzer::disconnected);

	/* Garbage collect.  */
	if (!any_jobs)
		m_all_jobs.clear ();
}

void AnalyzeDialog::gtp_startup_success ()
{
	analyzer_state_changed ();
	queue_next ();
}

void AnalyzeDialog::gtp_failure (const QString &err)
{
	clear_eval_data ();
	QMessageBox msg(QString (QObject::tr("Error")), err,
			QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default,
			Qt::NoButton, Qt::NoButton);
	msg.exec();
}

void AnalyzeDialog::gtp_exited ()
{
	clear_eval_data ();
	QMessageBox::warning (this, PACKAGE, QObject::tr ("GTP process exited unexpectedly."));
	analyzer_state_changed ();
}

void AnalyzeDialog::start_engine ()
{
	if (analyzer_state () != analyzer::disconnected)
		return;
	int idx = engineComboBox->currentIndex ();
	if (idx < 0 || idx >= m_engines.count ())
		return;

	boardsizeSpinBox->setEnabled (false);

	const Engine &e = m_engines.at (idx);
	m_current_komi = e.komi;
	start_analyzer (e, e.boardsize.toInt (), 7.5, 0, false);
}

void AnalyzeDialog::start_job ()
{
	QString f = filenameEdit->text ();
	std::shared_ptr<game_record> gr = record_from_file (f, nullptr);
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
	if (b.size_x () != boardsizeSpinBox->value ()) {
		QMessageBox::warning (this, PACKAGE,
				      tr ("File has a different boardsize than selected!"));
		return;
	}
	filenameEdit->setText ("");
	int komi_val = komiComboBox->currentIndex ();

	engine_komi k = komi_val == 2 ? engine_komi::both : komi_val == 1 ? engine_komi::maybe_swap : engine_komi::dflt;
	m_all_jobs.emplace_front (this, f, gr, secondsEdit->text ().toInt (), maxlinesEdit->text ().toInt (), k);
	job *j = &m_all_jobs.front ();
	insert_job (m_jobs, jobView, j);

	update_progress ();
	if (analyzer_state () == analyzer::paused) {
		queue_next ();
	}
}
