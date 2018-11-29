/*
 *   tables.cpp = part of mainwindow
 */
#include "tables.h"
#include "mainwin.h"
#include "maintable.h"
#include "playertable.h"
#include "gamestable.h"
#include "gs_globals.h"
#include <math.h>

// prepare tables (clear, ...)
void ClientWindow::prepare_tables(InfoType cmd)
{
	switch (cmd)
	{
		case WHO: // delete player table
		{
			Q3ListViewItemIterator lv(ListView_players);
			for (Q3ListViewItem *lvi; (lvi = (*lv));)
			{
				lv++;
				delete lvi;
			}

			// set number of players to 0
			myAccount->num_players = 0;
			myAccount->num_watchedplayers = 0;
			// use this for fast filling
			playerListEmpty = true;
			statusUsers->setText(" P: 0 / 0 ");
			break;
		}

		case GAMES: // delete games table
		{
			Q3ListViewItemIterator lv(ListView_games);
			for (Q3ListViewItem *lvi; (lvi = (*lv));)
			{
				lv++;
				delete lvi;
			}

			// set number of games to 0
			myAccount->num_games = 0;
			statusGames->setText(" G: 0 / 0 ");
			break;
		}

		case CHANNELS:
		{
			// delete channel info
			channellist.clear();
			statusChannel->setText("");
/*			QListViewItemIterator lv(ListView_ch);
			for (QListViewItem *lvi; (lvi = (*lv));)
			{
				lv++;
				delete lvi;
			}*/
			// delete tooltips too
			QToolTip::remove(statusChannel);
			QToolTip::add(statusChannel, tr("Current channels and users"));
			break;
		}

		default: // unknown command???
			qWarning("unknown Command in 'prepare_tables()'");
			break;

	}
}

// return the rank of a given name
QString ClientWindow::getPlayerRk(QString player)
{
	Q3ListViewItemIterator lvp(ListView_players);
	Q3ListViewItem *lvpi;

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
	Q3ListViewItemIterator lvp(ListView_players);
	Q3ListViewItem *lvpi;

	if (DODEBUG)
		qDebug() << QString("getPlayerExcludeListEntry(%1)").arg(player);

	// look for players in playerlist
	for (; (lvpi = *lvp); lvp++)
	{
		// check if names are identical
		if (lvpi->text(1) == player)
		{
			if (DODEBUG)
				qDebug() << QString("text(1) = %1, player = %2").arg(lvpi->text(1)).arg(player);

			return lvpi->text(6);
		}
	}

	return QString::null;
}

