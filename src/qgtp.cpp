#include <QProcess>

#include "qgo.h"
#include "setting.h"
#include "qgtp.h"
#include "gogame.h"

GTP_Process *GTP_Controller::create_gtp (const Engine &engine, int size, double komi, int hc, bool show_dialog)
{
	m_id.engine = engine.title.toStdString ();
	QString ekstr = engine.komi;
	bool ok;
	double ekomi = ekstr.toFloat (&ok);
	m_id.komi_set = !ekstr.isEmpty () && ok;
	if (m_id.komi_set) {
		m_id.komi = ekomi;
	} else
		m_id.komi = komi;

	GTP_Process *g = new GTP_Process (m_parent, this, engine, size, komi, hc, show_dialog);
	return g;
}

GTP_Process::GTP_Process(QWidget *parent, GTP_Controller *c, const Engine &engine,
			 int size, float komi, int hc, bool show_dialog)
	: m_dlg (parent, TextView::type::gtp), m_controller (c), m_size (size), m_komi (komi), m_hc (hc)
{
	const QString &prog = engine.path;
	const QString &args = engine.args;
	req_cnt = 1;

	if (show_dialog) {
		m_dlg.show ();
		m_dlg.activateWindow ();
	}
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

void GTP_Process::pause_callback (const QString &)
{
	m_controller->gtp_switch_ready ();
}

/* Like pause, but the controller wants to be called back when the program has
   responded.  */
void GTP_Process::initiate_analysis_switch ()
{
	send_request ("known_command known_command", &GTP_Process::pause_callback);
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

		bool err = output[0] != '=';
		output.remove (0, 1);
		int len = output.length ();
		int n_digits = 0;
		while (n_digits < len && output[n_digits].isDigit ())
			n_digits++;
		while (n_digits < len && output[n_digits].isSpace ())
			n_digits++;
		int cmd_nr = output.left (n_digits).toInt ();
		auto &rcv_map = err ? m_err_receivers : m_receivers;
		QMap<int, t_receiver>::const_iterator map_iter = rcv_map.constFind (cmd_nr);
		if (map_iter == m_receivers.constEnd ()) {
			m_controller->gtp_failure (tr ("Invalid response from GTP engine"));
			quit ();
			return;
		}
		t_receiver rcv = *map_iter;
		m_receivers.remove (cmd_nr);
		m_err_receivers.remove (cmd_nr);
		output.remove (0, n_digits);
		if (rcv != nullptr)
			(this->*rcv) (output);
	}
}

void GTP_Process::default_err_receiver (const QString &)
{
	m_controller->gtp_failure (tr ("Invalid response from GTP engine"));
	quit ();
}

