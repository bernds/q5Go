/*
* move.h
*/

#ifndef MOVE_H
#define MOVE_H

#include "globals.h"
#include <qstring.h>
#include <q3intdict.h>

class Matrix;

class Move
{
public:
	Move(int board_size);
	Move(StoneColor c, int mx, int my, int n, GameMode mode, const Matrix &mat, const QString &s=NULL);
	Move(StoneColor c, int mx, int my, int n, GameMode mode, const QString &s=NULL);
	~Move();
	
	bool equals(Move *m);
	int getX() const { return x; }
	int getY() const { return y; }
	void setX(int n) { x = n; }
	void setY(int n) { y = n; }
	StoneColor getColor() const { return stoneColor; }
	void setColor(StoneColor c) { stoneColor = c; }
	int getCapturesBlack() const { return capturesBlack; }
	int getCapturesWhite() const { return capturesWhite; }
	void setCaptures(int cb, int cw) { capturesBlack = cb; capturesWhite = cw; }
	Matrix* getMatrix() { return matrix; }
	void setMatrix(Matrix *m) { matrix = m; }
	void setMoveNumber(int n) { moveNum = n; }
	int getMoveNumber() const { return moveNum; }
	GameMode getGameMode() const { return gameMode; }
	void setGameMode(GameMode m) { gameMode = m; }
	QString &getNodeName() { return nodeName; }
	void setNodeName(const QString &s) { nodeName = s; }
	QString &getComment() { return comment; }
	void setComment(const QString &s) {
		comment = s;
#if (QT_VERSION >= 0x030200)
		comment.squeeze();
#endif

}
	QString &getUnknownProperty() { return unknownProperty; }
	void setUnknownProperty(const QString &s) { unknownProperty = s; }
	const QString saveMove(bool isRoot);
	bool isTerritoryMarked() const { return terrMarked; }
	void setTerritoryMarked(bool b=true) { terrMarked = b; }
	void insertFastLoadMark(int x, int y, MarkType markType, const QString &txt=0);
	bool isScored() const { return scored; }
	void setScore(float b, float w) { scored = true; scoreBlack = b; scoreWhite = w; }
	void setScored(bool b=true) { scored = b; }
	float getScoreBlack() const { return scoreBlack; }
	float getScoreWhite() const { return scoreWhite; }
	void setOpenMoves(int mv) { openMoves = mv; }
	int getOpenMoves() { return openMoves; }
	void setTimeLeft(float time) { timeLeft = time; }
	float getTimeLeft() { return timeLeft; }
	void setTimeinfo(bool ti) { timeinfo = ti; }
	bool getTimeinfo() { return timeinfo; }
	// PL[] info: show if stone color keeps equal
	void clearPLinfo() { PLinfo = false; }
	void setPLinfo(StoneColor sc) { PLinfo = true; PLnextMove = sc; }
	bool getPLinfo() { return PLinfo; }
	StoneColor getPLnextMove() { return PLnextMove; }
	
	Move *brother, *son, *parent, *marker;
	bool checked;
	Q3IntDict<FastLoadMark> *fastLoadMarkDict;
  bool isPassMove();

  
private:
	StoneColor stoneColor, PLnextMove;
	int x, y, moveNum, capturesBlack, capturesWhite, openMoves;
	float scoreBlack, scoreWhite;
	float timeLeft;
	Matrix *matrix;
	GameMode gameMode;
	QString comment;
	QString unknownProperty;
	QString nodeName;
	bool terrMarked, scored, timeinfo, PLinfo;
};

#endif
