#include <utility>
#include <QProcess>

#include "qgo.h"
#include "setting.h"
#include "qgtp.h"
#include "gogame.h"
#include "timing.h"

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#define SPLIT QString
#else
#define SPLIT Qt
#endif

GTP_Process *GTP_Controller::create_gtp (const Engine &engine, int size_x, int size_y, double komi, bool show_dialog)
{
	analyzer_id id;
	id.engine = engine.title.toStdString ();
	QString ekstr = engine.komi;
	bool ok;
	double ekomi = ekstr.toFloat (&ok);
	id.komi_set = !ekstr.isEmpty () && ok;
	if (id.komi_set) {
		id.komi = ekomi;
	} else
		id.komi = komi;

	GTP_Process *g = new GTP_Process (m_parent, this, engine, size_x, size_y, komi, show_dialog);
	m_id_map[g] = id;
	return g;
}

GTP_Process::GTP_Process(QWidget *parent, GTP_Controller *c, const Engine &engine,
			 int size_x, int size_y, float komi, bool show_dialog)
	: m_name (engine.title), m_dlg (parent, TextView::type::gtp), m_controller (c), m_size_x (size_x), m_size_y (size_y), m_komi (komi)
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
	m_controller->gtp_exited (this);
}

void GTP_Process::slot_abort_request (bool)
{
	m_receivers.clear ();
	quit ();
	m_dlg.hide ();
	m_controller->gtp_exited (this);
}

void GTP_Process::slot_startup_messages ()
{
	m_stderr_buffer += readAllStandardError ();
	if (m_stopped) {
		m_stderr_buffer.clear ();
		return;
	}
	for (;;) {
		int idx = m_stderr_buffer.indexOf ("\n");
		if (idx < 0)
			return;

		QString output = m_stderr_buffer.left (idx).trimmed ();
		append_text (output, Qt::black);
		m_stderr_buffer = m_stderr_buffer.mid (idx + 1);
	}
}

void GTP_Process::startup_part2 (const QString &response)
{
	if (response != "2") {
		handle_failure (tr ("GTP engine reported unsupported protocol version"));
		return;
	}
	if (m_size_x == m_size_y)
		send_request (QString ("boardsize ") + QString::number (m_size_x), &GTP_Process::startup_part3);
	else
		send_request (QString ("rectangular_boardsize ") + QString::number (m_size_x) + " " + QString::number (m_size_y),
			      &GTP_Process::startup_part3, &GTP_Process::rect_board_err_receiver);
}

void GTP_Process::startup_part3 (const QString &)
{
	send_request ("clear_board", &GTP_Process::startup_part4);
}

void GTP_Process::startup_part4 (const QString &)
{
	send_request ("komi " + QString::number (m_komi), &GTP_Process::startup_part5);
}

void GTP_Process::startup_part5 (const QString &)
{
	send_request ("known_command lz-analyze", &GTP_Process::startup_part6);
}

void GTP_Process::startup_part6 (const QString &response)
{
	if (response == "true")
		m_analyze_lz = true;

	send_request ("known_command kata-analyze", &GTP_Process::startup_part7);
}

void GTP_Process::startup_part7 (const QString &response)
{
	if (response == "true")
		m_analyze_kata = true;

	send_request ("known_command kata-set-rules", &GTP_Process::startup_part8);
}

void GTP_Process::startup_part8 (const QString &response)
{
	if (response == "true") {
		m_kata_rules = true;
		send_request ("kata-get-rules", &GTP_Process::startup_part9);
	}
	startup_success ();
}

void GTP_Process::startup_part9 (const QString &response)
{
	m_kata_orig_rules = response;
	qDebug () << "kata rules: " << response;
	startup_success ();
}

void GTP_Process::startup_success ()
{
	/* Set this before calling startup success, as the callee may want to examine it.  */
	m_started = true;
	m_dlg.textEdit->setTextColor (Qt::darkGray);
	m_dlg.append ("[...]\n");
	m_dlg.textEdit->setTextColor (Qt::black);
	m_dlg.hide ();
	m_dlg.remember_cursor ();
	m_controller->gtp_startup_success (this);
}

