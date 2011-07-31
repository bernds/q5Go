/*
* xmlparser.cpp
*/

#include "xmlparser.h"
#include "boardhandler.h"
#include "board.h"
#include "stonehandler.h"
#include "tree.h"

XMLParser::XMLParser(BoardHandler *bh)
: boardHandler(bh)
{
	CHECK_PTR(boardHandler);
	
	toRemove.setAutoDelete(TRUE);
	stack.setAutoDelete(FALSE);
	movesStack.setAutoDelete(TRUE);
	gameData = NULL;
}

XMLParser::~XMLParser()
{
	clearData();
}

bool XMLParser::startDocument()
{
	// qDebug("START DOCUMENT");
	
	// Init some data
	clearData();
	tree = boardHandler->getTree();
	CHECK_PTR(tree);
	moves = 0;
	var = false;
	isRoot = true;
	isComment = false;
	isCommentPar = false;
	nodeDone = false;
	varStart = false;
	gameData = new GameData;
	
	return true;
}

bool XMLParser::endDocument()
{
	// qDebug("END DOCUMENT");
	clearData();
	return true;
}

void XMLParser::clearData()
{
	commentStr = "";
	stack.clear();
	toRemove.clear();
}

bool XMLParser::convertPosition(const QString &atStr, int &x, int &y)
{
	bool check;
	x = atStr.at(0).latin1() - 'A' + (atStr.at(0).latin1() < 'J' ? 1 : 0);
	y = boardHandler->board->getBoardSize() - atStr.right(atStr.length()-1).toInt(&check) + 1;
	if (!check)
	{
		qWarning("Failed to parse XML attribute: %s", atStr.latin1());
		return false;
	}
	return true;
}

void XMLParser::createNode()
{
	if (!isRoot && !nodeDone)
		boardHandler->createMoveSGF(modeNormal, var);
	var = false;
	isRoot = false;
	nodeDone = true;
	
	if (varStart)
	{
		position = toRemove.pop();
		if (position != NULL)
		{
			boardHandler->getStoneHandler()->removeStone(position->x, position->y);
			boardHandler->updateCurrentMatrix(stoneErase, position->x, position->y);
			// qDebug("Removing %d %d from boardHandler and matrix.", position->x, position->y);
		}
	}
	
	varStart = false;
}

void XMLParser::startVariation()
{
	// qDebug("VARIATION START");
	
	stack.push(tree->getCurrent());
	moveNum = new MoveNum;
	moveNum->n = moves;
	movesStack.push(moveNum);
	varStart = true;
}

void XMLParser::endVariation()
{
	// qDebug("VARIATION END");
	
	if (!movesStack.isEmpty() && !stack.isEmpty())
	{
		Move *m = stack.pop();
		CHECK_PTR(m);
		int x = movesStack.pop()->n;
		
		// qDebug("Var END moves = %d, moves from stack = %d", moves, x);
		
		for (int i=moves; i > x; i--)
		{
			position = toRemove.pop();
			if (position == NULL)
				continue;
			boardHandler->getStoneHandler()->removeStone(position->x, position->y);
			// qDebug("Removing %d %d from stoneHandler.", position->x, position->y);
		}
		
		moves = x;
		
		boardHandler->getStoneHandler()->updateAll(m->getMatrix(), false);
		
		tree->setCurrent(m);
	}
}

bool XMLParser::startElement(const QString&, const QString&,
				     const QString &qName, const QXmlAttributes &atts)
{
	// qDebug("START ELEMENT: %s", qName.latin1());
	
	// Black / White
	if (qName == "Black" || qName == "White")
	{
		StoneColor col = (qName == "Black" ? stoneBlack : stoneWhite);
		int x, y;
		QString atStr = atts.value("at");
		
		/* qDebug("MOVE - COLOR = %s NUMBER = %s, AT = %s",
		col == stoneBlack ? "B" : "W",
		atts.value("number").latin1(),
		atStr.latin1()); */
		
		if (!convertPosition(atStr, x, y))
			return false;
		
		createNode();
		boardHandler->setModeSGF(modeNormal);
		boardHandler->addStoneSGF(col, x, y, true);  // TODO NEWNODE
		
		// Remember this move for later, to remove from the matrix.
		position = new Position;
		position->x = x;
		position->y = y;
		toRemove.push(position);
		moves ++;
	}
	
	// Variation
	else if (qName == "Variation")
		
	{
		startVariation();
		var = true;
	}
	
	// Nodes
	else if (qName == "Nodes")
		startVariation();
	
	// Node
	else if (qName == "Node")
	{
		createNode();
	}
	
	// Comment
	else if (qName == "Comment")
	{
		// qDebug("COMMENT START");
		isComment = true;
	}
	
	// P
	else if (qName == "P" && isComment)
	{
		isCommentPar = true;
	}
	
	// Mark
	else if (qName == "Mark")
	{
		QString atStr = atts.value("at"), typeStr = atts.value("type");
		int x, y;
		
		// qDebug("MARK - TYPE = %s, AT = %s", typeStr.latin1(), atStr.latin1());
		
		if (!convertPosition(atStr, x, y))
			return false;
		
		MarkType markType;
		
		if (typeStr == "triangle")
			markType = markTriangle;
		else if (typeStr == "square")
			markType = markSquare;
		else if (typeStr == "circle")
			markType = markCircle;
		else
			markType = markCross;
		
		tree->getCurrent()->getMatrix()->insertMark(x, y, markType);
	}
	
	// AddBlack / AddWhite
	else if (qName == "AddBlack" || qName == "AddWhite")
	{
		StoneColor col = (qName == "AddBlack" ? stoneBlack : stoneWhite);
		int x, y;
		QString atStr = atts.value("at");
		
		/* qDebug("ADD - COLOR = %s, AT = %s",
		col == stoneBlack ? "B" : "W",
		atStr.latin1()); */
		
		if (!convertPosition(atStr, x, y))
			return false;
		
		boardHandler->setModeSGF(modeEdit);
		boardHandler->addStoneSGF(col, x, y, true);  // TODO NEWNODE
	}
	
	// Delete
	else if (qName == "Delete")
	{
		int x, y;
		QString atStr = atts.value("at");
		
		// qDebug("Delete - AT = %s", atStr.latin1());
		
		if (!convertPosition(atStr, x, y))
			return false;
		
		boardHandler->setModeSGF(modeEdit);
		boardHandler->removeStone(x, y, true, false);
	}
	
	// Game name
	else if (qName == "GoGame")
		gameData->gameName = atts.value("name");
	
	// Board size
	else if (qName == "BoardSize")
		informationState = infoBoardSize;
	
	// Komi
	else if (qName == "Komi")
		informationState = infoKomi;
	
	// Handicap
	else if (qName == "Handicap")
		informationState = infoHandicap;
	
	// Black player
	else if (qName == "BlackPlayer")
		informationState = infoPlayerBlack;
	
	// Black rank
	else if (qName == "BlackRank")
		informationState = infoRankBlack;
	
	// White player
	else if (qName == "WhitePlayer")
		informationState = infoPlayerWhite;
	
	// White rank
	else if (qName == "WhiteRank")
		informationState = infoRankWhite;
	
	// Result
	else if (qName == "Result")
		informationState = infoResult;
	
	// Copyright
	else if (qName == "Copyright")
		informationState = infoCopyright;
	
	
	return true;
}

