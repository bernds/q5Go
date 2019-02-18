/*
 * board.h
 */

#ifndef ANALYZEDLG_H
#define ANALYZEDLG_H

#include <vector>
#include <map>
#include <forward_list>

#include "defines.h"
#include "setting.h"
#include "goboard.h"
#include "gogame.h"
#include "qgtp.h"

#include "ui_analyze_gui.h"

class MainWindow;

class AnalyzeDialog : public QMainWindow, public Ui::AnalyzeDialog, public GTP_Eval_Controller
{
	Q_OBJECT

	struct job;
	friend struct job;
	struct display {
		std::vector<job *> jobs;
		QMap<int, job *> map;
		QStandardItemModel model;
	};
	/* Keeps all jobs so that they can be deleted once the dialog is destroyed.
	   Or we could clear it manually if we have no running jobs in the displays.  */
	std::forward_list<job> m_all_jobs;
	struct display m_jobs, m_done;

	int m_job_count = 0;

	struct job
	{
		AnalyzeDialog *m_dlg;
		QString m_title;
		std::shared_ptr<game_record> m_game;
		MainWindow *m_win {};
		QMetaObject::Connection m_connection;
		int m_n_seconds;
		int m_n_lines;
		stone_color m_side;
		bool m_analyze_all;

		std::vector<game_state *> m_queue;
		size_t m_initial_size;
		size_t m_done = 0;

		display *m_display;
		int m_idx;

		job (AnalyzeDialog *dlg, QString &title, std::shared_ptr<game_record> gr, int n_seconds, int n_lines,
		     stone_color col, bool all);
		~job ();
		game_state *select_request (bool pop);
		void show_window (bool done);
	private:
		void collect_positions (game_state *);
	};

	job *m_requester;
	int m_seconds_count;
	QIntValidator m_seconds_vald { 1, 86400 };
	QIntValidator m_lines_vald { 1, 100 };

	QString m_last_dir;
	int m_running_boardsize;

	void queue_next ();

	void select_file ();
	void start_engine ();
	void start_job ();

	/* Maintaining the job queue listviews and assorted data structures.  */
	void insert_job (display &, QListView *, job *);
	void remove_job (display &, job *);

	void update_progress ();
	void update_buttons (display &, QListView *, QProgressBar *, QToolButton *, QToolButton *);
	job *selected_job (bool done);
	void open_in_progress_window (bool done);
	void discard_job (bool done);

	/* Virtuals from Gtp_Controller.  */
	virtual void eval_received (const QString &, int) override;
	virtual void analyzer_state_changed () override;

	virtual void closeEvent (QCloseEvent *) override;
public:
	AnalyzeDialog (QWidget *parent, const QString &filename);
	~AnalyzeDialog();

	/* Virtuals from Gtp_Controller.  */
	virtual void gtp_startup_success () override;
	virtual void gtp_exited () override;
	virtual void gtp_failure (const QString &) override;
};

extern AnalyzeDialog *analyze_dialog;

#endif
