/*
 *   tables.cpp = part of mainwindow
 */
#include "tables.h"
#include "clientwin.h"
#include "misc.h"
#include "playertable.h"
#include "gamestable.h"
#include "gs_globals.h"
#include <math.h>

void ClientWindow::update_player_stats ()
{
	statusUsers->setText (" P: " + QString::number (num_players) + " / " + QString::number (num_watchedplayers) + " ");
}

void ClientWindow::update_game_stats ()
{
	statusGames->setText(" G: " + QString::number(num_games) + " / " + QString::number(num_observedgames) + " ");
}

void ClientWindow::update_observed_games (int count)
{
	num_observedgames = count;
	update_game_stats ();
}

void ClientWindow::prepare_game_list ()
{
	QTreeWidgetItemIterator lvii (ListView_games);
	for (GamesTableItem *lvi; (lvi = static_cast<GamesTableItem*>(*lvii));) {
		lvii++;
		lvi->clear_up_to_date ();
	}
}

void ClientWindow::prepare_player_list ()
{
	QTreeWidgetItemIterator lvii (ListView_players);
	for (PlayerTableItem *lvi; (lvi = static_cast<PlayerTableItem*>(*lvii));) {
		lvii++;
		lvi->clear_up_to_date ();
	}
}

void ClientWindow::finish_game_list ()
{
	Update_Locker l (ListView_games);
	QTreeWidgetItemIterator lvii (ListView_games);
	for (GamesTableItem *lvi; (lvi = static_cast<GamesTableItem*>(*lvii));) {
		lvii++;
		if (!lvi->is_up_to_date ()) {
			num_games--;
			delete lvi;
		}
	}
	update_game_stats ();
}

void ClientWindow::finish_player_list ()
{
	Update_Locker l (ListView_players);
	QTreeWidgetItemIterator lvii (ListView_players);
	for (PlayerTableItem *lvi; (lvi = static_cast<PlayerTableItem*>(*lvii));) {
		lvii++;
		if (!lvi->is_up_to_date ()) {
			num_players--;
			delete lvi;
		}
	}
	update_player_stats ();
}

void ClientWindow::prepare_channels ()
{
	channellist.clear ();
	statusChannel->setText ("");
	statusChannel->setToolTip (tr ("Current channels and users"));
}

// return the rank of a given name
QString ClientWindow::getPlayerRk(QString player)
{
	QTreeWidgetItemIterator lvp(ListView_players);
	QTreeWidgetItem *lvpi;

	// look for players in playerlist
	for (; (lvpi = *lvp); lvp++)
	{
		// check if names are identical
		if (lvpi->text(1) == player)
		{
			return lvpi->text(2);
		}
	}

	return QString::null;
}

// check for exclude list entry of a given name
QString ClientWindow::getPlayerExcludeListEntry(QString player)
{
	QTreeWidgetItemIterator lvp(ListView_players);
	QTreeWidgetItem *lvpi;

	// look for players in playerlist
	for (; (lvpi = *lvp); lvp++)
		if (lvpi->text(1) == player)
			return lvpi->text(6);

	return QString::null;
}

void ClientWindow::server_add_game (Game* g)
{
	// insert into ListView
	QTreeWidgetItemIterator lv (ListView_games);

	bool found = false;
	GamesTableItem *lvi_mem = nullptr;

	if (g->H.isEmpty() && !num_games)
	{
		// skip games until initial table has loaded
		qDebug() << "game skipped because no init table";
		return;
	}

	// check if game already exists
	QTreeWidgetItemIterator lvii = lv;
	for (GamesTableItem *lvi; (lvi = static_cast<GamesTableItem*>(*lvii)) && !found;)
	{
		lvii++;
		// compare game id
		if (lvi->text(0) == g->nr)
		{
			found = true;
			lvi_mem = lvi;
		}
	}

	QString excludeMark = "";
	QString myMark = "B";

	// check if exclude entry is done later
	if (!g->H.isEmpty()) //g->status.length() > 1)
	{
		QString emw;
		QString emb;

		// no: do it now
		emw = getPlayerExcludeListEntry(g->wname);
		emb = getPlayerExcludeListEntry(g->bname);

		// ensure that my game is listed first
		if (emw == "M" || emb == "M")
		{
			myMark = "A";
			excludeMark = "M";

			// I'm playing, thus I'm open, except teaching games
			if (emw != "M" || emb != "M")
			{
				// checkbox open
				slot_checkbox(0, true);
			}
		}
		else if (emw == "W" || emb == "W")
		{
			excludeMark = "W";
		}
	}

	// update player info if this is not a 'who'-result or if it's me
	if (g->H.isEmpty() || myMark == "A") //g->status.length() < 2)
	{
		QTreeWidgetItemIterator lvp(ListView_players);
		PlayerTableItem *lvpi;
		int pl_found = 0;

		// look for players in playerlist
		for (; (lvpi = static_cast<PlayerTableItem *>(*lvp)) && pl_found < 2;) {
			// check if names are identical
			if (lvpi->text(1) == g->wname || lvpi->text(1) == g->bname) {
				pl_found++;
				Player pl = lvpi->get_player ();
				pl.play_str = g->nr;
				lvpi->update_player (pl);

				// check if players has a rank
				if (g->wrank == "??" || g->brank == "??")
				{
					// no rank given in case of continued game -> set rank in games table
					if (lvpi->text(1) == g->wname)
						g->wrank = lvpi->text(2);

					// no else case! bplayer could be identical to wplayer!
					if (lvpi->text(1) == g->bname)
						g->brank = lvpi->text(2);
				}
			}

			lvp++;
		}

//			ListView_games->sortItems (3, Qt::AscendingOrder);
	}
	QString rkw = myMark + rkToKey(g->wrank) + g->wname.toLower() + ":" + excludeMark;
	QString rkb = myMark + rkToKey(g->brank) + g->bname.toLower() + ":" + excludeMark;
	g->sort_rk_w = rkw;
	g->sort_rk_b = rkb;
	if (found) {
		lvi_mem->update_game (*g);
	} else {
		// from GAMES command or game info{...}
		new GamesTableItem(ListView_games, *g);

		// increase number of games
		num_games++;
		update_game_stats ();
	}
}


