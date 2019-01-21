/*
 *   parser.h
 */

#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QObject>
#include "gs_globals.h"
#include "tables.h"
#include "qgo_interface.h"

class Player;
class Game;
class GameInfo;

class Parser : public QObject
{
	Q_OBJECT

signals:
	// emit if info is found
	void signal_player(Player*, bool);
  	void signal_statsPlayer(Player*);
	void signal_game(Game*);
	void signal_gamemove(Game*);
	void signal_move(GameInfo*);
	void signal_message(QString);
	void signal_svname(GSName&);
	void signal_accname(QString&);
	void signal_status(Status);
	void signal_connclosed();
	void signal_talk(const QString&, const QString&, bool);
	void signal_checkbox(int, bool);
	void signal_channelinfo(int, const QString&);
	void signal_kibitz(int, const QString&, const QString&);
	void signal_title(const GameInfo *, const QString&);
	void signal_komi(const QString&, const QString&, bool);
	void signal_freegame(bool);
	void signal_suggest(const QString&, const QString&, const QString&, const QString&, int);
	void signal_matchrequest(const QString&, bool);
	void signal_matchCanceled(const QString&);
	void signal_matchcreate(const QString&, const QString&);
	void signal_notopen(const QString&);
	void signal_removestones(const QString&, const QString&);
	void signal_komirequest(const QString&, int, float, bool);
	void signal_opponentopen(const QString&);
	void signal_result(const QString&, const QString&, bool, const QString&);
	void signal_requestDialog(const QString&, const QString&, const QString&, const QString&);
	void signal_undo(const QString&, const QString&);
	void signal_addToObservationList(int);
	void signal_shout(const QString&, const QString&);
	void signal_timeAdded(int, bool);
	void signal_room(const QString&, bool );
	void signal_addSeekCondition(const QString&, const QString&, const QString&, const QString&, const QString&);
	void signal_clearSeekCondition();
	void signal_cancelSeek();
	void signal_SeekList(const QString&, const QString&);
	void signal_refresh(int);
	void signal_dispute(const QString&, const QString&);
	void signal_set_observe(const QString&);
	//void signal_undoRequest(const QString&);

public:
	Parser();
	~Parser();
	InfoType  put_line(const QString&);
//	Player    get_player();
	Player    *get_statsPlayer() {return statsPlayer ;}
	Game	  get_game();

	void       set_gsname(const GSName);
	GSName     get_gsname();
	void       set_myname(const QString &n) { myname = n; }
	QString    get_buffer();
	InfoType   cmdsent(const QString&);

private:
	InfoType   cmd1(const QString&);
	InfoType   cmd2(const QString&);
	InfoType   cmd5(const QString&);
	InfoType   cmd7(const QString&);
	InfoType   cmd8(const QString&);
	InfoType   cmd9(QString&);
	InfoType   cmd11(const QString&);
	InfoType   cmd14(const QString&);
	InfoType   cmd15(const QString&);
	InfoType   cmd19(const QString&);
	InfoType   cmd20(const QString&);
	InfoType   cmd21(const QString&);
	InfoType   cmd22(const QString&);
	InfoType   cmd24(const QString&);
	InfoType   cmd27(const QString&);
	InfoType   cmd28(const QString&);
	InfoType   cmd32(const QString&);
	InfoType   cmd42(const QString&);
	InfoType   cmd48(const QString&);
	InfoType   cmd49(const QString&);
	InfoType   cmd63(const QString&);
	QString    myname;
	QString    buffer;
	GSName     gsName;
	Player     *aPlayer;
	Player     *statsPlayer;
	Game       *aGame;
	GameInfo   *aGameInfo;
	int        memory;
	QString    memory_str;
};

#endif
