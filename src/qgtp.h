#ifndef QGTP_H
#define QGTP_H

#include <QProcess>

#include "goboard.h"
#include "goeval.h"
#include "setting.h"
#include "textview.h"

class game_state;
class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;

class GTP_Process;

class GTP_Controller
{
	friend class GTP_Process;
	QWidget *m_parent;

protected:
	analyzer_id m_id;

	GTP_Controller (QWidget *p) : m_parent (p) { }
	GTP_Process *create_gtp (const Engine &engine, int size, double komi, bool show_dialog = true);
public:
	virtual void gtp_played_move (int x, int y) = 0;
	virtual void gtp_played_pass () = 0;
	virtual void gtp_played_resign () = 0;
	virtual void gtp_startup_success () = 0;
	virtual void gtp_setup_success () = 0;
	virtual void gtp_exited () = 0;
	virtual void gtp_failure (const QString &) = 0;
	virtual void gtp_eval (const QString &, bool)
	{
	}
	virtual void gtp_switch_ready () { }
};

enum class analyzer { disconnected, starting, running, paused };

class GTP_Eval_Controller : public GTP_Controller
{
	bool m_last_request_flipped {};

protected:
	using GTP_Controller::GTP_Controller;
	~GTP_Eval_Controller();
	GTP_Process *m_analyzer {};
	double m_analyzer_komi = 0;

	bool m_pause_eval = false;
	/* Set if we are in the process of changing positions to analyze.  We send
	   a request to stop analyzing the current position to the GTP process and
	   set this variable.  It is cleared when a response arrives.
	   This solves the problem of receiving updates for an old position.  */
	bool m_switch_pending = false;
	/* Set if we should continue to receive updates, but do not want to update
	   the evaluation data.  Used by the board display to pause when the user
	   holds the right button.  */
	bool m_pause_updates = false;

	double m_primary_eval;
	game_state *m_eval_state {};

	void clear_eval_data ();

	void start_analyzer (const Engine &engine, int size, double komi, bool show_dialog = true);
	void stop_analyzer ();
	void pause_eval_updates (bool on) { m_pause_updates = on; }
	bool pause_analyzer (bool on, go_game_ptr, game_state *);
	void initiate_switch ();
	void request_analysis (go_game_ptr, game_state *, bool flip = false);
	virtual void eval_received (const QString &, int, bool) = 0;
	virtual void analyzer_state_changed () { }
	virtual void notice_analyzer_id (const analyzer_id &, bool) { }
public:
	analyzer analyzer_state ();

	virtual void gtp_played_move (int, int) override { /* Should not happen.  */ }
	virtual void gtp_played_resign () override { /* Should not happen.  */ }
	virtual void gtp_played_pass () override { /* Should not happen.  */ }
	virtual void gtp_setup_success () override { /* Should not happen.  */ }
	virtual void gtp_eval (const QString &, bool) override;
	virtual void gtp_switch_ready () override;
};

class GTP_Process : public QProcess
{
	Q_OBJECT

	QString m_buffer;
	QString m_stderr_buffer;

	TextView m_dlg;
	GTP_Controller *m_controller;

	int m_dlg_lines = 0;
	int m_size;
	/* The komi we've requested with the "komi" command.  The engine may have
	   ignored it.  */
	double m_komi;

	/* A tree in which we keep track of what the move history, in sync with the GTP engine.  */
	game_state *m_moves {};
	game_state *m_last_move {};

	bool m_started = false;
	bool m_stopped = false;

	bool m_analyze_lz = false;
	bool m_analyze_kata = false;

	typedef void (GTP_Process::*t_receiver) (const QString &);
	QMap <int, t_receiver> m_receivers;
	QMap <int, t_receiver> m_err_receivers;

	/* Number of the next request.  */
	int req_cnt;
	void send_request(const QString &, t_receiver = nullptr, t_receiver = &GTP_Process::default_err_receiver);

	void startup_part2 (const QString &);
	void startup_part3 (const QString &);
	void startup_part4 (const QString &);
	void startup_part5 (const QString &);
	void startup_part6 (const QString &);
	void startup_part7 (const QString &);
	void setup_success (const QString &);
	void receive_move (const QString &);
	void pause_callback (const QString &);
	void internal_quit ();
	void default_err_receiver (const QString &);
	void dup_move (game_state *, bool);
	void append_text (const QString &, const QColor &col);

public slots:
	void slot_started ();
	void slot_finished (int exitcode, QProcess::ExitStatus status);
	void slot_error (QProcess::ProcessError);
	void slot_receive_stdout ();
	void slot_startup_messages ();
	void slot_abort_request (bool);

public:
	GTP_Process (QWidget *parent, GTP_Controller *c, const Engine &engine,
		     int size, float komi, bool show_dialog = true);
	~GTP_Process ();
	bool started () { return m_started; }
	bool stopped () { return m_stopped; }

	void clear_board () { send_request ("clear_board"); }
	void setup_board (game_state *, double, bool);
	void setup_initial_position (game_state *);
	void request_move (stone_color col);
	void played_move (stone_color col, int x, int y);
	void undo_move ();
	void komi (double);
	void played_move_pass (stone_color col);
	void played_move_resign (stone_color col);
	void analyze (stone_color col, int interval);
	void pause_analysis ();
	void initiate_analysis_switch ();

	void quit ();

	QDialog *dialog () { return &m_dlg; }
};

#endif
