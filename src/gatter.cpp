/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <vector>
#include <QtGui>

#include "defines.h"
#include "gatter.h"


 /**
  * Initialises the gatter intersections and hoshis points
  **/
Gatter::Gatter(QGraphicsScene *Canvas, int size)
{
	int i,j;

	board_size = size;

	VGatter.reserve(board_size);
	HGatter.reserve(board_size);
	for (i=0; i<board_size; i++)
	{
		std::vector<QGraphicsLineItem *> row,col;
		row.reserve(board_size);
		col.reserve(board_size);
		VGatter.push_back(row);
		HGatter.push_back(col);

		for (j=0; j<board_size; j++)
		{
			VGatter[i].push_back(new QGraphicsLineItem(0,Canvas));
			HGatter[i].push_back(new QGraphicsLineItem(0,Canvas));
			Q_CHECK_PTR(VGatter[i][j]);
			Q_CHECK_PTR(HGatter[i][j]);
		}
	}

	int edge_dist = (board_size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (board_size + 1) / 2;
	int high = board_size + 1 - edge_dist;
	if (board_size % 2 && board_size > 9)
	{
		hoshisList.insert(middle*board_size + low , new QGraphicsEllipseItem(0,Canvas));
		hoshisList.insert(middle*board_size + middle , new QGraphicsEllipseItem(0,Canvas));
		hoshisList.insert(middle*board_size + high , new QGraphicsEllipseItem(0,Canvas));
		hoshisList.insert(low*board_size + middle , new QGraphicsEllipseItem(0,Canvas));
		hoshisList.insert(high*board_size + middle , new QGraphicsEllipseItem(0,Canvas));
	}
	hoshisList.insert(low*board_size + low ,new QGraphicsEllipseItem(0,Canvas));
	hoshisList.insert(high*board_size + low , new QGraphicsEllipseItem(0,Canvas));
	hoshisList.insert(high*board_size + high , new QGraphicsEllipseItem(0,Canvas));
	hoshisList.insert(low*board_size + high ,new QGraphicsEllipseItem(0,Canvas));


	QMapIterator<int,QGraphicsEllipseItem*> it( hoshisList );

	for ( ; it.hasNext() ; )
	{
		it.next();
		it.value()->setBrush(Qt::SolidPattern);
		it.value()->setPen(Qt::NoPen);
	}

	showAll();
}

 /**
  * Destroys the gatter
  **/
Gatter::~Gatter()
{
	int i,j;


	for (i=0; i<board_size; i++)
 	{
		for (j=0; j<board_size; j++)
 		{
 			delete VGatter[i][j];
			delete HGatter[i][j];
		}
	VGatter[i].clear();
	HGatter[i].clear();
	}

	VGatter.clear();
	HGatter.clear();

	for (QMap<int,QGraphicsEllipseItem*>::const_iterator i = hoshisList.begin();
		 i != hoshisList.end(); ++i)
		delete *i;
	hoshisList.clear();
}



 /**
  * Calculates the gatter intersections and hoshis position
  **/
void Gatter::resize(int offsetX, int offsetY, double square_size)
{
	int i,j;
	QGraphicsEllipseItem *e;
	QMapIterator<int,QGraphicsEllipseItem*> it( hoshisList );

	int size = square_size / 5;

	// Round size top be odd (hoshis)
	if (size % 2 == 0)
		size--;
	if ((size < 7) && (size>2))
		size = 7;
	else if (size <= 2)
		size = 3;


	for (i=0; i<board_size; i++)
		for (j=0; j<board_size; j++)
		{

			HGatter[i][j]->setLine(int(offsetX + square_size * ( i - 0.5*(i!=0))),
						offsetY + square_size * j,
						int(offsetX + square_size * ( i + 0.5 * (i+1 != board_size))),
						offsetY + square_size * j );

			VGatter[i][j]->setLine(offsetX + square_size *  i,
						int(offsetY + square_size * ( j - 0.5*(j!=0))),
						offsetX + square_size *  i,
						int(offsetY + square_size * ( j + 0.5 * (j+1 != board_size))));


			if (hoshisList.contains(board_size*(i+1)+j+1))
			{
				e = hoshisList.value(board_size*(i+1)+j+1);
				e->setRect(offsetX + square_size * i - size/2,
					offsetY + square_size * j- size/2,
					size ,
					size );
			}
		}
}

 /**
  * Resets all interctions and hoshis to be shown
  **/
void Gatter::showAll()
{
	int i,j;

	for (i=0; i<board_size; i++)
		for (j=0; j<board_size; j++)
		{
			VGatter[i][j]->show();
			HGatter[i][j]->show();
		}

	QMapIterator<int,QGraphicsEllipseItem*> it( hoshisList );

	for ( ; it.hasNext() ; )
	{
		it.next();
		it.value()->show();
	}
}

 /**
  * Hides an intersection (when placing a letter mark)
  **/
void Gatter::hide(int i, int j)
{
	QGraphicsEllipseItem *e;

	if (( i<1) || (i > board_size) || ( j<1) || (j > board_size))
		return;

	VGatter[i-1][j-1]->hide();
	HGatter[i-1][j-1]->hide();

	if (hoshisList.contains(board_size*i + j))
	{
		e = hoshisList.value(board_size*i+j);
		e->hide();
	}

}

 /**
  * shows an intersection (when removing a letter mark)
  **/
void Gatter::show(int i, int j)
{
	QGraphicsEllipseItem *e;

	if (( i<1) || (i > board_size) || ( j<1) || (j > board_size))
		return;

	VGatter[i-1][j-1]->show();
	HGatter[i-1][j-1]->show();

	if (hoshisList.contains(board_size*i + j))
	{
		e = hoshisList.value(board_size*i+j);
		e->show();
	}
}

