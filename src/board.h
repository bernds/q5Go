/*
 * board.h
 */

#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <map>

#include <QGraphicsView>
#include <QDateTime>
#include <QPainter>
#include <Q3PtrList>
#include <Q3IntDict>
//Added by qt3to4:
#include <QWheelEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>

#include "defines.h"
#include "setting.h"
#include "boardhandler.h"
#include "stone.h"
#include "move.h"
#include "gatter.h"

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class GameData;
class NodeResults;
class QNewGameDlg;


class Board : public QGraphicsView
{
	Q_OBJECT

public:
	Board(QWidget *parent=0, QGraphicsScene *c = 0);
	~Board();
	void clearData();
	void initGame(GameData *d, bool sgf=false);
	void setModified(bool m=true);
	void updateCaption();
	ImageHandler* getImageHandler() { return imageHandler; }
	BoardHandler* getBoardHandler() { return boardHandler; }
	InterfaceHandler* getInterfaceHandler();
	Stone* addStoneSprite(StoneColor c, int x, int y, bool &shown);
	void updateCanvas() { canvas->update(); }
	void increaseSize();
	void decreaseSize();
	QString getCandidateFileName();
	void hideAllStones();
	void showAllStones();
	void hideAllMarks();
	bool openSGF(const QString &fileName);
	bool saveBoard(const QString &fileName) { return boardHandler->saveBoard(fileName); }
	void setShowCoords(bool b);
	void setShowSGFCoords(bool b);
	void setShowCursor(bool b);
	void setVariationDisplay(VariationDisplay d);
	void setMode(GameMode mode) { boardHandler->setMode(mode); }
	void addStone(StoneColor sc, int x, int y, bool sound = 0) { boardHandler->addStone(sc, x, y, sound); }
	void setHandicap(int h) { boardHandler->setHandicap(h); }
	GameMode getGameMode() { return boardHandler->getGameMode(); }
	void setMarkType(MarkType t) { boardHandler->setMarkType(t); }
	MarkType getMarkType() const { return boardHandler->getMarkType(); }
	void setMark(int x, int y, MarkType t, bool update=true, QString txt = QString::null, bool overlay=true);
	void removeMark(int x, int y, bool update=true);
	void setMarkText(int x, int y, const QString &txt);
	Mark* hasMark(int x, int y);
	void updateLastMove(StoneColor c, int x, int y);
	void setCurStoneColor();
	void removeLastMoveMark();
	void checkLastMoveMark(int x, int y);
	bool nextMove(bool autoplay=false) { return boardHandler->nextMove(autoplay); }
	void previousMove() { boardHandler->previousMove(); }
	void nextVariation() { boardHandler->nextVariation(); }
	void previousVariation() { boardHandler->previousVariation(); }
	void nextComment() { boardHandler->nextComment(); }
	void previousComment() { boardHandler->previousComment(); }
	void gotoFirstMove() { boardHandler->gotoFirstMove(); }
	void gotoLastMove() { boardHandler->gotoLastMove(); }
	void gotoLastMoveByTime() { boardHandler->gotoLastMoveByTime(); }
	void gotoMainBranch() { boardHandler->gotoMainBranch(); }
	void gotoVarStart() { boardHandler->gotoVarStart(); }
	void gotoNextBranch() { boardHandler->gotoNextBranch(); }
	void gotoNthMove(int n) { boardHandler->gotoNthMove(n); }
	void gotoNthMoveInVar(int n) { boardHandler->gotoNthMoveInVar(n); }
	void navIntersection();
	void cutNode() { boardHandler->cutNode(); }
	void pasteNode(bool brother=false) { boardHandler->pasteNode(brother); }
	void deleteNode() { boardHandler->deleteNode(); }
	void clearNode() { boardHandler->clearNode(); }
	void duplicateNode() { boardHandler->duplicateNode(); }
	bool swapVariations() { return boardHandler->swapVariations(); }
	void doPass();
	void doSinglePass() { boardHandler->doPass(); }
	void doAdjourn() { emit signal_adjourn(); }
	void doUndo() { emit signal_undo(); }
	void doResign();// { emit signal_resign(); }
	void doRefresh() { emit signal_refresh(); }
	void doEditBoardInNewWindow() { emit signal_editBoardInNewWindow(); }
	void setVarGhost(StoneColor c, int x, int y);
	bool hasVarGhost(StoneColor c, int x, int y);
	void removeGhosts();
	short getCurrentX() const { return curX; }
	short getCurrentY() const { return curY; }
	int getCurrentMoveNumber() const;
	GameData* getGameData() { return boardHandler->getGameData(); }
	void setGameData(GameData *gd) { boardHandler->setGameData(gd); }
	void exportPicture(const QString &fileName, const QString &filter, bool toClipboard=false);
	void exportASCII() { boardHandler->exportASCII(); }
	bool importASCII(const QString &fileName, bool fromClipboard=false)
		{ return boardHandler->importASCII(fileName, fromClipboard); }
	bool importSGFClipboard() { return boardHandler->importSGFClipboard(); }
	bool exportSGFtoClipB() { return boardHandler->exportSGFtoClipB(); }
	void doCountDone();
	void numberMoves() { boardHandler->numberMoves(); }
	void markVariations(bool sons) { boardHandler->markVariations(sons); }
	void setBoardSize(int s) { board_size = s; }
	int getBoardSize() const { return board_size; }
#ifndef NO_DEBUG
	void debug();
#endif

