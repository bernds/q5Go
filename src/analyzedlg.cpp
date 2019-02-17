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

	openButton->setEnabled (false);

	connect (enqueueButton, &QPushButton::clicked, [=] (bool) { start_job (); });
	connect (fileselButton, &QPushButton::clicked, [=] (bool) { select_file (); });
	connect (openButton, &QPushButton::clicked, [=] (bool) { open_in_progress_window (); });
	jobView->setModel (&m_job_model);
	connect (jobView, &QListView::clicked, [=] (QModelIndex) { update_progress (); });
	connect (jobView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [=] (const QItemSelection &, const QItemSelection &) { update_progress (); });

	connect (configureButton, &QPushButton::clicked, [=] (bool) { client_window->dlgSetPreferences (3); });
	connect (engineStartButton, &QPushButton::clicked, [=] (bool) { start_engine (); });
	connect (engineLogButton, &QPushButton::clicked, [=] (bool) { m_analyzer->dialog ()->show (); });

	connect (closeButton, &QPushButton::clicked, [=] (bool) { close (); });

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
	if (m_jobs.size () != 0) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning (this, PACKAGE,
					       tr ("Jobs are still running.  Do you wish to terminate the engine and discard the jobs?"),
					       QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

		if (choice == QMessageBox::Cancel) {
			e->ignore ();
			return;
		}
	}
	stop_analyzer ();
	m_job_model.clear ();
	m_jobs.clear ();
	m_job_map.clear ();
	update_progress ();
	e->accept ();
}

void AnalyzeDialog::analyzer_state_changed ()
{
	analyzer s = analyzer_state ();
	switch (s) {
	case analyzer::disconnected:
		engineStatusLabel->setText (tr ("Status: not running"));
		break;
	case analyzer::starting:
		engineStatusLabel->setText (tr ("Status: starting up"));
		break;
	case analyzer::paused:
		engineStatusLabel->setText (tr ("Status: idle"));
		break;
	case analyzer::running:
		engineStatusLabel->setText (tr ("Status: working"));
		break;
	}
	engineStartButton->setEnabled (s == analyzer::disconnected);
	engineLogButton->setEnabled (m_analyzer != nullptr && !m_analyzer->dialog ()->isVisible ());
}

AnalyzeDialog::job::job (AnalyzeDialog *dlg, QString &title, std::shared_ptr<game_record> gr, int n_seconds, int n_lines,
			 stone_color col, bool all)
	: m_dlg (dlg), m_game (gr), m_n_seconds (n_seconds), m_n_lines (n_lines),
	  m_side (col), m_analyze_all (all)
{
	collect_positions (m_game->get_root ());
	m_initial_size = m_queue.size ();
	QStandardItem *item = new QStandardItem (title);
	item->setDropEnabled (false);
	item->setEditable (false);
	m_idx = m_dlg->m_job_count++;
	m_dlg->m_job_map.insert (m_idx, this);
	item->setData (m_idx, Qt::UserRole + 1);
	m_dlg->m_job_model.appendRow (item);
	m_dlg->jobView->setCurrentIndex (item->index ());
}

AnalyzeDialog::job::~job ()
{
	if (m_win)
		disconnect (m_connection);
	for (int i = 0; i < m_dlg->m_job_model.rowCount (); i++) {
		QStandardItem *item = m_dlg->m_job_model.item (i);
		int jidx = item->data (Qt::UserRole + 1).toInt ();
		if (jidx == m_idx) {
			m_dlg->m_job_model.removeRow (i);
			break;
		}
	}
	m_dlg->m_job_map.remove (m_idx);
}

void AnalyzeDialog::job::collect_positions (game_state *st)
{
	for (auto it: st->children ())
		collect_positions (it);
	m_queue.push_back (st);
}

game_state *AnalyzeDialog::job::select_request (bool pop)
{
	if (m_queue.size () == 0)
		return nullptr;

	game_state *st = m_queue.back ();
	if (pop)
		m_queue.pop_back ();
	return st;
}

void AnalyzeDialog::job::show_window ()
{
	if (m_win == nullptr) {
		m_win = new MainWindow (nullptr, m_game, modeBatch);
		m_connection = connect (m_win, &MainWindow::signal_closeevent,
					[=] () { m_win = nullptr; m_dlg->update_progress (); });
	}
	m_win->show ();
	m_win->activateWindow ();
}

