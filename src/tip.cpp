/*
* tip.cpp
*/

#include "tip.h"
#include "board.h"
#include "qgo.h"
//Added by qt3to4:
#include <QLabel>

/*
* Tip
*/
/* UNUSED
Tip::Tip(QWidget *parent)
: QToolTip(parent)
{
}

Tip::~Tip()
{
}

void Tip::maybeTip(const QPoint &pos)
{
	if (!setting->readBoolEntry("BOARD_COORDS_TIP"))
		return;
	
	int x = ((Board*)parentWidget())->getCurrentX(),
		y = ((Board*)parentWidget())->getCurrentY();
	
	if (x == -1 || y == -1)
		return;
	
	tip(QRect(pos.x(), pos.y(), 1, 1),
		QString(QChar(static_cast<const char>('A' + (x<9?x:x+1) - 1))) +       // A -> T (skip I)
		" " + QString::number(((Board*)parentWidget())->getBoardSize()-y+1));  // 19 -> 1
}
*/

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

	setText(" " + t1 + //QString(QChar(static_cast<const char>('A' + (x<9?x:x+1) - 1))) +       // A -> T (skip I)
		" " + t2 +//QString::number(board_size-y+1) + 
		" ");   




                             // 19 -> 1
}
