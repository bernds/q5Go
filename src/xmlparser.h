/*
* xmlparser.h
*/

#include <qxml.h>
#include "parserdefs.h"
#include "move.h"
#include <q3ptrstack.h>

class BoardHandler;
class Tree;
class GameData;

enum InformationState
{
	infoNone, infoBoardSize, infoHandicap, infoKomi, infoPlayerBlack, infoPlayerWhite,
		infoRankBlack, infoRankWhite, infoDate, infoResult, infoCopyright
};

class XMLParser : public QXmlDefaultHandler
{
public:
	XMLParser(BoardHandler *bh);
	~XMLParser();
	
	bool parse(QString filename);
	
protected:
	void clearData();
	bool convertPosition(const QString &atStr, int &x, int &y);
	void createNode();
	void startVariation();
	void endVariation();
	bool startDocument();
	bool endDocument();
	bool startElement(const QString&, const QString&, const QString&, const QXmlAttributes&);
	bool endElement(const QString&, const QString&, const QString&);
	bool characters(const QString&);
	
private:
	BoardHandler *boardHandler;
	Q3PtrStack<Position> toRemove;
	Q3PtrStack<Move> stack;
	Q3PtrStack<MoveNum> movesStack;
	int moves;
	Position *position;
	MoveNum *moveNum;
	Tree *tree;
	bool var, isRoot, isComment, isCommentPar, nodeDone, varStart;
	QString commentStr;
	GameData *gameData;
	InformationState informationState;
};
