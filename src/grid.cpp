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
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

#include "defines.h"
#include "goboard.h"
#include "grid.h"


 /**
  * Initialises the grid intersections and hoshis points
  **/
Grid::Grid (QGraphicsScene *canvas, const go_board &ref, const bit_array &hoshis)
	: m_ref_board (ref),
	  m_hoshi_map (hoshis), VGrid (ref.bitsize ()), HGrid (ref.bitsize ()),
	  m_hoshis (hoshis.popcnt ()), m_hoshi_pos (hoshis.popcnt ())
{
	for (auto &i: VGrid)
		canvas->addItem (&i);
	for (auto &i: HGrid)
		canvas->addItem (&i);
	for (auto &i: m_hoshis) {
		i.setBrush (Qt::SolidPattern);
		i.setPen (Qt::NoPen);
		canvas->addItem (&i);
	}

	int n = 0;
	for (int x = 0; x < ref.size (); x++)
		for (int y = 0; y < ref.size (); y++) {
			int p = ref.bitpos (x, y);
			if (hoshis.test_bit (p))
				m_hoshi_pos[n++] = std::pair<int, int> (x, y);
		}

	showAll ();
}

 /**
  * Calculates the grid intersections and hoshis position
  **/
void Grid::resize (int offsetX, int offsetY, double square_size)
{
	int i,j;
	int size = square_size / 5;

	for (i = 0; i < m_ref_board.size (); i++)
		for (j = 0; j < m_ref_board.size (); j++) {
			int bp = m_ref_board.bitpos (i, j);
			bool first_col = i == 0;
			bool last_col = i + 1 == m_ref_board.size ();
			bool first_row = j == 0;
			bool last_row = j + 1 == m_ref_board.size ();
			HGrid[bp].setLine(int(offsetX + square_size * (i - 0.5 * !first_col)),
					   offsetY + square_size * j,
					   int(offsetX + square_size * (i + 0.5 * !last_col)),
					   offsetY + square_size * j);

			VGrid[bp].setLine(offsetX + square_size * i,
					   int(offsetY + square_size * (j - 0.5 * !first_row)),
					   offsetX + square_size *  i,
					   int(offsetY + square_size * (j + 0.5 * !last_row)));
		}

	// Round size top be odd (hoshis)
	if (size % 2 == 0)
		size--;
	if (size < 7 && size > 2)
		size = 7;
	else if (size <= 2)
		size = 3;

	for (size_t i = 0; i < m_hoshis.size (); i++) {
		int x = m_hoshi_pos[i].first;
		int y = m_hoshi_pos[i].second;

		QGraphicsEllipseItem *e = &m_hoshis[i];

		e->setRect(offsetX + square_size * x - size/2, offsetY + square_size * y - size/2,
			   size , size);
	}
}

 /**
  * Resets all interctions and hoshis to be shown
  **/
void Grid::showAll ()
{
	for (auto &i: VGrid)
		i.show ();
	for (auto &i: HGrid)
		i.show ();
	for (auto &i: m_hoshis)
		i.show ();
}

 /**
  * Hides an intersection (when placing a letter mark)
  **/
void Grid::hide (int x, int y)
{
	int bp = m_ref_board.bitpos (x, y);

	VGrid[bp].hide();
	HGrid[bp].hide();

	for (size_t i = 0; i < m_hoshis.size (); i++)
		if (m_hoshi_pos[i] == std::pair<int, int> (x, y)) {
			m_hoshis[i].hide ();
			break;
		}
}
