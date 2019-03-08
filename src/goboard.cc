#include <string>
#include <exception>
#include <iostream>

#include "goboard.h"

void bit_array::debug () const
{
	for (unsigned bit = 0; bit < m_n_bits; bit++) {
		uint64_t val = m_bits[bit / 64];
		printf ("%d", (int)((val >> (bit % 64)) & 1));
	}
	putchar ('\n');
}

void bit_array::debug (int linesz) const
{
	int linecnt = 0;
	for (unsigned bit = 0; bit < m_n_bits; bit++) {
		uint64_t val = m_bits[bit / 64];
		printf ("%d", (int)((val >> (bit % 64)) & 1));
		if (++linecnt == linesz)
			linecnt = 0, putchar ('\n');
	}
	if (m_n_bits % linesz != 0)
		putchar ('\n');
}


/* Keep some precomputed bit arrays, one for each board size, which
   have the left and right columns masked out.  These can be used in
   shift-and-and operations to find neighbours.  */

static std::map<std::pair<int, int>, bit_array *> left_masks;
static std::map<std::pair<int, int>, bit_array *> right_masks;
static std::map<std::pair<int, int>, bit_array *> left_columns;
static std::map<std::pair<int, int>, bit_array *> right_columns;
static std::map<std::pair<int, int>, bit_array *> top_rows;
static std::map<std::pair<int, int>, bit_array *> bottom_rows;

static const bit_array *create_boardmask_left (int w, int h)
{
	auto it = left_masks.find ({w, h});
	if (it != left_masks.end ())
		return it->second;
	bit_array *m = new bit_array (w * h, true);
	for (int i = 0; i < h; i++)
		m->clear_bit (i * w);
	left_masks.insert ({ {w, h}, m});
	return m;
}

static const bit_array *create_boardmask_right (int w, int h)
{
	auto it = right_masks.find ({w, h});
	if (it != right_masks.end ())
		return it->second;
	bit_array *m = new bit_array (w * h, true);
	for (int i = 0; i < h; i++)
		m->clear_bit (i * w + w - 1);
	right_masks.insert ({ {w, h}, m});
	return m;
}

const bit_array *create_column_left (int w, int h)
{
	auto it = left_columns.find ({w, h});
	if (it != left_columns.end ())
		return it->second;
	bit_array *m = new bit_array (w * h);
	for (int i = 0; i < h; i++)
		m->set_bit (i * w);
	left_columns.insert ({ {w, h}, m});
	return m;
}

static const bit_array *create_column_right (int w, int h)
{
	auto it = right_columns.find ({w, h});
	if (it != right_columns.end ())
		return it->second;
	bit_array *m = new bit_array (w * h);
	for (int i = 0; i < h; i++)
		m->set_bit (i * w + w - 1);
	right_columns.insert ({ {w, h}, m});
	return m;
}

const bit_array *create_row_top (int w, int h)
{
	auto it = top_rows.find ({w, h});
	if (it != top_rows.end ())
		return it->second;
	bit_array *m = new bit_array (w * h);
	for (int i = 0; i < w; i++)
		m->set_bit (i);
	top_rows.insert ({ {w, h}, m});
	return m;
}

static const bit_array *create_row_bottom (int w, int h)
{
	auto it = bottom_rows.find ({w, h});
	if (it != bottom_rows.end ())
		return it->second;
	bit_array *m = new bit_array (w * h);
	for (int i = 0; i < w; i++)
		m->set_bit (i + (w * (h - 1)));
	bottom_rows.insert ({ {w, h}, m});
	return m;
}