	bool fastLoad, isModified, lockResize;
	void sendcomment(const QString &text) { emit signal_sendcomment(text); }

	// in case of match
	void set_myColorIsBlack(bool b) { myColorIsBlack = b; }
	bool get_myColorIsBlack() { return myColorIsBlack; }
	void set_isLocalGame(bool isLocal);
	bool get_isLocalGame() { return isLocalGame; }
	void refreshDisplay() { Move *m = boardHandler->getTree()->getCurrent(); updateLastMove(m->getColor(), m->getX(), m->getY()); }
	bool startComputerPlay(QNewGameDlg *dlg,const QString &fileName, const QString &computer_path);
	void set_antiClicko(bool b) { antiClicko = b; }


public slots:
	void updateComment();
	void updateComment2();
	void modifiedComment();
	void changeSize();
	void gotoMove(Move *m) { boardHandler->gotoMove(m); }

signals:
	void coordsChanged(int, int, int,bool);
	void signal_sendcomment(const QString&);
	void signal_addStone(enum StoneColor, int, int);
	void signal_Stone_Computer(enum StoneColor, int, int);
	void signal_undo();
	void signal_adjourn();
	void signal_resign();
	void signal_pass();
	void signal_done();
	void signal_refresh();
	void signal_editBoardInNewWindow();

protected:
	void calculateSize();
	void drawBackground();
	void drawGatter();
	void drawCoordinates();
	void drawStarPoint(int x, int y);
	void resizeBoard(int w, int h);
	int convertCoordsToPoint(int c, int o);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent *e);
	void mouseWheelEvent(QWheelEvent *e);
	void leaveEvent(QEvent*);
	void resizeEvent(QResizeEvent*);
	void updateMarkColor(StoneColor c, int x, int y);

private:
	void setupCoords();
	void clearCoords();

	QGraphicsScene *canvas;
	QList<QGraphicsSimpleTextItem*> hCoords1, hCoords2 ,vCoords1, vCoords2;
	Gatter *gatter;
	ImageHandler *imageHandler;
	BoardHandler *boardHandler;
	static const int margin, coord_margin;
	int board_size, offset, offsetX, offsetY, board_pixel_size, table_size;
	int coord_offset;
	double square_size;
	bool showCoords;
	bool showSGFCoords;
	bool showCursor;
	bool antiClicko;
	bool gatter_created;
	Q3PtrList<Mark> *marks;
	Q3PtrList<Stone> *ghosts;
	Mark *lastMoveMark;
	bool numberPool[400];
	bool letterPool[52];
	Stone *curStone;
	short curX, curY;

	QTime wheelTime;
	Qt::ButtonState mouseState;
	NodeResults *nodeResultsDlg;

	bool isHidingStones; // QQQ

#ifdef Q_WS_WIN
	bool resizeDelayFlag;
#endif

	// in case of match
	bool myColorIsBlack;
	bool isLocalGame;
	bool navIntersectionStatus;
};

#endif
