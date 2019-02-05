#include "goboard.h"
#include "gogame.h"
#include "svgbuilder.h"

bool game_state::valid_move_p (int x, int y, stone_color col)
{
	return m_board.valid_move_p (x, y, col);
}

const go_board game_state::child_moves (const game_state *excluding) const
{
	go_board b (m_board.size_x (), m_board.size_y ());
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
	: m_rep (1 + max_child_width, main_var.height ())
{
	m_rep.ior (main_var.m_rep, 1, 0);
	main_var.m_off_y = 0;
	m_rep.set_bit (0, 0);
}

void visual_tree::add_variation (visual_tree &other)
{
	m_rep.set_max_width (other.width () + 1);

	/* Find the highest offset that does not cause overlap.  */
	int i = height ();
	while (i-- > 0)
		if (m_rep.test_overlap (other.m_rep, 1, i))
			break;

	other.m_off_y = i + 1;
	m_rep.set_max_height (std::max (height (), other.m_off_y + other.height ()));
	m_rep.ior (other.m_rep, 1, other.m_off_y);

	/* Show conflicts for the connecting lines as well.  */
	for (int y = 1; y < other.m_off_y; y++)
		m_rep.set_bit (0, y);
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
void game_state::render_visualization (int cx, int cy, int size, const draw_line &line_fn, bool first)
{
	size_t n_children = m_children.size ();

	if (m_visual_collapse && n_children > 0) {
		line_fn (cx, cy, cx + size, cy, true);
		return;
	}

	if (n_children > 0 && !m_visual_collapse) {
		if (first) {
			game_state *p = this;
			int count = 0;
			while (p && !p->m_visual_collapse && p->m_children.size () > 0) {
				count++;
				p = p->m_children[0];
			}
			line_fn (cx, cy, cx + size * count, cy, false);
		}
		game_state *last = m_children.back ();
		int yoff = last->m_visualized.y_offset () - 1;
		if (n_children > 1 && yoff > 0) {
			line_fn (cx, cy + size * 0.45, cx, cy + size * yoff, false);
		}
		for (auto it: m_children) {
			int y0 = it->m_visualized.y_offset () * size;
			int y1 = y0;
			if (y0 > 0) {
				y0 -= size;
				line_fn (cx, cy + y0, cx + size, cy + y1, false);
			}
		}
	}

	for (auto &it: m_children) {
		int yoff = it->m_visualized.y_offset ();
		it->render_visualization (cx + size, cy + size * yoff, size,
					  line_fn, it != m_children[0]);
	}
}

void game_state::extract_visualization (int x, int y,
					visual_tree::bit_rect &stones_w,
					visual_tree::bit_rect &stones_b,
					visual_tree::bit_rect &edits,
					visual_tree::bit_rect &collapsed)
{
	size_t n_children = m_children.size ();

	if (m_visual_collapse && n_children > 0) {
		collapsed.set_bit (x, y);
		return;
	}
	switch (m_move_color) {
	case none:
		edits.set_bit (x, y);
		break;
	case white:
		stones_w.set_bit (x, y);
		break;
	case black:
		stones_b.set_bit (x, y);
		break;
	}

	for (auto &it: m_children) {
		int yoff = it->m_visualized.y_offset ();
		it->extract_visualization (x + 1, y + yoff, stones_w, stones_b, edits, collapsed);
	}
}

void game_state::render_active_trace (int cx, int cy, int size, const add_point &point_fn,
				      const draw_line &line_fn)
{
	size_t n_children = m_children.size ();
	if (n_children == 0 || m_active > 0 || m_parent == nullptr || m_parent->m_active != 0)
		point_fn (cx, cy);
	if (n_children == 0)
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

game_state *game_state::locate_by_vis_coords (int x, int y, int off_x, int off_y)
{
	if (off_x == x && off_y == y)
		return this;
	if (x < off_x || y < off_y)
		return nullptr;
	if (x >= off_x + m_visualized.width () || y >= off_y + m_visualized.height ())
		return nullptr;
	for (auto &it: m_children) {
		game_state *ret = it->locate_by_vis_coords (x, y, off_x + 1, off_y + it->m_visualized.y_offset ());
		if (ret != nullptr)
			return ret;
	}
	return nullptr;
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


void navigable_observer::next_move ()
{
	game_state *next = m_state->next_move ();
	if (next == nullptr)
		return;
	move_state (next);
}

void navigable_observer::previous_move ()
{
	game_state *next = m_state->prev_move ();
	if (next == nullptr)
		return;
	move_state (next);
}

void navigable_observer::previous_comment ()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->comment () != "") {
			move_state (st);
			break;
		}
	}
}

void navigable_observer::next_comment ()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->comment () != "") {
			move_state (st);
			break;
		}
	}
}

