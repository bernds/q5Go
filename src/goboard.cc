#include <string>
#include <exception>

#include "goboard.h"

void bit_array::debug () const
{
	for (unsigned bit = 0; bit < m_n_bits; bit++) {
		uint64_t val = m_bits[bit / 64];
		printf ("%d", (int)((val >> (bit % 64)) & 1));
	}
	putchar ('\n');
}

/* Keep some precomputed bit arrays, one for each board size, which
   have the left and right columns masked out.  These can be used in
   shift-and-and operations to find neighbours.  */
static std::map<int, bit_array> left_masks;
static std::map<int, bit_array> right_masks;

static const bit_array &get_boardmask_left (int n)
{
	auto it = left_masks.find (n);
	if (it != left_masks.end ())
		return it->second;
	bit_array a (n * n, true);
	for (int i = 0; i < n; i++)
		a.clear_bit (i * n);
	left_masks.insert (std::pair<int,bit_array> (n, a));
	it = left_masks.find (n);
	return it->second;
}

static const bit_array &get_boardmask_right (int n)
{
	auto it = right_masks.find (n);
	if (it != right_masks.end ())
		return it->second;
	bit_array a (n * n, true);
	for (int i = 0; i < n; i++)
		a.clear_bit (i * n + n - 1);
	right_masks.insert (std::pair<int,bit_array> (n, a));
	it = right_masks.find (n);
	return it->second;
}

bool bit_array::intersect_p (const bit_array &other, int shift) const
{
	shift = -shift;

	// int sgn = shift / std::abs (shift);

	/* Examples:
	   shift -3: right shift by 3.
	   Dest word 0 receives [word1:0-3 :: word0:3-64]
	   shift 3:  left shift by 3.
	   Dest word 0 receives [word0:0-61 :: 000 ]
	   Dest word 1 receives [word1:0-61 :: word0:62-64 ]
	   shift 64: left shift by 64.
	   Dest word 0 receives 64 bits from src word -1.  */

	shift += other.m_n_elts * 64;
	int wordshift = (shift + 63) / 64 - other.m_n_elts - 1;
	int bitshift = shift % 64;

	uint64_t last = 0;
	if (wordshift >= 0 && wordshift < other.m_n_elts)
		last = other.m_bits[wordshift];
	for (int i = 0; i < m_n_elts; i++) {
		wordshift++;
		uint64_t curr = 0;
		if (wordshift >= 0 && wordshift < other.m_n_elts)
			curr = other.m_bits[wordshift];
		uint64_t val = m_bits[i];
		if (bitshift != 0) {
			uint64_t andval = curr << (64 - bitshift);
			andval |= last >> bitshift;
			val &= andval;
		} else {
			val &= curr;
		}
		if (val != 0)
			return true;
		last = curr;
	}
	return false;
}

/* Extend a bit mask in all directions.  */
void go_board::flood_step (bit_array &next, const bit_array &fill,
			   const bit_array &masked_left, const bit_array &masked_right)
{
	next.ior (fill, -1, masked_left);
	next.ior (fill, 1, masked_right);
	next.ior (fill, m_sz);
	next.ior (fill, -m_sz);
}

int go_board::count_liberties (const bit_array &stones)
{
	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	bit_array liberties (m_sz * m_sz);
	flood_step (liberties, stones, masked_left, masked_right);
	liberties.andnot (*m_stones_w);
	liberties.andnot (*m_stones_b);
	return liberties.popcnt ();
}

/* For debugging purposes.  */
void go_board::dump_ascii () const
{
	for (int y = 0; y < m_sz; y++) {
		for (int x = 0; x < m_sz; x++) {
			int bp = bitpos (x, y);
			if (m_stones_w->test_bit (bp))
				putchar ('O');
			else if (m_stones_b->test_bit (bp))
				putchar ('X');
			else
				putchar ('.');
		}
		putchar ('\n');
	}
}

void go_board::dump_bitmap (const bit_array &ba) const
{
	for (int y = 0; y < m_sz; y++) {
		for (int x = 0; x < m_sz; x++) {
			int bp = bitpos (x, y);
			if (ba.test_bit (bp))
				putchar ('X');
			else
				putchar ('.');
		}
		putchar ('\n');
	}
}

