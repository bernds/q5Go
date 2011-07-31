/*
 *   playertable.cpp
 */

#include "playertable.h"
#include "misc.h"
#include "setting.h"

#include <qheader.h>
#include <qvariant.h>
#include <qwhatsthis.h>
#include <qevent.h>
#include <qlistview.h>


PlayerTable::PlayerTable(QWidget *parent, const char *name, bool /*modal*/, WFlags fl)
	: QListView(parent, name, fl)
{
	addColumn(QObject::tr("Stat", "PlayerTable: status [X!SQ]"));
	addColumn(QObject::tr("Name", "PlayerTable: player's name"));
	addColumn(QObject::tr("Rk", "PlayerTable: rank"));
	addColumn(QObject::tr("pl", "PlayerTable: playing in game"));
	addColumn(QObject::tr("ob", "PlayerTable: observing game"));
	addColumn(QObject::tr("Idle", "PlayerTable: idle time"));
	addColumn("X");
	setColumnAlignment(3, AlignRight);
	setColumnAlignment(4, AlignRight);
	setColumnAlignment(5, AlignRight);
	setColumnAlignment(6, AlignRight);
	setProperty( "focusPolicy", (int)QListView::NoFocus );
	setProperty( "resizePolicy", (int)QListView::AutoOneFit );

	// set sorting order for players by rank
	setSorting(2);
	setAllColumnsShowFocus(true);
}


void PlayerTable::showOpen(bool show)
{
	QListViewItemIterator lv(this);
	
	for (; lv.current() ;lv++)
	{
		// player is not open or is playing a match
		if ((lv.current()->text(0).contains('X')) || (!lv.current()->text(3).contains('-')))
			lv.current()->setVisible(!show);
		
	}

}
/*
 *   PlayerTableItem
 */

PlayerTableItem::PlayerTableItem(PlayerTable *parent, const char* name)
	: QListViewItem(parent, name)  
{
;
}

PlayerTableItem::PlayerTableItem(PlayerTableItem *parent, const char* name)
	: QListViewItem(parent, name)  
{
;
}

PlayerTableItem::PlayerTableItem(PlayerTable *parent, QString label1, QString label2,
                QString label3, QString label4, QString label5,
                QString label6, QString label7, QString label8,
				QString label9, QString label10, QString label11, QString label12, QString label13)
	: QListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
{
	open = label1.contains('X') == 0;
	if (label7)
	{
		watched = label7[0] == 'W';
		its_me = label7[0] == 'M';
		exclude = label7[0] == 'X';
	}
	else
	{
		watched = false;
		its_me = false;
		exclude = false;
	}
	seeking = false;
	
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

PlayerTableItem::~PlayerTableItem()
{
}

void PlayerTableItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int alignment )
{
	QColorGroup _cg( cg );


	if (itemPos() % (2*height()))
		_cg.setColor(QColorGroup::Base, setting->colorAltBackground);//QColor::QColor(242,242,242,QColor::Rgb));//cg.color(QColorGroup::Midlight));//QColor::QColor("AliceBlue")); 
  
	if (its_me)
		_cg.setColor(QColorGroup::Text, Qt::blue);
	else if (watched)
		_cg.setColor(QColorGroup::Text, Qt::darkGreen);
	else if (!open)
		_cg.setColor(QColorGroup::Text, Qt::gray);
	else if (exclude)
		_cg.setColor(QColorGroup::Text, Qt::red);
	else if (seeking)
		_cg.setColor(QColorGroup::Text, Qt::magenta);

	QListViewItem::paintCell(p, _cg, column, width, alignment);

}

void PlayerTableItem::ownRepaint()
{
	open = text(0).contains('X') == 0;
	if (text(6))
	{
		watched = text(6).at(0) == 'W';
		its_me = text(6).at(0) == 'M';
		exclude = text(6).at(0) == 'X';
	}
	else
	{
		watched = false;
		its_me = false;
		exclude = false;
	}
}

int PlayerTableItem::compare(  QListViewItem *p, int col, bool ascending ) const
{
	if (col == 2)
		return  text(12).compare( p->text(12) );

	return  QListViewItem::compare( p, col , ascending );
}


void PlayerTableItem::set_nmatchSettings(Player *p)
{
	nmatch = p->nmatch;

	nmatch_black = 		p->nmatch_black ;
	nmatch_white = 		p->nmatch_white;
	nmatch_nigiri = 	p->nmatch_nigiri ;
	nmatch_handicapMin = 	p->nmatch_handicapMin;
	nmatch_handicapMax  = 	p->nmatch_handicapMax;
	nmatch_timeMin = 	p->nmatch_timeMin;
	nmatch_timeMax  = 	p->nmatch_timeMax;
	nmatch_BYMin = 		p->nmatch_BYMin;
	nmatch_BYMax = 		p->nmatch_BYMax;
	nmatch_stonesMin = 	p->nmatch_stonesMin;
	nmatch_stonesMax = 	p->nmatch_stonesMax;
	nmatch_KoryoMin = 	p->nmatch_KoryoMin;
	nmatch_KoryoMax = 	p->nmatch_KoryoMax ;

	nmatch_settings =  !(p->nmatch_settings == "No match conditions");

}

