#include "goboard.h"
#include "gogame.h"

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