void go_board::identify_units ()
{
	m_units_w.clear ();
	m_units_b.clear ();

	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	bit_array handled (m_sz * m_sz);

	for (int i = 0; i < m_sz * m_sz; i++) {
		if (handled.test_bit (i))
			continue;
		bit_array *stones;
		stone_color col = none;
		if (m_stones_w->test_bit (i)) {
			col = white;
			stones = m_stones_w;
		} else if (m_stones_b->test_bit (i)) {
			col = black;
			stones = m_stones_b;
		} else
			continue;
		std::vector<stone_unit> &units = col == black ? m_units_b : m_units_w;
		bit_array unit (m_sz * m_sz);
		unit.set_bit (i);
		bit_array next (unit);
		for (;;) {
			flood_step (next, unit, masked_left, masked_right);
			next.and1 (*stones);
			if (next == unit)
				break;
			unit = next;
		}
#ifdef DEBUG
		if (handled.intersect_p (unit))
			throw std::logic_error ("overlapping groups");
#endif
		handled.ior (unit);

		unsigned n_liberties = count_liberties (unit);
		stone_unit su (next, n_liberties);

		units.push_back (su);
	}
}

void go_board::toggle_alive (int x, int y, bool flood)
{
	int bp = bitpos (x, y);
	stone_color col = none;
	if (m_stones_b->test_bit (bp))
		col = black;
	else if (m_stones_w->test_bit (bp))
		col = white;
	else
		return;

	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	const bit_array &other_stones = col == black ? *m_stones_w : *m_stones_b;
	bit_array fill (m_sz * m_sz);
	fill.set_bit (bp);
	if (flood) {
		bit_array next (fill);
		for (;;) {
			flood_step (next, fill, masked_left, masked_right);
			next.andnot (other_stones);
			if (next == fill)
				break;
			fill = next;
		}
	}

	for (auto &it: m_units_w)
		if (it.m_stones.intersect_p (fill)) {
			it.m_seki = false;
			it.m_alive = !it.m_alive;
			int mult = it.m_alive ? -1 : 1;
			m_caps_b += mult * it.m_stones.popcnt ();
		}
	for (auto &it: m_units_b)
		if (it.m_stones.intersect_p (fill)) {
			it.m_seki = false;
			it.m_alive = !it.m_alive;
			int mult = it.m_alive ? -1 : 1;
			m_caps_w += mult * it.m_stones.popcnt ();
		}
}

void go_board::toggle_seki (int x, int y)
{
	int bp = bitpos (x, y);

	for (auto &it: m_units_w)
		if (it.m_stones.test_bit (bp)) {
			it.m_alive = true;
			it.m_seki = !it.m_seki;
			return;
		}
	for (auto &it: m_units_b)
		if (it.m_stones.test_bit (bp)) {
			it.m_alive = true;
			it.m_seki = !it.m_seki;
			return;
		}
}

/* Called when loading an SGF and encountering a position with territory markers.
   Update our captures and territory so that the correct result can be shown.  */
void go_board::territory_from_markers ()
{
	for (int i = 0; i < m_sz * m_sz; i++) {
		if (m_marks[i] == mark::terr) {
			if (m_stones_w->test_bit (i))
				m_caps_b++;
			else if (m_stones_b->test_bit (i))
				m_caps_w++;
			if (m_mark_extra[i] == 0)
				m_score_w++;
			else
				m_score_b++;
		}
	}
}

/* Expand FILL until it reaches the borders made up by W_STONES and B_STONES.
   Store into the neighbours variables whether the border we touch contains
   white or black stones.  */
void go_board::scoring_flood_fill (bit_array &fill, const bit_array &masked_left, const bit_array &masked_right,
				   const bit_array &w_stones, const bit_array &b_stones,
				   bool &neighbours_w, bool &neighbours_b)
{
	bit_array next (fill);
	for (;;) {
		flood_step (next, fill, masked_left, masked_right);
		if (next.intersect_p (w_stones))
			neighbours_w = true;
		if (next.intersect_p (b_stones))
			neighbours_b = true;
		next.andnot (w_stones);
		next.andnot (b_stones);
		if (next == fill)
			break;
		fill = next;
	}
}

