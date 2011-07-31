/*
 *  gamestable.cpp
 */

#include "gamestable.h"
#include "misc.h"
#include "setting.h"

#include <qheader.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qwidget.h>

GamesTable::GamesTable( QWidget *parent, const char *name, bool /*modal*/, WFlags fl)
	: QListView(parent, name, fl)
{
	addColumn(QObject::tr("Id", "GamesTable Id number"));
	addColumn(QObject::tr("White", "GamesTable White name"));
	addColumn(QObject::tr("WR", "GamesTable White Rank"));
	addColumn(QObject::tr("Black", "GamesTable Black name"));
	addColumn(QObject::tr("BR", "GamesTable Black Rank"));
	addColumn(QObject::tr("Mv", "GamesTable Move"));
	addColumn(QObject::tr("Sz", "GamesTable Size"));
	addColumn(QObject::tr("H", "GamesTable Handicap"));
	addColumn(QObject::tr("K", "GamesTable Komi"));
	addColumn(QObject::tr("By", "GamesTable Byoyomi time"));
	addColumn(QObject::tr("FR", "GamesTable Free/Rated type of game"));
	addColumn(QObject::tr("Ob", "GamesTable number of Observers"));
	setColumnAlignment(0, AlignRight);
	setColumnAlignment(1, AlignLeft);
	setColumnAlignment(3, AlignLeft);
	setColumnAlignment(5, AlignRight);
	setColumnAlignment(6, AlignRight);
	setColumnAlignment(7, AlignRight);
	setColumnAlignment(8, AlignRight);
	setColumnAlignment(9, AlignRight);
	setColumnAlignment(10, AlignRight);
	setColumnAlignment(11, AlignRight);
	setProperty("focusPolicy", (int)QListView::NoFocus );
	setProperty("resizePolicy", (int)QListView::AutoOneFit );

	// set sorting order for games by wrank
	setSorting(2);
	setAllColumnsShowFocus(true);

	//setItemMargin(2);

}

GamesTable::~GamesTable()
{
}


/*
 *   GamesTableItem
 */

GamesTableItem::GamesTableItem(GamesTable *parent, const char* name)
	: QListViewItem(parent, name)  
{
}

GamesTableItem::GamesTableItem(GamesTableItem *parent, const char* name)
	: QListViewItem(parent, name)  
{
}

GamesTableItem::GamesTableItem(GamesTable *parent, QString label1, QString label2,
                QString label3, QString label4, QString label5,
                QString label6, QString label7, QString label8,
				QString label9, QString label10, QString label11, QString label12, QString label13)
	: QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{

	
	// set name for watch/mark and ";" for correct recognition
	if (label13)
	{
		int len = label13.length() - 1;
		watched = label13[len] == 'W';
		its_me = label13[0] == 'A';
	}
	else
	{
		watched = false;
		its_me = false;
	}

	// QListViewItem only supports up to 8 labels, check for the rest
	if (label9)
	{
		setText(8, label9);
		if (label10)
		{
			setText(9, label10);
			if (label11)
			{
				setText(10, label11);
				if (label12)
				{
					setText(11, label12);
					if (label13)
					{
						setText(12, label13);
					}
				}
			}
		}
	}
}

GamesTableItem::~GamesTableItem()
{
}

void GamesTableItem::paintCell( QPainter *p, const QColorGroup &cg,
				 int column, int width, int alignment )
{
	QColorGroup _cg( cg );
//	QColor c = _cg.text();

  if (itemPos() % (2*height()))
    _cg.setColor(QColorGroup::Base, setting->colorAltBackground);//QColor::QColor(242,242,242,QColor::Rgb));//cg.color(QColorGroup::Midlight));//QColor::QColor("AliceBlue")); 

	if (its_me)
		_cg.setColor(QColorGroup::Text, Qt::blue);
	else if (watched)
		_cg.setColor(QColorGroup::Text, Qt::red);

	_cg.setColor(QColorGroup::Background, setting->colorBackground);

	QListViewItem::paintCell(p, _cg, column, width, alignment);

//	_cg.setColor(QColorGroup::Text, c);
}

void GamesTableItem::ownRepaint()
{
	if (text(12))
	{
		its_me = text(12).at(0) == 'A';
		watched = text(12).at(text(7).length()-1) == 'W';
	}
	else
	{
		watched = false;
		its_me = false;
	}
}

// for correct sorting by rk and optionally txt
QString GamesTableItem::key(int column, bool /*ascending*/) const
{
	switch (column)
	{

		// rank, however, considered to be most used...
		case 2:
			// return invisible column's text
			return text(12);

		// rank w/b
		// case 2 is original here, but it's slow
//		case 2:
		case 4:
			return rkToKey(text(column)) + text(column - 1).lower();
			break;

		// id, move, observe, Ob
		case 0:
		case 5:
		case 11:
			return text(column).stripWhiteSpace().rightJustify(3, '0') + text(12);
			break;

		// Sz, By
		case 6:
		case 9:
			return text(column).stripWhiteSpace().rightJustify(2, '0') + text(12);
			break;

		// K
		case 7:
			return text(column).stripWhiteSpace().rightJustify(5, '0') + text(12);
			break;
			
		default:
			return text(column).lower() + text(12);
			break;
	}
}
