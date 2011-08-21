/*
* globals.h
*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include "defines.h"
#include "parserdefs.h"
#include <QString>
#include <QObject>

/*
* Global structs
*/
class GameData
{
public:
	GameData(QString _playerBlack=QObject::tr("Black"),
		QString _playerWhite=QObject::tr("White"),
		QString _rankBlack="",
		QString _rankWhite="",
		int _size=19,
		int _handicap=0,
		float _komi=5.5,
		int _byoTime=0,
		int _byoStones=25,
		int _timelimit=0,
		int _byoPeriods=0,
		int _gameNumber=0,
		int _style = 1,
		QString _result="",
		QString _date="",
		QString _place="",
		QString _copyright="",
		QString _gameName="",
		QString _fileName="",
		QString _overtime="",
		assessType _freegame=noREQ,
		TimeSystem _timeSystem=canadian,
    bool _oneColorGo=false)
	{
		playerBlack = _playerBlack;
		playerWhite = _playerWhite;
		rankBlack = _rankBlack;
		rankWhite = _rankWhite;
		result = _result;
		date = _date;
		place = _place;
		copyright = _copyright;
		gameName = _gameName;
		fileName = _fileName;
		overtime = _overtime;
		size = _size;
		handicap = _handicap;
		gameNumber = _gameNumber;
		byoTime = _byoTime;
		byoPeriods = _byoPeriods;
		byoStones = _byoStones;
		style = _style;
		timelimit = _timelimit;
		komi = _komi;
		freegame = _freegame;
		timeSystem = _timeSystem;
    oneColorGo = _oneColorGo;
	}
	GameData(GameData *d)
	{
		if (d)
		{
			playerBlack = d->playerBlack;
			playerWhite = d->playerWhite;
			rankBlack = d->rankBlack;
			rankWhite = d->rankWhite;
			result = d->result;
			date = d->date;
			place = d->place;
			copyright = d->copyright;
			gameName = d->gameName;
			fileName = d->fileName;
			overtime = d->overtime;
			size = d->size;
			handicap = d->handicap;
			gameNumber = d->gameNumber;
			byoTime = d->byoTime;
			byoPeriods = d->byoPeriods;
			byoStones = d->byoStones;
			style = d->style;
			timelimit = d->timelimit;
			komi = d->komi;
			freegame = d->freegame;
			timeSystem = d->timeSystem;
      oneColorGo = d->oneColorGo;
		}
	}
	~GameData() {};

	QString    playerBlack, playerWhite, rankBlack, rankWhite, result,
	           date, place, copyright, gameName, fileName, overtime;
	int        size, handicap, gameNumber, byoTime, byoPeriods, byoStones, style, timelimit;
	float      komi;
	assessType freegame;
	TimeSystem timeSystem;
  bool oneColorGo;
};

struct FastLoadMark
{
	int x, y;
	MarkType t;
	QString txt;
};

/*
* Global variables declarations
*/

#ifdef Q_WS_WIN
extern QString applicationPath;
#endif

#endif
