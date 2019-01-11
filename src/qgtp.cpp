#include <iostream>

#include <stdlib.h>

#include <QProcess>

#include "defines.h"
#include "qgtp.h"

QGtp *Gtp_Controller::create_gtp (const QString &prog, const QString &args, int size, double komi, int hc, int level)
{
	QGtp *g = new QGtp (m_parent, this, prog, args, size, komi, hc, level);
	return g;
}

/* Function:  open a session
* Arguments: name and path of go program
* Fails:     never
* Returns:   nothing
*/
QGtp::QGtp(QWidget *parent, Gtp_Controller *c, const QString &prog, const QString &args,
	   int size, float komi, int hc, int level)
	: m_dlg (parent, TextView::type::gtp), m_controller (c), m_size (size), m_komi (komi), m_hc (hc), m_level (level)
{
	req_cnt = 1;

	m_process = new QProcess();
	m_dlg.show ();
	m_dlg.activateWindow ();
	connect (m_dlg.buttonAbort, &QPushButton::clicked, this, &QGtp::slot_abort_request);

	connect (m_process, &QProcess::started, this, &QGtp::slot_started);
	connect (m_process, &QProcess::errorOccurred, this, &QGtp::slot_error);
	// ??? Unresolved overload errors without this.
	void (QProcess::*fini)(int, QProcess::ExitStatus) = &QProcess::finished;
	connect (m_process, fini, this, &QGtp::slot_finished);
	connect (m_process, &QProcess::readyReadStandardError,
		 this, &QGtp::slot_startup_messages);
	connect (m_process, &QProcess::readyReadStandardOutput,
		 this, &QGtp::slot_receive_stdout);

	QStringList arguments;
	if (!args.isEmpty ())
		arguments = args.split (QRegExp ("\\s+"));
	m_process->start (prog, arguments);
}

QGtp::~QGtp()
{
	if (m_process)
		delete m_process;
}

void QGtp::slot_started ()
{
	send_request ("protocol_version", &QGtp::startup_part2);
}

void QGtp::slot_error (QProcess::ProcessError)
{
	m_receivers.clear ();
	m_dlg.hide ();
	m_controller->gtp_exited ();
}

void QGtp::slot_abort_request (bool)
{
	m_receivers.clear ();
	quit ();
	m_dlg.hide ();
	m_controller->gtp_exited ();
}

void QGtp::slot_startup_messages ()
{
	QString output = m_process->readAllStandardError ();
	m_dlg.append (output);
}

void QGtp::startup_part2 (const QString &response)
{
	if (response != "2") {
		m_controller->gtp_failure (wrong_protocol ());
		quit ();
		return;
	}
	send_request (QString ("boardsize ") + QString::number (m_size), &QGtp::startup_part3);
}

void QGtp::startup_part3 (const QString &)
{
	send_request ("clear_board", &QGtp::startup_part4);
}

void QGtp::startup_part4 (const QString &)
{
	char komi[20];
	sprintf (komi, "komi %.2f", m_komi);
	t_receiver rcv = &QGtp::startup_part6;
	if (m_hc > 2)
		rcv = &QGtp::startup_part5;
	send_request (komi, rcv);
}

void QGtp::startup_part5 (const QString &)
{
	send_request (QString ("fixed_handicap ") + QString::number (m_hc), &QGtp::startup_part6);
}

void QGtp::startup_part6 (const QString &)
{
	m_controller->gtp_startup_success ();
}

void QGtp::receive_move (const QString &move)
{
	if (move.toLower () == "resign") {
		m_controller->gtp_played_resign ();
	} else if (move.toLower () == "pass") {
		m_controller->gtp_played_pass ();
	} else {
		QChar sx = move[0];

		int i = sx.toLatin1() - 'A';
		if (i > 7)
			i--;
		int j = move.mid (1).toInt();
		m_controller->gtp_played_move (i, m_size - j);
	}
}

void QGtp::played_move (stone_color col, int x, int y)
{
	if (x >= 8)
		x++;
	try {
		char req[20];
		sprintf (req, "%c%d", 'A' + x, m_size - y);
		if (col == black)
			send_request ("play black " + QString (req));
		else
			send_request ("play white " + QString (req));
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::played_move_pass (stone_color col)
{
	try {
		if (col == black)
			send_request ("play black pass");
		else
			send_request ("play white pass");
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::request_move (stone_color col)
{
	if (col == black)
		send_request ("genmove black", &QGtp::receive_move);
	else
		send_request ("genmove white", &QGtp::receive_move);
}

/* Code */

// exit
void QGtp::slot_finished (int exitcode, QProcess::ExitStatus status)
{
	qDebug() << req_cnt << " quit";
	m_controller->gtp_exited ();
}


/* QProcess communication has been a source of problems - so we don't use the readyRead
   slots directly to receive GTP responses.  Instead, we have this intermediate function
   to collect ouput and pass along full output lines to receivers.  This way we can even
   queue up multiple commands at once.  */
void QGtp::slot_receive_stdout ()
{
	m_buffer += m_process->readAllStandardOutput ();

	for (;;) {
		/* Clear empty lines at the front of the buffer.  */
		int last_lf = -1;
		int pos = 0;
		while (pos < m_buffer.length () && m_buffer[pos].isSpace ()) {
			if (m_buffer[pos] == '\n')
				last_lf = pos;
			pos++;
		}
		if (last_lf >= 0)
			m_buffer = m_buffer.mid (last_lf + 1);
		if (m_receivers.isEmpty ())
			return;

		int idx = m_buffer.indexOf ("\n");
		if (idx < 0)
			return;

		QString output = m_buffer.left (idx).trimmed ();
		m_buffer = m_buffer.mid (idx + 1);

		m_dlg.textEdit->setTextColor (Qt::red);
		m_dlg.append (output);
		m_dlg.textEdit->setTextColor (Qt::black);

		if (output[0] != '=') {
			m_controller->gtp_failure (invalid_response ());
			quit ();
			return;
		}
		output.remove (0, 1);
		int len = output.length ();
		int n_digits = 0;
		while (n_digits < len && output[n_digits].isDigit ())
			n_digits++;
		while (n_digits < len && output[n_digits].isSpace ())
			n_digits++;
		int cmd_nr = output.left (n_digits).toInt ();
		QMap<int, t_receiver>::const_iterator map_iter = m_receivers.constFind (cmd_nr);
		if (map_iter == m_receivers.constEnd ()) {
			m_controller->gtp_failure (invalid_response ());
			quit ();
			return;
		}
		t_receiver rcv = *map_iter;
		m_receivers.remove (cmd_nr);
		output.remove (0, n_digits);
		if (rcv != nullptr)
			(this->*rcv) (output);
	}
}

void QGtp::send_request(const QString &s, t_receiver rcv)
{
	qDebug() << "send_request -> " << req_cnt << " " << s << "\n";
	m_receivers[req_cnt] = rcv;
#if 1
	m_dlg.textEdit->setTextColor (Qt::blue);
	m_dlg.append (s);
	m_dlg.textEdit->setTextColor (Qt::black);
#endif
	QString req = QString::number (req_cnt) + " " + s + "\n";
	m_process->write (req.toLatin1 ());
	m_process->waitForBytesWritten();
	req_cnt++;
}

void QGtp::quit ()
{
	disconnect (m_process, &QProcess::readyReadStandardOutput, nullptr, nullptr);
	disconnect (m_process, &QProcess::readyReadStandardError, nullptr, nullptr);
	send_request ("quit");
}
