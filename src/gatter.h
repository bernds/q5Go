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


#ifndef GATTER_H
#define GATTER_H

#include "imagehandler.h"

#include <vector>

#include <QtGui>
#include <q3intdict.h>

class Gatter
{
public:
	Gatter(Q3Canvas *Canvas, int board_size);
	~Gatter();
	void hide (int x, int y);
	void show (int x, int y);
	void resize(int offsetX, int offsetY, int square_size);
	void showAll();

private:
	int board_size;
	Q3Canvas *canvas;
	std::vector< std::vector<Q3CanvasLine *> > VGatter, HGatter ;
	Q3IntDict<Q3CanvasEllipse> hoshisList ;
};

#endif
