/*
 *   tables.cpp = part of mainwindow
 */
#include "common.h"
#include <math.h>

#include "tables.h"
#include "clientwin.h"
#include "misc.h"
#include "playertable.h"
#include "gamestable.h"
#include "gs_globals.h"

#include "ui_clientwindow_gui.h"
#include "ui_talk_gui.h"

template<class T>
void table_model<T>::reset ()
{
	beginResetModel ();
	m_entries.clear ();
	endResetModel ();
}

template<class T>
QVariant table_model<T>::data (const QModelIndex &index, int role) const
{
	int row = index.row ();
	int col = index.column ();

	if (row < 0)
		return QVariant ();

	size_t r = row;
	if (r >= m_entries.size ())
		return QVariant ();

	const T &e = m_entries[r];
	if (role == Qt::ForegroundRole)
		return e.foreground ();
	if (role == Qt::TextAlignmentRole)
		return T::justify_right (col) ? Qt::AlignRight : Qt::AlignLeft;
	if (role != Qt::DisplayRole)
		return QVariant ();
	return e.column (col);
}

template<class T>
const T *table_model<T>::find (const QModelIndex &index) const
{
	int row = index.row ();
	if (row < 0)
		return nullptr;

	size_t r = row;
	if (r >= m_entries.size ())
		return nullptr;
	return &m_entries[r];
}

template<class T>
bool table_model<T>::removeRows (int row, int count, const QModelIndex &parent)
{
	if (row < 0 || count < 1 || (size_t) (row + count) > m_entries.size ())
		return false;
	beginRemoveRows (parent, row, row + count - 1);
	m_entries.erase (m_entries.begin () + row, m_entries.begin () + (row + count));
	endRemoveRows ();
	return true;
}


template<class T>
int table_model<T>::sort_compare (const T &a, const T &b)
{
	int r = a.compare (b, m_column);
	if (r == 0)
		r = a.unique_column ().compare (b.unique_column ());
	if (m_order == Qt::AscendingOrder)
		return r;
	else
		return -r;
}

template<class T>
void table_model<T>::sort (int column, Qt::SortOrder o)
{
	m_column = column;
	m_order = o;
	beginResetModel ();
	std::sort (std::begin (m_entries), std::end (m_entries),
		   [this] (const T &a, const T &b) { return sort_compare (a, b) < 0; });
	endResetModel ();
}

template<class T>
void table_model<T>::update_from_map (const std::unordered_map<QString, T> &m)
{
	std::vector<T> new_entries;
	for (auto &it: m)
		if (!m_filter (it.second))
			new_entries.push_back (it.second);
	std::sort (std::begin (new_entries), std::end (new_entries),
		   [this] (const T &a, const T &b) { return sort_compare (a, b) < 0; });

	auto insert_at = [this, &new_entries] (size_t orig_oldp, size_t orig_newp, size_t newp)
		{
			// qDebug () << QString ("inserting %1 rows at %2\n").arg (newp - orig_newp).arg (orig_oldp);
			auto nbeg = std::begin (new_entries);
			auto obeg = std::begin (m_entries);
			beginInsertRows (QModelIndex (), orig_oldp, orig_oldp + newp - orig_newp - 1);
			m_entries.insert (obeg + orig_oldp, nbeg + orig_newp, nbeg + newp);
			endInsertRows ();
		};

	size_t oldp = 0;
	size_t newp = 0;
	size_t oldn = m_entries.size ();
	size_t newn = new_entries.size ();

	while (oldp < oldn && newp < newn) {
		const T &a = m_entries[oldp];
		const T &b = new_entries[newp];
		int c = sort_compare (a, b);
		size_t orig_oldp = oldp;
		size_t orig_newp = newp;
		if (c == 0 && a == b) {
			newp++;
			oldp++;
			continue;
		}
		if (c <= 0)
			oldp++;
		if (c >= 0)
			newp++;
		while (oldp < oldn && newp < newn) {
			const T &a1 = m_entries[oldp];
			const T &b1 = new_entries[newp];
			if (sort_compare (a1, b1) != c || (c == 0 && a1 == b1))
				break;
			if (c <= 0)
				oldp++;
			if (c >= 0)
				newp++;
		}
		if (c < 0) {
			// qDebug () << QString ("removing %1 rows at %2\n").arg (oldp - orig_oldp).arg (oldp);
			removeRows (orig_oldp, oldp - orig_oldp);
			oldn -= oldp - orig_oldp;
			oldp = orig_oldp;
		} else if (c > 0) {
			insert_at (orig_oldp, orig_newp, newp);
			oldp += newp - orig_newp;
			oldn += newp - orig_newp;
		} else {
			// qDebug () << QString ("data changed for %1 items at %2\n").arg (oldp - orig_oldp).arg (orig_oldp);
			while (orig_oldp < oldp)
				m_entries[orig_oldp++] = new_entries[orig_newp++];
			emit dataChanged (index (orig_oldp, 0), index (oldp - 1, 0));
		}
	}
	if (newp < newn) {
		// qDebug () << "insert at end\n";
		insert_at (oldp, newp, newn);
	} else if (oldp < oldn) {
		// qDebug () << QString ("removing %1 rows at end\n").arg (oldn - oldp);
		removeRows (oldp, oldn - oldp);
	}
}