go_board::go_board (int w, int h, bool torus_h, bool torus_v)
	: m_sz_x (w), m_sz_y (h), m_torus_h (torus_h), m_torus_v (torus_v),
	  m_masked_left (create_boardmask_left (w, h)), m_masked_right (create_boardmask_right (w, h)),
	  m_column_left (torus_h ? create_column_left (w, h) : nullptr),
	  m_column_right (torus_h ? create_column_right (w, h) : nullptr),
	  m_row_top (torus_v ? create_row_top (w, h) : nullptr),
	  m_row_bottom (torus_v ? create_row_bottom (w, h) : nullptr),
	  m_stones_b (new bit_array (w * h)), m_stones_w (new bit_array (w * h))
{
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

/* Try to reduce number of flood fill steps by initializing with a reasonable default:
   look down and to the right from the first position we found and initialize bits
   that are in/out of bounds as appropriate.  */
bit_array go_board::init_fill (int bp, const bit_array &bounds, bool in)
{
	bit_array fill (bitsize ());
	int rem_len = m_sz_x - bp % m_sz_x;
	for (int y = bp; y < bitsize (); y += m_sz_x)  {
		if (bounds.test_bit (y) != in)
			break;
		for (int i = 0; i < rem_len; i++) {
			if (bounds.test_bit (y + i) != in)
				break;
			fill.set_bit (y + i);
		}
	}
	return fill;
}

/* Extend a bit mask in all directions.  */
void go_board::flood_step (bit_array &next, const bit_array &fill)
{
	next.ior (fill, -1, *m_masked_left);
	next.ior (fill, 1, *m_masked_right);
	next.ior (fill, m_sz_x);
	next.ior (fill, -m_sz_x);
	if (m_torus_h) {
		next.ior (fill, m_sz_x - 1, *m_column_left);
		next.ior (fill, -m_sz_x + 1, *m_column_right);
	}
	if (m_torus_v) {
		next.ior (fill, m_sz_x * (m_sz_y - 1));
		next.ior (fill, -m_sz_x * (m_sz_y - 1));
	}
}

void go_board::flood_fill (bit_array &fill, const bit_array &boundary)
{
	bit_array next (fill);
	for (;;) {
		flood_step (next, fill);
		next.andnot (boundary);
		if (next == fill)
			break;
		fill = next;
	}
}

int go_board::count_liberties (const bit_array &stones)
{
	bit_array liberties (bitsize ());
	flood_step (liberties, stones);
	liberties.andnot (*m_stones_w);
	liberties.andnot (*m_stones_b);
	return liberties.popcnt ();
}

/* For debugging purposes.  */
void go_board::dump_ascii () const
{
	for (int y = 0; y < m_sz_y; y++) {
		for (int x = 0; x < m_sz_x; x++) {
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
	for (int y = 0; y < m_sz_y; y++) {
		for (int x = 0; x < m_sz_x; x++) {
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

	bit_array handled (bitsize ());
#ifdef CHECKING
	unsigned found_w = 0;
	unsigned found_b = 0;
#endif
	for (int y = 0; y < m_sz_y; y++) {
		for (int x = 0; x < m_sz_x; x++) {
			int i = bitpos (x, y);
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
			bit_array unit = init_fill (i, *stones, true);

			bit_array next (unit);
			for (;;) {
				flood_step (next, unit);
				next.and1 (*stones);
				if (next == unit)
					break;
				unit = next;
			}
#ifdef CHECKING
			if (handled.intersect_p (unit))
				throw std::logic_error ("overlapping groups");
			if (col == white)
				found_w += unit.popcnt ();
			else
				found_b += unit.popcnt ();
#endif
			handled.ior (unit);
			unsigned n_liberties = count_liberties (unit);
			units.emplace_back (next, n_liberties);
		}
	}
#ifdef CHECKING
	if (found_w != m_stones_w->popcnt () || found_b != m_stones_b->popcnt ())
		throw std::logic_error ("unit search didn't find all stones.");
#endif
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

	const bit_array &other_stones = col == black ? *m_stones_w : *m_stones_b;
	bit_array fill (bitsize ());
	fill.set_bit (bp);
	if (flood)
		flood_fill (fill, other_stones);

	for (auto &it: m_units_w)
		if (it.m_stones.intersect_p (fill)) {
			it.m_seki = false;
			it.m_alive = !it.m_alive;
			int mult = it.m_alive ? -1 : 1;
			m_dead_w += mult * it.m_stones.popcnt ();
		}
	for (auto &it: m_units_b)
		if (it.m_stones.intersect_p (fill)) {
			it.m_seki = false;
			it.m_alive = !it.m_alive;
			int mult = it.m_alive ? -1 : 1;
			m_dead_b += mult * it.m_stones.popcnt ();
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
	for (int i = 0; i < bitsize (); i++) {
		if (m_marks[i] == mark::terr) {
			if (m_stones_w->test_bit (i))
				m_dead_w++;
			else if (m_stones_b->test_bit (i))
				m_dead_b++;
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
void go_board::scoring_flood_fill (bit_array &fill,
				   const bit_array &w_stones, const bit_array &b_stones,
				   bool &neighbours_w, bool &neighbours_b)
{
	bit_array next (fill);
	for (;;) {
		flood_step (next, fill);
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
void go_board::find_territory_units (const bit_array &w_stones, const bit_array &b_stones)
{
	m_units_t.clear ();
	m_units_st.clear ();
	bit_array handled (w_stones);
	handled.ior (b_stones);
	bit_array dead_stones = *m_stones_w;
	dead_stones.ior (*m_stones_b);
	dead_stones.andnot (w_stones);
	dead_stones.andnot (b_stones);

	for (int i = 0; i < bitsize (); i++) {
		i = handled.ffz (i);
		if (i == bitsize ())
			break;
#ifdef CHECKING
		if (i > bitsize () || handled.test_bit (i))
			throw std::logic_error ("ffz didn't work");
#endif
		bit_array fill (bitsize ());
		fill.set_bit (i);
		bool neighbours_b = false, neighbours_w = false;
		scoring_flood_fill (fill, w_stones, b_stones, neighbours_w, neighbours_b);
		if (neighbours_w != neighbours_b)
			m_units_t.emplace_back (fill, neighbours_w, neighbours_b,
						fill.intersect_p (dead_stones));
		else
			m_units_st.emplace_back (fill, neighbours_w, neighbours_b, false);
		handled.ior (fill);
	}
#ifdef CHECKING
	if (handled.popcnt () != bitsize ())
		throw std::logic_error ("didn't find all territory");
#endif
}

/* Enclosed areas, as per Benson's algorithm.  Flood fill through any area not containing stones of
   a given color, then remove intersections occupied by a stone of the opposite color.  We return
   a pair of values: the area and the surrounding stones.  */
std::vector<go_board::enclosed_area> go_board::find_eas (const bit_array &stones, const bit_array &other_stones)
{
	std::vector<enclosed_area> ea;
	bit_array handled = stones;
	for (int i = 0; i < bitsize (); i++) {
		i = handled.ffz (i);
		if (i == bitsize ())
			break;

		bit_array fill = init_fill (i, stones, false);
		flood_fill (fill, stones);

		bit_array border (fill);
		flood_step (border, fill);
		border.andnot (fill);
		fill.andnot (other_stones);

		handled.ior (fill);

		ea.emplace_back (std::move (fill), std::move (border));
	}
	return ea;
}

/* Benson's algorithm to find pass-alive stones.  Sets m_n_vital in all units so that if it is >= 2,
   the unit is alive.  */
void go_board::benson (std::vector<stone_unit> &units, const bit_array &other_stones)
{
	std::vector<bit_array> unit_liberties;
	std::vector<size_t> tentative;
	tentative.reserve (units.size ());
	unit_liberties.reserve (units.size ());
	/* Calculate liberties bitsets for all units, and create the tentative array
	   holding the index of each unit - this will be reduced over time during the
	   algorithm's loop.  */
	for (auto &it: units) {
		unit_liberties.emplace_back (bitsize ());
		bit_array &liberties = unit_liberties.back ();
		flood_step (liberties, it.m_stones);
		liberties.andnot (*m_stones_w);
		liberties.andnot (*m_stones_b);
		tentative.push_back (tentative.size ());
	}
	bit_array stones (bitsize ());
	for (auto it: tentative)
		stones.ior (units[it].m_stones);
	auto eas = find_eas (stones, other_stones);

	for (;;) {
		bool changed = false;
		for (auto it: tentative)
			units[it].m_n_vital = 0;
		for (auto &ea: eas) {
			for (auto idx: tentative) {
				if (ea.area.subset_of (unit_liberties[idx]))
					units[idx].m_n_vital++;
			}
		}
		tentative.erase (std::remove_if (std::begin (tentative), std::end (tentative),
						 [&] (size_t idx)
						 {
							 bool remove = units[idx].m_n_vital < 2;
							 changed |= remove;
							 return remove;
						 }),
				 std::end (tentative));
		stones.clear ();
		for (auto it: tentative)
			stones.ior (units[it].m_stones);

		eas.erase (std::remove_if (std::begin (eas), std::end (eas),
					   [&] (enclosed_area &ea)
					   {
						   bool remove = !ea.border.subset_of (stones);
						   changed |= remove;
						   return remove;
					   }),
				 std::end (eas));

		if (!changed)
			break;
	}
}

/* The final step for calculating scores, shared by the simple and complex variants.  */
void go_board::finish_scoring_markers (const bit_array *do_not_count)
{
	bit_array terr_w (bitsize ());
	bit_array terr_b (bitsize ());
	bit_array pass_alive (bitsize ());

#if 0 /* Useful for visualizing the algorithm's result, but should not be shown in a
	 normal scoring mode.  */
	for (auto &it: m_units_b)
		if (it.m_n_vital >= 2)
			pass_alive.ior (it.m_stones);
	for (auto &it: m_units_w)
		if (it.m_n_vital >= 2)
			pass_alive.ior (it.m_stones);
#endif

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
	for (int i = 0; i < bitsize (); i++) {
		if (terr_w.test_bit (i)) {
			m_marks[i] = mark::terr;
			m_mark_extra[i] = 0;
		}
		if (terr_b.test_bit (i)) {
			m_marks[i] = mark::terr;
			m_mark_extra[i] = 1;
		}
		if (pass_alive.test_bit (i)) {
			m_marks[i] = mark::square;
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

	bit_array w_stones (bitsize ());
	bit_array b_stones (bitsize ());
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
	find_territory_units (w_stones, b_stones);
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

	bit_array w_stones (bitsize ());
	bit_array b_stones (bitsize ());
	bit_array dead_stones (bitsize ());
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
	find_territory_units (w_stones, b_stones);

#if 0
	benson (m_units_w, *m_stones_b);
	benson (m_units_b, *m_stones_w);
#endif

	bit_array cand_territory (bitsize ());
	for (auto &t: m_units_t)
		cand_territory.ior (t.m_terr);
	bit_array false_eyes (bitsize ());

	/* Discover false eyes.  The condition is that we must have a unit of
	   stones which borders only one point of candidate territory, and that
	   point is not fully surrounded by that unit.
	   This also handles cases like a hane on the first line with a
	   missing connection: the false eye point can be part of a larger
	   area of territory.  */
	for (;;) {
		bool changed = false;
		for (auto it: live_units) {
			bit_array borders (bitsize ());
			flood_step (borders, it->m_stones);
			borders.and1 (cand_territory);
			if (borders.popcnt () != 1)
				continue;
			bit_array b_libs (bitsize ());
			flood_step (b_libs, borders);
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
	for (int i = 0; i < bitsize (); i++)
		if (false_eyes.test_bit (i))
			m_marks[i] = mark::falseeye;

	bit_array real_territory (bitsize ());
	bit_array nonseki_stones (bitsize ());

	/* Now discover territories that contain dead stones, and units which border
	   them.
	   Also discover units that do not border any territories: these are later
	   used for identifying sekis .  */
	for (auto &t: m_units_t) {
		/* Could have been a false eye.  */
		if (t.m_terr.popcnt () == 0)
			continue;

		bit_array borders (bitsize ());
		flood_step (borders, t.m_terr);

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

	bit_array seki_stones (bitsize ());
	for (auto it: live_units)
		if (it->m_seki)
			seki_stones.ior (it->m_stones);
	for (int i = 0; i < bitsize (); i++)
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
			bit_array borders (bitsize ());
			flood_step (borders, it->m_stones);
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
			bit_array borders (bitsize ());
			flood_step (borders, t.m_terr);
			if (borders.intersect_p (nonseki_stones)) {
				real_territory.ior (t.m_terr);
				t.m_contains_dead = true;
				changed = true;
			}
		}
		if (!changed)
			break;
	}

	bit_array seki_neighbours (bitsize ());
#if 0
	/* Now look for units that are not marked dead, but did not
	   border any territory.  Mark their neighbours as involved in
	   seki.

	   This is not a complete detection of seki situations, but handles a
	   lot of cases.  */
	for (auto &it: m_units_w) {
		if (it.m_any_terr || !it.m_alive)
			continue;
		bit_array nb (bitsize ());
		flood_step (nb, it.m_stones);
		for (auto &nit: m_units_b) {
			if (nit.m_alive && !nit.m_real_terr && nb.intersect_p (nit.m_stones))
				nit.m_seki_neighbour = true;
		}
	}
	for (auto &it: m_units_b) {
		if (it.m_any_terr || !it.m_alive)
			continue;
		bit_array nb (bitsize ());
		flood_step (nb, it.m_stones);
		for (auto &nit: m_units_w) {
			if (nit.m_alive && !nit.m_real_terr && nb.intersect_p (nit.m_stones))
				nit.m_seki_neighbour = true;
		}
	}
	/* Now identify empty intersections adjacent to strings that have a seki
	   neighbour.  These points should not be counted for territory.  */
	for (auto it: live_units) {
		if (it->m_seki_neighbour)
			flood_step (seki_neighbours, it->m_stones);
	}
#else
	for (auto it: live_units)
		if (it->m_seki)
			flood_step (seki_neighbours, it->m_stones);
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
#ifdef CHECKING
	if (stone_at (x, y) != none)
		throw std::logic_error ("placing stone on top of another");
#endif
	std::vector<stone_unit> &opponent_units = col == black ? m_units_w : m_units_b;
	std::vector<stone_unit> &player_units = col == black ? m_units_b : m_units_w;
	bit_array *opponent_stones = col == black ? m_stones_w : m_stones_b;
	bit_array *player_stones = col == black ? m_stones_b : m_stones_w;
	player_stones->set_bit (bitpos (x, y));

	bit_array pos (bitsize ());
	bit_array pos_neighbours (bitsize ());
	pos.set_bit (bitpos (x, y));
	flood_step (pos_neighbours, pos);

	int n_caps = 0;
	int n_removed = 0;
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
				n_removed++;
			} else
				it.m_n_liberties--;
		}
#ifdef CHECKING
		size_t old_cnt = opponent_units.size ();
#endif
		opponent_units.erase (std::remove_if (opponent_units.begin (), opponent_units.end (),
						      [](const stone_unit &unit) { return unit.m_n_liberties == -1; }),
				      opponent_units.end ());
#ifdef CHECKING
		if (opponent_units.size () + n_removed != old_cnt)
			throw std::logic_error ("didn't remove enough units");
#endif
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
		player_units.emplace_back (pos, count_liberties (pos));
		first_neighbour = &player_units.back ();
	} else if (n_caps == 0) {
		first_neighbour->m_n_liberties = count_liberties (first_neighbour->m_stones);
	}
	if (first_neighbour->m_n_liberties == 0 && process_captures) {
#ifdef DEBUG
		std::cerr << "suicide move found\n";
#endif
		player_stones->andnot (first_neighbour->m_stones);
		if (col == black)
			m_caps_w += first_neighbour->m_stones.popcnt ();
		else
			m_caps_b += first_neighbour->m_stones.popcnt ();
		first_neighbour->m_n_liberties = -1;
		/* Recalculate liberties for everything.  */
		n_caps = 1;
	}
	player_units.erase (std::remove_if (player_units.begin (), player_units.end (),
					    [](const stone_unit &unit) { return unit.m_n_liberties == -1; }),
			    player_units.end ());

	if (n_caps > 0) {
		recalc_liberties ();
	}
	verify_invariants ();
#if 0 && defined CHECKING
	identify_units ();
	verify_invariants ();
#endif
}

bool go_board::valid_move_p (int x, int y, stone_color col)
{
	if (stone_at (x, y) != none)
		return false;

	/* Simplest case: test for plainly enough liberties.  */
	bit_array pos (bitsize ());
	pos.set_bit (bitpos (x, y));

	if (count_liberties (pos) > 0)
		return true;

	/* Look at surrounding units.  */
	bit_array pos_neighbours (bitsize ());
	flood_step (pos_neighbours, pos);

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

void go_board::verify_invariants ()
{
#ifdef CHECKING
	if (m_stones_b->intersect_p (*m_stones_w))
		throw std::logic_error ("white stones and black stones overlap");
	int wcnt = m_stones_w->popcnt ();
	for (auto &it: m_units_w) {
		if (it.m_n_liberties <= 0)
			throw std::logic_error ("white group was not removed.");
		if (it.m_n_liberties != count_liberties (it.m_stones))
			throw std::logic_error ("incorrect liberties on white group.");
		wcnt -= it.m_stones.popcnt ();
	}
	if (wcnt != 0)
		throw std::logic_error ("white stone count inconsistency");
	int bcnt = m_stones_b->popcnt ();
	for (auto &it: m_units_b) {
		if (it.m_n_liberties <= 0)
			throw std::logic_error ("group was not removed.");
		if (it.m_n_liberties != count_liberties (it.m_stones))
			throw std::logic_error ("incorrect liberties on black group.");
		bcnt -= it.m_stones.popcnt ();
	}
	if (bcnt != 0)
		throw std::logic_error ("black stone count inconsistency");

	bit_array stones = *m_stones_w;
	stones.ior (*m_stones_b);
	for (auto &it: m_units_w) {
		bit_array m = it.m_stones;
		stones.andnot (m);
		m.andnot (*m_stones_w);
		if (m.popcnt () > 0)
			throw std::logic_error ("white unit contains stones not on the board");
	}
	for (auto &it: m_units_b) {
		bit_array m = it.m_stones;
		stones.andnot (m);
		m.andnot (*m_stones_b);
		if (m.popcnt () > 0)
			throw std::logic_error ("black unit contains stones not on the board");
	}
	if (stones.popcnt () != 0)
		throw (std::logic_error ("board contains stones not found in units"));
#endif
}

#ifdef TEST
#include <stdlib.h>

int main ()
{
	for (int round = 0; round < 100; round++) {
		int sz = rand () % 200 + 100;
		bit_array arr (sz);
		int *vals = new int[sz] ();
		int count = 0;
		for (int i = 0; i < sz; i++)
			if (rand () % 2) {
				vals[count++] = i;
				arr.set_bit (i);
			}
		int verify;
		int pos = 0;
		for (verify = 0; verify < sz; verify++) {
			verify = arr.ffs (verify);
			if (verify > sz)
				abort ();
			if (verify == sz)
				break;
			if (pos >= count)
				abort ();
			if (verify != vals[pos])
				abort ();
			pos++;
		}
		if (pos != count)
			abort ();
		delete[] vals;
	}
	printf ("Tests OK.\n");
	return 0;
}
#endif