/* Fill m_units_t with blocks of territory, where W_STONES and B_STONES are the boundaries.
   Also sets mark::dead for every stone not inside these two sets, since it's convenient
   to do it here.  */
void go_board::find_territory_units (const bit_array &w_stones, const bit_array &b_stones,
				     const bit_array &masked_left, const bit_array &masked_right)
{
	m_units_t.clear ();
	m_units_st.clear ();
	bit_array handled (m_sz * m_sz);
	bit_array dead_stones = *m_stones_w;
	dead_stones.ior (*m_stones_b);
	dead_stones.andnot (w_stones);
	dead_stones.andnot (b_stones);
	for (int i = 0; i < m_sz * m_sz; i++) {
		if (handled.test_bit (i))
			continue;

		if (w_stones.test_bit (i) || b_stones.test_bit (i))
			continue;

		bit_array fill (m_sz * m_sz);
		fill.set_bit (i);
		bool neighbours_b = false, neighbours_w = false;
		scoring_flood_fill (fill, masked_left, masked_right, w_stones, b_stones,
				    neighbours_w, neighbours_b);
		if (neighbours_w != neighbours_b)
			m_units_t.push_back (terr_unit (fill, neighbours_w, neighbours_b,
							fill.intersect_p (dead_stones)));
		else
			m_units_st.push_back (terr_unit (fill, neighbours_w, neighbours_b,
							 false));
		handled.ior (fill);
	}
}

/* The final step for calculating scores, shared by the simple and complex variants.  */
void go_board::finish_scoring_markers (const bit_array *do_not_count)
{
	bit_array terr_w (m_sz * m_sz);
	bit_array terr_b (m_sz * m_sz);
	for (auto &t: m_units_t) {
		bool counted = true;
		if (do_not_count && t.m_terr.intersect_p (*do_not_count))
			counted = false;
		if (counted) {
			if (t.m_nb_b) {
				terr_b.ior (t.m_terr);
			} else {
				terr_w.ior (t.m_terr);
			}
		}
	}
	m_score_w += terr_w.popcnt ();
	m_score_b += terr_b.popcnt ();
	for (int i = 0; i < m_sz * m_sz; i++) {
		if (terr_w.test_bit (i)) {
			m_marks[i] = mark::terr;
			m_mark_extra[i] = 0;
		}
		if (terr_b.test_bit (i)) {
			m_marks[i] = mark::terr;
			m_mark_extra[i] = 1;
		}
	}
}

/* Identify territories, in a simple fashion: if we find an empty space, we
   count it for the side that surrounds it, ignoring possible sekis.  */
void go_board::calc_scoring_markers_simple ()
{
	init_marks (true);

	m_score_b = 0;
	m_score_w = 0;

	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	bit_array w_stones (m_sz * m_sz);
	bit_array b_stones (m_sz * m_sz);
	for (auto &it: m_units_w) {
		it.m_any_terr = it.m_real_terr = it.m_seki_neighbour = false;
		if (it.m_alive)
			w_stones.ior (it.m_stones);
	}
	for (auto &it: m_units_b) {
		it.m_any_terr = it.m_real_terr = it.m_seki_neighbour = false;
		if (it.m_alive)
			b_stones.ior (it.m_stones);
	}
	find_territory_units (w_stones, b_stones, masked_left, masked_right);
	finish_scoring_markers (nullptr);
	m_units_t.clear ();
	m_units_st.clear ();
}

/* Identify territories, trying a little harder to identify sekis and
   false eyes.  */