// take a new game from parser
void ClientWindow::slot_game(Game* g)
{
	// insert into ListView
	Q3ListViewItemIterator lv(ListView_games);

	if (g->running)
	{
		bool found = false;
		GamesTableItem *lvi_mem = NULL;

		// check if game already exists
		if (!playerListEmpty)
		{
			Q3ListViewItemIterator lvii = lv;
			for (GamesTableItem *lvi; (lvi = static_cast<GamesTableItem*>(lvii.current())) && !found;)
			{
				lvii++;
				// compare game id
				if (lvi->text(0) == g->nr)
				{
					found = true;
					lvi_mem = lvi;
				}
			}
		}
		else if (g->H.isEmpty() && !myAccount->num_games)
		{
			// skip games until initial table has loaded
			qDebug() << "game skipped because no init table";
			return;
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
		
		if (found)
		{
			// supersede entry
			//lvi_mem->setText(0, g->nr);
			lvi_mem->setText(1, g->wname);
			lvi_mem->setText(2, g->wrank);
			lvi_mem->setText(3, g->bname);
			lvi_mem->setText(4, g->brank);
			lvi_mem->setText(5, g->mv);
			lvi_mem->setText(6, g->Sz);
			lvi_mem->setText(7, g->H);
			lvi_mem->setText(8, g->K);
			lvi_mem->setText(9, g->By);
			lvi_mem->setText(10, g->FR);
			lvi_mem->setText(11, g->ob);
//			lvi_mem->setText(6, g->status + " (" + g->ob + ")");
			lvi_mem->setText(12, myMark + rkToKey(g->wrank) + g->wname.lower() + ":" + excludeMark);

			lvi_mem->ownRepaint();
		}
		else
		{
			// from GAMES command or game info{...}

			if (!g->H.isEmpty())
			{
				lv = new GamesTableItem(ListView_games,
					g->nr,
					" " + g->wname,
					g->wrank,
					" " + g->bname,
					g->brank,
					g->mv,
					g->Sz,
					g->H,
					g->K,
					g->By,
					g->FR,
					g->ob);
			}
			else
			{
				lv = new GamesTableItem(ListView_games,
					g->nr,
					" " + g->wname,
					g->wrank,
					" " + g->bname,
					g->brank,
					g->mv,
					g->Sz);
			}

			(*lv)->setText(12, myMark + rkToKey(g->wrank) + g->wname.lower() + ":" + excludeMark);

			static_cast<GamesTableItem*>((*lv))->ownRepaint();

			// increase number of games
			myAccount->num_games++;
			statusGames->setText(" G: " + QString::number(myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");
		}

		// update player info if this is not a 'who'-result or if it's me
		if (g->H.isEmpty() || myMark == "A") //g->status.length() < 2)
		{
			Q3ListViewItemIterator lvp(ListView_players);
			Q3ListViewItem *lvpi;
			int found = 0;

			// look for players in playerlist
			for (; (lvpi = *lvp) && found < 2;)
			{
				// check if names are identical
				if (lvpi->text(1) == g->wname || lvpi->text(1) == g->bname)
				{
					lvpi->setText(3, g->nr);
					found++;

					// check if players has a rank
					if (g->wrank == "??" || g->brank == "??")
					{
						// no rank given in case of continued game -> set rank in games table
						if (lvpi->text(1) == g->wname)
						{
							(*lv)->setText(2, lvpi->text(2));
							// correct sorting of col 2 -> set col 12
							(*lv)->setText(12, myMark + rkToKey(lvpi->text(2)) + lvpi->text(1).lower() + ":" + excludeMark);
						}

						// no else case! bplayer could be identical to wplayer!
						if (lvpi->text(1) == g->bname)
							(*lv)->setText(4, lvpi->text(2));

						static_cast<GamesTableItem*>((*lv))->ownRepaint();
					}
				}

				lvp++;
			}

			ListView_games->sort();
		}
	}
	else
	{
		// from game info {...}
		bool found = false;
		QString game_id;

		if (g->nr != "@")
		{
			for (Q3ListViewItem *lvi; (lvi = (*lv)) && !found;)
			{
				lv++;
				// compare game id
				if (lvi->text(0) == g->nr)
				{
					lv++;
					delete lvi;
					found = true;;
				}
			}

			// used for player update below
			game_id = g->nr;
		}
		else
		{
			for (Q3ListViewItem *lvi; (lvi = (*lv)) && !found;)
			{
				lv++;
				// look for name
				if (lvi->text(1) == myAccount->acc_name ||
				    lvi->text(3) == myAccount->acc_name)
				{
					// used for player update below
					game_id = lvi->text(0);

					lv++;
					delete lvi;
					found = true;;
				}
			}
		}

		if (!found)
			qWarning("game not found");
		else
		{
			// decrease number of games
			myAccount->num_games--;
			statusGames->setText(" G: " + QString::number(myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");

			Q3ListViewItemIterator lvp(ListView_players);
			Q3ListViewItem *lvpi;
			int found = 0;

			// look for players in playerlist
			for (; (lvpi = *lvp) && found < 2;)
			{
				// check if numbers are identical
				if (lvpi->text(3) == game_id)
				{
					lvpi->setText(3, "-");
					found++;
				}

				lvp++;
			}
		}
	}
}

// take a new player from parser
void ClientWindow::slot_player(Player *p, bool cmdplayers)
{
	// insert into ListView

  	Q3ListViewItemIterator lv(ListView_players);  

	QPoint pp(0,0);
	Q3ListViewItem *topViewItem = ListView_players->itemAt(pp);
  	bool deleted_topViewItem = false;
  
	if (p->online)
	{
		// check if it's an empty list, i.e. all items deleted before
		if (cmdplayers && !playerListEmpty)
		{
			for (PlayerTableItem *lvi; (lvi = static_cast<PlayerTableItem*>((*lv)));)
			{
				lv++;
				// compare names
				if (lvi->text(1) == p->name)
				{
					// check if new player info is less than old
					if (p->info != "??")
					{
						// new entry has more info
						lvi->setText(0, p->info);
						//p->name,
						lvi->setText(2, p->rank);
						lvi->setText(3, p->play_str);
						lvi->setText(4, p->obs_str);
						lvi->setText(5, p->idle);
						//mark,
						lvi->setText(12, rkToKey(p->rank) + p->name.lower());

						if (extUserInfo && myAccount->get_gsname() == IGS)
						{
							lvi->setText(7, p->extInfo);
							lvi->setText(8, p->won);
							lvi->setText(9, p->lost);
							lvi->setText(10, p->country);
							lvi->setText(11, p->nmatch_settings);
						}
						lvi->set_nmatchSettings(p);					
						//lvi->nmatch = p->nmatch;

						lvi->ownRepaint();
					}

					if (p->name == myAccount->acc_name)
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
						myAccount->set_rank(p->rank);
					}

					return;
				}
			}
		}
		else if (!cmdplayers && !myAccount->num_players)
		{
			qDebug() << "player skipped because no init table";
			// skip players until initial table has loaded
			return;
		}


		QString mark;

		// check for watched players
		if (watch.contains(";" + p->name + ";"))
		{
			mark = "W";

			// sound for entering - no sound while "who" cmd is executing
			if (!cmdplayers)
				qgo->playEnterSound();
			else if (p->name == myAccount->acc_name)
				// it's me
				// - only possible if 'who'/'user' cmd is executing
				// - I am on the watchlist, however
				// don't count!
				myAccount->num_watchedplayers--;

			myAccount->num_watchedplayers++;
		}
		// check for excluded players
		else if (exclude.contains(";" + p->name + ";"))
		{
			mark = "X";
		}

		// check for open/looking state
		if (cmdplayers)
		{
			if (p->name == myAccount->acc_name)
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
				myAccount->set_rank(p->rank);
				mark = "M";
			}
		}


		// from WHO command or {... has connected}
		if (extUserInfo && myAccount->get_gsname() == IGS)
		{
			PlayerTableItem *lv1 = new PlayerTableItem(ListView_players,
					p->info,
					p->name,
					p->rank,
					p->play_str,
					p->obs_str,
					p->idle,
					mark,
					p->extInfo,
					p->won,
					p->lost,
					p->country,
					p->nmatch_settings);
			lv1->setText(12, rkToKey(p->rank) + p->name.lower());
			lv1->set_nmatchSettings(p);
		}
		else
		{
			PlayerTableItem *lv1 = new PlayerTableItem(ListView_players,
					p->info,
					p->name,
					p->rank,
					p->play_str,
					p->obs_str,
					p->idle,
					mark);
			lv1->setText(12, rkToKey(p->rank) + p->name.lower());
			lv1->set_nmatchSettings(p);
		}

		// increase number of players
		myAccount->num_players++;
		statusUsers->setText(" P: " + QString::number(myAccount->num_players) + " / " + QString::number(myAccount->num_watchedplayers) + " ");
		

		//if (!cmdplayers)
		//	ListView_players->sort() ;

	}
	else
	{
		// {... has disconnected}
		bool found = false;
		for (Q3ListViewItem *lvi; (lvi = (*lv)) && !found;)
		{
			lv++;
			// compare names
			if (lvi->text(1) == p->name)
			{
				// check if it was a watched player
				if (lvi->text(6) == "W")
				{
					qgo->playLeaveSound();
					myAccount->num_watchedplayers--;
				}

				lv++;
				if (lvi == topViewItem)     // are we trying to delete the 'anchor' of the list viewport ?
					deleted_topViewItem = true  ;
				delete lvi;
				found = true;;

				// decrease number of players
				myAccount->num_players--;
				statusUsers->setText(" P: " + QString::number(myAccount->num_players) + " / " + QString::number(myAccount->num_watchedplayers) + " ");
			}
		}

		if (!found)
			qWarning("disconnected player not found: " + p->name);
	}

	if (! deleted_topViewItem) //don't try to refer to a deleted element ...
	{
		int ip = topViewItem->itemPos();
		ListView_players->setContentsPos(0,ip);
	}
}



void ClientWindow::slot_addToObservationList(int flag)
{
	if (flag == -1)
	{
		if (myAccount->num_observedgames != 0)
			myAccount->num_observedgames--;
	}
	else if (flag == -2)
		myAccount->num_observedgames++;
	else
		myAccount->num_observedgames = flag;

	// update status bar
	statusGames->setText(" G: " + QString::number(myAccount->num_games) + " / " + QString::number(myAccount->num_observedgames) + " ");
}

// get channelinfo: ch nr + people
void ClientWindow::slot_channelinfo(int nr, const QString &txt)
{
	qDebug() << "slot_channelinfo(): " << txt;
	QString tipstring("");
	Channel *h;
	Channel *ch = 0;

	// check if entering a channel
	if (txt == QString("*on*"))
	{
		switch (myAccount->get_gsname())
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
		bool found = false;
		for (h = channellist.first(); h != 0 && !found; h = channellist.next())
		{
			// compare numbers
			if (h->get_nr() == nr)
			{
				found = true;
				channellist.remove();
			}
		}
	}

	// check if I'm in given channel
	bool flag_add;
	if (txt.contains(myAccount->acc_name) && !txt.contains("Topic:") && !txt.contains("Title:"))
		flag_add = true;
	else
		flag_add = false;

	// delete string
	statusChannel->setText("");

	// check if channel exists in list
	bool found = false;
	for (h = channellist.first(); h != 0 && !found; h = channellist.next())
	{
		// compare numbers
		if (h->get_nr() == nr)
		{
			found = true;
			ch = h;
		}
	}

	// if not found insert channel at it's sorted position
	if (!found)
	{
		// init channel
		ch = new Channel(nr);
		channellist.inSort(ch);
	}

	// now insert channel to list
	if (flag_add)
	{
		QString text = txt.simplifyWhiteSpace();
		int count = text.count(" ") + 1;
		// set user list and user count
		ch->set_channel(nr, QString(), txt, count);
	}
	else if (txt.contains("Topic:") || txt.contains("Title:"))
	{
		// set channel's title
		ch->set_channel(nr, txt);
	}

	// reset tooltip
	QToolTip::remove(statusChannel);

	// set new tooltip
	for (h = channellist.first(); h != 0; h = channellist.next())
	{
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

	QToolTip::add(statusChannel, tipstring);
}

// user buttons
void ClientWindow::slot_pbuser1()
{
	slot_toolbaractivated(setting->readEntry("USER1_2"));
}
void ClientWindow::slot_pbuser2()
{
	slot_toolbaractivated(setting->readEntry("USER2_2"));
}
void ClientWindow::slot_pbuser3()
{
	slot_toolbaractivated(setting->readEntry("USER3_2"));
}
void ClientWindow::slot_pbuser4()
{
	slot_toolbaractivated(setting->readEntry("USER4_2"));
}

// tell, say, kibitz...
void ClientWindow::slot_message(QString txt, QColor c)
{
	//QColor c0 = MultiLineEdit2->color();
	MultiLineEdit2->setColor(c);
	slot_message( txt);
	MultiLineEdit2->setColor(Qt::black);
}  


void ClientWindow::slot_message(QString txt)
{
	// Scroll at bottom of text, set cursor to end of line
	if (MultiLineEdit2->text().endsWith("\n") && txt == "\n")
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
void ClientWindow::slot_svname(GSName &gs)
{
	// save local at 'gsname'
	// and change caption
	myAccount->set_gsname(gs);
	myAccount->set_caption();
}

// account name found
void ClientWindow::slot_accname(QString &name)
{
	// save local at 'gsname'
	// and change caption
	myAccount->set_accname(name);
	myAccount->set_caption();
}

// status found
void ClientWindow::slot_status(Status s)
{
	myAccount->set_status(s);
	myAccount->set_caption();
}

// correct viewport
void ClientWindow::slot_playerContentsMoving(int /*x*/, int /*y*/)
{
/*	QPoint p(0,0);
	QListViewItem *i = ListView_players->itemAt(p);

	if (i)
	{
		ListView_players->clearSelection();
		ListView_players->setSelected(i, true);
		ListView_players->clearSelection();
	}                         */
}

// correct viewport
void ClientWindow::slot_gamesContentsMoving(int /*x*/, int /*y*/)
{
	QPoint p(0,0);
	Q3ListViewItem *i = ListView_games->itemAt(p);

	if (i)
	{
		ListView_games->clearSelection();
		ListView_games->setSelected(i, true);
		ListView_games->clearSelection();
	}
}


/*
 *   account & caption
 */

Account::Account(QWidget* parent)
{
	// init
	this->parent = parent;
	standard = PACKAGE + QString("V") + VERSION;

	set_offline();
}

Account::~Account()
{
}

// set caption
void Account::set_caption()
{
	if (gsName == GS_UNKNOWN || acc_name.isNull())
	{
		// server unknown or no account name
		// -> standard caption
		parent->setCaption(standard);
	}
  else
	{
		if (status == GUEST)
			parent->setCaption(svname + " - " + acc_name + " (guest)");
		else
			parent->setCaption(svname + " - " + acc_name);
	}
}

// set go server name
void Account::set_gsname(GSName gs)
{
	gsName = gs;

	// now we know which server
	switch (gsName)
	{
		case IGS: svname.sprintf("IGS");
			break;

		case NNGS: svname.sprintf("NNGS");
			break;

		case LGS: svname.sprintf("LGS");
			break;

		case WING: svname.sprintf("WING");
			break;

		case CTN: svname.sprintf("CTN");
			break;

		case CWS: svname.sprintf("CWS");
			break;

		default: svname.sprintf("unknown Server");
			break;
	}

	// set account name
	if (acc_name.isNull())
	{
		// acc_name should be set...
		acc_name.sprintf("Lulu");
		qWarning("set_gsname() - acc_name not found!");
	}
	
	if (status == OFFLINE)
		status = (enum Status) REGISTERED;
}

// set account name
void Account::set_accname(QString &name)
{
	acc_name = name;
}

// set status
void Account::set_status(Status s)
{
	status = s;
}

// set to offline mode
void Account::set_offline()
{
	gsName = GS_UNKNOWN;
	svname = QString::null;
	acc_name = QString::null;
	status = OFFLINE;

	set_caption();

	num_players = 0;
	num_watchedplayers = 0;
	num_observedgames = 0;
	num_games = 0;
}

// get some variables
Status Account::get_status()
{
	return status;
}

GSName Account::get_gsname()
{
	return gsName;
}


/*
 *   Host - Class to save Host info
 */

Host::Host(const QString &title, const QString &host, const unsigned int port, const QString &login, const QString &pass, const QString &cod)
{
	t = title;
	h = host;
	pt = port;
	lg = login;
	pw = pass;
	cdc = cod;
}

/*
 *   List to help keeping things sorted
 */

int HostList::compareItems(Item d1, Item d2)
{
	Host *s1 = static_cast<Host*>(d1);
	Host *s2 = static_cast<Host*>(d2);

	CHECK_PTR(s1);
	CHECK_PTR(s2);

	if (s1 > s2)
		return 1;
	else if (s1 < s2)
		return -1;
	else
		// s1 == s2;
		return 0;
}

int ChannelList::compareItems(Item d1, Item d2)
{
	Channel *s1 = static_cast<Channel*>(d1);
	Channel *s2 = static_cast<Channel*>(d2);

	CHECK_PTR(s1);
	CHECK_PTR(s2);

	if (s1 > s2)
		return 1;
	else if (s1 < s2)
		return -1;
	else
		// s1 == s2;
		return 0;
}


/*
 *   Talk - Class to handle  Talk Dialog Windows
 */

int Talk::counter = 0;

Talk::Talk(const QString &playername, QWidget *parent, bool isplayer)
  : QDialog (parent, playername)
{
	setupUi(this);
	name = playername;

	// create a new tab
	QString s = "MultiLineEdit1_" + QString::number(++counter);
	MultiLineEdit1->setName(s.ascii()) ;
  
	MultiLineEdit1->setCurrentFont(setting->fontComments); 

	s = "LineEdit1_" + QString::number(++counter);
	LineEdit1->setName(s.ascii());
	LineEdit1->setFont(setting->fontComments);

	// do not add a button for shouts* or channels tab
	if ( (name.find('*') != -1) || (!isplayer))
	{

		delete pb_releaseTalkTab;
		delete pb_match;
		delete stats_layout;
	}
	
}

Talk::~Talk()
{
}

// release current Tab
void Talk::slot_pbRelTab()
{
	emit signal_pbRelOneTab(this);	
}

void Talk::slot_returnPressed()
{
	// read tab
	QString txt = LineEdit1->text();
	emit signal_talkto(name, txt);
	LineEdit1->clear();
}

void Talk::slot_match()
{
  QString txt= name+ " " + stats_rating->text();
	emit signal_matchrequest(txt,true);
}



// write to txt field in dialog
// if null string -> check edit field
void Talk::write(const QString &text) const
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
	MultiLineEdit1->append(txt); //eb16
}

void Talk::setTalkWindowColor(QPalette pal)
{
	MultiLineEdit1->setPalette(pal);
	LineEdit1->setPalette(pal);
}
