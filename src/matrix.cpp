/*
* matrix.cpp
*/

#include "matrix.h"
#include <stdlib.h>
#include <QString>
#ifndef NO_DEBUG
#include <iostream>
#endif

using namespace std;

Matrix::Matrix(int s)
: size(s)
{
	ASSERT(size > 0 && size <= 36);
	
	init();
}

Matrix::Matrix(const Matrix &m)
{
	size = m.getSize();
	ASSERT(size > 0 && size <= 36);
	
	init();
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] = m.at(i, j);
}

Matrix::~Matrix()
{
	ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		delete [] matrix[i];
	delete [] matrix;
}

void Matrix::init()
{
	matrix = new short*[size];
	CHECK_PTR(matrix);
	
	for (int i=0; i<size; i++)
	{
		matrix[i] = new short[size];
		CHECK_PTR(matrix[i]);
		
		for (int j=0; j<size; j++)
			matrix[i][j] = stoneNone;
	}
}

void Matrix::clear()
{
	ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] = stoneNone;
}

#ifndef NO_DEBUG
void Matrix::debug() const
{
	QDebug dbg = qDebug ();
	ASSERT(size > 0 && size <= 36);
	
	int i, j;

	{
		QDebug dbg1 = qDebug ();
		qDebug () << "\n  ";
		for (i=0; i<size; i++)
			dbg1 << (i+1)%10 << " ";
	}
	for (i=0; i<size; i++)
	{
		QDebug dbg2 = qDebug ();
		dbg2 << (i+1)%10 << " ";
		for (j=0; j<size; j++)
		{
#if 1
			switch (abs(matrix[j][i]))
			{
			case stoneNone:
			case stoneErase: dbg2 << ". "; break;
			case stoneBlack: dbg2 << "B "; break;
			case stoneWhite: dbg2 << "W "; break;
			case markSquare*10: dbg2 << "[ "; break;
			case markCircle*10: dbg2 << "O "; break;
			case markTriangle*10: dbg2 << "T "; break;
			case markCross*10: dbg2 << "X "; break;
			case markText*10: dbg2 << "A "; break;
			case markNumber*10: dbg2 << "1 "; break;
			case markSquare*10+stoneBlack: dbg2 << "S "; break;
			case markCircle*10+stoneBlack: dbg2 << "C "; break;
			case markTriangle*10+stoneBlack: dbg2 << "D "; break;
			case markCross*10+stoneBlack: dbg2 << "R "; break;
			case markText*10+stoneBlack: dbg2 << "A "; break;
			case markNumber*10+stoneBlack: dbg2 << "N "; break;
			case markSquare*10+stoneWhite: dbg2 << "s "; break;
			case markCircle*10+stoneWhite: dbg2 << "c "; break;
			case markTriangle*10+stoneWhite: dbg2 << "d "; break;
			case markCross*10+stoneWhite: dbg2 << "r "; break;
			case markText*10+stoneWhite: dbg2 << "a "; break;
			case markNumber*10+stoneWhite: dbg2 << "n "; break;
			default: dbg2 << "? ";
			}
#else
			dbg2 << matrix[j][i] << " ";
#endif
		}
		dbg2 << (i+1)%10;
	}

	{
		QDebug dbg3 = qDebug ();
		dbg3 << "  ";
		for (i=0; i<size; i++)
			dbg3 << (i+1)%10 << " ";
	}		
	if (markTexts.size() != 0)
	{
		qDebug () << markTexts.size() << " mark texts in the storage.";
		for (mapType::const_iterator it = markTexts.constBegin(); it != markTexts.constEnd(); ++it)
			qDebug () << (QString)(*it);
	}
}
#endif

void Matrix::insertStone(int x, int y, StoneColor c, GameMode mode)
{
	ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = abs(matrix[x-1][y-1] / 10 * 10) + c;
	if (mode == modeEdit)
		matrix[x-1][y-1] *= -1;
}