template<class T>
QModelIndex table_model<T>::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

template<class T>
QModelIndex table_model<T>::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

template<class T>
int table_model<T>::rowCount (const QModelIndex &) const
{
	return m_entries.size ();
}

template<class T>
int table_model<T>::columnCount (const QModelIndex &) const
{
	return T::column_count ();
}

template<class T>
QVariant table_model<T>::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	return T::header_column (section);
}


void ClientWindow::update_player_stats ()
{
	int num_players = m_player_table.size ();
	statusUsers->setText (" P: " + QString::number (num_players) + " / " + QString::number (num_watchedplayers) + " ");
}

void ClientWindow::update_game_stats ()
{
	int num_games = m_games_table.size ();
	statusGames->setText(" G: " + QString::number(num_games) + " / " + QString::number(num_observedgames) + " ");
}

void ClientWindow::update_observed_games (int count)
{
	num_observedgames = count;
	update_game_stats ();
}

void ClientWindow::prepare_game_list ()
{
	m_games_table.clear ();
}

void ClientWindow::prepare_player_list ()
{
	m_player_table.clear ();
}

void ClientWindow::finish_game_list ()
{
	m_games_init_complete = true;
	m_games_model.update_from_map (m_games_table);
	update_game_stats ();
	ui->ListView_games->resize_columns ();
}

void ClientWindow::finish_player_list ()
{
	m_player_init_complete = true;
	m_player_model.update_from_map (m_player_table);
	update_player_stats ();
	ui->ListView_players->resize_columns ();
}

void ClientWindow::prepare_channels ()
{
	channellist.clear ();
	statusChannel->setText ("");
	statusChannel->setToolTip (tr ("Current channels and users"));
}

// return the rank of a given name
QString ClientWindow::getPlayerRk (QString player)
{
	auto it = m_player_table.find (player);
	if (it != std::end (m_player_table)) {
		return it->second.rank;
	}
	return QString ();
}

void ClientWindow::update_tables ()
{
	if (!m_player7_active) {
		m_games_model.update_from_map (m_games_table);
		update_game_stats ();
	}
	if (!m_playerlist_active) {
		m_player_model.update_from_map (m_player_table);
		update_player_stats ();
	}
}

void ClientWindow::server_add_game (const Game &g_in)
{
	Game g = g_in;

	// skip individual games until initial table has loaded
	if (g.H.isEmpty() && !m_games_init_complete)
		return;

	QString excludeMark = "";
	QString myMark = "B";

	// check if exclude entry is done later
	if (!g.H.isEmpty()) //g.status.length() > 1)
	{
		QString emw;
		QString emb;

		// no: do it now
		emw = exclude.contains(";" + g.wname + ";");
		emb = exclude.contains(";" + g.bname + ";");

		// ensure that my game is listed first
		if (emw == "M" || emb == "M")
		{
			myMark = "A";
			excludeMark = "M";

			// I'm playing, thus I'm open, except teaching games
			if (emw != "M" || emb != "M")
				set_open_mode (true);
		}
		else if (emw == "W" || emb == "W")
		{
			excludeMark = "W";
		}
	}

	// update player info if this is not a 'who'-result or if it's me
	if (g.H.isEmpty() || myMark == "A") //g.status.length() < 2)
	{
		auto w_it = m_player_table.find (g.wname);
		auto b_it = m_player_table.find (g.bname);
		if (w_it != std::end (m_player_table)) {
			w_it->second.play_str = g.nr;
			if (g.wrank == "??")
				g.wrank = w_it->second.rank;
		}
		if (b_it != std::end (m_player_table)) {
			b_it->second.play_str = g.nr;
			if (g.brank == "??")
				g.brank = b_it->second.rank;
		}
	}
	QString rkw = myMark + rkToKey (g.wrank) + g.wname.toLower () + ":" + excludeMark;
	QString rkb = myMark + rkToKey (g.brank) + g.bname.toLower () + ":" + excludeMark;
	g.sort_rk_w = rkw;
	g.sort_rk_b = rkb;

	m_games_table[g.nr] = g;
}

