/*
 * sgfparser.h
 */

#ifndef SGFPARSER_H
#define SGFPARSER_H

#include <qstring.h>
//Added by qt3to4:
#include <Q3TextStream>

class BoardHandler;
class Tree;
class Q3TextStream;
class Move;
struct ASCII_Import;
class XMLParser;
class GameData;

class SGFParser
{
public:
	SGFParser(BoardHandler *bh);
	~SGFParser();

	bool parse(const QString &fileName, const QString &filter=QString::null, bool fastLoad=false);
	bool parseString(const QString &toParse);
	bool doWrite(const QString &fileName, Tree *tree);
	bool exportSGFtoClipB(QString *str, Tree *tree);
	bool parseASCII(const QString &fileName, ASCII_Import *charset, bool isFilename=true);
    
protected:
	QString loadFile(const QString &fileName);
	bool doParse(const QString &toParseStr, bool fastLoad=false);
	bool corruptSgf(int where=0, QString reason=QString::null);
	int minPos(int n1, int n2, int n3);
	bool initGame(const QString &toParse, const QString &fileName);
	bool parseProperty(const QString &toParse, const QString &prop, QString &result);
	void traverse(Move *t, GameData *gameData);
	void writeGameHeader(GameData *gameData);
	bool parseASCIIStream(Q3TextStream *stream, ASCII_Import *charset);
	bool doASCIIParse(const QString &toParse, int &y, ASCII_Import *charset);
	bool checkBoardSize(const QString &toParse, ASCII_Import *charset);
//	void convertOldSgf(QString &toParse);
	bool initStream(Q3TextStream *stream);
	bool writeStream(Tree *tree);

private:
	BoardHandler *boardHandler;
	Q3TextStream *stream;
	bool isRoot;
	XMLParser *xmlParser;
	int asciiOffsetX, asciiOffsetY;
};

#endif