void navigable_observer::previous_count ()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->prev_move ();
		if (st != nullptr && st->get_start_count ()) {
			move_state (st);
			break;
		}
	}
}

void navigable_observer::next_count ()
{
	game_state *st = m_state;

	while (st != nullptr) {
		st = st->next_move ();
		if (st != nullptr && st->get_start_count ()) {
			move_state (st);
			break;
		}
	}
}

void navigable_observer::next_variation ()
{
	game_state *next = m_state->next_sibling (true);
	if (next == nullptr)
		return;
	if (next != m_state)
		move_state (next);
}

void navigable_observer::previous_variation ()
{
	game_state *next = m_state->prev_sibling (true);
	if (next == nullptr)
		return;
	if (next != m_state)
		move_state (next);
}

void navigable_observer::goto_first_move ()
{
	game_state *st = m_state;

	for (;;) {
		game_state *next = st->prev_move ();
		if (next == nullptr) {
			if (st != m_state)
				move_state (st);
			break;
		}
		st = next;
	}
}

void navigable_observer::goto_last_move ()
{
	game_state *st = m_state;

	for (;;) {
		game_state *next = st->next_move ();
		if (next == nullptr) {
			if (st != m_state)
				move_state (st);
			break;
		}
		st = next;
	}
}

void navigable_observer::find_move (int x, int y)
{
	game_state *st = m_state;
	if (!st->was_move_p () || st->get_move_x () != x || st->get_move_y () != y)
		st = m_game->get_root ();

	for (;;) {
		st = st->next_move ();
		if (st == nullptr)
			return;
		if (st->was_move_p () && st->get_move_x () == x && st->get_move_y () == y)
			break;
	}
	move_state (st);
}

void navigable_observer::goto_nth_move (int n)
{
	game_state *st = m_game->get_root ();
	while (st->move_number () < n && st->n_children () > 0) {
		st = st->next_move (true);
	}
	move_state (st);
}

void navigable_observer::goto_nth_move_in_var (int n)
{
	game_state *st = m_state;

	while (st->move_number () != n) {
		game_state *next = st->move_number () < n ? st->next_move () : st->prev_move ();
		/* Some added safety... this should not happen.  */
		if (next == nullptr)
			break;
		st = next;
	}
	move_state (st);
}

void navigable_observer::goto_main_branch ()
{
	game_state *st = m_state;
	game_state *go_to = st;
	while (st != nullptr) {
		while (st->has_prev_sibling ())
			st = go_to = st->prev_sibling (true);
		st = st->prev_move ();
	}
	if (go_to != st)
		move_state (go_to);
}

void navigable_observer::goto_var_start ()
{
	game_state *st = m_state->prev_move ();
	if (st == nullptr)
		return;
	/* Walk back, ending either at the root or at a node which has
	   more than one sibling.  */
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *prev = st->prev_move ();
		if (prev == nullptr)
			break;
		st = prev;
	}
	move_state (st);
}

void navigable_observer::goto_next_branch ()
{
	game_state *st = m_state;
	for (;;) {
		if (st->n_siblings () > 0)
			break;
		game_state *next = st->next_move ();
		if (next == nullptr)
			break;
		st = next;
	}
	if (st != m_state)
		move_state (st);
}