void GTP_Process::show_messages ()
{
	m_dlg.show ();
}

void GTP_Process::append_text (const QString &txt, const QColor &col)
{
	/* If we let the text grow without bounds, eventually Qt will
	   hang after a few hours of analysis.  */
	if (m_dlg_lines == 200) {
		m_dlg.delete_cursor_line ();
	} else
		m_dlg_lines++;
	m_dlg.textEdit->setTextColor (col);
	m_dlg.append (txt);
}

bool GTP_Process::setup_timing (const time_settings &t)
{
	if (t.system == time_system::none) {
		send_request ("time_settings 1 1 0");
	} else if (t.system == time_system::absolute) {
		send_request (QString ("time_settings %1 0 0").arg (t.main_time.count ()));
	} else if (t.system == time_system::canadian) {
		send_request (QString ("time_settings %1 %2 %3").arg (t.main_time.count ()).arg (t.period_time.count ()).arg (t.canadian_stones));
	} else
		return false;
	return true;
}

void GTP_Process::send_remaining_time (stone_color col, const QString &gtp_time)
{
	send_request (QString ("time_left ") + (col == white ? "white " : "black ") + gtp_time);
}

namespace {
std::pair<int, int> string_to_coords (const QString &move, int size_y)
{
	auto letter_to_coord = [] (QChar c) -> int {
		int x = c.toLatin1() - 'A';
		if (x > 7)
			x--;
		return x;
	};

	int x = letter_to_coord (move[0]);
	int off = 1;
	QChar sx = move[off];
	if (sx.isLetter ()) {
		x = (x + 1) * 25 + letter_to_coord (sx);
		off++;
	}
	int j = move.mid (off).toInt();
	int y = size_y - j;
	return std::make_pair (x, y);
}
}

void GTP_Process::receive_move (const QString &move)
{
	if (move.isEmpty ())
		return;
	if (move.length () >= 10 && move.left (10) == "info move ") {
		m_controller->gtp_eval (this, move, m_analyze_kata);
		return;
	}

	bool is_play = (m_analyze_lz || m_analyze_kata) && move.length () >= 5 && move.left (5) == "play ";
	QString m = is_play ? move.mid (5) : move;
	if (m.toLower () == "resign")
		m_controller->gtp_played_resign (this);
	else if (m.toLower () == "pass")
		m_controller->gtp_played_pass (this);
	else {
		auto coords = string_to_coords (m, m_size_y);
		int x = coords.first;
		int y = coords.second;
		if (m_last_move != nullptr)
			m_last_move = m_last_move->add_child_move (x, y, m_genmove_col);
		m_controller->gtp_played_move (this, x, y);
	}
}

