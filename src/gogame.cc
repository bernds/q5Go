#include "goboard.h"
#include "gogame.h"
#include "svgbuilder.h"

bool game_state::valid_move_p (int x, int y, stone_color col)
{
	return m_board.valid_move_p (x, y, col);
}

const go_board game_state::child_moves (const game_state *excluding) const
{
	go_board b (m_board.size ());
	size_t n = 0;
	for (auto &it: m_children) {
		if (it == excluding)
			continue;
		/* It's unclear if letters should skip the current variation,
		   which would make them consistent when displaying any of
		   the siblings.  But that would mean that the main branch
		   would start labelling variations with B instead of A,
		   which is not what we want.  So, increment here instead of
		   before the previous test.  */
		n++;
		if (it->m_move_x < 0)
			continue;
		b.set_stone (it->m_move_x, it->m_move_y, it->m_move_color);
		b.set_mark (it->m_move_x, it->m_move_y, mark::letter, n - 1);
	}
	return b;
}

visual_tree::visual_tree (visual_tree &main_var, int max_child_width)
	: m_w (1 + max_child_width), m_h (main_var.m_h)
{
	for (int i = 0; i < m_h; i++) {
		m_rep.push_back (bit_array (m_w));
		m_rep[i].ior (main_var.m_rep[i], 1);
	}
	main_var.m_off_y = 0;
	m_rep[0].set_bit (0);
}

void visual_tree::add_variation (visual_tree &other)
{
	if (other.m_w + 1 > m_w) {
		m_w = other.m_w + 1;
		for (auto &vec: m_rep)
			vec.grow (m_w);
	}

	int i;
	/* Calculate the amount of overlap, as I lines.  */
	for (i = 0; i < m_h - 1; i++) {
		if (m_rep[m_h - i - 1].intersect_p (other.m_rep[0], 1))
			break;
	}
	other.m_off_y = m_h - i;
	m_h = std::max (m_h, other.m_off_y + other.m_h);
	for (int j = 0; j < other.m_h; j++) {
		if (j >= i)
			m_rep.push_back (bit_array (m_w));
		m_rep[other.m_off_y + j].ior (other.m_rep[j], 1);
	}
	/* Show conflicts for the connecting lines as well.  */
	for (int y = 0; y < other.m_off_y; y++)
		m_rep[y].set_bit (0);
}

bool game_state::update_visualization ()
{
	bool changes = false;
	int max_width = 1;
	for (auto &it: m_children) {
		changes |= it->update_visualization ();
		max_width = std::max (max_width, it->m_visualized.width ());
	}
	if (!changes && m_visual_ok)
		return false;
	if (m_children.size () == 0 || m_visual_collapse) {
		m_visualized = visual_tree (m_children.size () > 0 && m_visual_collapse);
	} else {
		bool first = true;
		for (auto &it: m_children) {
			if (first)
				m_visualized = visual_tree (it->m_visualized, max_width);
			else
				m_visualized.add_variation (it->m_visualized);
			first = false;
		}
	}
	m_visual_ok = true;
	return true;
}

/* CX and CY are the cumulative offsets from the root node, and should count in pixels.
   They give the center of the node; modulo that they are multiples of SIZE.  */
void game_state::render_visualization (int cx, int cy, int size,
				       const draw_circle &circle_fn, const draw_special &special_fn, const draw_line &line_fn)
{
	size_t n_children = m_children.size ();

	if (m_visual_collapse && n_children > 0) {
		line_fn (cx, cy, cx + size, cy, true);
		special_fn (this, cx, cy, true);
		return;
	}

	if (n_children > 0 && !m_visual_collapse) {
		game_state *last = m_children.back ();
		int yoff = last->m_visualized.y_offset () - 1;
		if (n_children > 1 && yoff > 0) {
			line_fn (cx, cy + size * 0.45, cx, cy + size * yoff, false);
		}
		for (auto it: m_children) {
			int y0 = it->m_visualized.y_offset () * size;
			int y1 = y0;
			if (y0 > 0)
				y0 -= size;
			line_fn (cx, cy + y0, cx + size, cy + y1, false);
		}
	}
	if (m_move_color == none)
		special_fn (this, cx, cy, false);
	else
		circle_fn (this, cx, cy, m_move_color);

	for (auto &it: m_children) {
		int yoff = it->m_visualized.y_offset ();
		it->render_visualization (cx + size, cy + size * yoff, size,
					  circle_fn, special_fn, line_fn);
	}
}

void game_state::render_active_trace (int cx, int cy, int size, const add_point &point_fn,
				      const draw_line &line_fn)
{
	point_fn (cx, cy);
	if (m_children.size () == 0)
		return;

	game_state *c = m_children[m_active];
	int yoff = c->m_visualized.y_offset ();
	if (m_active > 0 && yoff > 1)
		point_fn (cx, cy + size * (yoff - 1));
	if (m_visual_collapse) {
		line_fn (cx, cy, cx + size, cy, true);
		return;
	}
	c->render_active_trace (cx + size, cy + yoff * size, size, point_fn, line_fn);
}

bool game_state::locate_visual (int x, int y, const game_state *active, int &ax, int &ay)
{
	if (this == active) {
		ax = x;
		ay = y;
		return true;
	}
	for (auto &it: m_children) {
		int yoff = it->m_visualized.y_offset ();

		if (it->locate_visual (x + 1, y + yoff, active, ax, ay)) {
			if (m_visual_collapse) {
				ax = x;
				ay = y;
			}
			return true;
		}
	}
	return false;
}

bool game_state::vis_expand_one ()
{
	if (!m_visual_collapse)
		return false;
	toggle_vis_collapse ();
	for (auto it: m_children)
		if (!it->m_visual_collapse)
			it->toggle_vis_collapse ();
	return true;
}
