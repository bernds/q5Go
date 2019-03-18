/*
 *   parser.cpp
 */

#include "parser.h"
#include "qgo_interface.h"
#include "clientwin.h"

#include <qregexp.h>
#include <iostream>
using namespace std;

// Parsing of Go Server messages
Parser::Parser(ClientWindow *cw, qGoIF *qgoif) : m_client_win (cw), m_qgoif (qgoif)
{
	// generate buffers
	aPlayer = new Player;
  	statsPlayer = new Player;
	aGame = new Game;
	aGameInfo = new GameInfo;
	memory = 0;
	memory_str = QString::null;
	myname = QString::null;

	// init
	gsName = GS_UNKNOWN;
//	cmd = NONE;
}

Parser::~Parser()
{
	delete aGameInfo;
	delete aGame;
	delete aPlayer;
	delete statsPlayer;
}

// put a line from host to parser
// if info is recognized, a signal is sent, and, however,
// the return type indicates the type of information

InfoType Parser::put_line(const QString &txt)
{
	QString line = txt.trimmed();

	if (line.length() == 0)
	{
		// skip empty lines but write message if
		// a) not logged in
		// b) help files
		if (gsName == GS_UNKNOWN || memory_str.contains("File"))
		{
			emit signal_message(txt);
			return MESSAGE;
		}

		// white space only
		return WS;
	}

	// skip console commands
	if (line.indexOf(CONSOLECMDPREFIX,0) != -1)
		return NONE;

	// check for connection status
	if (line.indexOf("Connection closed",0,Qt::CaseInsensitive) != -1)
	{
		emit signal_connclosed();
		emit signal_message(txt);
		gsName = GS_UNKNOWN;
		return IT_OTHER;
	}

	//
	// LOGON MODE ----------------------
	//
	// try to find out server and set mode
	if (gsName == GS_UNKNOWN)
	{
		if (line.indexOf("IGS entry on",0) != -1)
		{
			gsName = IGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("LGS #",0) != -1)
		{
			gsName = LGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("NNGS #",0) != -1)
		{
			gsName = NNGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// suggested by Rod Assard for playing with NNGS version 1.1.14
		if (line.indexOf("Server (NNGS)",0) != -1)
		{
			gsName = NNGS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("WING #",0) != -1)
		{
			gsName = WING;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		if (line.indexOf("CTN #",0) != -1)
		{
			gsName = CTN;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// adapted from NNGS, chinese characters
		if (line.indexOf("CWS #",0) != -1 || line.indexOf("==CWS",0) != -1)
		{
			gsName = CWS;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// critical: TO BE WATCHED....
		if (line.indexOf("#>",0) != -1)
		{
			gsName = DEFAULT;
			emit signal_svname(gsName);
			return SERVERNAME;
		}

		// account name
		if (line.indexOf("Your account name is",0) != -1)
		{
			buffer = line.right(line.length() - 21);
			buffer.replace(QRegExp("[\".]"), "");
			emit signal_accname(buffer);
			return ACCOUNT;
		}

		// account name as sent from telnet.cpp
		if (line.indexOf("...sending:") != -1)
		{
			buffer = line.section('{', 1, 1).section('}', 0, 0);
			if (buffer.isEmpty ())
				return IT_OTHER;
			emit signal_accname(buffer);
			return ACCOUNT;
		}

		if ((line.indexOf("guest account",0) != -1) || line.contains("logged in as a guest"))
		{
			emit signal_status(Status::guest);
			return STATUS;
		}

		if (line.at(0) != '9' && !memory)
		{
			emit signal_message(txt);
			return MESSAGE;
		}
	}

	//
	// LOGON HAS DONE, now parse: ----------------------
	//
	// get command type:
	bool ok;
	int cmd_nr = line.section(' ', 0, 0).toInt(&ok);
	if (!ok && memory_str.contains("CHANNEL"))
	{
		// special case: channel info
		cmd_nr = 9;
	}
	else if (!ok || (memory_str.contains("File") && !line.contains("File")))
	{
		// memory_str == "File": This is a help message!
		// skip action if entering client mode
		if (line.indexOf("Set client to be True", 0, Qt::CaseInsensitive) != -1)
			return IT_OTHER;

		if (memory == 14)
			// you have message
			emit signal_shout(tr("msg*"), line);
		else
			emit signal_message(txt);

		if (line.indexOf("#>") != -1
		    && !memory_str.isNull() && !memory_str.contains("File"))
			return NOCLIENTMODE;

		return MESSAGE;
	}
	else
	{
		// remove command number
		line = line.remove(0, 2).trimmed();
	}

	// correct cmd_nr for special case; if quiet is set to false a game may not end...
	if (cmd_nr == 9 && line.contains("{Game"))
	{
		qDebug("command changed: 9 -> 21 !!!");
		cmd_nr = 21;
	}

	// case 42 is equal to 7
	if (cmd_nr == 42 && gsName != IGS)
	{
		// treat as game info
		cmd_nr = 7;
		qDebug("command changed: 42 -> 7 !!!");
	}

	// process for different servers has particular solutions
	// command mode -> expect result
	switch (cmd_nr)
	{
	case 1:
		return cmd1(line);
	case 2:
		return cmd2(line);
	case 5:
		return cmd5(line);
	case 7:
		return cmd7(line);
	case 8:
		return cmd8(line);
	case 9:
		return cmd9(line);
	case 11:
		return cmd11(line);
	case 14:
		return cmd14(line);
	case 15:
		return cmd15(line);
	case 19:
		return cmd19(line);
	case 20:
		return cmd20(line);
	case 21:
		return cmd21(line);
	case 22:
		return cmd22(line);
#if 0 // TODO
		// STORED
		// 9 Stored games for frosla:
		// 23           frosla-physician
	case 23:
		break;
#endif

	case 24:
		return cmd24(line);
		// results
		//25 File
		//curio      [ 5d*](W) : lllgolll   [ 4d*](B) H 0 K  0.5 19x19 W+Resign 22-04-47 R
		//curio      [ 5d*](W) : was        [ 4d*](B) H 0 K  0.5 19x19 W+Time 22-05-06 R
		//25 File
	case 25:
		break;
	case 27:
		return cmd27(txt);
	case 28:
		return cmd28(line);
	case 32:
		return cmd32(line);

		// Setting your . to xxxx
	case 40:
		break;

	case 42:
		return cmd42(txt);

	case 48:
		return cmd48(line);

	case 49:
		return cmd49(line);

	case 63:
		return cmd63(line);

	default:
		emit signal_message(line);
		return MESSAGE;
	}

	return IT_OTHER;
}

// PROMPT
InfoType Parser::cmd1(const QString &line)
{
	if (memory_str.contains("File") || memory_str.contains("STATS"))
		// if ready this cannont be a help message
		memory_str = QString::null;
	emit signal_message("\n");
	return READY;
}

// BEEP
InfoType Parser::cmd2(const QString &line)
{
	if (line.contains("Game saved"))
	{
		return IT_OTHER;
	}
	return BEEP;
}

// ERROR message
//	5 Player "xxxx" is not open to match requests.
//	5 You cannot observe a game that you are playing.
//	5 You cannot undo in this game
//	5 Opponent's client does not support undoplease
InfoType Parser::cmd5(const QString &line)
{
	if (line.contains("No user named"))
	{
		QString name = line.section('"', 1, 1);
		emit signal_talk(name, "@@@", true);
	}
	else if (line.contains("is not open to match requests"))
	{
		QString opp = line.section('"', 1, 1);
		emit signal_notopen(opp);
	}
	else if (line.contains("player is currently not accepting matches"))
	{
		// IGS: 5 That player is currently not accepting matches.
		emit signal_notopen(0);
	}

	else if (line.contains("You cannot undo") || line.contains("client does not support undoplease"))
	{
		// not the cleanest way : we should send this to a messagez box
		emit signal_kibitz(0, 0, line);
		return KIBITZ;
	}

	// Debug: 5 There is a dispute regarding your match(nmatch):
	// Debug: 5 yfh2test request: B 3 19 420 900 25 0 0 0
	// Debug: 5 yfh22 request: W 3 19 600 900 25 0 0 0

	else if (line.contains("request:"))// && (element(line, 0, " ") != myname))
	{
		QString p = line.section(' ', 0, 0);
		if (p == myname)
		{
			memory_str = line;
			return MESSAGE;
		}

		if (memory_str.contains(myname + " request"))
		{
			memory_str = QString::null;
			return MESSAGE;
		}

		QString nmatch_dispute = line.section(' ', -1, -1);

		emit signal_dispute(p, nmatch_dispute);
		return MESSAGE;
	}

	emit signal_message(line);

	return  MESSAGE;
}

// games
// 7 [##] white name [ rk ] black name [ rk ] (Move size H Komi BY FR) (###)
// 7 [41]      xxxx10 [ 1k*] vs.      xxxx12 [ 1k*] (295   19  0  5.5 12  I) (  1)
// 7 [118]      larske [ 7k*] vs.      T08811 [ 7k*] (208   19  0  5.5 10  I) (  0)
// 7 [255]          YK [ 7d*] vs.         SOJ [ 7d*] ( 56   19  0  5.5  4  I) ( 18)
// 7 [42]    TetsuyaK [ 1k*] vs.       ezawa [ 1k*] ( 46   19  0  5.5  8  I) (  0)
// 7 [237]       s2884 [ 3d*] vs.         csc [ 2d*] (123   19  0  0.5  6  I) (  0)
// 7 [67]    atsukann [14k*] vs.    mitsuo45 [15k*] ( 99   19  0  0.5 10  I) (  0)
// 7 [261]      lbeach [ 3k*] vs.    yurisuke [ 3k*] (194   19  0  5.5  3  I) (  0)
// 7 [29]      ppmmuu [ 1k*] vs.       Natta [ 2k*] (141   19  0  0.5  2  I) (  0)
// 7 [105]      Clarky [ 2k*] vs.       gaosg [ 2k*] ( 65   19  0  5.5 10  I) (  1)
InfoType Parser::cmd7(const QString &line)
{
	if (line.contains("##"))
		// skip first line
		return GAME7_START;
#if 0
	if (!line.contains('['))
	{
		// no GAMES result -> leave GAMES mode
		return IT_OTHER;
	}
#endif
	QRegExp gamesre("\\[\\s*(\\d+)\\s*\\]\\s+"
			"([^\\s]+)\\s+\\[\\s*([^\\]\\s]*)\\s*\\]\\s+" "vs.\\s+"
			"([^\\s]+)\\s+\\[\\s*([^\\]\\s]*)\\s*\\]\\s+"
			"\\(\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+([\\d-.]+)\\s+(\\d+)\\s+([^\\s\\)]+)\\)\\s+"
			"\\(\\s*(\\d+)\\).*");

	if (gamesre.indexIn (line) == -1) {
		return GAME7;
	}

	// get info line
	aGame->mv = gamesre.cap (6);
	aGame->Sz = gamesre.cap (7);
	aGame->H = gamesre.cap (8);
	aGame->K = gamesre.cap (9);
	aGame->By = gamesre.cap (10);
	aGame->FR = gamesre.cap (11);

	aGame->nr = gamesre.cap (1);
	aGame->wname = gamesre.cap (2);
	aGame->wrank = gamesre.cap (3);
	aGame->bname = gamesre.cap (4);
	aGame->brank = gamesre.cap (5);
	aGame->ob = gamesre.cap (12);

	// indicate game to be running
	aGame->running = true;
	aGame->oneColorGo = false;

	emit signal_game(aGame);
	emit signal_gamemove(aGame);

	return GAME7;
}

// "8 File"
InfoType Parser::cmd8(const QString &line)
{
	if (memory_str.contains("File"))
	{
		// toggle
		memory_str = QString::null;
		memory = 0;
	}
	else if (memory != 0 && memory_str == "CHANNEL")
	{
		emit signal_channelinfo(memory, line);
		memory_str = QString::null;
		return IT_OTHER;
	}

	else if (line.contains("File"))
	{
		// the following lines are help messages
		memory_str = line;
		// check if NNGS message cmd is active -> see '9 Messages:'
		if (memory != 14)
			memory = 8;
	}
	return HELP;
}

// INFO: stats, channelinfo
// NNGS, LGS: (NNGS new: 2nd line w/o number!)
//	9 Channel 20 Topic: [xxx] don't pay your NIC bill and only get two players connected
//	9  xxxx1 xxxx2 xxxx3 xxxx4 frosla
//	9 Channel 49 Topic: Der deutsche Kanal (German)
//	9  frosla
//
//	-->  channel 49
//	9 Channel 49 turned on.
//	9 Channel 49 Title:
//
//	9 guest has left channel 49.
//
//	9 Komi set to -3.5 in match 10
// - in my game:
// -   opponent:
//      9 Komi is now set to -3.5
// -   me:
//      9 Set the komi to -3.5
// NNGS, LGS:
//	9 I suggest that ditto play White against made:
//	9 For 19x19:  Play an even game and set komi to 1.5.
//	9 For 13x13:  Play an even game and set komi to 6.5.
//	9 For   9x9:  Play an even game and set komi to 4.5.
// or:
//	9 I suggest that pem play Black against eek:
//	For 19x19:  Take 3 handicap stones and set komi to -2.5.
//	For 13x13:  Take 1 handicap stones and set komi to -6.5.
//	For   9x9:  Take 1 handicap stones and set komi to 0.5.
// or:
//	   I suggest that you play White against Cut:
//
//	9 Match [19x19] in 1 minutes requested with xxxx as White.
//	9 Use <match xxxx B 19 1 10> or <decline xxxx> to respond.
//
//	9 Match [5] with guest17 in 1 accepted.
//	9 Creating match [5] with guest17.
//
//	9 Requesting match in 10 min with frosla as Black.
//	9 guest17 declines your request for a match.
//	9 frosla withdraws the match offer.
//
//	9 You can check your score with the score command
//	9 You can check your score with the score command, type 'done' when finished.
//	9 Removing @ K8
//
//	9 Use adjourn to adjourn the game.
//
// NNGS: cmd 'user' NOT PARSED!
//	9  Info     Name       Rank  19  9  Idle Rank Info
//	9  -------- ---------- ---- --- --- ---- ---------------------------------
//	9  Q  --  5 hhhsss      2k*   0   0  1s  NR                                    
//	9     --  5 saturn      2k    0   0 18s  2k                                    
//	9     --  6 change1     1k*   0   0  7s  NR                                    
//	9 S   -- -- mikke       1d    0  18 30s  shodan in sweden                      
//	9   X -- -- ksonney     NR    0   0 56s  NR                                    
//	9     -- -- kou         6k*   0   0 23s  NR                                    
//	9 SQ! -- -- GnuGo      11k*   0   0  5m  Estimation based on NNGS rating early 
//	9   X -- -- Maurice     3k*   0   0 24s  2d at Hamilton Go Club, Canada; 3d in 
InfoType Parser::cmd9(QString &line)
{
	// status messages
	if (line.contains("Set open to be"))
	{
		bool val = (line.indexOf("False") == -1);
		emit signal_checkbox(0, val);
	}
	else if (line.contains("Setting you open for matches"))
		emit signal_checkbox(0, true);
	else if (line.contains("Set looking to be"))
	{
		bool val = (line.indexOf("False") == -1);
		emit signal_checkbox(1, val);
	}
	// 9 Set quiet to be False.
	else if (line.contains("Set quiet to be"))
	{
		bool val = (line.indexOf("False") == -1);
		emit signal_checkbox(2, val);
	}
	else if (line.indexOf("Channel") == 0)
	{
		// channel messages
		QString e1 = line.section(' ', 1, 1);
		if (e1.at(e1.length()-1) == ':')
			e1.truncate(e1.length()-1);
		int nr = e1.toInt();

		if (line.contains("turned on."))
		{
			// turn on channel
			emit signal_channelinfo(nr, QString("*on*"));
		}
		else if (line.contains("turned off."))
		{
			// turn off channel
			emit signal_channelinfo(nr, QString("*off*"));
		}
		else if (!line.contains("Title:") || gsName == GS_UNKNOWN)
		{
			// keep in memory to parse next line correct
			memory = nr;
			emit signal_channelinfo(memory, line);
			memory_str = "CHANNEL";
		}
//				return IT_OTHER;
	}
	else if (memory != 0 && memory_str == "CHANNEL")
	{
		emit signal_channelinfo(memory, line);

		// reset memory
		memory = 0;
		memory_str = QString::null;
//				return IT_OTHER;
	}
	// IGS: channelinfo
	// 9 #42 Title: Untitled -- Open
	// 9 #42    broesel    zero815     Granit
	else if (line.contains("#"))
	{
		int nr = line.section(' ', 0, 0).mid(1).toInt();
		QString msg = line.section(' ', -1, -1);
		emit signal_channelinfo(nr, msg);
	}
	// NNGS: channels
	else if (line.contains("has left channel") || line.contains("has joined channel"))
	{
		QString e1 = line.section(' ', 3, 3).remove('.');
		int nr = e1.toInt();

		// turn on channel to get full info
		emit signal_channelinfo(nr, QString("*on*"));
	}
	else if (line.contains("Game is titled:"))
	{
		// This is preceded by a command 15 move, which has left the game ID
		// in aGameInfo.
		QString t = line.section(':', 1);
		qDebug() << "got game title " << t << " for " << aGameInfo->nr;
		emit signal_title(aGameInfo, t);
		return IT_OTHER;
	}
	else if (line.contains("offers a new komi "))
	{
		// NNGS: 9 physician offers a new komi of 1.5.
		QString komi = line.section(' ', 6, 6);
		if (komi.at(komi.length()-1) == '.')
			komi.truncate(komi.length() - 1);
		QString opponent = line.section(' ', 0, 0);

		// true: request
		emit signal_komi(opponent, komi, true);
	}
	else if (line.contains("Komi set to"))
	{
		// NNGS: 9 Komi set to -3.5 in match 10
		QString komi = line.section(' ', 3, 3);
		QString game_id = line.section(' ', 6, 6);

		// false: no request
		emit signal_komi(game_id, komi, false);
	}
	else if (line.contains("wants the komi to be"))
	{
		// IGS: 9 qGoDev wants the komi to be  1.5
		QString komi = line.section(' ', 6, 6);
		QString opponent = line.section(' ', 0, 0);

		// true: request
		emit signal_komi(opponent, komi, true);
	}
	else if (line.contains("Komi is now set to"))
	{
		// 9 Komi is now set to -3.5. -> oppenent set for our game
		QString komi = line.section(' ', 5, 5);
		// error? "9 Komi is now set to -3.5.9 Komi is now set to -3.5"
		if (komi.contains(".9"))
			komi = komi.left(komi.length() - 2);

		// false: no request
		emit signal_komi(QString(), komi, false);
	}
	else if (line.contains("Set the komi to"))
	{
		// NNGS: 9 Set the komi to -3.5 - I set for own game
		QString komi = line.section(' ', 4, 4);

		// false: no request
		emit signal_komi(QString(), komi, false);
	}
	else if (line.contains("game will count"))
	{
		// IGS: 9 Game will not count towards ratings.
		//      9 Game will count towards ratings.
		emit signal_freegame(false);
	}
	else if (line.contains("game will not count", Qt::CaseInsensitive))
	{
		// IGS: 9 Game will not count towards ratings.
		//      9 Game will count towards ratings.
		emit signal_freegame(true);
	}
	else if ((line.contains("[") || line.contains("yes")) && line.length() < 6)
	{
		// 9 [20] ... channelinfo
		// 9 yes  ... ayt
		return IT_OTHER;
	}
	else if (line.contains("has restarted your game") ||
		 line.contains("has restored your old game"))
	{
		if (line.contains("restarted"))
			// memory_str -> see case 15 for continuation
			memory_str = line.section(' ', 0, 0);
	}
	else if (line.contains("I suggest that"))
	{
		memory_str = line;
		return IT_OTHER;
	}
	else if (line.contains("and set komi to"))
	{
		// suggest message ...
		if (memory_str.isNull())
			// something went wrong...
			return IT_OTHER;

		QString newline = line.simplified();

		QString p1 = memory_str.section(' ', 3, 3);
		// @@@ should be 7?
		QString p2 = memory_str.section(' ', 6, 6).remove(':');
		bool p1_play_white = memory_str.contains("play White");

		QString h, k;
		if (newline.contains("even game"))
			h = "0";
		else
			h = newline.section(' ', 3, 3);

		// @@@ 10?
		k = newline.section(' ', 9, 9).remove(QRegExp("\\.$"));

		int size = 19;
		if (newline.contains("13x13"))
			size = 13;
		else if (newline.contains("9x 9"))
		{
			size = 9;
			memory_str = QString::null;
		}

		if (p1_play_white)
			emit signal_suggest(p1, p2, h, k, size);
		else
			emit signal_suggest(p2, p1, h, k, size);

		return IT_OTHER;
	}
	// 9 Match [19x19] in 1 minutes requested with xxxx as White.
	// 9 Use <match xxxx B 19 1 10> or <decline xxxx> to respond.
	// 9 NMatch requested with yfh2test(B 3 19 60 600 25 0 0 0).
	// 9 Use <nmatch yfh2test B 3 19 60 600 25 0 0 0> or <decline yfh2test> to respond.
	else if (line.contains("<decline") && line.contains("match"))
	{
		QRegExp re("<(n?match[^>]*)>");
		if (re.indexIn(line) == -1) {
			return IT_OTHER;
		}
		// false -> not my request: used in clientwin.cpp
		emit signal_matchrequest(re.cap(1), false);
	}
	// 9 Match [5] with guest17 in 1 accepted.
	// 9 Creating match [5] with guest17.
	else if (line.contains("Creating match"))
	{
		QRegExp re("\\[\\s*(\\d+)\\s*\\]\\s+with\\s+([^\\s\\.]+)(?:\\sin.*accepted)?\\..*");
		if (re.indexIn(line) == -1) {
			return IT_OTHER;
		}
		QString nr = re.cap(1);
		QString opp = re.cap(2);

		emit signal_matchcreate(nr, opp);
		// automatic opening of a dialog tab for further conversation
		emit signal_talk(opp, "", true);
	}
	else if (line.contains("Match") && line.contains("accepted"))
	{
		QRegExp re("\\[\\s*(\\d+)\\s*\\]\\s+with\\s+([^\\s]+)\\s+.*");
		if (re.indexIn(line) == -1) {
			return IT_OTHER;
		}
		QString nr = re.cap(1);
		QString opp = re.cap(2);
		emit signal_matchcreate(nr, opp);
	}
	// 9 frosla withdraws the match offer.
	// 9 guest17 declines your request for a match.
	else if (line.contains("declines your request for a match") ||
		 line.contains("withdraws the match offer"))
	{
		QString opp = line.section(' ', 0, 0);
		emit signal_notopen(opp);
	}
	//9 yfh2test declines undo
	else if (line.contains("declines undo"))
	{
		// not the cleanest way : we should send this to a message box
		emit signal_kibitz(0, line.section(' ', 0, 0), line);
		return KIBITZ;
	}

	//9 yfh2test left this room
	//9 yfh2test entered this room
	else if (line.contains("this room"))
		emit signal_refresh(10);

	//9 Requesting match in 10 min with frosla as Black.
	else if (line.contains("Requesting match in"))
	{
		QString opp = line.section(' ', 6, 6);
		emit signal_opponentopen(opp);
	}
	// NNGS: 9 Removing @ K8
	// IGS:	9 Removing @ B5
	//     49 Game 265 qGoDev is removing @ B5
	else if (line.contains("emoving @"))
	{
		if (gsName != IGS)
		{
			QString pt = line.section('@', 1).trimmed();
			emit signal_removestones(pt, 0);
		}
	}
	// 9 You can check your score with the score command, type 'done' when finished.
	else if (line.contains("check your score with the score command"))
	{
#if 0
		if (gsName == IGS)
			// IGS: store and wait until game number is known
			memory_str = QString("rmv@"); // -> continuation see 15
		else
#endif
			emit signal_removestones(0, 0);
	}
	// IGS: 9 Board is restored to what it was when you started scoring
	else if (line.contains("what it was when you"))
	{
		emit signal_removestones(0, 0);
	}
	// WING: 9 Use <adjourn> to adjourn, or <decline adjourn> to decline.
	else if (line.contains("Use adjourn to") || line.contains("Use <adjourn> to"))
	{
		qDebug("parser->case 9: Use adjourn to");
		emit signal_requestDialog("adjourn", 0, 0, 0);
	}
	// 9 frosla requests to pause the game.
	else if (line.contains("requests to pause"))
	{
		emit signal_requestDialog("pause", 0, 0, 0);
	}
	else if (line.contains("been adjourned"))
	{
		// remove game from list - special case: own game
		aGame->nr = "@";
		aGame->running = false;

		emit signal_game(aGame);
	}
	// 9 Game 22: frosla vs frosla has adjourned.
	else if (line.contains("has adjourned"))
	{
		// remove game from list
		aGame->nr = line.section(' ', 1, 1).remove(':');
		aGame->running = false;

		// for information
		aGame->Sz = "has adjourned.";

		emit signal_game(aGame);
		m_qgoif->game_end (aGame->nr, "has adjourned.");
	}
	// 9 Removing game 30 from observation list.
	else if (line.contains("from observation list"))
	{
		// is done from qGoIF
		// emit signal_addToObservationList(-1);
		aGame->nr = line.section(' ', 2, 2);
		aGame->Sz = "-";
		aGame->running = false;

		m_qgoif->game_end (aGame->nr, "-");
		return IT_OTHER;
	}
	// 9 Adding game to observation list.
	else if (line.contains("to observation list"))
	{
		// is done from qGoIF
		// emit signal_addToObservationList(-2);
		return IT_OTHER;
	}
	// 9 Games currently being observed:  31, 36, 45.
	else if (line.contains("Games currently being observed"))
	{
		if (line.contains("None"))
		{
			emit signal_addToObservationList(0);
		}
		else
		{
			// don't work correct at IGS!!!
			int i = line.count(',');
			qDebug() << QString("observing %1 games").arg(i+1);
//			emit signal_addToObservationList(i+1);
		}

//		return IT_OTHER;
	}
	// 9 1 minutes were added to your opponents clock
	else if (line.contains("minutes were added"))
	{
		int t = line.section(' ', 0, 0).toInt();
		emit signal_timeAdded(t, false);
	}
	// 9 Your opponent has added 1 minutes to your clock.
	else if (line.contains("opponent has added"))
	{
		int t = line.section(' ', 4, 4).toInt();
		emit signal_timeAdded(t, true);
	}
	// NNGS: 9 Game clock paused. Use "unpause" to resume.
	else if (line.contains("Game clock paused"))
	{
		emit signal_timeAdded(-1, true);
	}
	// NNGS: 9 Game clock resumed.
	else if (line.contains("Game clock resumed"))
	{
		emit signal_timeAdded(-1, false);
	}
	// 9 Increase frosla's time by 1 minute
	else if (line.contains("s time by"))
	{
		int t = line.section(' ', 4, 4).toInt();
		if (line.contains(myname))
			emit signal_timeAdded(t, true);
		else
			emit signal_timeAdded(t, false);
	}
	// 9 Setting your . to Banana  [text] (idle: 0 minutes)
	else if (line.contains("Setting your . to"))
	{
		QRegExp textre("\\[([^\\]]+)]");
		if (textre.indexIn(line) != -1)
		{
			QString player = line.section(' ', 4, 4);
			// true = player
			emit signal_talk(player, "[" + textre.cap(1) + "]", true);
		}
	}
	// NNGS: 9 Messages: (after 'message' cmd)
	//       8 File
	//       <msg>
	//       8 File
	//       9 Please type "erase" to erase your messages after reading
	else if (line.contains("Messages:"))
	{
		// parse like IGS cmd nr 14: Messages
		memory = 14;
	}
	// IGS:  9 You have a message. To read your messages, type:  message
	// NNGS: 9 You have 1 messages.  Type "messages" to display them
	else if (line.contains("You have") && line.contains("messages"))
	{
#if 0 // already done??
		// remove cmd nr
		line = txt.trimmed();
		line = line.remove(0, 2);
#endif
		emit signal_message(line);
		return YOUHAVEMSG;
	}
	// 9 Observing game  2 (chmeng vs. myao) :
	// 9        shanghai  9k*           henry 15k
	// 9 Found 2 observers.
	else if (line.contains("Observing game ", Qt::CaseSensitive))
	{
		// right now: only need for observers of teaching game
		// game number
		bool ok;
		memory = line.section(' ', 2, 2).toInt(&ok);
		if (ok)
		{
			memory_str = "observe";
			m_qgoif->observer_list_start (memory);

			return KIBITZ;
		}
	}
	else if (memory_str == "observe" && line.contains("."))
	{
		m_qgoif->observer_list_end (memory);

		memory = 0;
		memory_str = QString::null;

		return KIBITZ;
	}
	else if (memory_str == "observe")
	{
		QString name;
		QString rank;
		for (int i = 0; !(name = line.section(' ', i, i, QString::SectionSkipEmpty)).isEmpty(); i++)
		{
			++i;
			rank = line.section(' ', i, i);
			m_qgoif->observer_list_entry (memory, name, rank);
		}

		return KIBITZ;
	}
	else if (line.contains("****") && line.contains("Players"))
	{
		// maybe last line of a 'user' cmd
		aPlayer->extInfo = "";
		aPlayer->won = "";
		aPlayer->lost = "";
		aPlayer->country = "";
		aPlayer->nmatch_settings = "";

		// remove cmd nr
		//line = txt.trimmed();
		//line = line.remove(0, 2);
		//emit signal_message(line);
		return PLAYER42_END;
	}
	// 9 qGoDev has resigned the game.
	else if ((line.contains("has resigned the game"))||
		 (line.contains("has run out of time")))
	{
		m_qgoif->game_end (line.section(' ', 0, 0), QString (), line);
		return IT_OTHER;
	}
#if 0
	//eb5 has run out of time.
	else if (line.contains("has run out of time"))
	{
		emit signal_kibitz(0, element(line, 0, " "), line);
	}
#endif

	//9 Player:      yfh2
	//9 Game:        go (1)
	//9 Language:    default
	//9 Rating:      6k*  23
	//9 Rated Games:     21
	//9 Rank:  8k  21
	//9 Wins:        13
	//9 Losses:      16
	//9 Idle Time:  (On server) 0s
	//9 Address:  yfh2@tiscali.fr
	//9 Country:  France
	//9 Reg date: Tue Nov 18 04:01:05 2003
	//9 Info:  yfh2
	//9 Defaults (help defs):  time 0, size 0, byo-yomi time 0, byo-yomi stones 0
	//9 Verbose  Bell  Quiet  Shout  Automail  Open  Looking  Client  Kibitz  Chatter
	//9     Off    On     On     On        On   Off      Off      On      On   On

	else if (line.contains("Player:"))
	{
		statsPlayer->name = line.section(':', 1).trimmed();
		statsPlayer->extInfo = "";
		statsPlayer->won = "";
		statsPlayer->lost = "";
		statsPlayer->country = "";
		statsPlayer->nmatch_settings = "";
		statsPlayer->rank = "";
		statsPlayer->info = "";
		statsPlayer->address = "";
		statsPlayer->play_str = "";
		statsPlayer->obs_str = "";
		statsPlayer->idle = "";
		statsPlayer->rated = "";
		// not sure it is the best way : above code seem to make use of "signal"
		// but we don't need this apparently for handling stats
		memory_str = "STATS";
		return STATS;
	}

	else if (line.contains("Address:"))
	{
		statsPlayer->address = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("Last Access"))
	{
		statsPlayer->idle = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("Rating:"))
	{
		statsPlayer->rank = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Wins:"))
	{
		statsPlayer->won = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("Losses:"))
	{
		statsPlayer->lost = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("Country:") || line.contains("From:"))   //IGS || LGS
	{
		statsPlayer->country = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Defaults"))    //IGS
	{
		statsPlayer->extInfo = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("Experience on WING"))  //WING
	{
		statsPlayer->extInfo = line;
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("User Level:")) //LGS
	{
		statsPlayer->extInfo = line;
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Info:") && !line.contains("Rank Info:"))
	{
		if (! statsPlayer->info.isEmpty())
			statsPlayer->info.append("\n");
		statsPlayer->info.append(line);
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Playing in game:"))       //IGS and LGS
	{
		statsPlayer->play_str = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}
	else if (line.contains("(playing game"))       //WING
	{
		statsPlayer->play_str = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Rated Games:"))
	{
		statsPlayer->rated = line.section(':', 1).trimmed();
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Games Rated:"))    //WING syntax
	{
#if 0
		statsPlayer->rated = element(line, 2, " ");
		statsPlayer->won = element(line, 0, "("," ");
		statsPlayer->lost = element(line, 6, " ");
		emit signal_statsPlayer(statsPlayer);
#endif
		return IT_OTHER;
	}

	else if (line.contains("Idle Time:"))
	{
		if (gsName == WING)
			statsPlayer->idle = line.section(':', 1).trimmed();
		else
			statsPlayer->idle = line.section(')', 1).trimmed();

		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Last Access"))
	{
		statsPlayer->idle = "not on";
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.left(5) == "19x19")     //LGS syntax
	{
		statsPlayer->rank = line.section(' ', 4, 4);
		statsPlayer->rated = line.section(' ', 7, 7);
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	else if (line.contains("Wins/Losses"))     //LGS syntax
	{
		statsPlayer->won = line.section(' ', 2, 2);
		statsPlayer->lost = line.section(' ', 4, 4);
		emit signal_statsPlayer(statsPlayer);
		return IT_OTHER;
	}

	//9 ROOM 01: FREE GAME ROOM;PSMNYACF;Ž©—R‘Î‹ÇŽº;19;1;10;1,19,60,600,30,25,10,60,0
	//9 ROOM 10: SLOW GAME ROOM;PSMNYACF;½Û°‘Î‹ÇŽº;19;1;20
	//9 ROOM 92: PANDA OPEN ROOM;X;ŒŽ—á‘å‰ï;19;10;15
	else if (! line.left(5).compare("ROOM "))
	{
		QString t1 = line.section(';', 0, 0).trimmed();
		QString t2 = line.section(';', 1, 1).trimmed();

		//(element(line, 0,";")=="X") || (element(line, 0,";")=="P");
		emit signal_room(t1.section(' ', 1), t2 == "X" || t2 == "P");
		return IT_OTHER;
	}
#if 0
	else if (line.contains("Idle"))
	{
		statsPlayer->extInfo = line.section(' ', 4);
		return STATS;
	}
#endif
#if 0
	// remove cmd nr
	line = txt.trimmed();
	line = line.remove(0, 2);
#endif
	if (memory_str != "STATS")
		emit signal_message(line);
	return MESSAGE;
}

// 11 Kibitz Achim [ 3d*]: Game TELRUZU vs Anacci [379]
// 11    will B resign?
InfoType Parser::cmd11(const QString &line)
{
	if (line.contains("Kibitz"))
	{
		// who is kibitzer
		memory_str = line.section(':', 0, 0).mid(7);
		// game number
		memory = line.section(' ', -1).remove('[').remove(']').toInt();
	}
	else
	{
		if (memory_str.isNull())
			// something went wrong...
			return IT_OTHER;

		emit signal_kibitz(memory, memory_str, line);
		memory = 0;
		memory_str = QString::null;
	}
	return KIBITZ;
}

// messages
// 14 File
// frosla 11/15/02 14:00: Hallo
// 14 File
InfoType Parser::cmd14(const QString &line)
{
	if (memory_str.contains("File"))
	{
		// toggle
		memory_str = QString::null;
		memory = 0;
	}
	else if (line.contains("File"))
	{
		// the following lines are help messages
		memory_str = line;
		memory = 14;
	}
	return IT_OTHER;
}

// MOVE
// 15 Game 43 I: xxxx (4 223 16) vs yyyy (5 59 13)
// 15 TIME:21:lowlow(W): 1 0/60 479/600 14/25 0/0 0/0 0/0
// 15 144(B): B12
// IGS: teaching game:
// 15 Game 167 I: qGoDev (0 0 -1) vs qGoDev (0 0 -1)
InfoType Parser::cmd15(const QString &line)
{
	if (line.contains("Game"))
	{
		QRegExp gamere ("Game\\s+(\\d+)\\s+"
				"([^:]+)\\s*:\\s+([^\\s]+)\\s+"
				"\\(\\s*(\\d+)\\s+(\\d+)\\s+([\\d-]+)\\)\\s+"
				"vs\\s+([^\\s]+)\\s+"
				"\\(\\s*(\\d+)\\s+(\\d+)\\s+([\\d-]+)\\).*");

		if (!gamere.exactMatch (line)) {
			return IT_OTHER;
		}

		aGameInfo->nr = gamere.cap (1);
		aGameInfo->type = gamere.cap (2);
		aGameInfo->wname = gamere.cap (3);
		aGameInfo->wprisoners = gamere.cap (4);
		aGameInfo->wtime = gamere.cap (5);
		aGameInfo->wstones = gamere.cap (6);
		aGameInfo->bname = gamere.cap (7);
		aGameInfo->bprisoners = gamere.cap (8);
		aGameInfo->btime = gamere.cap (9);
		aGameInfo->bstones = gamere.cap (10);

		if (memory_str == QString("rmv@"))
		{
			// continue removing
			emit signal_removestones(0, aGameInfo->nr);
			memory_str = QString::null;
		}
		else if ((!memory_str.isNull() || gsName == IGS || gsName == WING )
			 && (aGameInfo->bname == myname || aGameInfo->wname == myname))
		{
			// if a stored game is restarted then send start message first
			// -> continuation of case 9
			aGame->nr = aGameInfo->nr;
			aGame->wname = aGameInfo->wname;
			aGame->bname = aGameInfo->bname;
			aGame->Sz = "@@";
			aGame->running = true;

			emit signal_gamemove(aGame);

			// reset memory
			memory_str = QString::null;
		}

		// it's a kind of time info
		aGameInfo->mv_col = "T";
	}
	else if (line.contains("TIME"))
	{
		QRegExp timere ("TIME:\\s*(\\d+)\\s*:([^:]+)\\([BW]\\):\\s*"
				"(\\d+)\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)\\s+(\\d+)/(\\d+)\\s+.*");

		if (!timere.exactMatch (line)) {
			return IT_OTHER;
		}
		aGameInfo->mv_col = "T";
		aGameInfo->nr = timere.cap (1);
		QString time1 = timere.cap (4);
		QString time2 = timere.cap (6);
		QString stones = timere.cap (8);

		if (line.contains("(W)"))
		{
			aGameInfo->wname = timere.cap (2);
			aGameInfo->wtime = (time1.toInt()==0 ? time2 : time1);
			aGameInfo->wstones = (time1.toInt()==0 ?stones: "-1");
		}
		else if (line.contains("(B)"))
		{
			aGameInfo->bname = timere.cap (2);
			aGameInfo->btime = (time1.toInt()==0 ? time2 : time1);
			aGameInfo->bstones = (time1.toInt()==0 ? stones: "-1");
		}
		else //never know with IGS ...
			return IT_OTHER;
	}
	else if (line.contains("GAMERPROPS"))
	{
		return IT_OTHER;
	}
	else
	{
		QRegExp movere ("(\\d+)\\s*\\(([BW])\\):\\s*([^\\s].*)");

		if (!movere.exactMatch (line)) {
			return IT_OTHER;
		}
		aGameInfo->mv_nr = movere.cap (1);
		aGameInfo->mv_col = movere.cap (2);
		aGameInfo->mv_pt = movere.cap (3);
	}

	emit signal_set_observe(aGameInfo->nr);
	emit signal_move(aGameInfo);
	return MOVE;
}

// SAY
// 19 *xxxx*: whish you a nice game  :)
// NNGS - new:
//  19 --> frosla hallo
InfoType Parser::cmd19(const QString &line)
{
	if (line.left(3) == "-->")
		emit signal_kibitz(0, 0, line.section(' ', 2));
	else if (line.contains(":"))
		emit signal_kibitz(0, 0, line.section(':', 1));
	else
		emit signal_kibitz(0, 0, line.section(' ', 1));
	return IT_OTHER;
}

//20 yfh2 (W:O): 4.5 to NaiWei (B:#): 4.0
InfoType Parser::cmd20(const QString &line)
{
	aGame->nr = "@";
	aGame->running = false;

	QString player1 = line.section(' ', 2, 2);
	QString player2 = line.section(' ', 6, 6);
	if ( line.indexOf("W:") < line.indexOf("B:"))
		aGame->Sz = "W " + player1 + " B " + player2;
	else
		aGame->Sz = "B " + player1 + " W " + player2;

	m_qgoif->game_end (player1, player2, aGame->Sz);
	return IT_OTHER;
}

// SHOUT - a info from the server
InfoType Parser::cmd21(const QString &line)
{
	// case sensitive
	if (line.contains(" connected.}"))
	{
		// {guest1381 [NR ] has connected.}
		QRegExp re("\\{\\s*([^\\s]+)\\s+\\[\\s*([^\\]\\s]+)\\s*\\]");
		if (re.indexIn(line) == -1) {
			qDebug () << "parse failure: " << line;
			return IT_OTHER;
		}
		aPlayer->name = re.cap(1);
		aPlayer->rank = re.cap(2);
		aPlayer->info = "??";
		aPlayer->play_str = "-";
		aPlayer->obs_str = "-";
		aPlayer->idle = "-";
		aPlayer->online = true;

		// false -> no "players" cmd preceded
		emit signal_player(aPlayer, false);
		return PLAYER;
	}
	else if (line.contains("has disconnected"))
	{
		// {xxxx has disconnected}
		aPlayer->name = line.section(' ', 0, 0).mid(1);
		aPlayer->online = false;

		emit signal_player(aPlayer, false);
		return PLAYER;
	}
	else if (line.contains("{Game"))
	{
		// {Game 198: xxxx1 vs xxxx2 @ Move 33}
		if (line.contains("@"))
		{
			// game has continued
			aGame->nr = line.section(' ', 1, 1).remove(':');
			aGame->wname = line.section(' ', 2, 2);
			aGame->wrank = "??";
			aGame->bname = line.section(' ', 4, 4);
			aGame->brank = "??";
			aGame->mv = line.section(' ', 7, 7).remove('}');
			aGame->Sz = "@";
			aGame->H = QString::null;
			aGame->running = true;

			emit signal_game(aGame);
			emit signal_gamemove(aGame);
			return GAME;
		}

		// {Game 155: xxxx vs yyyy has adjourned.}
		// {Game 76: xxxx vs yyyy : W 62.5 B 93.0}
		// {Game 173: xxxx vs yyyy : White forfeits on time.}
		// {Game 116: xxxx17 vs yyyy49 : Black resigns.}
		// IGS:
		// 21 {Game 124: Redmond* vs NaiWei* : Black lost by Resign}
		// 21 {Game 184: Redmond* vs NaiWei* : White lost by 1.0}
		if (line.contains("resigns.")		||
		    line.contains("adjourned.")	||
		    line.contains(" : W ")	||
		    line.contains(" : B ")	||
		    line.contains("forfeits on")	||
		    line.contains("lost by"))
		{
			// remove game from list
			aGame->nr = line.section(' ', 1, 1).remove(':');
			aGame->running = false;

			// for information
			aGame->Sz = line.section(' ', 5).remove('}');
			if (aGame->Sz.isEmpty())
				aGame->Sz = "-";
			else if (aGame->Sz.indexOf(":") != -1)
				aGame->Sz.remove(0,2);

			emit signal_game(aGame);
			m_qgoif->game_end (aGame->nr, aGame->Sz);
			return GAME;
		}
	}
	else if (line.contains("{Match"))
	{
		// {Match 116: xxxx [19k*] vs. yyyy1 [18k*] }
		// {116:xxxx[19k*]yyyy1[18k*]}
		// WING: {Match 41: o4641 [10k*] vs. Urashima [11k*] H:2 Komi:3.5}
		QRegExp re("\\{[\\w\\s]*(\\d+):\\s*"
			   "([\\w\\d]+)\\s*\\[\\s*([^\\s\\]]+)\\s*\\]"
			   "(?:\\s+vs.\\s+)?"
			   "([\\w\\d]+)\\s*\\[\\s*([^\\s\\]]+)\\s*\\]\\s+\\}.*");

		if (re.indexIn(line) == -1) {
			qDebug () << "parse failure: " << line;
			return IT_OTHER;
		}

		aGame->wname = re.cap (2);
		aGame->bname = re.cap (4);
#if 0
		// skip info for own games; full info is coming soon
		if (aGame->wname == myname || aGame->bname == myname)
		{
			emit signal_message(line);
			return IT_OTHER;
		}
#endif
		aGame->nr = re.cap(1);
		aGame->wrank = re.cap(3);
		aGame->brank = re.cap(5);
		aGame->mv = "-";
		aGame->Sz = "-";
		aGame->H = QString::null;
		aGame->running = true;

		if (gsName == WING && aGame->wname == aGame->bname)
			// WING doesn't send 'create match' msg in case of teaching game
			emit signal_matchcreate(aGame->nr, aGame->bname);

		emit signal_game(aGame);
		return GAME;
	}
	// !xxxx!: a game anyone ?
	else if (line.contains("!: "))
	{
		QString player = line.section('!', 1, 1);
		emit signal_shout(player, line);
		return SHOUT;
	}

	emit signal_message(line);
	return MESSAGE;
}

// CURRENT GAME STATUS
// 22 Pinkie  3d* 21 218 9 T 5.5 0
// 22 aura  3d* 24 276 16 T 5.5 0
// 22  0: 4441000555033055001
// 22  1: 1441100000011000011
// 22  2: 4141410105013010144
// 22  3: 1411411100113011114
// 22  4: 1131111111100001444
// 22  5: 0100010001005011111
// 22  6: 0055000101105000010
// 22  7: 0050011110055555000
// 22  8: 1005000141005055010
// 22  9: 1100113114105500011
// 22 10: 1111411414105010141
// 22 11: 4144101101100111114
// 22 12: 1411300103100000144
// 22 13: 1100005000101001144
// 22 14: 1050505550101011144
// 22 15: 0505505001111001444
// 22 16: 0050050111441011444
// 22 17: 5550550001410001144
// 22 18: 5555000111411300144
InfoType Parser::cmd22(const QString &line)
{
	/* NNGS reports game results several times with different command numbers.  */
	if (line.contains ("{Game")) {
		return IT_OTHER;
	}
	if (!line.contains(":"))
	{
		QString player = line.section(' ', 0, 0, QString::SectionSkipEmpty);
		QString cap = line.section(' ', 2, 2, QString::SectionSkipEmpty);
		QString komi = line.section(' ', 6, 6, QString::SectionSkipEmpty);
		emit signal_result(player, cap, true, komi);
	}
	else
	{
		QString row = line.section(':', 0, 0).trimmed();
		QString results = line.section(':', 1, 1).trimmed();
		qDebug() << row << " " << results;
		emit signal_result(row, results, false, 0);
	}

#if 0
	emit signal_message(line);
#endif
	return IT_OTHER;
}

//  24 *xxxx*: CLIENT: <cgoban 1.9.12> match xxxx wants handicap 0, komi 5.5, free
//  24 *jirong*: CLIENT: <cgoban 1.9.12> match jirong wants handicap 0, komi 0.5
//  24 *SYSTEM*: shoei canceled the nmatch request.
//  24 *SYSTEM*: xxx requests .

// NNGS - new version:
//  24 --> frosla CLIENT: <qGo 0.0.15b7> match frosla wants handicap 0, komi 0.5, free
//  24 --> frosla  Hallo
InfoType Parser::cmd24(const QString &line)
{
	int pos;
	QRegExp re("\\*([^\\*]+)\\*: CLIENT:.*wants handicap\\s+(\\d+), komi\\s+([\\d.-]+)");
	if (re.indexIn (line) == -1) {
		re = QRegExp ("-->\\s+([^\\*]+)\\s+CLIENT:.*wants handicap\\s+(\\d+), komi\\s+([\\d.-]+)");
	}
	if (re.indexIn (line) != -1)
	{
		bool free = line.contains("free");
		QString opp = re.cap(1);
		QString h = re.cap(2);
		QString k = re.cap(3);
		float komi = k.toFloat();

		emit signal_komirequest(opp, h.toInt(), komi, free);
		emit signal_message(line);

		// it's tell, but don't open a window for that kind of message...
		return TELL;
	}

	//check for cancelled game offer
	if (line.contains("*SYSTEM*"))
	{
		QString opp = line.section(' ', 1, 1);
		emit signal_message(line.mid(11));

		if  (line.contains("canceled") &&  line.contains("match request"))
			emit signal_matchCanceled(opp);

		if  (line.contains("requests undo"))
			emit signal_requestDialog("undo","noundo",0,opp);

		return IT_OTHER;
	}

	// check for NNGS type of msg
	QString e1,e2;
	if ((pos = line.indexOf("-->")) != -1 && pos < 3)
	{
		e1 = line.section(' ', 1, 1, QString::SectionSkipEmpty);
		e2 = "> " + line.section(' ', 2).trimmed();
	}
	else
	{
		e1 = line.section('*', 1, 1);
		e2 = "> " + line.section(':', 1).trimmed();
	}

	// emit player + message + true (=player)
	emit signal_talk(e1, e2, true);

	return TELL;
}

// who
// 27  Info Name Idle Rank | Info Name Idle Rank
// 27  SX --   -- xxxx03      1m     NR  |  Q! --   -- xxxx101    33s     NR  
// 0   4 6    11  15         26     33   38 41
// 27     --  221 DAISUKEY   33s     8k  |   X172   -- hiyocco     2m    19k*
// 27  Q  --  216 Saiden      7s     1k* |     --   53 toshiao    11s    10k 
// 27     48   -- kyouji      3m    11k* |     --   95 bengi       5s     4d*
// IGS:
//        --   -- kimisa      1m     2k* |  Q  --  206 takabo     45s     1k*
//      X 39   -- Marin       5m     2k* |     --   53 KT713      18s     2d*
//        --   34 mat21       2m    14k* |     --    9 entropy    28s     4d 
// NNGS:
// 27   X --   -- arndt      21s     5k  |   X --   -- biogeek    58s    20k   
// 27   X --   -- buffel     21s     4k  |  S  --    5 frosla     12s     7k   
// 27  S  --   -- GoBot       1m     NR  |     --    5 guest17     0s     NR   
// 27     --    3 hama        3s    15k  |     --   -- niki        5m     NR   
// 27   ! --   -- viking4    55s    19k* |  S  --    3 GnuGo       6s    14k*  
// 27   X --   -- kossa      21m     5k  |   X --   -- leif        5m     3k*  
// 27     --    6 ppp        18s     2k* |     --    6 chmeng     20s     1d*  
// 27                 ********  14 Players 3 Total Games ********

// WING:
// 0        9    14                      38      46   51
// 27   X 13   -- takeo6      1m     2k  |   ! 26   -- ooide       6m     2k*  
// 27   ! --   -- hide1234    9m     2k* |  QX --   -- yochi       0s     2k*  
// 27   g --   86 karasu     45s     2k* |  Sg --   23 denakechi  16s     1k*  
// 27   g 23   43 kH03       11s     1k* |   g --   43 kazusige   24s     1k*  
// 27   g --   50 jan        24s     1k* |  QX --   -- maigo      32s     1d   
// 27   g --   105 kume1       5s     1d* |   g --   30 yasumitu   24s     1d   
// 27  Qf --   13 motono      2m     1d* |   X 13   -- tak7       57s     1d*  
// 27   ! 50   -- okiek       8m     1d* |   X --   -- DrO        11s     1d*  
// 27   f --   103 hiratake   35s     8k  |   g --   33 kushinn    21s     8k*
// 27   g --   102 teacup      1m     1d* |   g --   102 Tadao      32s     1d*  
InfoType Parser::cmd27(const QString &txt)
{
	int pos;
	// search for first line
	if (txt.contains("Idle") && (txt.indexOf("Info")) != -1)
	{
		// skip
		return PLAYER27_START;
	}
	else if (txt.contains("**"))
	{
		// no player line
		// xx_END used for correct inserting into player's list
		return PLAYER27_END;
	}

	// indicate player to be online
	aPlayer->online = true;

	if (gsName == WING)
	{
		// shifts take care of too long integers
		int shift1 = (txt[9] == ' ' ? 0 : 1);
		int shift2 = (txt[14+shift1] == ' ' ? shift1 : shift1+1);
		int shift3 = (txt[46+shift2] == ' ' ? shift2 : shift2+1);
		int shift4 = (txt[51+shift3] == ' ' ? shift3 : shift3+1);

		if (txt[15+shift2] != ' ')
		{
			// parse line
			aPlayer->info = txt.mid(4,2);
			aPlayer->obs_str = txt.mid(7,3);
			aPlayer->play_str = txt.mid(12+shift1,3).trimmed();
			aPlayer->name = txt.mid(15+shift2,11).trimmed();
			aPlayer->idle = txt.mid(26+shift2,3);
			if (txt[33+shift2] == ' ')
			{
				if (txt[36] == ' ')
					aPlayer->rank = txt.mid(34+shift2,2);
				else
					aPlayer->rank = txt.mid(34+shift2,3);
			}
			else
			{
				if (txt[36+shift2] == ' ')
					aPlayer->rank = txt.mid(33+shift2,3);
				else
					aPlayer->rank = txt.mid(33+shift2,4);
			}

			// check if line ok, true -> cmd "players" preceded
			emit signal_player(aPlayer, true);
		}
		else
			qDebug() << "WING - player27 dropped (1): " << txt;

		// position of delimiter between two players
		pos = txt.indexOf('|');
		// check if 2nd player in a line && player (name) exists
		if (pos != -1 && txt[52+shift4] != ' ')
		{
			// parse line
			aPlayer->info = txt.mid(41+shift2,2);
			aPlayer->obs_str = txt.mid(44+shift2,3);
			aPlayer->play_str = txt.mid(49+shift3,3).trimmed();
			aPlayer->name = txt.mid(52+shift4,11).trimmed();
			aPlayer->idle = txt.mid(63+shift4,3);
			if (txt[70+shift4] == ' ')
			{
				if (txt[73+shift4] == ' ')
					aPlayer->rank = txt.mid(71+shift4,2);
				else
					aPlayer->rank = txt.mid(71+shift4,3);
			}
			else
			{
				if (txt[73+shift4] == ' ')
					aPlayer->rank = txt.mid(70+shift4,3);
				else
					aPlayer->rank = txt.mid(70+shift4,4);
			}

			// true -> cmd "players" preceded
			emit signal_player(aPlayer, true);
		}
		else
			qDebug() << "WING - player27 dropped (2): " << txt;
	}
	else
	{
		if (txt[15] != ' ')
		{
			// parse line
			aPlayer->info = txt.mid(4,2);
			aPlayer->obs_str = txt.mid(6,3);
			aPlayer->play_str = txt.mid(11,3).trimmed();
			aPlayer->name = txt.mid(15,11).trimmed();
			aPlayer->idle = txt.mid(26,3);
			aPlayer->nmatch = false;
			if (txt[33] == ' ')
			{
				if (txt[36] == ' ')
					aPlayer->rank = txt.mid(34,2);
				else
					aPlayer->rank = txt.mid(34,3);
			}
			else
			{
				if (txt[36] == ' ')
					aPlayer->rank = txt.mid(33,3);
				else
					aPlayer->rank = txt.mid(33,4);
			}

			// check if line ok, true -> cmd "players" preceded
			emit signal_player(aPlayer, true);
		}
		else
			qDebug() << "player27 dropped (1): " << txt;

		// position of delimiter between two players
		pos = txt.indexOf('|');
		// check if 2nd player in a line && player (name) exists
		if (pos != -1 && txt[52] != ' ')
		{
			// parse line
			aPlayer->info = txt.mid(41,2);
			aPlayer->obs_str = txt.mid(43,3);
			aPlayer->play_str = txt.mid(48,3).trimmed();
			aPlayer->name = txt.mid(52,11).trimmed();
			aPlayer->idle = txt.mid(63,3);
			aPlayer->nmatch = false;
			if (txt[70] == ' ')
			{
				if (txt[73] == ' ')
					aPlayer->rank = txt.mid(71,2);
				else
					aPlayer->rank = txt.mid(71,3);
			}
			else
			{
				if (txt[73] == ' ')
					aPlayer->rank = txt.mid(70,3);
				else
					aPlayer->rank = txt.mid(70,4);
			}

			// true -> cmd "players" preceded
			emit signal_player(aPlayer, true);
		}
		else
			qDebug() << "player27 dropped (2): " << txt;
	}

	return PLAYER27;
}

// 28 guest17 undid the last move (J16).
// 15 Game 7 I: frosla (0 5363 -1) vs guest17 (0 5393 -1)
// 15   2(B): F17
// IGS:
// 28 Undo in game 64: MyMaster vs MyMaster:  N15
InfoType Parser::cmd28(const QString &line)
{
	if (line.contains("undid the last move"))
	{
		// now: just look in qgo_interface if move_nr has decreased...
		// but send undo-signal anyway: in case of undo while scoring it's necessary
		QString player = line.section(' ', 0, 0);
		QString move = line.section(' ', 5, 5).remove('(').remove(").");
		emit signal_undo(player, move);
	}
	else if (line.contains("Undo in game"))
	{
		QString player = line.section(' ', 3, 3);
		player.truncate(player.length() - 1);
		QString move = line.section(':', -1, -1).trimmed();
		emit signal_undo(player, move);
	}

	// message anyway
	emit signal_message(line);
	return IT_OTHER;
}

// IGS
// -->  ; \20
// 32 Changing into channel 20.
// 32 Welcome to cyberspace.
//
// 32 49:qgodev: Title is now: (qgodev) qGo development
// 32 20:qGoDev: Person joining channel
// 32 20:frosla: hi
// 32 20:qGoDev: Person leaving channel

//
// 1 5
// -->  channels
// 9 #1 Title: Professional Channel -- Open
//
// 9 #20 Title: Untitled -- Open
// 9 #20     frosla
// #> 23 till: hello all  <-- this is what the members in channel 23 see:
//			(channel number and who sent the message)
//
// NNGS:
// channel talk: "49:xxxx: hi"
InfoType Parser::cmd32(const QString &line)
{
	QString e1,e2;

	if (line.contains("Changing into channel"))
	{
		e1 = line.section(' ', -1).remove('.');
		int nr = e1.toInt();
		emit signal_channelinfo(nr, QString("*on*"));
		emit signal_talk(e1, "", false);
		//emit signal_message(line);

		return IT_OTHER;
	}
	else if (line.contains("Welcome to cyberspace"))
	{
		emit signal_message(line);
		return IT_OTHER;
	}
	else if (line.contains("Person joining channel"))
	{
		int nr = line.section(':', 0, 0).toInt();
		emit signal_channelinfo(nr, QString("*on*"));
		return IT_OTHER;
	}
	else if (line.contains("Person leaving channel"))
	{
		int nr = line.section(':', 0, 0).toInt();
		emit signal_channelinfo(nr, QString("*on*"));
		return IT_OTHER;
	}

	// emit (channel number, player + message , false =channel )

	switch (gsName)
	{
	case IGS:
		e1=line.section(':', 0, 0);
		e2="> " + line.section(':', 1).trimmed();
		break;

	default:
		e1=line.section(':', 0, 0);
		e2="> " + line.section(':', 1).trimmed();
		break;
	}
	//emit signal_talk(line.section(':', 0, 0), line.section(':', 1).trimmed() + "\n", false);
	emit signal_talk(e1, e2, false);
	return IT_OTHER;
}

// long user report equal to 7
// 7 [255]          YK [ 7d*] vs.         SOJ [ 7d*] ( 56   19  0  5.5  4  I) ( 18)
// 42 [88]          AQ [ 2d*] vs.        lang [ 2d*] (177   19  0  5.5  6  I) (  1)

// IGS: cmd 'user'
// 42 Name        Info            Country  Rank Won/Lost Obs  Pl Idle Flags Language
// 0  3           15              31       40   45 48    54   59 62   67    72
// 42    guest15                  --        NR    0/   0  -   -   10s    Q- default 
// 42    guest13                  --        NR    0/   0  -   -   24s    Q- default 
// 42         zz  You may be rec  France    NR    0/   0  -   -    0s    -X default 
// 42     zz0002  <none>          Japan     NR    0/   0  -   -    1m    QX default 
// 42     zz0003  <none> ()       Japan     NR    0/   0  -   -    1m    SX default 
// 42       anko                  Taiwan    2k* 168/  97  -   -    1s    -- default 
// 42     MUKO12  1/12 ã€œ 1/15    Japan     4k* 666/ 372  17  -    1m    -X default 
// 42        mof  [Igc2000 1.1]   Sweden    2k* 509/ 463 124  -    0s    -X default 
// 42     obiyan  <None>          Japan     3k* 1018/ 850  -   50  11s    -- default 
// 42    uzumaki                  COM       1k    0/   0  -   -    1s    -X default 
// 42 HansWerner  [Igc2000 1.2]   Germany   1k   11/   3 111  -    1m    QX default 
// 42    genesis                  Germany   1k* 592/ 409  -  136  14s    -- default 
// 42    hamburg                  Germany   3k* 279/ 259  -  334  13s    -- default 
// 42        Sie                  Germany   1k*   6/   7  -   68  39s    -- default 
// 42     borkum                  Germany   4k* 163/ 228  -  100   7s    -- default 
// 42     casumy  45-3            Germany   1d    0/   0  -  133   2s    Q- default 
// 42      stoni                  Germany   2k* 482/ 524  -  166   7s    -- default 
// 42   Beholder  <None>          Germany   NR    0/   0 263  -    5s    QX default 
// 42     xiejun  1/10-15         Germany   3d* 485/ 414 179  -   49s    -X default
// 9                 ******** 1120 Players 247 Total Games ********

// IGS: new cmd 'userlist' for nmatch information
//          10        20        30        40        50        60        70        80        80        90       100       110       120
// 01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
// 42 Name        Info            Country  Rank Won/Lost Obs  Pl Idle Flags Language
// 42      -----  <none>          Japan    15k+ 2108/2455  -   -    1m    Q- default  T BWN 0-9 19-19 600-600 1200-1200 25-25 0-0 0-0 0-0
// 42      -----           Japan     3d   11/  13  -  222  46s    Q- default  T BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
// 42       Neil  <None>          USA      12k  136/  86  -   -    0s    -X default  T BWN 0-9 19-19 60-60 60-3600 25-25 0-0 0-0 0-0
InfoType Parser::cmd42(const QString &txt)
{
	if (txt[40] == 'R')
	{
		// skip
		return PLAYER42_START;
	}
	//                        1                   2
	//                        Name                Info
	QRegExp re = QRegExp("42 ([A-Za-z0-9 ]{,10})  (.{1,14})  "
	//                    3
	//                    Country
			     "([a-zA-Z][a-zA-Z. /]{,6}|--     )  "
	//                    4
	//                    Rank
			     "([0-9 ][0-9][kdp].?| +BC) +"
	//                    5               6
	//                    Won             Lost
			     "([0-9 ]+[0-9]+)/([0-9 ]+[0-9]+) +"
	//                    7           8
	//                    Obs         Pl
			     "([0-9]+|-) +([0-9]+|-) +"
	//                    9               10                   11    12
	//                    Idle            Flags
			     "([A-Za-z0-9]+) +([^ ]{,2}) +default  ([TF])(.*)?");
	if (re.indexIn(txt) < 0)
	{
		qDebug() << txt.toUtf8().data();
		qDebug() << "No match";
		return IT_OTHER;
	}
	// 42       Neil  <None>          USA      12k  136/  86  -   -    0s    -X default  T BWN 0-9 19-19 60-60 60-3600 25-25 0-0 0-0 0-0

	aPlayer->name = re.cap(1).trimmed();
	aPlayer->extInfo = re.cap(2);
	aPlayer->country = re.cap(3).trimmed();
	if (aPlayer->country == "--")
		aPlayer->country = "";
	aPlayer->rank = re.cap(4).trimmed();
	aPlayer->won = re.cap(5).trimmed();
	aPlayer->lost = re.cap(6).trimmed();
	aPlayer->obs_str = re.cap(7).trimmed();
	aPlayer->play_str = re.cap(8).trimmed();
	aPlayer->idle = re.cap(9).trimmed();
	aPlayer->info = re.cap(10).trimmed();
	aPlayer->nmatch = re.cap(11) == "T";

	aPlayer->nmatch_settings = "";
	QString nmatchString = "";
	// we want to format the nmatch settings to a readable string
	if (aPlayer->nmatch)
	{
		// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
		nmatchString = re.cap(12).trimmed();
		if (! nmatchString.isEmpty())
		{
			aPlayer->nmatch_black = nmatchString.contains("B");
			aPlayer->nmatch_white = nmatchString.contains("W");
			aPlayer->nmatch_nigiri = nmatchString.contains("N");

			QString elt1 = nmatchString.section(' ', 1);
			QString elt2 = nmatchString.section(' ', 2);
			QString elt3 = nmatchString.section(' ', 3);
			QString elt4 = nmatchString.section(' ', 4);
			QString elt5 = nmatchString.section(' ', 4);
			aPlayer->nmatch_timeMin = elt3.section('-', 0).toInt();
			aPlayer->nmatch_timeMax = elt3.section('-', 1).toInt();
			QString t1min = QString::number(aPlayer->nmatch_timeMin / 60);
			QString t1max = QString::number(aPlayer->nmatch_timeMax / 60);
			if (t1min != t1max)
				t1min.append(" to ").append(t1max);

			QString t2min = elt4.section('-', 0);
			QString t2max = elt4.section('-', 1);
			aPlayer->nmatch_BYMin = t2min.toInt();
			aPlayer->nmatch_BYMax = t2max.toInt();

			QString t3min = QString::number(t2min.toInt() / 60);
			QString t3max = QString::number(t2max.toInt() / 60);

			if (t2min != t2max)
				t2min.append(" to ").append(t2max);

			if (t3min != t3max)
				t3min.append(" to ").append(t3max);

			QString s1 = elt5.section('-', 0);
			QString s2 = elt5.section('-', 1);
			aPlayer->nmatch_stonesMin = s1.toInt();
			aPlayer->nmatch_stonesMax = s2.toInt();

			if (s1 != s2)
				s1.append("-").append(s2);

			if (s1 == "1")
				aPlayer->nmatch_settings = "Jap. BY : " +
					t1min + " min. + " +
					t2min + " secs./st. ";
			else
				aPlayer->nmatch_settings = "Can. BY : " +
					t1min + " min. + " +
					t3min + " min. /" +
					s1 + " st. ";

			QString h1 = elt1.section('-', 0);
			QString h2 = elt1.section('-', 1);
			aPlayer->nmatch_handicapMin = h1.toInt();
			aPlayer->nmatch_handicapMax = h2.toInt();
			if (h1 != h2)
				h1.append("-").append(h2);

			aPlayer->nmatch_settings.append("(h " + h1 + ")");
		}
		else
			aPlayer->nmatch_settings = "No match conditions";
	}

	// indicate player to be online
	aPlayer->online = true;

	// check if line ok, true -> cmd 'players' or 'users' preceded
	emit signal_player(aPlayer, true);

	return PLAYER42;
}

// IGS:48 Game 354 qGoDev requests an adjournment
InfoType Parser::cmd48(const QString &line)
{
#if 0 // have a look at case 9
	if (line.contains("requests an adjournment"))
	{
		QString nr = line.section(' ', 1, 1);
		QString opp = line.section(' ', 2, 2);
		switch (gsName)
		{
		case IGS:
			// IGS: you cannot decline, just cancel
			emit signal_requestDialog("adjourn", 0, nr, opp);
			break;

		case NNGS:
		default:
			// we will see if correct...
			emit signal_requestDialog("adjourn", "decline adjourn", nr, opp);
			break;
		}
	}
#endif
	return IT_OTHER;
}

// IGS: 49 Game 42 qGoDev is removing @ C5
InfoType Parser::cmd49(const QString &line)
{
	if (line.contains("is removing @"))
	{
		QString pt = line.section(' ', 6, 6);
		QString game = line.section(' ', 1, 1);
		QString who = line.section(' ', 2, 2);
		emit signal_removestones(pt, game);
		if (!game.isEmpty())
		{
			pt = "removing @ " + pt;
			emit signal_kibitz(game.toInt(), who, pt);
		}
	}
	return IT_OTHER;
}

// IGS : seek syntax, answer to 'seek config_list' command
// 63 CONFIG_LIST_START 4
// 63 CONFIG_LIST 0 60 600 25 0 0
// 63 CONFIG_LIST 1 60 480 25 0 0
// 63 CONFIG_LIST 2 60 300 25 0 0
// 63 CONFIG_LIST 3 60 900 25 0 0
// 63 CONFIG_LIST_END

// 63 OPPONENT_FOUND

//63 ENTRY_LIST_START 5
//63 ENTRY_LIST HKL 60 480 25 0 0 19 1 1 0
//63 ENTRY_LIST tgor 60 480 25 0 0 19 1 1 0
//63 ENTRY_LIST sun756 60 600 25 0 0 19 1 1 0
//63 ENTRY_LIST horse 60 600 25 0 0 19 2 2 0
//63 ENTRY_LIST masaeaki 60 300 25 0 0 19 1 1 0
//63 ENTRY_LIST_END
InfoType Parser::cmd63(const QString &line)
{
	if (line.contains("CONFIG_LIST "))
	{
		QString nb = line.section(' ', 1, 1);
		QString time = line.section(' ', 2, 2);
		QString BY = line.section(' ', 3, 3);
		QString Stone_number = line.section(' ', 4, 4);
		QString bline = line.section(' ', 1);
		emit signal_addSeekCondition(nb,time,BY,Stone_number,bline );
		return IT_OTHER;
	}

	else if (line.contains("CONFIG_LIST_START"))
	{
		emit signal_clearSeekCondition();
		return IT_OTHER;
	}

	else if (line.contains("OPPONENT_FOUND") || line.contains("ENTRY_CANCEL"))
	{
		emit signal_cancelSeek();
		return IT_OTHER;
	}

	else if (line.contains("ERROR"))
	{
		emit signal_cancelSeek();
		emit signal_message(line);
		return IT_OTHER;
	}

	else if (line.contains("ENTRY_LIST_START"))
	{
		//emit signal_clearSeekList();
		return IT_OTHER;
	}

	else if (line.contains("ENTRY_LIST "))
	{
		QString player = line.section(' ', 1, 1);
		QString condition =
			line.section(' ', 7, 7)+"x"+line.section(' ', 7, 7)+" - " +
			QString::number(int(line.section(' ', 2, 2).toInt()/60))+
			" +" +
			QString::number(int(line.section(' ', 3, 3).toInt()/60)).rightJustified(3) +
			"' (" + line.section(' ', 4, 4) + ") H -"+
			line.section(' ', 8, 8) + " +" + line.section(' ', 9, 9);

		emit signal_SeekList(player,condition);
	}
	return IT_OTHER;
}

GSName Parser::get_gsname()
{
	return gsName;
}

void Parser::set_gsname(const GSName gs)
{
	gsName = gs;
}
