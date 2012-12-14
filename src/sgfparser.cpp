/*
* sgfparser.cpp
*/

#include "qgo.h"
#include "sgfparser.h"
#include "boardhandler.h"
#include "config.h"
#include "board.h"
#include "move.h"
#include "tree.h"
#include "stonehandler.h"
#include "matrix.h"
#include "globals.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <qfile.h>
#include <q3textstream.h>
#include <q3progressdialog.h>
#include "parserdefs.h"
#include <qmessagebox.h>
#include <q3strlist.h>
#include <qregexp.h>
#include <q3ptrstack.h>
#include <algorithm>

#define STR_OFFSET 2000
/* #define DEBUG_CODEC */


// Note: This class is a DIRTY UGLY HACK.
// It probably does all the stuff books tell you never ever to do.
// But it speeds up sgf reading of large files (Kogo) significantly.

class MyString
{
public:
	MyString()
	{
	}

	MyString(const QString &src)
	{
		Str = src;
		strLength = src.length();
	}

	virtual ~MyString()
	{
	}

	virtual const QChar at(uint i) const { return Str.at(i); }

	virtual uint next_nonspace (uint i) const
	{
		// ignore lower characters, too
		while (at(i).isSpace() || // == ' ' || at(i) == '\n' || at(i) == '\t' ||
			   at(i) >= 'a' && at(i) <= 'z')
			i++;
		return i;
	}

	virtual int find(const char *c, unsigned int index) const
	{
		return Str.indexOf (c, index);
	}

	virtual int find(char c, unsigned int index) const
	{
		return Str.indexOf (c, index);
	}

	virtual unsigned int length() const
	{
		return strLength;
	}

	unsigned int strLength;

private:
	QString Str;
};

class MySimpleString : public MyString
{
public:
	MySimpleString(const QString &src)
	{
		Str = src.latin1();
		strLength = src.length();
	}

	virtual ~MySimpleString()
	{
	}

	const QChar at(uint i) const { return Str[i]; }

	int find(const char *c, unsigned int index) const
	{
		if (index >= strLength)
			return -1;

		// Offset. Hope that is enough. TODO Long comments check?
		unsigned int l = index+STR_OFFSET<strLength ? index+STR_OFFSET : strLength,
			cl = strlen(c),
			i,
			found;

		do {
			found = i = 0;
			do {
				if (Str[index+i] != c[i])
					break;
				found ++;
				if (found == cl)
					return index;
			} while (i++ < cl);
		} while (index++ < l);
		if (index == l)
			return -1;
		return index;
	}

	int find(char c, unsigned int index) const
	{
		if (index >= strLength)
			return -1;

		// Offset. Hope that is enough. TODO Long comments check?
		unsigned int l = index+STR_OFFSET<strLength ? index+STR_OFFSET : strLength;

		while (Str[index] != c && index++ < l);
		if (index == l)
			return -1;
		return index;
	}

private:
	const char* Str;
};
// End dirty ugly hack. :*)

SGFParser::SGFParser(BoardHandler *bh)
: boardHandler(bh)
{
	CHECK_PTR(boardHandler);
	stream = NULL;
}

SGFParser::~SGFParser()
{
}

bool SGFParser::parse(const QString &fileName)
{
	if (fileName.isNull() || fileName.isEmpty())
	{
		qWarning("No filename given!");
		return false;
	}

	CHECK_PTR(boardHandler);

	QString toParse = loadFile(fileName);
	if (toParse.isNull() || toParse.isEmpty())
		return false;
	// Convert old sgf/mgt format into new
//	if (toParse.find("White[") != -1)  // Do a quick test if this is necassary.
//		convertOldSgf(toParse);

	if (!initGame(toParse, fileName))
		return corruptSgf();
	return doParse(toParse);
}

// Called from clipboard SGF import
bool SGFParser::parseString(const QString &toParse)
{
	if (toParse.isNull() || toParse.isEmpty() || !initGame(toParse, NULL))
		return corruptSgf();
	return doParse(toParse);
}

QString SGFParser::loadFile(const QString &fileName)
{
	qDebug() << "Trying to load file " << fileName;

	QFile file(fileName);

	if (!file.exists())
	{
		QMessageBox::warning(0, PACKAGE, Board::tr("Could not find file:") + " " + fileName);
		return NULL;
	}

	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(0, PACKAGE, Board::tr("Could not open file:") + " " + fileName);
		return NULL;
	}

	Q3TextStream txt(&file);
	if (!initStream(&txt))
	{
		QMessageBox::critical(0, PACKAGE, Board::tr("Invalid text encoding given. Please check preferences!"));
		return NULL;
	}

	QString toParse;

	// Read file in string toParse
	while (!txt.eof())
		toParse.append(txt.readLine() + "\n");

	file.close();
#ifdef DEBUG_CODEC
	QMessageBox::information(0, "READING", toParse);
#endif

	return toParse;
}

bool SGFParser::initStream(Q3TextStream *stream)
{
	QTextCodec *codec = NULL;

	stream->setEncoding(Q3TextStream::Locale);
#ifdef DEBUG_CODEC
	QMessageBox::information(0, "LOCALE", QTextCodec::locale());
#endif

	switch (static_cast<Codec>(setting->readIntEntry("CODEC")))
	{
	case codecNone:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "NONE");
#endif
		break;

	case codecBig5:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "Big5");
#endif
		codec = QTextCodec::codecForName("Big5");
		break;

	case codecEucJP:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "EUC JP");
#endif
		codec = QTextCodec::codecForName("EUC-JP");
		break;

	case codecJIS:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "JIS");