void Matrix::removeStone(int x, int y)
{
	ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = abs(matrix[x-1][y-1] / 10 * 10);
}

void Matrix::eraseStone(int x, int y)
{
	ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] = (abs(matrix[x-1][y-1] / 10 * 10) + stoneErase) * -1;
}

short Matrix::at(int x, int y) const
{
	ASSERT(x >= 0 && x < size &&
		y >= 0 && y < size);
	
	return matrix[x][y];
}

void Matrix::set(int x, int y, int n)
{
	ASSERT(x >= 0 && x < size &&
		y >= 0 && y < size);
	
	matrix[x][y] = n;
}

void Matrix::insertMark(int x, int y, MarkType t)
{
	//ASSERT(x > 0 && x <= size && y > 0 && y <= size);
	if (!(x > 0 && x <= size && y > 0 && y <= size))
    return;
    
	matrix[x-1][y-1] = (abs(matrix[x-1][y-1]) + 10*t) * (matrix[x-1][y-1] < 0 ? -1 : 1);
}

void Matrix::removeMark(int x, int y)
{
	ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);
	
	matrix[x-1][y-1] %= 10;

	markTexts.remove (coordsToKey(x, y));
}

void Matrix::clearAllMarks()
{
	ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			matrix[i][j] %= 10;

	markTexts.clear ();
}

void Matrix::clearTerritoryMarks()
{
	ASSERT(size > 0 && size <= 36);
	
	int data;
	
	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
			if ((data = abs(matrix[i][j] / 10)) == markTerrBlack ||
				data == markTerrWhite)
				matrix[i][j] %= 10;
}

void Matrix::absMatrix()
{
	ASSERT(size > 0 && size <= 36);
	
	for (int i=0; i<size; i++)
	{
		for (int j=0; j<size; j++)
		{
			matrix[i][j] = abs(matrix[i][j]);
			if (matrix[i][j] == stoneErase)
				matrix[i][j] = stoneNone;
			if (matrix[i][j] % 10 == stoneErase)
				matrix[i][j] = matrix[i][j] / 10 * 10;
		}
	}
}

void Matrix::setMarkText(int x, int y, const QString &txt)
{
	ASSERT(x > 0 && x <= size &&
		y > 0 && y <= size);

	markTexts[coordsToKey(x, y)] = txt;
}

const QString Matrix::getMarkText(int x, int y)
{
	long key = coordsToKey(x, y);
	if (!markTexts.contains (key))
		return QString::null;
	return markTexts[key];
}

const QString Matrix::saveMarks()
{
	ASSERT(size > 0 && size <= 36);
	
	QString txt, sSQ = "", sCR = "", sTR = "", sMA = "", sLB = "", sTB = "", sTW = "";
	int i, j, colw = 0, colb = 0;
	
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			switch (abs(matrix[i][j] / 10))
			{
			case markSquare:
				if (sSQ.isEmpty())
					sSQ += "SQ";
				sSQ += "[" + coordsToString(i, j) + "]";
				break;
			case markCircle:
				if (sCR.isEmpty())
					sCR += "CR";
				sCR += "[" + coordsToString(i, j) + "]";
				break;
			case markTriangle:
				if (sTR.isEmpty())
					sTR += "TR";
				sTR += "[" + coordsToString(i, j) + "]";
				break;
			case markCross:
				if (sMA.isEmpty())
					sMA += "MA";
				sMA += "[" + coordsToString(i, j) + "]";
				break;
			case markText: 
			case markNumber:
				if (sLB.isEmpty())
					sLB += "LB";
				sLB += "[" + coordsToString(i, j);
				sLB += ":";
				txt = getMarkText(i+1, j+1);
				if (txt.isNull() || txt.isEmpty())
					sLB += "?";  // Whoops
				else
					sLB += txt;
				sLB += "]";
				break;
			case markTerrBlack:
				if (sTB.isEmpty())
				{
					sTB += "\nTB";
					colb = 0;
				}
				sTB += "[" + coordsToString(i, j) + "]";
				if (++colb % 15 == 0)
					sTB += "\n  ";
				break;
			case markTerrWhite:
				if (sTW.isEmpty())
				{
					sTW += "\nTW";
					colw = 0;
				}
				sTW += "[" + coordsToString(i, j) + "]";
				if (++colw % 15 == 0)
					sTW += "\n  ";
				break;
			default: continue;
			}
		}
	}
	
	return sSQ + sCR + sTR + sMA + sLB + sTB + sTW;
}