void GTP_Process::played_move (stone_color col, int x, int y)
{
	if (m_last_move != nullptr)
		m_last_move = m_last_move->add_child_move (x, y, col);
	char req[20];
	int first = x / 25;
	x = x % 25;
	if (x >= 8)
		x++;

	if (first > 0)
		sprintf (req, "%c%c%d", 'A' + first - 1, 'A' + x, m_size_y - y);
	else
		sprintf (req, "%c%d", 'A' + x, m_size_y - y);
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

void GTP_Process::komi (double km)
{
	if (km == m_komi)
		return;

	m_komi = km;
	send_request ("komi " + QString::number (m_komi), nullptr, &GTP_Process::komi_err_receiver);
}

void GTP_Process::request_move (stone_color col, bool analyze)
{
	QString cmd = (analyze && m_analyze_kata ? "kata-genmove_analyze"
		       : analyze && m_analyze_lz ? " lz-genmove_analyze"
		       : "genmove");
	QString centis = analyze && m_analyze_lz ? " 100" : "";
	m_genmove_col = col;
	if (col == black)
		send_request (cmd + " black" + centis, &GTP_Process::receive_move);
	else
		send_request (cmd + " white" + centis, &GTP_Process::receive_move);
}

void GTP_Process::set_rules (go_rules r)
{
	if (!m_kata_rules)
		return;
	if (r == m_rules)
		return;
	m_rules = r;
	QString cmd = m_restart_analysis;
	switch (r) {
	case go_rules::unknown:
		// Reset the program's ruleset in case we couldn't make a guess.
	case go_rules::engine_dflt:
		send_request ("kata-set-rules " + m_kata_orig_rules, nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::tt:
		send_request ("kata-set-rules tromp-taylor", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::japanese:
		send_request ("kata-set-rules japanese", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::chinese:
		send_request ("kata-set-rules chinese", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::kgs_chinese:
		send_request ("kata-set-rules chinese-kgs", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::ogs_chinese:
		send_request ("kata-set-rules chinese-ogs", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::old_chinese:
		// ??? Unclear if this is the correct string. It's the one KataGo recommends for Chinese rules,
		// plus "ALL" for group tax, and without "friendlyPassOk" which causes errors
		send_request ("kata-set-rules {\"hasButton\":false,\"ko\":\"POSITIONAL\", \"scoring\":\"AREA\", \"suicide\":false, \"tax\":\"ALL\", \"whiteHandicapBonus\":\"N\"}",
			      nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::korean:
		send_request ("kata-set-rules korean", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::aga:
		send_request ("kata-set-rules aga", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::bga:
		send_request ("kata-set-rules bga", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::nz:
		send_request ("kata-set-rules new-zealand", nullptr, &GTP_Process::rules_err_receiver);
		break;
	case go_rules::ing:
		// No idea how to best map this to KataGo.
		send_request ("kata-set-rules tromp-taylor", nullptr, &GTP_Process::rules_err_receiver);
		break;
	}
	if (!cmd.isEmpty ()) {
		send_request (cmd, &GTP_Process::receive_eval);
		m_restart_analysis = cmd;
	}
}

void GTP_Process::request_score ()
{
	send_request ("known_command final_score", &GTP_Process::score_callback_1);
}

void GTP_Process::score_callback_1 (const QString &s)
{
	if (s != "true") {
		m_controller->gtp_report_score (this, "");
		return;
	}
	send_request ("final_score", &GTP_Process::score_callback_2);
}

void GTP_Process::score_callback_2 (const QString &s)
{
	m_controller->gtp_report_score (this, s);
}

void GTP_Process::undo_move ()
{
	send_request ("undo");
	game_state *st = m_last_move;
	if (st != nullptr) {
		m_last_move = st->prev_move ();
		m_manager.release_game_state (st);
	}
}

static stone_color maybe_flip (stone_color col, bool flip)
{
	if (!flip)
		return col;
	return flip_color (col);
}

void GTP_Process::dup_move (game_state *from, bool flip)
{
	stone_color c = maybe_flip (from->get_move_color (), flip);
	played_move (c, from->get_move_x (), from->get_move_y ());
}

void GTP_Process::setup_board (game_state *st, double km, bool flip)
{
	const go_board &b = st->get_board ();

	/* Check for shortcuts.
	   @@@ Should handle flip as well, but need to do something about position_equal_p.  */
	if (m_last_move != nullptr && !flip) {
		game_state *st_parent = st->prev_move ();
		if (st->was_move_p () && st_parent->get_board ().position_equal_p (m_last_move->get_board ())) {
			dup_move (st, flip);
			return;
		}
		game_state *our_parent = m_last_move->prev_move ();
		if (our_parent != nullptr && our_parent->get_board ().position_equal_p (st->get_board ())) {
			undo_move ();
			return;
		}
	}

	/* Must clear this before doing played_move calls for the initial setup.  */
	m_manager.release_game_state (m_moves);
	m_moves = nullptr;
	m_last_move = nullptr;

	std::vector<game_state *> moves;
	while (st->was_move_p () && !st->root_node_p ()) {
		moves.push_back (st);
		st = st->prev_move ();
	}
	const go_board &startpos = st->get_board ();

	clear_board ();
	komi (km);

	for (int i = 0; i < b.size_x (); i++)
		for (int j = 0; j < b.size_y (); j++) {
			/* This gives better behavior if the GTP process dies or misbehaves.  */
			if (stopped ())
				return;
			stone_color c = maybe_flip (startpos.stone_at (i, j), flip);
			if (c != none)
				played_move (c, i, j);
		}

	/* Allocate this here so that m_last_move is nullptr during previous calls to
	   played_move and they don't try to track the position.  */
	m_moves = m_manager.create_game_state (startpos, maybe_flip (st->to_move (), flip));
	m_last_move = m_moves;

	while (!moves.empty ()) {
		st = moves.back ();
		moves.pop_back ();
		dup_move (st, flip);
	}
}

void GTP_Process::setup_success (const QString &)
{
	m_controller->gtp_setup_success (this);
}

void GTP_Process::setup_initial_position (game_state *st)
{
	setup_board (st, m_komi, false);
	send_request ("known_command known_command", &GTP_Process::setup_success);
}

void GTP_Process::analyze (stone_color col, int interval)
{
	QString cmd = m_analyze_kata ? "kata-analyze " : "lz-analyze ";
	if (col == black)
		cmd += "black " + QString::number (interval);
	else
		cmd += "white " + QString::number (interval);
	send_request (cmd, &GTP_Process::receive_eval);
	m_restart_analysis = cmd;
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
	m_controller->gtp_exited (this);
}


/* QProcess communication has been a source of problems - so we don't use the readyRead
   slots directly to receive GTP responses.  Instead, we have this intermediate function
   to collect ouput and pass along full output lines to receivers.  This way we can even
   queue up multiple commands at once.  */
void GTP_Process::slot_receive_stdout ()
{
	m_buffer += readAllStandardOutput ();

	if (m_stopped) {
		m_buffer.clear ();
		return;
	}

	while (!m_stopped) {
		int len = m_buffer.length ();
		if (len == 0)
			return;
		if (m_within_reply && (m_buffer[0] == '\n' || (len > 1 && m_buffer[0] == '\r' && m_buffer[1] == '\n'))) {
			if (m_end_receiver != nullptr)
				(this->*m_end_receiver) (QString ());
			m_cur_receiver = nullptr;
			m_within_reply = false;
		}

		/* Clear empty lines at the front of the buffer.  */
		int last_lf = -1;
		int pos = 0;
		while (pos < len && m_buffer[pos].isSpace ()) {
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
		m_buffer = m_buffer.mid (idx + 1);

		append_text (output, Qt::red);

		qDebug () << (m_within_reply ? "CONT: " : "REPLY: ") << output;
		if (!m_within_reply) {
			if (m_receivers.isEmpty ())
				continue;

			bool err = output[0] != '=';
			output.remove (0, 1);
			int len = output.length ();
			int n_digits = 0;
			while (n_digits < len && output[n_digits].isDigit ())
				n_digits++;
			while (n_digits < len && output[n_digits].isSpace ())
				n_digits++;
			if (n_digits == 0)
				err = true;
			int cmd_nr = output.left (n_digits).toInt ();
			auto &rcv_map = err ? m_err_receivers : m_receivers;
			QMap<int, t_receiver>::const_iterator map_iter = rcv_map.constFind (cmd_nr);
			if (map_iter == rcv_map.constEnd ()) {
				handle_failure (tr ("Invalid response from GTP engine"));
				return;
			}
			qDebug () << "reply for command " << cmd_nr;
			m_cur_receiver = *map_iter;
			m_receivers.remove (cmd_nr);
			m_err_receivers.remove (cmd_nr);
			output.remove (0, n_digits);
			m_within_reply = true;
			QMap<int, t_receiver>::const_iterator map2_iter = m_end_receivers.constFind (cmd_nr);
			if (map2_iter != m_end_receivers.constEnd ()) {
				m_end_receiver = *map2_iter;
				m_end_receivers.remove (cmd_nr);
			} else
				m_end_receiver = nullptr;
		}
		if (m_cur_receiver != nullptr)
			(this->*m_cur_receiver) (output);
	}
}

void GTP_Process::handle_failure (const QString &msg)
{
	// Call quit before reporting the failure so that we suppress all further communication.
	quit ();
	m_controller->gtp_failure (this, msg);
}

void GTP_Process::default_err_receiver (const QString &)
{
	handle_failure (tr ("Invalid response from GTP engine"));
}

void GTP_Process::komi_err_receiver (const QString &errstr)
{
	// Since SGF files with insane komi values exist, don't cause a hard error during analysis.
	m_controller->gtp_komi_failure (this, errstr);
}

void GTP_Process::rules_err_receiver (const QString &errstr)
{
	// @@@ Do something reasonable.
}

void GTP_Process::rect_board_err_receiver (const QString &)
{
	handle_failure (tr ("GTP engine '%1' does not support rectangular boards.").arg (m_name));
}

void GTP_Process::receive_eval (const QString &output)
{
	if (output.length () >= 10 && output.left (10) == "info move ")
		m_controller->gtp_eval (this, output, m_analyze_kata);
}

void GTP_Process::send_request (const QString &s, t_receiver rcv, t_receiver err_rcv)
{
	qDebug() << "send_request -> " << req_cnt << " " << s << "\n";
	m_receivers[req_cnt] = rcv;
	m_err_receivers[req_cnt] = err_rcv;
#if 1
	append_text (s, Qt::blue);
#endif
	QString req = QString::number (req_cnt) + " " + s + "\n";
	write (req.toLatin1 ());
	waitForBytesWritten();
	req_cnt++;

	// Set to a nonempty string only in analysis.
	m_restart_analysis = QString ();
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

void GTP_Eval_Controller::show_analyzer_dialog ()
{
	if (m_analyzer != nullptr)
		m_analyzer->show_messages ();
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

void GTP_Eval_Controller::set_rules (go_rules r)
{
	m_rules = r;
	if (m_analyzer != nullptr)
		m_analyzer->set_rules (r);
}

void GTP_Eval_Controller::set_analysis_state (go_game_ptr gr, game_state *st)
{
	if (m_eval_game != nullptr)
		m_eval_game->release_game_state (m_eval_state);
	const go_board &b = st->get_board ();
	stone_color to_move = st->to_move ();
	m_eval_game = gr;
	m_eval_state = gr->create_game_state (b, to_move);
}

void GTP_Eval_Controller::setup_for_analysis (go_game_ptr gr, game_state *st, bool flip)
{
	initiate_switch ();

	set_analysis_state (gr, st);

	m_analyzer->set_rules (m_rules);
	m_analyzer->setup_board (st, gr->info ().komi, flip);
}

void GTP_Eval_Controller::request_analysis (go_game_ptr gr, game_state *st, bool flip)
{
	bool was_paused = m_pause_eval;
	m_pause_eval = false;
	if (was_paused)
		analyzer_state_changed ();

	if (analyzer_state () != analyzer::running)
		return;

	setup_for_analysis (gr, st, flip);

	stone_color to_move = st->to_move ();
	if (flip)
		to_move = flip_color (to_move);

	m_last_request_flipped = flip;

	m_analyzer->analyze (to_move, 100);
}

void GTP_Eval_Controller::clear_eval_data ()
{
	if (m_eval_game != nullptr)
		m_eval_game->release_game_state (m_eval_state);
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

void GTP_Eval_Controller::start_analyzer (const Engine &engine, int size, double komi, bool show_dialog)
{
	if (m_analyzer != nullptr) {
		m_analyzer->quit ();
		delete m_analyzer;
		m_analyzer = nullptr;
	}
	m_analyzer_komi = komi;
	m_analyzer = create_gtp (engine, size, komi, show_dialog);
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
bool GTP_Eval_Controller::pause_analyzer ()
{
	if (m_analyzer == nullptr || !m_analyzer->started () || m_analyzer->stopped ())
		return false;
	if (m_pause_eval)
		return false;
	m_pause_eval = true;
	clear_eval_data ();
	m_analyzer->pause_analysis ();
	analyzer_state_changed ();
	return true;
}

void GTP_Eval_Controller::gtp_eval (GTP_Process *p, const QString &s, bool kata_format)
{
	if (m_pause_updates || m_pause_eval || m_switch_pending)
		return;

	bool prune = setting->readBoolEntry("ANALYSIS_PRUNE");

	QStringList moves = s.split ("info move ", SPLIT::SkipEmptyParts);
	if (moves.isEmpty ())
		return;

	stone_color to_move = m_eval_state->to_move ();

	int an_maxmoves = setting->readIntEntry ("ANALYSIS_MAXMOVES");
	int count = 0;
	m_primary_eval = 0.5;
	QString primary_move;
	int primary_visits = 0;

	bool flip = m_last_request_flipped;

	analyzer_id id = m_id_map[p];
	if (flip)
		id.komi = -id.komi;

	m_eval_game->release_state_children (m_eval_state);

	bool found_score = false;
	for (auto &e: moves) {
		QRegularExpression mvre ("^(\\S+)\\s+");
		auto mv = mvre.match (e);
		if (!mv.hasMatch ())
			continue;
		QRegularExpression visre ("\\s+visits\\s+(\\d+)\\s+");
		auto vism = visre.match (e);
		if (!vism.hasMatch ())
			continue;
		QRegularExpression wrre ("\\s+winrate\\s+([^\\s]+)\\s+");
		auto wrm = wrre.match (e);
		if (!wrm.hasMatch ())
			continue;
		QRegularExpression pvre ("\\s+pv\\s+(.*)$");
		auto pvm = pvre.match (e);
		if (!pvm.hasMatch ())
			continue;
		QRegularExpression scoremre ("\\s+scoreMean\\s+([^\\s]+)\\s+");
		auto scoremm = scoremre.match (e);
		QRegularExpression scoredre ("\\s+scoreStdev\\s+([^\\s]+)\\s+");
		auto scoredm = scoredre.match (e);
		bool have_score = scoremm.hasMatch () && scoredm.hasMatch ();

		QString move = mv.captured (1);
		int visits = vism.captured (1).toInt ();
		double wr = kata_format ? wrm.captured (1).toFloat () : wrm.captured (1).toInt () / 10000.;
		QString pv = pvm.captured (1);
		double scorem = have_score ? scoremm.captured (1).toFloat () : 0;
		double scored = have_score ? scoredm.captured (1).toFloat () : 0;
		if (have_score && to_move == white)
			scorem = -scorem;
		found_score |= have_score;

		/* The winrate also does not need flipping, it is given for the side to move,
		   and since we flip both the stones and the side to move, it comes out correct
		   in both cases.  */
		if (count == 0) {
			primary_move = move;
			m_primary_eval = wr;
			m_primary_have_score = have_score;
			m_primary_score = scorem;
			primary_visits = visits;
			m_eval_state->set_eval_data (visits, to_move == white ? 1 - wr : wr,
						     scorem, scored, id);
		}
#if 0
		qDebug () << move << " wr " << wr << " visits " << visits << " PV: " << pv;
#endif
		QStringList pvmoves = pv.split (" ", SPLIT::SkipEmptyParts);
		if (count < 52 && (!prune || pvmoves.length () > 1 || visits >= 2)) {
			game_state *cur = m_eval_state;
			bool pv_first = true;
			for (auto &pm: pvmoves) {
				int szx = m_eval_state->get_board ().size_x ();
				int szy = m_eval_state->get_board ().size_y ();

				auto coords = string_to_coords (pm, szy);
				int i = coords.first;
				int j = coords.second;

				if (i >= 0 && i < szx && j >= 0 && j < szy) {
					game_state *next = cur->add_child_move (i, j);
					/* The program might have given us an invalid move.  Don't
					   crash if it did.  */
					if (next == nullptr)
						break;
					if (pv_first) {
						cur->set_mark (i, j, mark::letter, count);
						next->set_eval_data (visits, to_move == white ? 1 - wr : wr,
								     scorem, scored, id);
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
	notice_analyzer_id (id, found_score);

	if (!primary_move.isNull ())
		eval_received (id, primary_move, primary_visits, found_score);
}
