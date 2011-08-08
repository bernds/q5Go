/*
 * boardhandler.h
 */

#ifndef BOARDHANDLER_H
#define BOARDHANDLER_H

#include "defines.h"
#include "tree.h"
#include "globals.h"
#include "setting.h"
#include "sgfparser.h"
#include "stonehandler.h"           //SL added eb 10
#include "qgtp.h"
#include <q3ptrstack.h>

#define MARK_TERRITORY_DONE_BLACK 997
#define MARK_TERRITORY_DONE_WHITE 998

class Board;
class Move;
//class StoneHandler;
class SGFParser;
class Matrix;
class GameData;
class InterfaceHandler;
class QGtp;             //SL added eb 12
class QNewGameDlg;             //SL added eb 12

class BoardHandler
{
public:
	BoardHandler(Board *b);
	~BoardHandler();

	void updateMove(Move *m=0, bool ignore_update = false);
  
	void clearData();

	void checkAllPositions() {stoneHandler->checkAllPositions();}       //SL added eb 10
	StoneHandler* getStoneHandler() const { return stoneHandler; } 
	Tree* getTree() const { return tree; }
	QGtp * getGtp() const { return gtp ;}
	void initGame(GameData* d, bool sgf=false);
	void prepareBoard();
	void prepareComputerBoard();
	void setHandicap(int handicap);
	GameData* getGameData() { return gameData; }
	void setGameData(GameData *gd) { gameData = gd; }
	int hasStone(int x, int y);
	void addStone(StoneColor c, int x, int y, bool sound = true);
	void addStoneSGF(StoneColor c, int x, int y, bool new_node=true);
	bool removeStone(int x, int y, bool hide=false, bool new_node=true);
	void removeDeadStone(int x, int y);
	bool nextMove(bool autoplay=false);
	void previousMove();
	void nextVariation();
	void previousVariation();
	void nextComment();           //added eb
	void previousComment();       //end add
	void gotoFirstMove();
	void gotoLastMove();
	void gotoLastMoveByTime();
	void gotoMainBranch();
	void gotoVarStart();
	void gotoNextBranch();
	void gotoNthMove(int n);
	void gotoNthMoveInVar(int n);
	void gotoMove(Move *m);
	void cutNode();
	void pasteNode(bool brother=false);
	void deleteNode();
	void clearNode(bool brother=true);
	void duplicateNode();
	bool swapVariations();
	void doPass(bool sgf=false);
	bool getBlackTurn();
	GameMode getGameMode() const { return gameMode; }
	void setMode(GameMode mode);
	void setModeSGF(GameMode mode) { gameMode = mode; }
	MarkType getMarkType() const { return markType; }
	void setMarkType(MarkType t) { markType = t; }
	void editMark(int x, int y, MarkType t, const QString &txt=QString::null);
	int getNumBrothers() { return tree->getNumBrothers(); }
	int getNumSons() { return tree->getNumSons(); }
	bool hasParent();
	bool hasPrevBrother() { return tree->hasPrevBrother(); }
	bool hasNextBrother() { return tree->hasNextBrother(); }
	void updateComment(QString text=QString::null);
	void updateCurrentMatrix(StoneColor c, int x, int y);
	bool loadSGF(const QString &fileName);
	bool openComputerSession(QNewGameDlg *dlg, const QString &fileName,
				 const QString &computer_path=QString::null); //SL added eb 12
	bool saveBoard(const QString &fileName);
	void exportASCII();
	bool importASCII(const QString &fileName, bool fromClipBoard=false);
	bool importSGFClipboard();
	bool exportSGFtoClipB();
	void createMoveSGF(GameMode mode=modeNormal, bool brother=false);
	void setCaptures(StoneColor c, int n);
	void countScore();
	void enterScoreMode(int cb, int cw);
	void leaveScoreMode();
	void markDeadStone(int x, int y);
	void markSeki(int x, int y);
	void findMoveByPos(int x, int y);
	void findMoveByPosInVar(int x, int y);
	bool findMoveInVar(int x, int y);
	void numberMoves();
	void markVariations(bool sons);
 	Move *lastValidMove;
  
#ifndef NO_DEBUG
	void debug();
#endif

	Board *board;
	bool markedDead;
	bool local_stone_sound;

	bool display_incoming_move;     //SL added eb 9
	bool getDisplay_incoming_move() { return display_incoming_move;} ;  //SL added eb 9
  
	Q3PtrStack<Move> *nodeResults;

protected:
	void addMove(StoneColor c, int x, int y, bool clearMarks = true);
	void editMove(StoneColor c, int x, int y);
//	void updateMove(Move *m=0);
	void updateVariationGhosts();
	void createNode(const Matrix &mat, bool brother=false, bool setEditMode=true);
	void traverseTerritory(Matrix *m, int x, int y, StoneColor &col);
	bool checkNeighbourTerritory(Matrix *m, const int &x, const int &y, StoneColor &col);

private:
	StoneHandler *stoneHandler;               
	SGFParser *sgfParser;
	int currentMove, capturesBlack, capturesWhite, caps_black, caps_white;
	Tree *tree;
//	Move *lastValidMove;                      //SL added eb 9
	GameMode gameMode;
	MarkType markType;
	GameData *gameData;
	Move *clipboardNode;
	QGtp * gtp;                             //SL added eb 12
};

#endif