void ClientWindow::server_remove_game (Game* g)
{
	QTreeWidgetItemIterator lv (ListView_games);

	// from game info {...}
	bool found = false;
	QString game_id;

	if (g->nr != "@") {
		for (QTreeWidgetItem *lvi; (lvi = *lv) && !found;)
		{
			lv++;
			// compare game id
			if (lvi->text(0) == g->nr)
			{
				lv++;
				delete lvi;
				found = true;
			}
		}

		// used for player update below
		game_id = g->nr;
	} else {
		for (QTreeWidgetItem *lvi; (lvi = *lv) && !found;) {
			lv++;
			// look for name
			if (lvi->text(1) == m_online_acc_name ||
			    lvi->text(3) == m_online_acc_name)
			{
				// used for player update below
				game_id = lvi->text(0);

				lv++;
				delete lvi;
				found = true;
			}
		}
	}

	if (!found)
		qWarning () << "game not found " << g->nr;
	else
	{
		// decrease number of games
		num_games--;
		update_game_stats ();

		QTreeWidgetItemIterator lvp(ListView_players);
		PlayerTableItem *lvpi;
		int pl_found = 0;

		// look for players in playerlist
		for (; (lvpi = static_cast<PlayerTableItem *>(*lvp)) && pl_found < 2;) {
			// check if numbers are identical
			if (lvpi->text(3) == game_id) {
				pl_found++;
				Player pl = lvpi->get_player ();
				pl.play_str = "-";
				lvpi->update_player (pl);
			}

			lvp++;
		}
	}
}

// take a new player from parser
void ClientWindow::server_remove_player (const QString &name)
{
  	QTreeWidgetItemIterator lv (ListView_players);
#if 0
	QPoint pp(0,0);
	QTreeWidgetItem *topViewItem = ListView_players->itemAt(pp);
  	bool deleted_topViewItem = false;
#endif

	bool found = false;
	for (QTreeWidgetItem *lvi; (lvi = *lv) && !found;)
	{
		lv++;
		// compare names
		if (lvi->text(1) == name)
		{
			// check if it was a watched player
			if (lvi->text(6) == "W")
			{
				qgo->playLeaveSound();
				num_watchedplayers--;
			}

			lv++;
#if 0
			if (lvi == topViewItem)     // are we trying to delete the 'anchor' of the list viewport ?
				deleted_topViewItem = true;
#endif
			delete lvi;
			found = true;;

			num_players--;
			update_player_stats ();
		}
	}

	if (!found)
		qWarning() << "disconnected player not found: " << name;

#if 0
	if (! deleted_topViewItem) //don't try to refer to a deleted element ...
	{
		int ip = topViewItem->itemPos();
		ListView_players->setContentsPos(0,ip);
	}
#endif
}