void AnalyzeDialog::open_in_progress_window ()
{
	const QModelIndexList &selected = jobView->selectionModel ()->selectedIndexes ();
	if (selected.length () == 0)
		return;

	QModelIndex i = selected.first ();
	QStandardItem *item = m_job_model.itemFromIndex (i);
	int jidx = item->data (Qt::UserRole + 1).toInt ();
	job *j = m_job_map[jidx];
	j->show_window ();
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
	if (m_jobs.size () != 0) {
		QStandardItem *item = m_job_model.item (0);
		int jidx = item->data (Qt::UserRole + 1).toInt ();
		job *j = m_job_map[jidx];
		game_state *st = j->select_request (false);
		if (st != nullptr && st->get_board ().size_x () == m_running_boardsize) {
			m_seconds_count = 0;
			m_requester = j;
			if (analyzer_state () == analyzer::paused)
				pause_analyzer (false, st);
			else
				request_analysis (st);
			return;
		}
	}
	pause_analyzer (true, nullptr);
}

void AnalyzeDialog::eval_received (const QString &, int)
{
	job *j = m_requester;
	if (++m_seconds_count < j->m_n_seconds)
		return;
	j->m_done++;
	update_progress ();

	game_state *st = j->select_request (true);
	if (j->m_queue.size () == 0) {
		m_jobs.erase (std::remove (std::begin (m_jobs), std::end (m_jobs), j), std::end (m_jobs));
		j->show_window ();
		j->m_win->setGameMode (modeNormal);
		delete j;
		update_progress ();
	} else {
		st->set_eval_data (*m_eval_state, false);
		auto variations = m_eval_state->take_children ();
		int count = 0;
		for (auto it: variations) {
			double wr = it->eval_wr_black ();
			QString cnt = QString::number (count + 1);
			QString wrb = QString::number (wr * 100);
			QString wrw = QString::number ((1 - wr) * 100);
			QString vis = QString::number (it->eval_visits ());
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
	}
	queue_next ();
}

void AnalyzeDialog::update_progress ()
{
	QItemSelectionModel *sel = jobView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool entries = m_job_model.rowCount () != 0;
	bool selection = selected.length () != 0;

	jobView->setEnabled (entries);
	progressBar->setEnabled (selection);
	if (!selection) {
		progressBar->setValue (0);
		openButton->setEnabled (false);
		openButton->setChecked (false);
		return;
	}
	QModelIndex i = selected.first ();
	QStandardItem *item = m_job_model.itemFromIndex (i);
	int jidx = item->data (Qt::UserRole + 1).toInt ();
	job *j = m_job_map[jidx];
	if (j == nullptr) {
		progressBar->setValue (0);
		openButton->setEnabled (false);
		openButton->setChecked (false);
		return;
	}
	progressBar->setRange (0, j->m_initial_size);
	progressBar->setValue (j->m_done);
	openButton->setChecked (j->m_win != nullptr);
	openButton->setEnabled (j->m_win == nullptr);
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

	Engine *e = client_window->analysis_engine ();
	if (e == nullptr) {
		QMessageBox::warning(this, PACKAGE, tr("You did not configure any analysis engine!"));
		return;
	}
	m_running_boardsize = e->boardsize ().toInt ();
	start_analyzer (*e, m_running_boardsize, 7.5, 0, false);
}

void AnalyzeDialog::start_job ()
{
	QString f = filenameEdit->text ();
	std::shared_ptr<game_record> gr = record_from_file (f);
	if (gr == nullptr)
		return;

	QFileInfo fi (f);
	m_last_dir = fi.dir ().absolutePath ();

	game_state *root = gr->get_root ();
	const go_board &b = root->get_board ();
	if (b.size_x () != b.size_y ()) {
		QMessageBox::warning(this, PACKAGE, tr("Analysis is supported only for square boards!"));
		return;
	}
	analyzer ast = analyzer_state ();
	if (ast != analyzer::disconnected) {
		if (b.size_x () != m_running_boardsize) {
			QMessageBox::warning(this, PACKAGE, tr("File has a different boardsize than expected by the running engine!"));
			return;
		}
	}
	filenameEdit->setText ("");
	job *j = new job (this, f, gr, secondsEdit->text ().toInt (), maxlinesEdit->text ().toInt (),
			  none, true);
	update_progress ();
	m_jobs.push_back (j);
	if (analyzer_state () == analyzer::paused) {
		queue_next ();
	}
}