void go_board::calc_scoring_markers_complex ()
{
	init_marks (true);

	m_score_b = 0;
	m_score_w = 0;

	std::vector<stone_unit *> live_units;
	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	bit_array w_stones (m_sz * m_sz);
	bit_array b_stones (m_sz * m_sz);
	bit_array dead_stones (m_sz * m_sz);
	for (auto &it: m_units_w) {
		it.m_any_terr = it.m_real_terr = it.m_seki_neighbour = false;
		if (it.m_alive) {
			w_stones.ior (it.m_stones);
			live_units.push_back (&it);
		} else
			dead_stones.ior (it.m_stones);
	}
	for (auto &it: m_units_b) {
		it.m_any_terr = it.m_real_terr = it.m_seki_neighbour = false;
		if (it.m_alive) {
			b_stones.ior (it.m_stones);
			live_units.push_back (&it);
		} else
			dead_stones.ior (it.m_stones);
	}
	find_territory_units (w_stones, b_stones, masked_left, masked_right);

	bit_array cand_territory (m_sz * m_sz);
	for (auto &t: m_units_t)
		cand_territory.ior (t.m_terr);
	bit_array false_eyes (m_sz * m_sz);

	/* Discover false eyes.  The condition is that we must have a unit of
	   stones which borders only one point of candidate territory, and that
	   point is not fully surrounded by that unit.
	   This also handles cases like a hane on the first line with a
	   missing connection: the false eye point can be part of a larger
	   area of territory.  */
	for (;;) {
		bool changed = false;
		for (auto it: live_units) {
			bit_array borders (m_sz * m_sz);
			flood_step (borders, it->m_stones, masked_left, masked_right);
			borders.and1 (cand_territory);
			if (borders.popcnt () != 1)
				continue;
			bit_array b_libs (m_sz * m_sz);
			flood_step (b_libs, borders, masked_left, masked_right);
			b_libs.andnot (it->m_stones);
			if (b_libs.popcnt () > 0) {
				/* We found a false eye.  Two cases: a single point,
				   in which case we remove it from the candidate
				   territory and iterate in order to possibly find
				   more false eyes, or a part of a larger area,
				   in which case this is just the first-line hane
				   case.  */
				false_eyes.ior (borders);
				for (auto &t: m_units_t) {
					if (!t.m_terr.intersect_p (borders))
						continue;
					t.m_terr.andnot (borders);
					if (t.m_terr.popcnt () == 0) {
						cand_territory.andnot (borders);
						changed = true;
					}
				}
			}
		}
		if (!changed)
			break;
	}
#if 0
	if (false_eyes.popcnt () > 0) {
		for (auto &t: m_units_t)
			t.m_terr.and (cand_territory);
		cand_territory.and_not (false_eyes);
	}
#endif
	for (int i = 0; i < m_sz * m_sz; i++)
		if (false_eyes.test_bit (i))
			m_marks[i] = mark::falseeye;

	bit_array real_territory (m_sz * m_sz);
	bit_array nonseki_stones (m_sz * m_sz);

	/* Now discover territories that contain dead stones, and units which border
	   them.
	   Also discover units that do not border any territories: these are later
	   used for identifying sekis .  */
	for (auto &t: m_units_t) {
		/* Could have been a false eye.  */
		if (t.m_terr.popcnt () == 0)
			continue;

		bit_array borders (m_sz * m_sz);
		flood_step (borders, t.m_terr, masked_left, masked_right);

		if (t.m_contains_dead)
			real_territory.ior (t.m_terr);

		if (t.m_nb_b) {
			for (auto &it: m_units_b) {
				if (!it.m_alive || !it.m_stones.intersect_p (borders))
					continue;
				it.m_any_terr = true;
			}
		} else {
			for (auto &it: m_units_w) {
				if (!it.m_alive || !it.m_stones.intersect_p (borders))
					continue;
				it.m_any_terr = true;
			}
		}
	}

	bit_array seki_stones (m_sz * m_sz);
	for (auto it: live_units)
		if (it->m_seki)
			seki_stones.ior (it->m_stones);
	for (int i = 0; i < m_sz * m_sz; i++)
		if (seki_stones.test_bit (i))
			m_marks[i] = mark::seki;

	/* Propagate the real territory markers.  Any unit that borders real
	   territory (i.e. an area which contains dead stones) is alive and not
	   in seki.  Therefore any other areas of territory it borders (excluding
	   false eyes) are also real territory, which may propagate liveness
	   further.  */
	for (;;) {
		bool changed = false;
		for (auto it: live_units) {
			if (it->m_real_terr)
				continue;
			bit_array borders (m_sz * m_sz);
			flood_step (borders, it->m_stones, masked_left, masked_right);
			if (borders.intersect_p (real_territory)) {
				nonseki_stones.ior (it->m_stones);
				it->m_real_terr = true;
				changed = true;
			}
		}
		if (!changed)
			break;

		for (auto &t: m_units_t) {
			if (t.m_contains_dead)
				continue;
			bit_array borders (m_sz * m_sz);
			flood_step (borders, t.m_terr, masked_left, masked_right);
			if (borders.intersect_p (nonseki_stones)) {
				real_territory.ior (t.m_terr);
				t.m_contains_dead = true;
				changed = true;
			}
		}
		if (!changed)
			break;
	}

	bit_array seki_neighbours (m_sz * m_sz);
#if 0
	/* Now look for units that are not marked dead, but did not
	   border any territory.  Mark their neighbours as involved in
	   seki.

	   This is not a complete detection of seki situations, but handles a
	   lot of cases.  */
	for (auto &it: m_units_w) {
		if (it.m_any_terr || !it.m_alive)
			continue;
		bit_array nb (m_sz * m_sz);
		flood_step (nb, it.m_stones, masked_left, masked_right);
		for (auto &nit: m_units_b) {
			if (nit.m_alive && !nit.m_real_terr && nb.intersect_p (nit.m_stones))
				nit.m_seki_neighbour = true;
		}
	}
	for (auto &it: m_units_b) {
		if (it.m_any_terr || !it.m_alive)
			continue;
		bit_array nb (m_sz * m_sz);
		flood_step (nb, it.m_stones, masked_left, masked_right);
		for (auto &nit: m_units_w) {
			if (nit.m_alive && !nit.m_real_terr && nb.intersect_p (nit.m_stones))
				nit.m_seki_neighbour = true;
		}
	}
	/* Now identify empty intersections adjacent to strings that have a seki
	   neighbour.  These points should not be counted for territory.  */
	for (auto it: live_units) {
		if (it->m_seki_neighbour)
			flood_step (seki_neighbours, it->m_stones, masked_left, masked_right);
	}
#else
	for (auto it: live_units)
		if (it->m_seki)
			flood_step (seki_neighbours, it->m_stones, masked_left, masked_right);
#endif
	finish_scoring_markers (&seki_neighbours);
 	m_units_t.clear ();
 	m_units_st.clear ();
}