// take a new player from parser
void ClientWindow::server_add_player (Player *p, bool cmdplayers)
{
	QTreeWidgetItemIterator lv(ListView_players);

	if (!cmdplayers && !num_players) {
		qDebug() << "player skipped because no init table";
		// skip players until initial table has loaded
		return;
	}

	if (cmdplayers)
	{
		for (PlayerTableItem *lvi; (lvi = static_cast<PlayerTableItem*>(*lv));)
		{
			lv++;
			// compare names
			if (lvi->text(1) == p->name)
			{
				// check if new player info is less than old
				if (p->info != "??")
				{
					// new entry has more info
					p->mark = lvi->text (6);
					p->sort_rk = rkToKey(p->rank) + p->name.toLower();
					lvi->update_player (*p);

#if 0
					lvi->set_nmatchSettings(p);
					//lvi->nmatch = p->nmatch;

					lvi->ownRepaint();
#endif
				}

				if (p->name == m_online_acc_name)
				{
					qDebug() << "updating my account info... (1)";
					// checkbox open
					bool b = (p->info.contains('X') == 0);
					slot_checkbox(0, b);
					// checkbox looking - don't set if closed
					if (p->info.contains('!') != 0)
						// "!" found
						slot_checkbox(1, true);
					else if (b)
						// "!" not found && open
						slot_checkbox(1, false);
					// checkbox quiet
					// NOT CORRECT REPORTED BY SERVER!
					//b = (p->info.contains('Q') != 0);
					//slot_checkbox(2, b);
					// -> WORKAROUND
					if (p->info.contains('Q') != 0)
						slot_checkbox(2, true);

					// get rank to calc handicap when matching
					m_online_rank = p->rank;
				}

				return;
			}
		}
	}

	QString mark;

	// check for watched players
	if (watch.contains(";" + p->name + ";"))
	{
		mark = "W";

		// sound for entering - no sound while "who" cmd is executing
		if (!cmdplayers)
			qgo->playEnterSound();
		else if (p->name == m_online_acc_name)
			// it's me
			// - only possible if 'who'/'user' cmd is executing
			// - I am on the watchlist, however
			// don't count!
			num_watchedplayers--;

		num_watchedplayers++;
	}
	// check for excluded players
	else if (exclude.contains(";" + p->name + ";"))
	{
		mark = "X";
	}

	// check for open/looking state
	if (cmdplayers)
	{
		if (p->name == m_online_acc_name)
		{
			qDebug() << "updating my account info...(2)";
			// checkbox open
			bool b = (p->info.contains('X') == 0);
			slot_checkbox(0, b);
			// checkbox looking
			b = (p->info.contains('!') != 0);
			slot_checkbox(1, b);
			// checkbox quiet
			// NOT CORRECT REPORTED BY SERVER!
			//b = (p->info.contains('Q') != 0);
			//slot_checkbox(2, b);
			// -> WORKAROUND
			if (p->info.contains('Q') != 0)
				slot_checkbox(2, true);

			// get rank to calc handicap when matching
			m_online_rank = p->rank;
			mark = "M";
		}
	}
	p->mark = mark;
	p->sort_rk = rkToKey(p->rank) + p->name.toLower();
	new PlayerTableItem(ListView_players, *p);
#if 0
	lv1->set_nmatchSettings(p);
#endif
	// increase number of players
	num_players++;
	update_player_stats ();

	//if (!cmdplayers)
	//	ListView_players->sort() ;
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
	MultiLineEdit2->setTextColor(c);
	slot_message( txt);

	MultiLineEdit2->setTextColor(Qt::black);
}


void ClientWindow::slot_message(QString txt)
{
	// Scroll at bottom of text, set cursor to end of line
	if (MultiLineEdit2->toPlainText().endsWith("\n") && txt == "\n")
		return ;

	MultiLineEdit2->append(txt);
}

// shout...
void ClientWindow::slot_shout(const QString &player, const QString &txt)
{
	if (getPlayerExcludeListEntry(player) == "X")
		return;

	// check if send to a special handle:
	if (player.length() && player.contains('*'))
		slot_talk(player, txt, true);
	else
		slot_talk("Shouts*", txt, true);
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
	: QDialog (parent)
{
	setupUi (this);
	setWindowTitle (playername);
	m_name = playername;

	// create a new tab
	MultiLineEdit1->setCurrentFont(setting->fontComments);
	LineEdit1->setFont(setting->fontComments);

	// do not add a button for shouts* or channels tab
	if (m_name.indexOf ('*') != -1 || !isplayer)
	{
		delete pb_releaseTalkTab;
		delete pb_match;
		delete stats_layout;
	} else {
		connect (pb_releaseTalkTab, &QPushButton::pressed, [this] () { emit signal_pbRelOneTab (this); });
		connect (pb_match, &QPushButton::pressed, this, &Talk::slot_match);
	}
	connect (LineEdit1, &QLineEdit::returnPressed, this, &Talk::slot_returnPressed);
}

bool Talk::lineedit_has_focus ()
{
	return LineEdit1->hasFocus ();
}

void Talk::append_to_mle (const QString &s)
{
	MultiLineEdit1->append (s);
}

void Talk::slot_returnPressed ()
{
	// read tab
	QString txt = LineEdit1->text ();
	emit signal_talkto (m_name, txt);
	LineEdit1->clear ();
}

void Talk::slot_match ()
{
	QString txt = m_name + " " + stats_rating->text ();
	client_window->handle_matchrequest (txt, true, true);
}

// write to txt field in dialog
// if null string -> check edit field
void Talk::write (const QString &text) const
{
	QString txt;

	// check which text to display
	if (!text.isEmpty())
		// ok, text given
		txt = text;
	else {
		txt = LineEdit1->text();

		if (txt.isEmpty())
			return;
		LineEdit1->clear();
	}

	// Scroll at bottom of text, set cursor to end of line
	MultiLineEdit1->append (txt); //eb16
}
