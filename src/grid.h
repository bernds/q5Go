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


#ifndef GRID_H
#define GRID_H

#include <vector>

#include <QtGui>
#include <QMap>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>

class Grid
{
	const go_board m_ref_board;
	const bit_array m_hoshi_map;
	std::vector<QGraphicsLineItem> m_hgrid, m_vgrid;
	std::vector<QGraphicsRectItem> m_covers;
	std::vector<QGraphicsEllipseItem> m_hoshis;
	std::vector<std::pair <int, int> > m_hoshi_pos;
	std::vector<std::pair <int, int> > m_hoshi_screen_pos;

public:
	Grid (QGraphicsScene *Canvas, const go_board &ref, const bit_array &hoshis);

	bit_array selected_items ();
	void set_removed_points (const bit_array &);
	void hide (int x, int y);
	void resize (const QRect &r, int shift_x, int shift_y, double square_size);
	bool apply_selection (const QRect &r);
};

class CoordDisplay
{
	const go_board m_ref_board;
	int m_h_dups, m_v_dups;
	std::vector<QGraphicsSimpleTextItem> m_coords_v1, m_coords_v2;
	std::vector<QGraphicsSimpleTextItem> m_coords_h1, m_coords_h2;
	int m_coord_offset, m_coord_margin;

public:
	CoordDisplay (QGraphicsScene *Canvas, const go_board &ref, int h, int v, int offs, int margin, bool sgf);
	void resize (const QRect &w, const QRect &b, int shift_x, int shift_y, double square_size, bool);
	void set_texts (bool);
	void retrieve_text (QString &xt, QString &yt, int x, int y);
};

#endif