#endif
		/* ??? */
		codec = QTextCodec::codecForName("JIS X 0201");
		break;

	case codecSJIS:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "Shift JIS");
#endif
		codec = QTextCodec::codecForName("Shift-JIS");
		break;

	case codecEucKr:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "EUC KR");
#endif
		codec = QTextCodec::codecForName("EUC-KR");
		break;

	case codecGBK:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "GBK");
#endif
		/* ??? */
		codec = QTextCodec::codecForName("GB18030-0");
		break;

	case codecTscii:
#ifdef DEBUG_CODEC
		QMessageBox::information(0, "CODEC", "TSCII");
#endif
		codec = QTextCodec::codecForName("TSCII");
		break;

	default:
		return false;
	}

	if (codec != NULL && stream != NULL)
		stream->setCodec(codec);

	return true;
}

bool SGFParser::doParse(const QString &toParseStr)
{
	if (toParseStr.isNull() || toParseStr.isEmpty())
	{
		qWarning("Failed loading from file. Is it empty?");
		return false;
	}

	const MyString *toParse = NULL;
	if (static_cast<Codec>(setting->readIntEntry("CODEC")) == codecNone)
		toParse = new MySimpleString(toParseStr);
	else
		toParse = new MyString(toParseStr);
	CHECK_PTR(toParse);

	int pos = 0,
		posVarBegin = 0,
		posVarEnd = 0,
		posNode = 0,
		moves = 0,
		i, x=-1, y=-1;
	unsigned int pointer = 0,
		strLength = toParse->length();
	bool black = true,
		setup = false,
		old_label = false,
		new_node = false;
	isRoot = true;
	bool remember_root;
	QString unknownProperty;
	State state;
	MarkType markType;
	QString moveStr, commentStr;
	Position *position;
	MoveNum *moveNum;
	Q3PtrStack<Move> stack;
	Q3PtrStack<MoveNum> movesStack;
	Q3PtrStack<Position> toRemove;
	stack.setAutoDelete(FALSE);
	movesStack.setAutoDelete(TRUE);
	toRemove.setAutoDelete(TRUE);
	Tree *tree = boardHandler->getTree();

	state = stateVarBegin;

	bool cancel = false;
	int progressCounter = 0;
	Q3ProgressDialog progress(Board::tr("Reading sgf file..."), Board::tr("Abort"), strLength,
		boardHandler->board, "progress", true);

	// qDebug("File length = %d", strLength);

	progress.setProgress(0);
	QString sss="";
	do {
		if (!(++progressCounter%10))
		{
			progress.setProgress(pointer);
			if (progress.wasCancelled())
			{
				cancel = true;
				break;
			}
		}

		// qDebug("POINTER = %d: %c", pointer, toParse->Str[pointer]);

		posVarBegin = toParse->find('(', pointer);
		posVarEnd = toParse->find(')', pointer);
		posNode = toParse->find(';', pointer);

		pos = minPos(posVarBegin, posVarEnd, posNode);
		// qDebug("VarBegin %d, VarEnd %d, Move %d, MINPOS %d", posVarBegin, posVarEnd, posNode, pos);

		// qDebug("State before switch = %d", state);

		// Switch states

		// Node -> VarEnd
		if (state == stateNode && pos == posVarEnd)
			state = stateVarEnd;

		// Node -> VarBegin
		if (state == stateNode && pos == posVarBegin)
			state = stateVarBegin;

		// VarBegin -> Node
		else if (state == stateVarBegin && pos == posNode)
			state = stateNode;

		// VarEnd -> VarBegin
		else if (state == stateVarEnd && pos == posVarBegin)
			state = stateVarBegin;

		// qDebug("State after switch = %d", state);

		// Do the work
		switch (state)
		{
		case stateVarBegin:
			if (pos != posVarBegin)
			{
				delete toParse;
				return corruptSgf(pos);
			}

			// qDebug("Var BEGIN at %d, moves = %d", pos, moves);

			stack.push(tree->getCurrent());
			moveNum = new MoveNum;
			moveNum->n = moves;
			movesStack.push(moveNum);
			pointer = pos + 1;
			break;

		case stateVarEnd:
			if (pos != posVarEnd)
			{
				delete toParse;
				return corruptSgf(pos);
			}

			// qDebug("VAR END");

			if (!movesStack.isEmpty() && !stack.isEmpty())
			{
				Move *m = stack.pop();
				CHECK_PTR(m);
				x = movesStack.pop()->n;

				// qDebug("Var END at %d, moves = %d, moves from stack = %d", pos, moves, x);

				for (i=moves; i > x; i--)
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
			pointer = pos + 1;
			break;

		case stateNode:
			if (pos != posNode)
			{
				delete toParse;
				return corruptSgf(pos);
			}

			// qDebug("Node at %d", pos);
			commentStr = QString();
			setup = false;
			markType = markNone;

			// Create empty node
			remember_root = isRoot;
			if (!isRoot)
			{
				boardHandler->createMoveSGF();
				unknownProperty = QString();
if (tree->getCurrent()->getTimeinfo())
	qWarning("*** Timeinfo set !!!!");
				//tree->getCurrent()->setTimeinfo(false);
			}
			else
				isRoot = false;

			new_node = true;

			Property prop;
			pos ++;

			do {
				uint tmppos=0;
				pos = toParse->next_nonspace (pos);

				// qDebug("READING PROPERTY AT %d: %c", pos, toParse->at(pos));

				// if (toParse->find("B[", pos) == pos)
				if (toParse->at(pos) == 'B' && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = moveBlack;
					pos = tmppos;
					black = true;
				}
				// else if (toParse->find("W[", pos) == pos)
				else if (toParse->at(pos) == 'W' && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = moveWhite;
					pos = tmppos;
					black = false;
				}
				else if (toParse->at(pos) == 'N' && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = nodeName;
					pos = tmppos;
				}
				else if (toParse->find("AB", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editBlack;
					pos = tmppos;
					setup = true;
					black = true;
				}
				else if (toParse->find("AW", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editWhite;
					pos = tmppos;
					setup = true;
					black = false;
				}
				else if (toParse->find("AE", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editErase;
					pos = tmppos;
					setup = true;
				}
				else if (toParse->find("TR", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markTriangle;
					pos = tmppos;
				}
				else if (toParse->find("CR", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markCircle;
					pos = tmppos;
				}
				else if (toParse->find("SQ", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markSquare;
					pos = tmppos;
				}
				else if (toParse->find("MA", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markCross;
					pos = tmppos;
				}
				// old definition
				else if (toParse->find("M", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = editMark;
					markType = markCross;
					pos = tmppos;
				}
				else if (toParse->find("LB", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markText;
					pos = tmppos;
					old_label = false;
				}
				// Added old L property. This is not SGF4, but many files contain this tag.
				else if (toParse->at(pos) == 'L' && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = editMark;
					markType = markText;
					pos = tmppos;
					old_label = true;
				}
				else if (toParse->at(pos) == 'C' && toParse->at(tmppos = toParse->next_nonspace (pos + 1)) == '[')
				{
					prop = comment;
					pos = tmppos;
				}
				else if (toParse->find("TB", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markTerrBlack;
					pos = tmppos;
					black = true;
				}
				else if (toParse->find("TW", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = editMark;
					markType = markTerrWhite;
					pos = tmppos;
					black = false;
				}
				else if (toParse->find("BL", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = timeLeft;
					pos = tmppos;
					black = true;
				}
				else if (toParse->find("WL", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = timeLeft;
					pos = tmppos;
					black = false;
				}
				else if (toParse->find("OB", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = openMoves;
					pos = tmppos;
					black = true;
				}
				else if (toParse->find("OW", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = openMoves;
					pos = tmppos;
					black = false;
				}
				else if (toParse->find("PL", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = nextMove;
					pos = tmppos;
				}
        else if (toParse->find("RG", pos) == pos && toParse->at(tmppos = toParse->next_nonspace (pos + 2)) == '[')
				{
					prop = unknownProp;
					pos = tmppos;
          setup = true;
				}
				// Empty node
				else if (toParse->at(pos) == ';' || toParse->at(pos) == '(' || toParse->at(pos) == ')')
				{
					qDebug("Found empty node at %d", pos);
					while (toParse->at(pos).isSpace())
						pos++;
					continue;
				}
				else
				{
					// handle like comment
					prop = unknownProp;
					pos = toParse->next_nonspace (pos);
//qDebug("SGF: next nonspace (1st):" + QString(toParse->at(pos)) + QString(toParse->at(pos+1)) + QString(toParse->at(pos+2)));
				}

				// qDebug("FOUND PROP %d, pos at %d now", prop, pos);

				// Next is one or more '[xx]'.
				// Only one in a move property, several in a setup propery
				do {
					if (toParse->at(pos) != '[' && prop != unknownProp)
					{
						delete toParse;
						return corruptSgf(pos);
					}

					// Empty type
					if (toParse->at(pos+1) == ']')
					{
						// CGoban stores pass as 'B[]' or 'W[]'
						if (prop == moveBlack || prop == moveWhite)
						{
							boardHandler->doPass(true);

							// Remember this move for later, to remove from the matrix.
							position = new Position;
							position->x = x;
							position->y = y;
							toRemove.push(position);
							moves ++;
						}

						pos += 2;
						continue;
					}

					switch (prop)
					{
					case moveBlack:
					case moveWhite:
						// rare case: root contains move or placed stone:
						if (remember_root)
						{
							qDebug() << "root contains stone -> node created";
							boardHandler->createMoveSGF();
							unknownProperty = QString();
							isRoot = false;
if (tree->getCurrent()->getTimeinfo())
	qWarning("*** Timeinfo set !!!!");
							//tree->getCurrent()->setTimeinfo(false);
						}
					case editBlack:
					case editWhite:
					case editErase:
					{
						x = toParse->at(pos+1).toAscii() - 'a' + 1;
						y = toParse->at(pos+2).toAscii() - 'a' + 1;

						int x1, y1;
						bool compressed_list;

						// check for compressed lists
						if (toParse->at(pos+3) == ':')
						{
							x1 = toParse->at(pos+4).toAscii() - 'a' + 1;
							y1 = toParse->at(pos+5).toAscii() - 'a' + 1;
							compressed_list = true;
						}
						else
						{
							x1 = x;
							y1 = y;
							compressed_list = false;
						}

						boardHandler->setModeSGF(setup || compressed_list ? modeEdit : modeNormal);

						int i, j;
						for (i = x; i <= x1; i++)
							for (j = y; j <= y1; j++)
							{
								if (i == 20 && j == 20)
									boardHandler->doPass(true);
								else if (prop == editErase)
								{
									boardHandler->removeStone(i, j, true, false);
								}
								else
								{
									boardHandler->addStoneSGF(black ? stoneBlack : stoneWhite, i, j, new_node);
								}
								// tree->getCurrent()->getMatrix()->debug();
								// qDebug("ADDING MOVE %s %d/%d", black?"B":"W", x, y);

								// Remember this move for later, to remove from the matrix.
								position = new Position;
								position->x = i;
								position->y = j;
								toRemove.push(position);
								moves ++;
							}

						new_node = false;

						if (compressed_list)
							// Advance pos by 7
							pos += 7;
						else
							// Advance pos by 4
							pos += 4;
						break;
					}

					case nodeName:
					{
						commentStr = QString();
						bool skip = false;

						while (toParse->at(++pos) != ']')
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug() << "SGF: Nodename string ended immediately";
								delete toParse;
								return corruptSgf(pos, "SGF: Nodename string ended immediately");
							}

							// white spaces
							if (toParse->at(pos) == '\\')
							{
								while (toParse->at(pos+1).isSpace() &&
									static_cast<unsigned int>(pos) < strLength-2)
									pos++;
								if (toParse->at(pos).isSpace())
									pos++;

								// case: "../<cr><lf>]"
								if (toParse->at(pos) == ']')
								{
									pos--;
									skip = true;
								}
							}

							// escaped chars: '\', ']', ':'
							if (!(toParse->at(pos) == '\\' &&
								(toParse->at(pos+1) == ']' ||
								 toParse->at(pos+1) == '\\' ||
								 toParse->at(pos+1) == ':')) &&
								 !skip &&
								 // no formatting
								!(toParse->at(pos) == '\n') &&
								!(toParse->at(pos) == '\r'))
								commentStr.append(toParse->at(pos));
						}

						// qDebug("Comment read: %s", commentStr.latin1());
						if (!commentStr.isEmpty())
							// add comment; skip 'C[]'
							tree->getCurrent()->setNodeName(commentStr);
						pos++;
						break;
					}

					case comment:
					{
						commentStr = QString();
						bool skip = false;

						while (toParse->at(++pos) != ']' ||
							(toParse->at(pos-1) == '\\' && toParse->at(pos) == ']'))
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug() << "SGF: Comment string ended immediately";
								delete toParse;
								return corruptSgf(pos, "SGF: Comment string ended immediately");
							}

							// white spaces
							if (toParse->at(pos) == '\\')
							{
								while (toParse->at(pos+1).isSpace() &&
									static_cast<unsigned int>(pos) < strLength-2)
									pos++;
								if (toParse->at(pos).isSpace())
									pos++;

								// case: "../<cr><lf>]"
								if (toParse->at(pos) == ']')
								{
									pos--;
									skip = true;
								}
							}

							// escaped chars: '\', ']', ':'
							if (!(toParse->at(pos) == '\\' &&
								(toParse->at(pos+1) == ']' ||
								 toParse->at(pos+1) == '\\' ||
								 toParse->at(pos+1) == ':')) &&
								 !skip)
								commentStr.append(toParse->at(pos));
						}

						// qDebug("Comment read: %s", commentStr.latin1());
						if (!commentStr.isEmpty ())
						{
							// add comment; skip 'C[]'
							tree->getCurrent()->setComment(commentStr);
						}
						pos ++;
						break;
					}

					case unknownProp:
					{
						// skip if property is known anyway
						bool skip = false;

						// save correct property name (or p.n. + '[')
						commentStr = toParse->at(pos);
						commentStr += toParse->at(tmppos = toParse->next_nonspace (pos + 1));
						pos = tmppos;

						// check if it's really necessary to hold properties
						// maybe they are handled at another position
						if (commentStr == "WR" ||
							commentStr == "BR" ||
							commentStr == "PW" ||
							commentStr == "PB" ||
							commentStr == "SZ" ||
							commentStr == "KM" ||
							commentStr == "HA" ||
							commentStr == "RE" ||
							commentStr == "DT" ||
							commentStr == "PC" ||
							commentStr == "CP" ||
							commentStr == "GN" ||
							commentStr == "OT" ||
							commentStr == "TM" ||
							// now: general options
							commentStr == "GM" ||
							commentStr == "ST" ||
							commentStr == "AP" ||
							commentStr == "FF")
						{
							skip = true;
						}
						sss= toParse->at(pos);
						while (toParse->at(++pos) != ']' ||
							(toParse->at(pos-1) == '\\' && toParse->at(pos) == ']'))
						{
							if (static_cast<unsigned int>(pos) > strLength-1)
							{
								qDebug() << "SGF: Unknown property ended immediately";
								delete toParse;
								return corruptSgf(pos, "SGF: Unknown property ended immediately");
							}
              sss= toParse->at(pos);
							if (!skip)
								commentStr.append(toParse->at(pos));
						}

						if (!skip)
							commentStr.append("]");

						// qDebug("Comment read: %s", commentStr.latin1());
						if (!commentStr.isEmpty() && !skip)
						{
							// cumulate unknown properties; skip empty property 'XZ[]'
							unknownProperty += commentStr;
							tree->getCurrent()->setUnknownProperty(unknownProperty);
						}
						pos ++;
            sss= toParse->at(pos);
						break;
					}

					case editMark:
						// set moveStr for increment labels of old 'L[]' property
						moveStr = "A";
						while (toParse->at(pos) == '[' &&
							static_cast<unsigned int>(pos) < strLength)
						{
							x = toParse->at(pos+1).toAscii() - 'a' + 1;
							y = toParse->at(pos+2).toAscii() - 'a' + 1;
							// qDebug("MARK: %d at %d/%d", markType, x, y);
							pos += 3;

							// 'LB' property? Then we need to get the text
							if (markType == markText && !old_label)
							{
								if (toParse->at(pos) != ':')
								{
									delete toParse;
									return corruptSgf(pos);
								}
								moveStr = "";
								while (toParse->at(++pos) != ']' &&
									static_cast<unsigned int>(pos) < strLength)
									moveStr.append(toParse->at(pos));
								// qDebug("LB TEXT = %s", moveStr.latin1());
								// It might me a number mark?
								bool check = false;
								moveStr.toInt(&check);  // Try to convert to Integer
								// treat integers as characters...
								check = false;

								if (check)
									tree->getCurrent()->getMatrix()->
										insertMark(x, y, markNumber);  // Worked, its a number
								else
									tree->getCurrent()->getMatrix()->
										insertMark(x, y, markType);    // Nope, its a letter
								tree->getCurrent()->getMatrix()->
									setMarkText(x, y, moveStr);
							}
							else
							{
								int x1, y1;
								bool compressed_list;

								// check for compressed lists
								if (toParse->at(pos) == ':')
								{
									x1 = toParse->at(pos+1).toAscii() - 'a' + 1;
									y1 = toParse->at(pos+2).toAscii() - 'a' + 1;
									compressed_list = true;
								}
								else
								{
									x1 = x;
									y1 = y;
									compressed_list = false;
								}

//								boardHandler->setModeSGF(setup || compressed_list ? modeEdit : modeNormal);

								int i, j;
								for (i = x; i <= x1; i++)
									for (j = y; j <= y1; j++)
									{
										tree->getCurrent()->getMatrix()->insertMark(i, j, markType);

										// auto increment for old property 'L'
										if (old_label)
										{
											tree->getCurrent()->getMatrix()->
												setMarkText(x, y, moveStr);
											QChar c1 = moveStr[0];
											if (c1 == 'Z')
												moveStr = QString("a");
											else
												moveStr = c1.unicode() + 1;
										}
									}

//								new_node = false;

								if (compressed_list)
									// Advance pos by 3
									pos += 3;
							}

							//old_label = false;
							pos ++;
							while (toParse->at(pos).isSpace()) pos++;
						}
						break;

					case openMoves:
					{
						QString tmp_mv;
						while (toParse->at(++pos) != ']')
							tmp_mv += toParse->at(pos);
						tree->getCurrent()->setOpenMoves(tmp_mv.toInt());
						pos++;

						if (!tree->getCurrent()->getTimeinfo())
						{
							tree->getCurrent()->setTimeinfo(true);
							tree->getCurrent()->setTimeLeft(0);
						}
						break;
					}

					case timeLeft:
					{
						QString tmp_mv;
						while (toParse->at(++pos) != ']')
							tmp_mv += toParse->at(pos);
						tree->getCurrent()->setTimeLeft(tmp_mv.toFloat());
						pos++;

						if (!tree->getCurrent()->getTimeinfo())
						{
							tree->getCurrent()->setTimeinfo(true);
							tree->getCurrent()->setOpenMoves(0);
						}
						break;
					}

					case nextMove:
						if (toParse->at(++pos) == 'W')
							tree->getCurrent()->setPLinfo(stoneWhite);
						else if (toParse->at(pos) == 'B')
							tree->getCurrent()->setPLinfo(stoneBlack);

						pos += 2;
						break;

					default:
						break;
		    }

		    while (toParse->at(pos).isSpace())
			    pos++;
        sss= toParse->at(pos);
		} while (setup && toParse->at(pos) == '[');

		//tree->getCurrent()->getMatrix()->debug();
		while (toParse->at(pos).isSpace())
			pos++;

	    } while (toParse->at(pos) != ';' && toParse->at(pos) != '(' && toParse->at(pos) != ')' &&
		    static_cast<unsigned int>(pos) < strLength);

	    // Advance pointer
	    pointer = pos;

	    break;

	default:
		delete toParse;
		return corruptSgf(pointer);
	}

    } while (pointer < strLength && pos >= 0);

    progress.setProgress(strLength);

    delete toParse;
    return !cancel;
}

bool SGFParser::corruptSgf(int where, QString reason)
{
	QMessageBox::warning(0, PACKAGE, Board::tr("Corrupt SGF file at position") + " " +
			     QString::number(where) + "\n\n" +
			     (reason.isNull() || reason.isEmpty() ? QString("") : reason));
	return false;
}

int SGFParser::minPos(int n1, int n2, int n3)
{
	int min;

	if (n1 != -1)
		min = n1;
	else if (n2 != -1)
		min = n2;
	else
		min = n3;

	if (n1 < min && n1 != -1)
		min = n1;

	if (n2 < min && n2 != -1)
		min = n2;

	if (n3 < min && n3 != -1)
		min = n3;

	return min;
}

// Return false: corrupt sgf, true: sgf okay. result = 0 when property not found
bool SGFParser::parseProperty(const QString &toParse, const QString &prop, QString &result)
{
	int pos, strLength=toParse.length();
	result = "";

	pos = toParse.find(prop+"[");
	if (pos == -1)
		return true;

	pos += 2;
	if (toParse[pos] != '[')
		return corruptSgf(pos);
	while (toParse[++pos] != ']' && pos < strLength)
		result.append(toParse[pos]);
	if (pos > strLength)
		return corruptSgf(pos);

	return true;
}

bool SGFParser::initGame(const QString &toParse, const QString &fileName)
{
	QString tmp="";
	GameData *gameData = new GameData;

	// White player name
	if (!parseProperty(toParse, "PW", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->playerWhite = tmp;
	else
		gameData->playerWhite = "White";

	// White player rank
	if (!parseProperty(toParse, "WR", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->rankWhite = tmp;
	else
		gameData->rankWhite = "";

	// Black player name
	if (!parseProperty(toParse, "PB", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->playerBlack = tmp;
	else
		gameData->playerBlack = "Black";

	// Black player rank
	if (!parseProperty(toParse, "BR", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->rankBlack = tmp;
	else
		gameData->rankBlack = "";

	// Board size
	if (!parseProperty(toParse, "SZ", tmp))
		return false;
	int columns, rows;
	switch(sscanf(tmp.toAscii().constData(), "%d:%d", &columns, &rows))
	{
	case 2:
		// different size is not supported, so use the maximum
		gameData->size = std::max(columns, rows);
		break;
	case 1:
		gameData->size = columns;
		break;
	default:
		gameData->size = 19;
	}

	// Komi
	if (!parseProperty(toParse, "KM", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->komi = tmp.toFloat();
	else
		gameData->komi = 0.0;

	// Handicap
	if (!parseProperty(toParse, "HA", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->handicap = tmp.toInt();
	else
		gameData->handicap = 0;

	// Result
	if (!parseProperty(toParse, "RE", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->result = tmp;
	else
		gameData->result = "";

	// Date
	if (!parseProperty(toParse, "DT", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->date = tmp;
	else
		gameData->date = "";

	// Place
	if (!parseProperty(toParse, "PC", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->place = tmp;
	else
		gameData->place = "";

	// Copyright
	if (!parseProperty(toParse, "CP", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->copyright = tmp;
	else
		gameData->copyright = "";

	// Game Name
	if (!parseProperty(toParse, "GN", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->gameName = tmp;
	else
		gameData->gameName = "";

	// Comments style
	if (!parseProperty(toParse, "ST", tmp))
		return false;
	if (!tmp.isEmpty())
		gameData->style = tmp.toInt();
	else
		gameData->style = 1;

	// Timelimit
	if (!parseProperty(toParse, "TM", tmp))
		return false;
	if (!tmp.isEmpty())
	{
		gameData->timelimit = tmp.toInt();
		gameData->timeSystem = absolute;
	}
	else
	{
		gameData->timelimit = 0;
		gameData->timeSystem = none;
	}

	// Overtime == time system
	if (!parseProperty(toParse, "OT", tmp))
		return false;
	if (!tmp.isEmpty())
	{
		gameData->overtime = tmp;
		if (tmp.contains("byo-yomi"))
		{
			// type: OT[5x30 byo-yomi]
			gameData->timeSystem = byoyomi;
			int pos1, pos2;
			if ((pos1 = tmp.find("x")) != -1)
			{
				pos2 = tmp.find("byo");
				QString time = tmp.mid(pos1+1, pos2-pos1-1);
				gameData->byoTime = time.toInt();
				gameData->byoPeriods = tmp.left(pos1).toInt();
				gameData->byoStones = 0;

				qDebug() << QString("byoyomi time system: %1 Periods at %2 seconds").arg(gameData->byoPeriods).arg(gameData->byoTime);
			}
		}
		else if (tmp.contains(":"))
		{
			// type: OT[3:30] = byo-yomi?;
			int pos1;
			gameData->timeSystem = byoyomi;
			if ((pos1 = tmp.find(":")) != -1)
			{
				QString time = tmp.left(pos1);
				int t = time.toInt()*60 + tmp.right(tmp.length() - pos1 - 1).toInt();
				gameData->byoTime = 30;
				gameData->byoPeriods = t/gameData->byoTime;
				gameData->byoStones = 0;

				qDebug() << QString("byoyomi time system: %1 Periods at %2 seconds").arg(gameData->byoPeriods).arg(gameData->byoTime);
			}
		}
		else if (tmp.contains("Canadian"))
		{
			// type: OT[25/300 Canadian]
			gameData->timeSystem = canadian;
			int pos1, pos2;
			if ((pos1 = tmp.find("/")) != -1)
			{
				pos2 = tmp.find("Can");
				QString time = tmp.mid(pos1+1, pos2-pos1-1);
				gameData->byoTime = time.toInt();
				gameData->byoStones = tmp.left(pos1).toInt();

				qDebug() << QString("Canadian time system: %1 seconds at %2 stones").arg(gameData->byoTime).arg(gameData->byoStones);
			}
		}

		// simple check
		if (gameData->byoStones < 0)
			gameData->byoStones = 0;
		if (gameData->byoTime <= 0)
		{
			gameData->byoTime = 0;
//			gameData->timeSystem = none;
		}
	}
	else
	{
		gameData->overtime = "";
//		gameData->timeSystem = none;
		gameData->byoStones = 0;
	}

	// Game number
	gameData->gameNumber = 0;

	gameData->fileName = fileName;

	boardHandler->board->initGame(gameData, true);  // Set sgf flag

	return true;
}

bool SGFParser::exportSGFtoClipB(QString *str, Tree *tree)
{
	CHECK_PTR(tree);

	if (stream != NULL)
		delete stream;
	stream = new Q3TextStream(str, QIODevice::WriteOnly);

	bool res = writeStream(tree);

	delete stream;
	stream = NULL;
	return res;
}

bool SGFParser::doWrite(const QString &fileName, Tree *tree)
{
	CHECK_PTR(tree);

	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(0, PACKAGE, Board::tr("Could not open file:") + " " + fileName);
		return false;
	}

	if (stream != NULL)
		delete stream;
	stream = new Q3TextStream(&file);

	bool res = writeStream(tree);

	file.flush();
	file.close();
	delete stream;
	stream = NULL;
	return res;
}

bool SGFParser::writeStream(Tree *tree)
{
	CHECK_PTR(stream);
	if (!initStream(stream))
	{
		QMessageBox::critical(0, PACKAGE, Board::tr("Invalid text encoding given. Please check preferences!"));
		delete stream;
		return false;
	}

	Move *root = tree->getRoot();
	if (root == NULL)
		return false;

	GameData *gameData = boardHandler->getGameData();

	// Traverse the tree recursive in pre-order
	isRoot = true;
	traverse(root, gameData);

	return true;
}

void SGFParser::writeGameHeader(GameData *gameData)
{
	// Assemble data for root node
	*stream << ";GM[1]FF[4]"						// We play Go, we use FF 4
		<< "AP[" << PACKAGE << ":" << VERSION << "]";		// Application name : Version
	if (gameData->style >= 0 && gameData->style <= 4)
		*stream << "ST[" << gameData->style << "]";
	else
		*stream << "ST[1]";						// We show vars of current node
	if (gameData->gameName.isEmpty())					// Skip game name if empty
		*stream << endl;
	else
		*stream << "GN[" << gameData->gameName << "]"		// Game Name
		<< endl;
	*stream << "SZ[" << gameData->size << "]"				// Board size
		<< "HA[" << gameData->handicap << "]"			// Handicap
		<< "KM[" << gameData->komi << "]";				// Komi
//		<< endl;

	if (gameData->timelimit != 0)
		*stream << "TM[" << gameData->timelimit << "]";		// Timelimit

	if (!gameData->overtime.isEmpty())
		*stream << "OT[" << gameData->overtime << "]" << endl;		// Overtime

	if (!gameData->playerWhite.isEmpty())
		*stream << "PW[" << gameData->playerWhite << "]";  // White name

	if (!gameData->rankWhite.isEmpty())
		*stream << "WR[" << gameData->rankWhite << "]";    // White rank

	if (!gameData->playerBlack.isEmpty())
		*stream << "PB[" << gameData->playerBlack << "]";  // Black name

	if (!gameData->rankBlack.isEmpty())
		*stream << "BR[" << gameData->rankBlack << "]";    // Black rank

	if (!gameData->result.isEmpty())
		*stream << "RE[" << gameData->result << "]";       // Result

	if (!gameData->date.isEmpty())
		*stream << "DT[" << gameData->date << "]";         // Date

	if (!gameData->place.isEmpty())
		*stream << "PC[" << gameData->place << "]";         // Place

	if (!gameData->copyright.isEmpty())
		*stream << "CP[" << gameData->copyright << "]";    // Copyright

	*stream << endl;
}

void SGFParser::traverse(Move *t, GameData *gameData)
{
	*stream << "(";
	int col = -1, cnt = 6;

	do {
		if (isRoot)
		{
			writeGameHeader(gameData);
			*stream << t->saveMove(isRoot);
			isRoot = false;
		}
		else
		{
			// do some formatting: add single B[]/W[] properties to one line, max: 10
			// if more properties, e.g. B[]PL[]C] -> new line
			QString txt = t->saveMove(false);
			int cnt_old = cnt;
			cnt = txt.length();
			if (col % 10 == 0 || col == 1 && cnt != 6 || cnt_old != 6 || col == -1)
			{
				*stream << endl;
				col = 0;
			}

			*stream << txt;
			col++;
		}

		Move *tmp = t->son;
		if (tmp != NULL && tmp->brother != NULL)
		{
			do {
				*stream << endl;
				traverse(tmp, NULL);
			} while ((tmp = tmp->brother) != NULL);
			break;
		}
	} while ((t = t->son) != NULL);
	*stream << endl << ")";
}

bool SGFParser::parseASCII(const QString &fileName, ASCII_Import *charset, bool isFilename)
{
	Q3TextStream *txt = NULL;
	bool result = false;
	asciiOffsetX = asciiOffsetY = 0;

#if 0
	qDebug("BLACK STONE CHAR %c\n"
		"WHITE STONE CHAR %c\n"
		"STAR POINT  CHAR %c\n"
		"EMPTY POINT CHAR %c\n"
		"HOR BORDER CHAR %c\n"
		"VER BORDER CHAR %c\n",
		charset->blackStone,
		charset->whiteStone,
		charset->starPoint,
		charset->emptyPoint,
		charset->hBorder,
		charset->vBorder);
#endif

	if (isFilename)  // Load from file
	{
		QFile file;

		if (fileName.isNull() || fileName.isEmpty())
		{
			QMessageBox::warning(0, PACKAGE, Board::tr("No filename given!"));
			delete txt;
			return false;
		}

		file.setName(fileName);
		if (!file.exists())
		{
			QMessageBox::warning(0, PACKAGE, Board::tr("Could not find file:") + " " + fileName);
			delete txt;
			return false;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::warning(0, PACKAGE, Board::tr("Could not open file:") + " " + fileName);
			delete txt;
			return false;
		}

		txt = new Q3TextStream(&file);
		if (!initStream(txt))
		{
			QMessageBox::critical(0, PACKAGE, Board::tr("Invalid text encoding given. Please check preferences!"));
			delete txt;
			return false;
		}

		result = parseASCIIStream(txt, charset);
		file.close();
	}
	else  // a string was passed instead of a filename, copy from clipboard
	{
		if (fileName.isNull() || fileName.isEmpty())
		{
			QMessageBox::warning(0, PACKAGE, Board::tr("Importing ASCII failed. Clipboard empty?"));
			delete txt;
			return false;
		}

		QString buf(fileName);
		txt = new Q3TextStream(buf, QIODevice::ReadOnly);
		if (!initStream(txt))
		{
			QMessageBox::critical(0, PACKAGE, Board::tr("Invalid text encoding given. Please check preferences!"));
			delete txt;
			return false;
		}

		result = parseASCIIStream(txt, charset);
	}

	delete txt;
	return result;
}

bool SGFParser::parseASCIIStream(Q3TextStream *stream, ASCII_Import *charset)
{
	CHECK_PTR(stream);

	Q3StrList asciiLines;
	asciiLines.setAutoDelete(TRUE);

	int i=0, first=-1, last=-1, y=1;
	bool flag=false;
	QString dummy = QString(QChar(charset->vBorder)).append(charset->vBorder).append(charset->vBorder);  // "---"

	while (!stream->atEnd())
	{
		QString tmp = stream->readLine();
		asciiLines.append(tmp.latin1());

		if (tmp.find('.') != -1)
			flag = true;

		if (tmp.find(dummy) != -1)
		{
			if (first == -1 && !flag)
				first = i;
			else
				last = i;
		}
		i++;
	}

	if (!flag)
	{
		GameData gd;
		QString  ascii = asciiLines.getFirst();

		// do some fast checks: one line string?
qDebug("no standard ASCII file");
		int nr = ascii.count("0") + ascii.count("b") + ascii.count("w");
		if (nr == 81)
		{
qDebug("found 9x9");
			gd.size = 9;
			gd.komi = 3.5;
		}
		else if (nr == 169)
		{
qDebug("found 13x13");
			gd.size = 13;
			gd.komi = 4.5;
		}
		else if (nr == 361)
		{
qDebug("found 19x19");
			gd.size = 19;
			gd.komi = 5.5;
		}
		else
		{
qDebug() << QString("found nr == %1").arg(nr);
			return false;
		}

		gd.handicap = 0;
		if (gd.size != boardHandler->board->getBoardSize())
			boardHandler->board->initGame(&gd, true);


		int i = 0;
		for (int y = 1; y <= gd.size; y++)
			for (int x = 1; x <= gd.size; x++)
			{
				while (ascii[i] != 'b' && ascii[i] != 'w' && ascii[i] != '0')
					i++;

				if (ascii[i] == 'b')
				{
					boardHandler->addStone(stoneBlack, x, y);
				}
				else if (ascii[i] == 'w')
				{
					boardHandler->addStone(stoneWhite, x, y);
				}

				i++;
			}

		asciiLines.clear();
		return true;
	}

	// qDebug("Y: FIRST = %d, LAST = %d", first, last);

	if (first == -1 && last != -1)
		asciiOffsetY = boardHandler->board->getBoardSize() - last;

	Q3StrListIterator it(asciiLines);
	for (; it.current() && y < boardHandler->board->getBoardSize(); ++it)
		if (!doASCIIParse(it.current(), y, charset))
			return false;

	asciiLines.clear();
	return true;
}

bool SGFParser::doASCIIParse(const QString &toParse, int &y, ASCII_Import *charset)
{
	int pos, x = 0, length = toParse.length();

	if (!checkBoardSize(toParse, charset))
		return false;

	for (pos=toParse.find(charset->emptyPoint, 0); pos<length; pos++)
	{
		// qDebug("READING %d/%d", x, y);
		if (x >= boardHandler->board->getBoardSize() - asciiOffsetX)  // Abort if right edge of board reached
			break;

		// Empty point or star point
		if (toParse[pos] == charset->emptyPoint ||
			toParse[pos] == charset->starPoint)
			x++;

		// Right border
		else if (x>0 && toParse[pos] == charset->hBorder)
			break;

		// White stone
		else if (toParse[pos] == charset->whiteStone && x && y)
		{
			x++;
			// qDebug("W %d/%d", x, y);
			boardHandler->addStone(stoneWhite, asciiOffsetX+x, asciiOffsetY+y);
		}

		// Black stone
		else if (toParse[pos] == charset->blackStone && x && y)
		{
			x++;
			// qDebug("B %d/%d", x, y);
			boardHandler->addStone(stoneBlack, asciiOffsetX+x, asciiOffsetY+y);
		}

		// Text label: a-z
		else if (toParse[pos] >= 'a' && toParse[pos] <= 'z')
		{
			x++;
			// qDebug("MARK: %d/%d - %c", x, y, toParse[pos].latin1());
			boardHandler->editMark(asciiOffsetX+x, asciiOffsetY+y, markText, toParse[pos]);
		}

		// Number label: 1-9
		else if (toParse[pos] >= '1' && toParse[pos] <= '9')
		{
			x++;
			// qDebug("NUMBER: %d/%d - %c", x, y, toParse[pos].latin1());
			boardHandler->editMark(asciiOffsetX+x, asciiOffsetY+y, markNumber, toParse[pos]);
		}
	}

	if (x)
		y++;

	return true;
}

bool SGFParser::checkBoardSize(const QString &toParse, ASCII_Import *charset)
{
	// Determine x offset
	int left = toParse.find(charset->hBorder),
		right = toParse.find(charset->hBorder, left+1);

	// qDebug("Left = %d, Right = %d", left, right);

	if (right == -1)
	{
		int first = toParse.find(charset->emptyPoint),
			tmp = toParse.find(charset->starPoint);
		first = first > tmp && tmp != -1 ? tmp : first;

		if (left > first)
			asciiOffsetX = boardHandler->board->getBoardSize() - (left - first)/2 - ((left-first)%2 ? 1 : 0);
		else
			asciiOffsetX = 0;
		// qDebug("ASSUMING PART OF BOARD ONLY. First = %d, ASCII_OFFSET_X = %d", first, asciiOffsetX);
	}
	else if (left > -1 && right > -1)
	{
		// qDebug("ASSUMING FULL BOARD. BOARD SIZE = %d", boardHandler->board->getBoardSize());
		asciiOffsetX = 0;
		if ((right - left)/2 != boardHandler->board->getBoardSize())  // TODO: Warning and abort?
			qWarning("Board size does not fit.");
	}
	else
		asciiOffsetX = 0;

	return true;
}
