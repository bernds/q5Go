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

#include "defines.h"
#include "goboard.h"
#include "grid.h"
#include "setting.h"

 /**
  * Initialises the grid intersections and hoshis points
  **/
Grid::Grid (QGraphicsScene *canvas, const go_board &ref, const bit_array &hoshis)
	: m_ref_board (ref), m_hoshi_map (hoshis),
	  m_hgrid (ref.bitsize ()), m_vgrid (ref.bitsize ()), m_covers (ref.bitsize ()),
	  m_hoshis (hoshis.popcnt ()), m_hoshi_pos (hoshis.popcnt ()), m_hoshi_screen_pos (hoshis.popcnt ())
{
	for (auto &i: m_vgrid)
		canvas->addItem (&i);
	for (auto &i: m_hgrid)
		canvas->addItem (&i);
	for (auto &i: m_covers) {
		canvas->addItem (&i);
		i.setBrush (QColor (128, 128, 255, 128));
		i.setPen (Qt::NoPen);
		i.setAcceptedMouseButtons (Qt::NoButton);
		i.setVisible (false);
	}
	for (auto &i: m_hoshis) {
		i.setBrush (Qt::SolidPattern);
		i.setPen (Qt::NoPen);
		canvas->addItem (&i);
	}

	int n = 0;
	for (int x = 0; x < ref.size_x (); x++)
		for (int y = 0; y < ref.size_y (); y++) {
			int p = ref.bitpos (x, y);
			if (hoshis.test_bit (p)) {
				m_hoshi_screen_pos[n] = { x, y };
				m_hoshi_pos[n++] = { x, y };
			}
		}

	showAll ();
}

 /**
  * Calculates the grid intersections and hoshis position
  **/
void Grid::resize (const QRect &rect, int shift_x, int shift_y, double square_size)
{
	int i,j;
	int size = square_size / 5;

	QPen pen;
	int scaled_w = 1;
	int widened_w = 2;

	int szx = m_ref_board.size_x ();
	int szy = m_ref_board.size_y ();
	for (i = 0; i < szx; i++)
		for (j = 0; j < szy; j++) {
			int x = (i + shift_x + szx) % szx;
			int y = (j + shift_y + szy) % szy;

			int bp = i + j * szx;
			bool first_col = x == 0;
			bool last_col = x + 1 == szx;
			bool first_row = y == 0;
			bool last_row = y + 1 == szy;

			pen.setWidth ((first_col || last_col) ? widened_w : scaled_w);
			m_vgrid[bp].setPen (pen);
			pen.setWidth ((first_row || last_row) ? widened_w : scaled_w);
			m_hgrid[bp].setPen (pen);

			if (m_ref_board.torus_h ())
				first_col = last_col = false;
			if (m_ref_board.torus_v ())
				first_row = last_row = false;
			m_hgrid[bp].setLine(int(rect.x () + square_size * (i - 0.5 * !first_col)),
					    rect.y () + square_size * j,
					    int(rect.x () + square_size * (i + 0.5 * !last_col)),
					    rect.y () + square_size * j);

			m_vgrid[bp].setLine(rect.x () + square_size * i,
					    int(rect.y () + square_size * (j - 0.5 * !first_row)),
					    rect.x () + square_size *  i,
					    int(rect.y () + square_size * (j + 0.5 * !last_row)));
			int left = rect.x () + square_size * (i - 0.5);
			int top = rect.y () + square_size * (j - 0.5);
			int right = rect.x () + square_size * (i + 0.5);
			int bot = rect.y () + square_size * (j + 0.5);
			m_covers[bp].setRect (left, top, right - left, bot - top);
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

		int rx = (x + szx - shift_x) % szx;
		int ry = (y + szy - shift_y) % szy;
		m_hoshi_screen_pos[i] = { rx, ry };
		e->setRect (rect.x () + square_size * rx - size/2,
			    rect.y () + square_size * ry - size/2,
			    size, size);
	}
}

/* Rectangle R1 is a selection rectangle given to us by the rubber band.  Apply it
   by affecting item visibility rather than selection state.  We could make the
   items selectable and use that, but it looks messy and ugly, hence this method.  */
bool Grid::apply_selection (const QRect &r1)
{
	bool result = false;
	QRectF r = r1;
	for (auto &c: m_covers) {
		bool shown = r.contains (c.rect ().center ());
		result |= shown;
		c.setVisible (shown);
		c.update ();
	}
	return result;
}

bit_array Grid::selected_items ()
{
	bit_array result (m_ref_board.bitsize ());
	for (int i = 0; i < m_ref_board.bitsize (); i++)
		if (m_covers[i].isVisible ())
			result.set_bit (i);

	return result;
}