void ClientWindow::server_remove_game (const QString &nr)
{
	bool found = false;
	QString game_id;

	if (nr != "@") {
		game_id = nr;
		auto it = m_games_table.find (nr);
		if (it != std::end (m_games_table)) {
			m_games_table.erase (it);
			found = true;
		}
	} else {
		auto beg = std::begin (m_games_table);
		auto end = std::end (m_games_table);
		for (auto it = beg; it != end; it++) {
			if (it->second.wname == m_online_acc_name || it->second.bname == m_online_acc_name) {
				game_id = it->second.nr;
				m_games_table.erase (it);
				found = true;
				break;
			}
		}
	}

	if (!found) {
		qWarning () << "game not found " << nr;
		return;
	}

	for (auto &pl: m_player_table) {
		if (pl.second.playing == game_id)
			pl.second.playing = "-";
		if (pl.second.observing == game_id)
			pl.second.observing = "-";
	}
}

// take a new player from parser
void ClientWindow::server_remove_player (const QString &name)
{
	auto it = m_player_table.find (name);
	if (it != std::end (m_player_table)) {
		if (it->second.mark == "W") {
			qgo->playLeaveSound();
			num_watchedplayers--;
		}
		m_player_table.erase (it);
	} else
		qWarning() << "disconnected player not found: " << name;
}

void ClientWindow::update_olq_state_from_player_info (const Player &p)
{
	/* ??? This is legacy code that doesn't make a whole amount of sense.
	   Should figure out if any of this is actually necessary (and correct) or not.  */

	qDebug() << "updating my account info...";
	// checkbox open
	bool b = (p.info.contains('X') == 0);
	set_open_mode (b);
	// checkbox looking - don't set if closed
	if (p.info.contains('!') != 0)
		// "!" found
		set_looking_mode (true);
	else if (b)
		// "!" not found && open
		set_looking_mode (false);
	// checkbox quiet
	// NOT CORRECT REPORTED BY SERVER!
	//b = (p.info.contains('Q') != 0);
	//set_quiet_mode (b);
	// -> WORKAROUND
	if (p.info.contains('Q') != 0)
		set_quiet_mode (true);

	// get rank to calc handicap when matching
	m_online_rank = p.rank;
}

// take a new player from parser
void ClientWindow::server_add_player (const Player &p_in, bool cmdplayers)
{
	Player p = p_in;

	// skip individual players until initial table has loaded
	if (!cmdplayers && !m_player_init_complete)
		return;

	if (cmdplayers)
	{
		auto old = m_player_table.find (p_in.name);
		if (old != std::end (m_player_table)) {
			// check if new player info is less than old
			if (p.info != "??")
			{
				// new entry has more info
				p.mark = old->second.mark;
				p.sort_rk = old->second.sort_rk;
			}
			old->second = p;
			if (p.name == m_online_acc_name)
				update_olq_state_from_player_info (p);
			return;
		}
	}

	QString mark;

	// check for watched players
	if (watch.contains(";" + p.name + ";"))
	{
		mark = "W";

		// sound for entering - no sound while "who" cmd is executing
		if (!cmdplayers)
			qgo->playEnterSound();
		else if (p.name == m_online_acc_name)
			// it's me
			// - only possible if 'who'/'user' cmd is executing
			// - I am on the watchlist, however
			// don't count!
			num_watchedplayers--;

		num_watchedplayers++;
	}
	// check for excluded players
	else if (exclude.contains(";" + p.name + ";"))
	{
		mark = "X";
	}

	// check for open/looking state
	if (cmdplayers && p.name == m_online_acc_name) {
		update_olq_state_from_player_info (p);
		mark = "M";
	}
	p.mark = mark;
	p.sort_rk = rkToKey (p.rank) + p.name.toLower ();

	m_player_table[p.name] = p;
}

