/*
* tip.cpp
*/

#include "tip.h"
#include "board.h"
#include "qgo.h"
#include <QLabel>

/*
* StatusTip
*/

StatusTip::StatusTip(QWidget *parent)
: QLabel(parent)
{
	setAlignment(Qt::AlignCenter | Qt::TextSingleLine);
}

StatusTip::~StatusTip()
{
}

void StatusTip::slotStatusTipCoords(int x, int y, int board_size, bool SGFCoords)
{
	emit clearStatusBar();
	QString t1,t2;	


	if(SGFCoords)
	{
		t1 = QString(QChar(static_cast<const char>('a' + x-1)));
		t2 = QString(QChar(static_cast<const char>('a' + y-1)));
	}
	else
	{
		t1 = QString(QChar(static_cast<const char>('A' + (x<9?x:x+1) - 1)));
		t2 = QString::number(board_size - y+1);
	}

	setText(" " + t1 + " " + t2 + " ");
}