void go_board::recalc_liberties ()
{
	for (auto &it: m_units_w)
		it.m_n_liberties = count_liberties (it.m_stones);
	for (auto &it: m_units_b)
		it.m_n_liberties = count_liberties (it.m_stones);
}

void go_board::add_stone (int x, int y, stone_color col, bool process_captures)
{
	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	std::vector<stone_unit> &opponent_units = col == black ? m_units_w : m_units_b;
	std::vector<stone_unit> &player_units = col == black ? m_units_b : m_units_w;
	bit_array *opponent_stones = col == black ? m_stones_w : m_stones_b;
	bit_array *player_stones = col == black ? m_stones_b : m_stones_w;
	player_stones->set_bit (bitpos (x, y));

	bit_array pos (m_sz * m_sz);
	bit_array pos_neighbours (m_sz * m_sz);
	pos.set_bit (bitpos (x, y));
	pos_neighbours.ior (pos, -1, masked_left);
	pos_neighbours.ior (pos, 1, masked_right);
	pos_neighbours.ior (pos, m_sz);
	pos_neighbours.ior (pos, -m_sz);

	int n_caps = 0;
	if (process_captures) {
		for (auto &it: opponent_units) {
			if (!it.m_stones.intersect_p (pos_neighbours))
				continue;
			if (it.m_n_liberties == 1) {
				/* Marker for "removed by this move". Zero-liberty groups
				   added elsewhere by editing could remain on the board.  */
				it.m_n_liberties = -1;
				n_caps += it.m_stones.popcnt ();

				bool changed = opponent_stones->andnot (it.m_stones);
				if (!changed)
					throw std::logic_error ("Removed stones do not exist");
			} else
				it.m_n_liberties--;
		}
		opponent_units.erase (std::remove_if (opponent_units.begin (), opponent_units.end (),
						      [](const stone_unit &unit) { return unit.m_n_liberties == -1; }),
				      opponent_units.end ());
		if (col == black)
			m_caps_b += n_caps;
		else
			m_caps_w += n_caps;
	}

	/* Merge with neighbours.  */
	stone_unit *first_neighbour = nullptr;
	for (auto &it: player_units) {
		if (!it.m_stones.intersect_p (pos_neighbours))
			continue;
		if (first_neighbour == nullptr) {
			first_neighbour = &it;
			it.m_stones.ior (pos);
			continue;
		}
		first_neighbour->m_stones.ior (it.m_stones);
		it.m_n_liberties = -1;
	}
	if (first_neighbour == nullptr) {
		stone_unit newunit (pos, count_liberties (pos));
		player_units.push_back (newunit);
	} else if (n_caps == 0) {
		first_neighbour->m_n_liberties = count_liberties (first_neighbour->m_stones);
	}
	if (n_caps > 0) {
		recalc_liberties ();
	}

	player_units.erase (std::remove_if (player_units.begin (), player_units.end (),
					    [](const stone_unit &unit) { return unit.m_n_liberties == -1; }),
			    player_units.end ());
	verify_invariants ();
}