void ClientWindow::update_who_rank (QComboBox *box, int idx)
{
	QString rk;
	/* Index 0 is an empty string, which means no limit.  */
	if (idx == 0)
		rk = rkToKey (box == ui->whoBox1 ? "BC" : "9p");
	/* Index 1 is "1p to 9p".  */
	else if (idx == 1 && box == ui->whoBox1)
		rk = rkToKey ("1p");
	else
		rk = rkToKey (box->currentText ());
	if (box == ui->whoBox1)
		m_who1_rk = rk;
	else
		m_who2_rk = rk;

	if (m_who1_rk < m_who2_rk) {
		if (box == ui->whoBox1)
			ui->whoBox2->setCurrentIndex (idx);
		else
			ui->whoBox1->setCurrentIndex (idx);
	}
	qDebug () << "who ranks: " << m_who1_rk << m_who2_rk;
	update_tables ();
}

bool ClientWindow::filter_game (const Game &)
{
	return false;
}

bool ClientWindow::filter_player (const Player &p)
{
	if (m_online_server == IGS && m_whoopen) {
		if (p.info.contains ('X') || !p.play_str.contains ('-'))
			return true;
	}
	/* Rank comparisons are inverted: pro rank keys start with "a",
	   dan ranks with "b", kyu with "c".  */
	if (p.sort_rk > m_who1_rk || p.sort_rk < m_who2_rk)
		return true;
	return false;
}

// get channelinfo: ch nr + people
void ClientWindow::slot_channelinfo(int nr, const QString &txt)
{
	qDebug() << "slot_channelinfo(): " << txt;
	QString tipstring("");
	Channel *ch = 0;

	// check if entering a channel
	if (txt == QString("*on*"))
	{
		switch (m_online_server)
		{
			case IGS:
				sendcommand("channel");
				break;

			default:
				sendcommand("inchannel " + QString::number(nr), false);
				break;
		}
		return;
	}
	else if (txt == QString("*off*"))
	{
		// check if channel exists in list
		for (auto h: channellist) {
			// compare numbers
			if (h->get_nr() == nr)
			{
				channellist.removeOne(h);
				break;
			}
		}
	}

	// check if I'm in given channel
	bool flag_add;
	if (txt.contains (m_online_acc_name) && !txt.contains ("Topic:") && !txt.contains ("Title:"))
		flag_add = true;
	else
		flag_add = false;

	// delete string
	statusChannel->setText("");

	// check if channel exists in list
	bool found = false;
	for (auto h: channellist) {
		// compare numbers
		if (h->get_nr() == nr)
		{
			found = true;
			ch = h;
			break;
		}
	}

	// if not found insert channel at it's sorted position
	if (!found)
	{
		// init channel
		ch = new Channel(nr);
		channellist.append (ch);
		std::sort (channellist.begin (), channellist.end (),
			   [] (Channel *a, Channel *b) { return *a < *b; });
	}

	// now insert channel to list
	if (flag_add)
	{
		QString text = txt.simplified();
		int count = text.count(" ") + 1;
		// set user list and user count
		ch->set_channel(nr, QString(), txt, count);
	}
	else if (txt.contains("Topic:") || txt.contains("Title:"))
	{
		// set channel's title
		ch->set_channel(nr, txt);
	}

	// set new tooltip
	for (auto h: channellist) {
		if (h->get_users().length() > 2)
		{
			// check if users are available; skipped if only title
			if (!tipstring.isEmpty())
				tipstring += "\n";
			tipstring += QString("%1: %2\n%3: %4").arg(h->get_nr()).arg(h->get_title()).arg(h->get_nr()).arg(h->get_users());
			if (statusChannel->text().length() > 2)
				statusChannel->setText(QString("%1 / CH%2:%3").arg(statusChannel->text()).arg(h->get_nr()).arg(h->get_count()));
			else
				statusChannel->setText(QString("CH%1:%2").arg(h->get_nr()).arg(h->get_count()));
		}
	}

	statusChannel->setToolTip (tipstring);
}

// tell, say, kibitz...
void ClientWindow::colored_message(QString txt, QColor c)
{
	QColor oldc = ui->MultiLineEdit2->textColor ();
	ui->MultiLineEdit2->setTextColor (c);
	slot_message (txt);

	ui->MultiLineEdit2->setTextColor (oldc);
}


