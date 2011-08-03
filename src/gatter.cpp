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
Gatter::Gatter(Q3Canvas *Canvas, int size)
{
	int i,j;

	board_size = size;
	canvas=Canvas;	

	VGatter.reserve(board_size);
	HGatter.reserve(board_size);
	for (i=0; i<board_size; i++)
	{	
		std::vector<Q3CanvasLine *> row,col;
		row.reserve(board_size);
		col.reserve(board_size);
		VGatter.push_back(row);
		HGatter.push_back(col);
		
		for (j=0; j<board_size; j++)
		{
			VGatter[i].push_back(new Q3CanvasLine(canvas));
			HGatter[i].push_back(new Q3CanvasLine(canvas));
			CHECK_PTR(VGatter[i][j]);
			CHECK_PTR(HGatter[i][j]);
		}
	}
	
	int edge_dist = (board_size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (board_size + 1) / 2;
	int high = board_size + 1 - edge_dist;
	if (board_size % 2 && board_size > 9)
	{
		hoshisList.insert(middle*board_size + low , new Q3CanvasEllipse(canvas));
		hoshisList.insert(middle*board_size + middle , new Q3CanvasEllipse(canvas));
		hoshisList.insert(middle*board_size + high , new Q3CanvasEllipse(canvas));
		hoshisList.insert(low*board_size + middle , new Q3CanvasEllipse(canvas));
		hoshisList.insert(high*board_size + middle , new Q3CanvasEllipse(canvas));
	}
	hoshisList.insert(low*board_size + low ,new Q3CanvasEllipse(canvas));
	hoshisList.insert(high*board_size + low , new Q3CanvasEllipse(canvas));
	hoshisList.insert(high*board_size + high , new Q3CanvasEllipse(canvas));
	hoshisList.insert(low*board_size + high ,new Q3CanvasEllipse(canvas));

	Q3IntDictIterator<Q3CanvasEllipse> it( hoshisList );
	for ( ; it.current(); ++it )
        	it.current()->setBrush(Qt::black);

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

	Q3IntDictIterator<Q3CanvasEllipse> it( hoshisList );
	for ( ; it.current(); ++it )
        	delete it.current(); 

}



 /**
  * Calculates the gatter intersections and hoshis position
  **/
void Gatter::resize(int offsetX, int offsetY, int square_size)
{
	int i,j;
	Q3CanvasEllipse *e;

	int size = square_size / 5;
	// Round size top be even
	if (size % 2 > 0)
		size--;
	if (size < 6)
		size = 6;


	for (i=0; i<board_size; i++)
		for (j=0; j<board_size; j++)
		{
			HGatter[i][j]->setPoints(int(offsetX + square_size * ( i - 0.5*(i!=0))), 
						offsetY + square_size * j,
						int(offsetX + square_size * ( i + 0.5 * (i+1 != board_size))), 
						offsetY + square_size * j );
			
			VGatter[i][j]->setPoints(offsetX + square_size *  i, 
						int(offsetY + square_size * ( j - 0.5*(j!=0))),
						offsetX + square_size *  i, 
						int(offsetY + square_size * ( j + 0.5 * (j+1 != board_size)))); 
			
			e=hoshisList.find(board_size*(i+1)+j+1);
			if (e)
			{
				e->setSize(size, size);
    				e->setX(offsetX + square_size * i);
    				e->setY(offsetY + square_size * j);
			}
		}

}

 /**
  * Resets all interctions and hoshis to be shown
  **/
void Gatter::showAll()
{
	int i,j;
	Q3CanvasEllipse *e;

	for (i=0; i<board_size; i++)
		for (j=0; j<board_size; j++)
		{
			VGatter[i][j]->show();
			HGatter[i][j]->show();
		}

	Q3IntDictIterator<Q3CanvasEllipse> it( hoshisList );
	for ( ; it.current(); ++it )
        	it.current()->show();
}

 /**
  * Hides an intersection (when placing a letter mark)
  **/
void Gatter::hide(int i, int j)
{
	Q3CanvasEllipse *e;
	
	if (( i<1) || (i > board_size) || ( j<1) || (j > board_size))
		return;

	VGatter[i-1][j-1]->hide();
	HGatter[i-1][j-1]->hide();

	e=hoshisList.find(board_size*i+j);
	if (e)
		e->hide();

}

 /**
  * shows an intersection (when removing a letter mark)
  **/
void Gatter::show(int i, int j)
{
	Q3CanvasEllipse *e;

	if (( i<1) || (i > board_size) || ( j<1) || (j > board_size))
		return;

	VGatter[i-1][j-1]->show();
	HGatter[i-1][j-1]->show();

	e=hoshisList.find(board_size*i+j);
	if (e)
		e->show();

}