bool go_board::valid_move_p (int x, int y, stone_color col)
{
	if (stone_at (x, y) != none)
		return false;

	/* Simplest case: test for plainly enough liberties.  */
	bit_array pos (m_sz * m_sz);
	pos.set_bit (bitpos (x, y));

	if (count_liberties (pos) > 0)
		return true;

	/* Look at surrounding units.  */
	const bit_array &masked_left = get_boardmask_left (m_sz);
	const bit_array &masked_right = get_boardmask_right (m_sz);

	bit_array pos_neighbours (m_sz * m_sz);
	pos_neighbours.ior (pos, -1, masked_left);
	pos_neighbours.ior (pos, 1, masked_right);
	pos_neighbours.ior (pos, m_sz);
	pos_neighbours.ior (pos, -m_sz);

	/* Extending a group of the same color?  */
	std::vector<stone_unit> &player_units = col == black ? m_units_b : m_units_w;
	for (auto &it: player_units) {
		if (!it.m_stones.intersect_p (pos_neighbours))
			continue;
		if (it.m_n_liberties > 1)
			return true;
	}

	/* A valid capture?  */
	std::vector<stone_unit> &opponent_units = col == black ? m_units_w : m_units_b;
	for (auto &it: opponent_units) {
		if (!it.m_stones.intersect_p (pos_neighbours))
			continue;
		if (it.m_n_liberties == 1)
			return true;
	}
	/* Slightly clunky: ko is checked later on, in add_child_move, by comparing
	   board positions.  */
	return false;
}

#ifdef TEST
int main ()
{
	bit_array nomask (0);
	{
		bit_array low (65);
		low.set_bit (0);
		bit_array high (65);
		high.set_bit (64);
		bit_array t1 (65), t2 (65);
		t1.ior (low, 64, nomask);
		t2.ior (high, -64, nomask);
		t1.debug ();
		t2.debug ();
	}
	{
		bit_array low (66);
		low.set_bit (0);
		bit_array high (66);
		high.set_bit (65);
		bit_array t1 (66), t2 (66);
		t1.ior (low, 65, nomask);
		t2.ior (high, -65, nomask);
		t1.debug ();
		t2.debug ();
	}
	{
		bit_array low (66);
		low.set_bit (0);
		low.set_bit (1);
		low.set_bit (2);
		bit_array high (66);
		high.set_bit (65);
		high.set_bit (64);
		high.set_bit (63);
		bit_array t1 (66), t2 (66);
		t1.ior (low, 63, nomask);
		t2.ior (high, -63, nomask);
		t1.debug ();
		t2.debug ();
	}

	bit_array a (20);
	bit_array b (20, true);
	bit_array c (20), d (20);
	a.set_bit (19);
	a.set_bit (6);
	b.clear_bit (5);
	b.clear_bit (2);
	b.clear_bit (1);
	c.ior (b, -1, nomask);
	d.ior (a, 4, nomask);
	a.debug ();
	b.debug ();
	c.debug ();
	d.debug ();

}
#endif