void Grid::set_removed_points (const bit_array &mask)
{
	for (int i = 0; i < m_ref_board.bitsize (); i++) {
		QPen p = m_vgrid[i].pen ();
		p.setColor (mask.test_bit (i) ? Qt::white : Qt::black);
		m_vgrid[i].setPen (p);
		m_vgrid[i].update ();
		p = m_hgrid[i].pen ();
		p.setColor (mask.test_bit (i) ? Qt::white : Qt::black);
		m_hgrid[i].setPen (p);
		m_hgrid[i].update ();
	}
}

 /**
  * Resets all interctions and hoshis to be shown
  **/
void Grid::showAll ()
{
	for (auto &i: m_vgrid)
		i.show ();
	for (auto &i: m_hgrid)
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

	m_vgrid[bp].hide();
	m_hgrid[bp].hide();

	for (size_t i = 0; i < m_hoshis.size (); i++)
		if (m_hoshi_screen_pos[i] == std::pair<int, int> (x, y)) {
			m_hoshis[i].hide ();
			break;
		}
}

CoordDisplay::CoordDisplay (QGraphicsScene *canvas, const go_board &ref, int hdups, int vdups, int offs, int margin, bool sgf)
	: m_ref_board (ref), m_h_dups (hdups), m_v_dups (vdups),
	  m_coords_v1 (ref.size_y ()), m_coords_v2 (ref.size_y ()), m_coords_h1 (ref.size_x ()), m_coords_h2 (ref.size_x ()),
	  m_coord_offset (offs), m_coord_margin (margin)
{
	for (auto &i: m_coords_h1)
		canvas->addItem (&i);
	for (auto &i: m_coords_h2)
		canvas->addItem (&i);
	for (auto &i: m_coords_v1)
		canvas->addItem (&i);
	for (auto &i: m_coords_v2)
		canvas->addItem (&i);
	set_texts (sgf);
}

void CoordDisplay::set_texts (bool sgf)
{
	for (int i = 0; i < m_ref_board.size_x (); i++) {
		QString hTxt;
		if (sgf)
			hTxt = QChar ('a' + i);
		else {
			int real_i = i < 8 ? i : i + 1;
			hTxt = QChar ('A' + real_i);
		}

		m_coords_h1[i].setText (hTxt);
		m_coords_h2[i].setText (hTxt);
	}
	for (int i = 0; i < m_ref_board.size_y (); i++) {
		QString vTxt;
		if (sgf)
			vTxt = QChar ('a' + i);
		else
			vTxt = QString::number (i + 1);

		m_coords_v1[i].setText (vTxt);
		m_coords_v2[i].setText (vTxt);
	}
}

void CoordDisplay::retrieve_text (QString &xt, QString &yt, int x, int y)
{
	xt = m_coords_h1[x].text ();
	yt = m_coords_v1[m_ref_board.size_y () - y - 1].text ();
}

void CoordDisplay::resize (const QRect &wrect, const QRect &brect, int shift_x, int shift_y,
			   double square_size, bool show)
{
	// centres the coordinates text within the remaining space at table edge
	const int center = m_coord_offset / 2 + m_coord_margin;

	int sz_x = m_ref_board.size_x ();
	int sz_y = m_ref_board.size_y ();
	for (int i = 0; i < sz_y; i++) {
		int y = (i + sz_y - shift_y) % sz_y;
		QGraphicsSimpleTextItem *coord = &m_coords_v1[y];
		int ypos = brect.y () + square_size * (m_v_dups + sz_y - i - 1) - coord->boundingRect().height() / 2;
		// Left side
		coord->setPos (wrect.x () + center - coord->boundingRect().width() / 2, ypos);

		if (show)
			coord->show();
		else
			coord->hide();

		// Right side
		coord = &m_coords_v2[y];
		coord->setPos (wrect.x () + wrect.width () - center - coord->boundingRect().width() / 2, ypos);

		if (show)
			coord->show();
		else
			coord->hide();
	}
	for (int i = 0; i < sz_x; i++) {
		int x = (i + shift_x) % sz_x;
		QGraphicsSimpleTextItem *coord = &m_coords_h1[x];
		int xpos = brect.x () + square_size * (m_h_dups + i) - coord->boundingRect().width() / 2;
		// Top
		coord->setPos (xpos, wrect.y () + center - coord->boundingRect().height() / 2);

		if (show)
			coord->show();
		else
			coord->hide();

		// Bottom
		coord = &m_coords_h2[x];
		coord->setPos (xpos, wrect.y () + wrect.height () - center - coord->boundingRect().height() / 2);

		if (show)
			coord->show();
		else
			coord->hide();
	}
}