void ClientWindow::slot_message(QString txt)
{
	// Scroll at bottom of text, set cursor to end of line
	if (ui->MultiLineEdit2->toPlainText().endsWith("\n") && txt == "\n")
		return ;

	ui->MultiLineEdit2->append(txt);
}

// shout...
void ClientWindow::slot_shout (const QString &player, const QString &txt)
{
	if (exclude.contains (";" + player + ";"))
		return;

	// check if send to a special handle:
	if (player.length () && player.contains ('*'))
		slot_talk (player, txt, true, false);
	else
		slot_talk ("Shouts*", txt, true, false);
}

// server name found
void ClientWindow::slot_svname (GSName &gs)
{
	m_online_server = gs;
	switch (gs)
	{
	case IGS: m_online_server_name = "IGS"; break;
	case NNGS: m_online_server_name = "NNGS"; break;
	case LGS: m_online_server_name = "LGS"; break;
	case WING: m_online_server_name = "WING"; break;
	default: m_online_server_name = tr ("Unknown server"); break;
	}

	if (m_online_status == Status::offline)
		m_online_status = Status::registered;

	update_caption ();
}

// account name found
void ClientWindow::slot_accname (QString &name)
{
	m_online_acc_name = name;
	update_caption ();
}

// status found
void ClientWindow::slot_status (Status s)
{
	m_online_status = s;
	update_caption ();
}

/*
 *   Talk - Class to handle  Talk Dialog Windows
 */

Talk::Talk (const QString &playername, QWidget *parent, bool isplayer)
	: QDialog (parent), ui (std::make_unique<Ui::TalkGui> ())
{
	ui->setupUi (this);
	setWindowTitle (playername);
	m_name = playername;

	// create a new tab
	ui->MultiLineEdit1->setCurrentFont(setting->fontComments);
	ui->LineEdit1->setFont(setting->fontComments);

	// do not add a button for shouts* or channels tab
	if (m_name.indexOf ('*') != -1 || !isplayer)
	{
		delete ui->pb_releaseTalkTab;
		delete ui->pb_match;
		delete ui->stats_layout;
	} else {
		connect (ui->pb_releaseTalkTab, &QPushButton::pressed, [this] () { emit signal_pbRelOneTab (this); });
		connect (ui->pb_match, &QPushButton::pressed, this, &Talk::slot_match);
	}
	connect (ui->LineEdit1, &QLineEdit::returnPressed, this, &Talk::slot_returnPressed);
	m_default_text_color = ui->MultiLineEdit1->textColor ();
}

Talk::~Talk ()
{
	// Empty destructor here, since Ui::TalkGui is incomplete in the header file.
}

bool Talk::lineedit_has_focus ()
{
	return ui->LineEdit1->hasFocus ();
}

void Talk::slot_returnPressed ()
{
	// read tab
	QString txt = ui->LineEdit1->text ();
	emit signal_talkto (m_name, txt);
	ui->LineEdit1->clear ();
	ui->LineEdit1->setFocus ();
}

void Talk::slot_match ()
{
	QString txt = m_name + " " + ui->stats_rating->text ();
	client_window->handle_matchrequest (txt, true, true);
}

// write to txt field in dialog
// if null string -> check edit field
void Talk::write (const QString &text, QColor col)
{
	QString txt;

	// check which text to display
	if (!text.isEmpty())
		// ok, text given
		txt = text;
	else {
		txt = ui->LineEdit1->text();

		if (txt.isEmpty())
			return;
		ui->LineEdit1->clear();
	}

	if (col.isValid ())
		ui->MultiLineEdit1->setTextColor (col);
	else
		ui->MultiLineEdit1->setTextColor (m_default_text_color);
	ui->MultiLineEdit1->append (txt);
}

void Talk::set_player (const Player &p)
{
	ui->stats_rating->setText (p.rank);
	ui->stats_info->setText (p.info);
	ui->stats_default->setText (p.extInfo);
	ui->stats_wins->setText (p.won);
	ui->stats_loss->setText (p.lost);
	ui->stats_country->setText (p.country);
	ui->stats_playing->setText (p.play_str);
	ui->stats_rated->setText (p.rated);
	ui->stats_address->setText (p.address);

	// stored either idle time or last access
	ui->stats_idle->setText (p.idle);
	ui->Label_Idle->setText (!p.idle.isEmpty() && p.idle.at(0).isDigit() ? "Idle :": "Last log :");
}

template class table_model<Game>;
template class table_model<Player>;
