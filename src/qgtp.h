#ifndef QGTP_H
#define QGTP_H

#include <QProcess>

#include "goboard.h"
#include "textview.h"

class gtp_exception : public std::exception
{
public:
	virtual QString errmsg () const = 0;
};

class process_timeout : public gtp_exception
{
public:
	process_timeout () { }
	QString errmsg () const { return QObject::tr ("Program timed out"); }
};

class invalid_response : public gtp_exception
{
public:
	invalid_response () { }
	QString errmsg () const { return QObject::tr ("Invalid response from program"); }
};

class wrong_protocol : public gtp_exception
{
public:
	wrong_protocol () { }
	QString errmsg () const { return QObject::tr ("Program reported unsupported protocol version"); }
};

#define IGTP_BUFSIZE 2048    /* Size of the response buffer */
#define OK 0
#define FAIL -1

class QGtp;

class Gtp_Controller
{
	friend class QGtp;
	QWidget *m_parent;

public:
	Gtp_Controller (QWidget *p) : m_parent (p) { }
	QGtp *create_gtp (const QString &program, const QString &args,
			  int size, double komi, int hc, int level);
	virtual void gtp_played_move (int x, int y) = 0;
	virtual void gtp_played_pass () = 0;
	virtual void gtp_played_resign () = 0;
	virtual void gtp_startup_success () = 0;
	virtual void gtp_exited () = 0;
	virtual void gtp_failure (const gtp_exception &) = 0;
};

class QGtp : public QObject
{
	Q_OBJECT

	QProcess *m_process;
	QString m_buffer;

	TextView m_dlg;
	Gtp_Controller *m_controller;

	int m_size;
	double m_komi;
	int m_hc;
	int m_level;

	typedef void (QGtp::*t_receiver) (const QString &);
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

public slots:
	void slot_started ();
	void slot_finished (int exitcode, QProcess::ExitStatus status);
	void slot_error (QProcess::ProcessError);
	void slot_receive_stdout ();
	void slot_startup_messages ();
	void slot_abort_request (bool);

// void slot_stateChanged(QProcess::ProcessState);

public:
	QGtp(QWidget *parent, Gtp_Controller *c, const QString &prog, const QString &args,
	     int size, float komi, int hc, int level);
	~QGtp();

	void request_move (stone_color col);
	void played_move (stone_color col, int x, int y);
	void played_move_pass (stone_color col);
	void played_move_resign (stone_color col);

	void quit ();
};

#endif
