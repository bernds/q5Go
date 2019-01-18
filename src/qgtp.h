#ifndef QGTP_H
#define QGTP_H

#include <QProcess>

#include "goboard.h"
#include "textview.h"

#define IGTP_BUFSIZE 2048    /* Size of the response buffer */
#define OK 0
#define FAIL -1

class GTP_Process;

class Gtp_Controller
{
	friend class GTP_Process;
	QWidget *m_parent;

public:
	Gtp_Controller (QWidget *p) : m_parent (p) { }
	GTP_Process *create_gtp (const QString &program, const QString &args,
			  int size, double komi, int hc, int level);
	virtual void gtp_played_move (int x, int y) = 0;
	virtual void gtp_played_pass () = 0;
	virtual void gtp_played_resign () = 0;
	virtual void gtp_startup_success () = 0;
	virtual void gtp_exited () = 0;
	virtual void gtp_failure (const QString &) = 0;
	virtual void gtp_eval (const QString &)
	{
	}
};

class GTP_Process : public QProcess
{
	Q_OBJECT

	QString m_buffer;

	TextView m_dlg;
	Gtp_Controller *m_controller;

	int m_size;
	double m_komi;
	int m_hc;
	int m_level;
	bool m_started = false;
	bool m_stopped = false;

	typedef void (GTP_Process::*t_receiver) (const QString &);
	QMap <int, t_receiver> m_receivers;

	/* Number of the next request.  */
	int req_cnt;
	void send_request(const QString &, t_receiver = nullptr);

	void startup_part2 (const QString &);
	void startup_part3 (const QString &);
	void startup_part4 (const QString &);
	void startup_part5 (const QString &);
	void startup_part6 (const QString &);
	void receive_move (const QString &);
	void internal_quit ();

public slots:
	void slot_started ();
	void slot_finished (int exitcode, QProcess::ExitStatus status);
	void slot_error (QProcess::ProcessError);
	void slot_receive_stdout ();
	void slot_startup_messages ();
	void slot_abort_request (bool);

// void slot_stateChanged(QProcess::ProcessState);

public:
	GTP_Process (QWidget *parent, Gtp_Controller *c, const QString &prog, const QString &args,
		     int size, float komi, int hc, int level);
	~GTP_Process ();
	bool started () { return m_started; }
	bool stopped () { return m_stopped; }

	void clear_board () { send_request ("clear_board"); }
	void request_move (stone_color col);
	void played_move (stone_color col, int x, int y);
	void played_move_pass (stone_color col);
	void played_move_resign (stone_color col);
	void analyze (stone_color col, int interval);
	void pause_analysis ();

	void quit ();
};

#endif
