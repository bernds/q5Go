#include <QProcess>

#include "qgtp.h"

GTP_Process *GTP_Controller::create_gtp (const Engine &engine, int size, double komi, int hc)
{
	GTP_Process *g = new GTP_Process (m_parent, this, engine, size, komi, hc);
	return g;
}

/* Function:  open a session
* Arguments: name and path of go program
* Fails:     never
* Returns:   nothing
*/
GTP_Process::GTP_Process(QWidget *parent, GTP_Controller *c, const Engine &engine,
			 int size, float komi, int hc)
	: m_dlg (parent, TextView::type::gtp), m_controller (c), m_size (size), m_komi (komi), m_hc (hc)
{
	const QString &prog = engine.path ();
	const QString &args = engine.args ();
	req_cnt = 1;

	m_dlg.show ();
	m_dlg.activateWindow ();
	connect (m_dlg.buttonAbort, &QPushButton::clicked, this, &GTP_Process::slot_abort_request);

	connect (this, &QProcess::started, this, &GTP_Process::slot_started);
	connect (this, &QProcess::errorOccurred, this, &GTP_Process::slot_error);
	// ??? Unresolved overload errors without this.
	void (QProcess::*fini)(int, QProcess::ExitStatus) = &QProcess::finished;
	connect (this, fini, this, &GTP_Process::slot_finished);
	connect (this, &QProcess::readyReadStandardError, this, &GTP_Process::slot_startup_messages);
	connect (this, &QProcess::readyReadStandardOutput, this, &GTP_Process::slot_receive_stdout);

	QStringList arguments;
	if (!args.isEmpty ())
		arguments = args.split (QRegExp ("\\s+"));
	QFileInfo fi (prog);
	QString wd = fi.dir ().absolutePath ();
	setWorkingDirectory (wd);
	m_dlg.textEdit->setTextColor (Qt::red);
	m_dlg.append ("Working directory: " + wd);
	m_dlg.textEdit->setTextColor (Qt::black);
	start (prog, arguments);
}

void GTP_Process::slot_started ()
{
	send_request ("protocol_version", &GTP_Process::startup_part2);
}

void GTP_Process::slot_error (QProcess::ProcessError)
{
	m_stopped = true;
	m_receivers.clear ();
	m_dlg.hide ();
	m_controller->gtp_exited ();
}

void GTP_Process::slot_abort_request (bool)
{
	m_receivers.clear ();
	quit ();
	m_dlg.hide ();
	m_controller->gtp_exited ();
}

void GTP_Process::slot_startup_messages ()
{
	QString output = readAllStandardError ();
	m_dlg.append (output);
}

void GTP_Process::startup_part2 (const QString &response)
{
	if (response != "2") {
		m_controller->gtp_failure (tr ("GTP engine reported unsupported protocol version"));
		quit ();
		return;
	}
	send_request (QString ("boardsize ") + QString::number (m_size), &GTP_Process::startup_part3);
}

void GTP_Process::startup_part3 (const QString &)
{
	send_request ("clear_board", &GTP_Process::startup_part4);
}

void GTP_Process::startup_part4 (const QString &)
{
	char komi[20];
	sprintf (komi, "komi %.2f", m_komi);
	t_receiver rcv = &GTP_Process::startup_part6;
	if (m_hc > 2)
		rcv = &GTP_Process::startup_part5;
	send_request (komi, rcv);
}

void GTP_Process::startup_part5 (const QString &)
{
	send_request (QString ("fixed_handicap ") + QString::number (m_hc), &GTP_Process::startup_part6);
}

void GTP_Process::startup_part6 (const QString &)
{
	/* Set this before calling startup success, as the callee may want to examine it.  */
	m_started = true;
	m_dlg.hide ();
	m_controller->gtp_startup_success ();
}

void GTP_Process::receive_move (const QString &move)
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