void GTP_Process::send_request(const QString &s, t_receiver rcv, t_receiver err_rcv)
{
	qDebug() << "send_request -> " << req_cnt << " " << s << "\n";
	m_receivers[req_cnt] = rcv;
	m_err_receivers[req_cnt] = err_rcv;
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

GTP_Eval_Controller::~GTP_Eval_Controller ()
{
	clear_eval_data ();
	delete m_analyzer;
}

analyzer GTP_Eval_Controller::analyzer_state ()
{
	if (m_analyzer == nullptr)
		return analyzer::disconnected;
	if (m_analyzer->stopped ())
		return analyzer::disconnected;
	if (!m_analyzer->started ())
		return analyzer::starting;
	if (m_pause_eval)
		return analyzer::paused;
	return analyzer::running;
}

void GTP_Eval_Controller::request_analysis (game_state *st, bool flip)
{
	if (analyzer_state () != analyzer::running)
		return;

	initiate_switch ();

	const go_board &b = st->get_board ();
	stone_color to_move = st->to_move ();
	delete m_eval_state;
	m_eval_state = new game_state (b, to_move);

	std::vector<game_state *> moves;
	while (st->was_move_p () && !st->root_node_p ()) {
		moves.push_back (st);
		st = st->prev_move ();
	}
	const go_board &startpos = st->get_board ();
	m_analyzer->clear_board ();
	for (int i = 0; i < b.size_x (); i++)
		for (int j = 0; j < b.size_y (); j++) {
			stone_color c = startpos.stone_at (i, j);
			if (flip)
				c = flip_color (c);
			if (c != none)
				m_analyzer->played_move (c, i, j);
		}
	while (!moves.empty ()) {
		st = moves.back ();
		moves.pop_back ();
		stone_color c = st->get_move_color ();
		if (flip)
			c = flip_color (c);
		m_analyzer->played_move (c, st->get_move_x (), st->get_move_y ());
	}

	if (flip)
		to_move = flip_color (to_move);

	m_last_request_flipped = flip;
	m_analyzer->analyze (to_move, 100);
}

void GTP_Eval_Controller::clear_eval_data ()
{
	delete m_eval_state;
	m_eval_state = nullptr;
}

void GTP_Eval_Controller::initiate_switch ()
{
	m_switch_pending = true;
	m_analyzer->initiate_analysis_switch ();
}

void GTP_Eval_Controller::gtp_switch_ready ()
{
	m_switch_pending = false;
}

void GTP_Eval_Controller::start_analyzer (const Engine &engine, int size, double komi, int hc, bool show_dialog)
{
	if (m_analyzer != nullptr) {
		m_analyzer->quit ();
		delete m_analyzer;
		m_analyzer = nullptr;
	}
	m_analyzer_komi = komi;
	m_analyzer = create_gtp (engine, size, komi, hc, show_dialog);
	analyzer_state_changed ();
}

void GTP_Eval_Controller::stop_analyzer ()
{
	clear_eval_data ();
	m_pause_eval = false;
	m_switch_pending = false;
	if (m_analyzer != nullptr && !m_analyzer->stopped ()) {
		m_analyzer->quit ();
		analyzer_state_changed ();
	}
}

/* Return true iff the state changed.  */
bool GTP_Eval_Controller::pause_analyzer (bool on, game_state *st)
{
	if (m_analyzer == nullptr || !m_analyzer->started () || m_analyzer->stopped ())
		return false;
	if (m_pause_eval == on)
		return false;
	m_pause_eval = on;
	if (on) {
		clear_eval_data ();
		m_analyzer->pause_analysis ();
	} else {
		request_analysis (st);
	}
		analyzer_state_changed ();
	return true;
}

void GTP_Eval_Controller::gtp_eval (const QString &s)
{
	if (m_pause_updates || m_switch_pending)
		return;

	bool prune = setting->readBoolEntry("ANALYSIS_PRUNE");

	QStringList moves = s.split ("info move ", QString::SkipEmptyParts);
	if (moves.isEmpty ())
		return;

	stone_color to_move = m_eval_state->to_move ();

	int an_maxmoves = setting->readIntEntry ("ANALYSIS_MAXMOVES");
	int count = 0;
	m_primary_eval = 0.5;
	QString primary_move;
	int primary_visits = 0;

	bool flip = m_last_request_flipped;

	analyzer_id id = m_id;
	if (flip)
		id.komi = -id.komi;
	notice_analyzer_id (id);

	for (auto &e: moves) {
//		m_board_win->append_comment (e);
		QRegExp re ("(\\S+)\\s+visits\\s+(\\d+)\\s+winrate\\s+(\\d+)\\s+prior\\s+(\\d+)\\s+order\\s+(\\d+)\\s+pv\\s+(.*)$");
		if (re.indexIn (e) == -1)
			continue;
		QString move = re.cap (1);
		int visits = re.cap (2).toInt ();
		int winrate = re.cap (3).toInt ();
		QString pv = re.cap (6);
		double wr = winrate / 10000.;
		/* The winrate also does not need flipping, it is given for the side to move,
		   and since we flip both the stones and the side to move, it comes out correct
		   in both cases.  */
		if (count == 0) {
			primary_move = move;
			m_primary_eval = wr;
			primary_visits = visits;
			m_eval_state->set_eval_data (visits, to_move == white ? 1 - wr : wr, id);
		}
		qDebug () << move << " PV: " << pv;

		QStringList pvmoves = pv.split (" ", QString::SkipEmptyParts);
		if (count < 52 && (!prune || pvmoves.length () > 1 || visits >= 2)) {
			game_state *cur = m_eval_state;
			bool pv_first = true;
			for (auto &pm: pvmoves) {
				QChar sx = pm[0];

				int i = sx.toLatin1 () - 'A';
				if (i > 7)
					i--;
				int szx = m_eval_state->get_board ().size_x ();
				int szy = m_eval_state->get_board ().size_y ();
				int j = szy - pm.mid (1).toInt ();
				if (i >= 0 && i < szx && j >= 0 && j < szy) {
					game_state *next = cur->add_child_move (i, j);
					/* The program might have given us an invalid move.  Don't
					   crash if it did.  */
					if (next == nullptr)
						break;
					if (pv_first) {
						cur->set_mark (i, j, mark::letter, count);
						next->set_eval_data (visits, to_move == white ? 1 - wr : wr, id);
						/* Leave it to a higher level to add a title if it wants
						   to place these variations into the actual file.  */
						next->set_figure (257, "");
					}
					cur = next;
				} else
					break;
				if (cur == nullptr)
					break;
				pv_first = false;
			}
		}
		count++;
		if (an_maxmoves > 0 && count == an_maxmoves)
			break;
	}
	if (!primary_move.isNull ())
		eval_received (primary_move, primary_visits);
}