bool XMLParser::endElement(const QString&, const QString&, const QString &qName)
{
	// qDebug("END ELEMENT: %s", qName.latin1());
	
	// Variation / Nodes
	if (qName == "Variation" || qName == "Nodes")
	{
		endVariation();
	}
	
	// Node
	else if (qName == "Black" || qName == "White" || qName == "Node")
	{
		nodeDone = false;
	}
	
	// Comment
	else if (qName == "Comment")
	{
		// qDebug("COMMENT END");
		isComment = false;
		
		// qDebug("COMMENT %s", commentStr.latin1());
		
		tree->getCurrent()->setComment(commentStr);
		
		commentStr = "";
	}
	
	// P
	else if (qName == "P" && isComment)
	{
		isCommentPar = false;
		commentStr.append("\n");
	}
	
	// Information
	else if (qName == "Information")
	{
		boardHandler->board->initGame(gameData, true);  // Set sgf flag
	}
	     
	
	return true;
}

bool XMLParser::characters(const QString &ch)
{
	if (ch.isEmpty())
		return true;
	
	if (isComment && isCommentPar)
		commentStr += ch;
	
	if (informationState != infoNone)
	{
		bool check;
		
		switch (informationState)
		{
		case infoBoardSize:
			gameData->size = ch.toInt(&check);
			if (!check)
			{
				qWarning("Failed to convert board size.");
				return false;
			}
			// qDebug("READ BOARD SIZE: %d", gameData->size);
			break;
			
		case infoKomi:
			gameData->komi = ch.toFloat(&check);
			if (!check)
			{
				qWarning("Failed to convert komi.");
				return false;
			}
			// qDebug("READ KOMI: %f", gameData->komi);
			break;
			
		case infoHandicap:
			gameData->handicap = ch.toInt(&check);
			if (!check)
			{
				qWarning("Failed to convert handicap.");
				return false;
			}
			// qDebug("READ HANDICAP: %d", gameData->handicap);
			break;
			
		case infoPlayerBlack:
			gameData->playerBlack = ch;
			// qDebug("BLACK PLAYER: %s", gameData->playerBlack.latin1());
			break;
			
		case infoPlayerWhite:
			gameData->playerWhite = ch;
			// qDebug("WHITE PLAYER: %s", gameData->playerWhite.latin1());
			break;
			
		case infoRankBlack:
			gameData->rankBlack = ch;
			// qDebug("BLACK RANK: %s", gameData->rankBlack.latin1());
			break;
			
		case infoRankWhite:
			gameData->rankWhite = ch;
			// qDebug("WHITE RANK: %s", gameData->rankWhite.latin1());
			break;
			
		case infoDate:
			gameData->date = ch;
			// qDebug("DATE: %s", gameData->date.latin1());
			break;
			
		case infoResult:
			gameData->result = ch;
			// qDebug("RESULT: %s", gameData->result.latin1());
			break;
			
		case infoCopyright:
			gameData->copyright = ch;
			// qDebug("COPYRIGHT: %s", gameData->copyright.latin1());
			break;
			
		default:
			break;
		}
		
		informationState = infoNone;
	}
	
	return true;
}

bool XMLParser::parse(QString filename)
{
	if (filename.isEmpty())
		return false;
	
	QFile xmlFile(filename);
	if (!xmlFile.exists())
	{
		qWarning("Did not find file: %s", filename.latin1());
		return false;
	}
	
	QXmlInputSource source(xmlFile);
	QXmlSimpleReader reader;
	reader.setContentHandler(this);
	reader.parse(source);
	
	return true;
}