void GTP_Process::played_move (stone_color col, int x, int y)
{
	if (x >= 8)
		x++;
	char req[20];
	sprintf (req, "%c%d", 'A' + x, m_size - y);
	if (col == black)
		send_request ("play black " + QString (req));
	else
		send_request ("play white " + QString (req));
}

void GTP_Process::played_move_pass (stone_color col)
{
	if (col == black)
		send_request ("play black pass");
	else
		send_request ("play white pass");
}

void GTP_Process::request_move (stone_color col)
{
	if (col == black)
		send_request ("genmove black", &GTP_Process::receive_move);
	else
		send_request ("genmove white", &GTP_Process::receive_move);
}

void GTP_Process::analyze (stone_color col, int interval)
{
	if (col == black)
		send_request ("lz-analyze black " + QString::number (interval));
	else
		send_request ("lz-analyze white " + QString::number (interval));
}

void GTP_Process::pause_analysis ()
{
	/* There doesn't seem to be a specific command for stopping the analysis
	   in Leela Zero, but sending anything else does the trick.  So just
	   pick something that is valid but doesn't affect the board position.  */
	send_request ("known_command known_command");
}

/* Code */

// exit
void GTP_Process::slot_finished (int exitcode, QProcess::ExitStatus status)
{
	m_stopped = true;
	qDebug() << req_cnt << " quit";
	m_controller->gtp_exited ();
}


/* QProcess communication has been a source of problems - so we don't use the readyRead
   slots directly to receive GTP responses.  Instead, we have this intermediate function
   to collect ouput and pass along full output lines to receivers.  This way we can even
   queue up multiple commands at once.  */
void GTP_Process::slot_receive_stdout ()
{
	m_buffer += readAllStandardOutput ();

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

		int idx = m_buffer.indexOf ("\n");
		if (idx < 0)
			return;

		QString output = m_buffer.left (idx).trimmed ();

		m_dlg.textEdit->setTextColor (Qt::red);
		m_dlg.append (output);
		m_dlg.textEdit->setTextColor (Qt::black);

		if (output.length () >= 10 && output.left (10) == "info move ") {
			m_buffer = m_buffer.mid (idx + 1);
			m_controller->gtp_eval (output);
			return;
		}

		if (m_receivers.isEmpty ())
			return;
		m_buffer = m_buffer.mid (idx + 1);

		if (output[0] != '=') {
			m_controller->gtp_failure (tr ("Invalid response from GTP engine"));
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
			m_controller->gtp_failure (tr ("Invalid response from GTP engine"));
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

void GTP_Process::send_request(const QString &s, t_receiver rcv)
{
	qDebug() << "send_request -> " << req_cnt << " " << s << "\n";
	m_receivers[req_cnt] = rcv;
#if 1
	m_dlg.textEdit->setTextColor (Qt::blue);
	m_dlg.append (s);
	m_dlg.textEdit->setTextColor (Qt::black);
#endif
	QString req = QString::number (req_cnt) + " " + s + "\n";
	write (req.toLatin1 ());
	waitForBytesWritten();
	req_cnt++;
}

void GTP_Process::internal_quit ()
{
	m_stopped = true;
	disconnect (this, &QProcess::readyReadStandardOutput, nullptr, nullptr);
	disconnect (this, &QProcess::readyReadStandardError, nullptr, nullptr);
	send_request ("quit");
}

void GTP_Process::quit ()
{
	void (QProcess::*fini)(int, QProcess::ExitStatus) = &QProcess::finished;
	disconnect (this, fini, nullptr, nullptr);
	internal_quit ();
}

GTP_Process::~GTP_Process ()
{
	m_stopped = true;
	disconnect (this, &QProcess::readyReadStandardOutput, nullptr, nullptr);
	disconnect (this, &QProcess::readyReadStandardError, nullptr, nullptr);
	disconnect (this, &QProcess::errorOccurred, nullptr, nullptr);
	void (QProcess::*fini)(int, QProcess::ExitStatus) = &QProcess::finished;
	disconnect (this, fini, nullptr, nullptr);
}