const QString Matrix::saveEditedMoves(Matrix *parent)
{
	ASSERT(size > 0 && size <= 36);
	
	QString sAB="", sAW="", sAE="";
	int i, j;
	
	for (i=0; i<size; i++)
	{
		for (j=0; j<size; j++)
		{
			switch (matrix[i][j] % 10)
			{
			case stoneBlack * -1:
				if (parent != NULL &&
					parent->at(i, j) == stoneBlack)
					break;
				if (sAB.isEmpty())
					sAB += "AB";
				sAB += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneWhite * -1:
				if (parent != NULL &&
					parent->at(i, j) == stoneWhite)
					break;
				if (sAW.isEmpty())
					sAW += "AW";
				sAW += "[" + coordsToString(i, j) + "]";
				break;
				
			case stoneErase * -1:
				if (parent != NULL &&
					(parent->at(i, j) == stoneNone ||
					parent->at(i, j) == stoneErase))
					break;
				if (sAE.isEmpty())
					sAE += "AE";
				sAE += "[" + coordsToString(i, j) + "]";
				break;
			}
		}
	}
	
	return sAB + sAW + sAE;
}

const QString Matrix::printMe(ASCII_Import *charset)
{
	ASSERT(size > 0 && size <= 36);
	
#if 0
	qDebug("BLACK STONE CHAR %c\n"
		"WHITE STONE CHAR %c\n"
		"STAR POINT  CHAR %c\n"
		"EMPTY POINT CHAR %c\n",
		charset->blackStone,
		charset->whiteStone,
		charset->starPoint,
		charset->emptyPoint);
#endif
	
	int i, j;
	QString str;
	
	str = "\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n";
	
	for (i=0; i<size; i++)
	{
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + " |";
		for (j=0; j<size; j++)
		{
			switch (abs(matrix[j][i] % 10))
			{
			case stoneBlack: str += QChar(charset->blackStone); str += " "; break;
			case stoneWhite: str += QChar(charset->whiteStone); str += " "; break;
			default:
				// Check for starpoints
				if (size > 9)  // 10x10 or larger
				{
					if ((i == 3 && j == 3) ||
						(i == size-4 && j == 3) ||
						(i == 3 && j == size-4) ||
						(i == size-4 && j == size-4) ||
						(i == (size+1)/2 - 1 && j == 3) ||
						(i == (size+1)/2 - 1 && j == size-4) ||
						(i == 3 && j == (size+1)/2 - 1) ||
						(i == size-4 && j == (size+1)/2 - 1) ||
						(i == (size+1)/2 - 1 && j == (size+1)/2 - 1))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				else  // 9x9 or smaller
				{
					if ((i == 2 && j == 2) ||
						(i == 2 && j == size-3) ||
						(i == size-3 && j == 2) ||
						(i == size-3 && j == size-3))
					{
						str += QChar(charset->starPoint);
						str += " ";
						break;
					}
				}
				
				str += QChar(charset->emptyPoint);
				str += " ";
				break;
			}
		}
		str = str.left(str.length()-1);
		str += "| ";
		if (size-i < 10) str += " ";
		str += QString::number(size-i) + "\n";
	}
	
	str += "   +";
	for (i=0; i<size-1; i++)
		str += "--";
	str += "-+\n    ";
	for (i=0; i<size; i++)
		str += QString(QChar(static_cast<const char>('A' + (i<8?i:i+1)))) + " ";
	str += "\n";
	
	return str;
}
