#ifndef PATTERN_H
#define PATTERN_H

#include "bitarray.h"
#include "coords.h"

#include <vector>
#include <utility>

class cand_match;
class go_pattern
{
protected:
	struct bw { uint32_t bits[2]; };
	std::vector<bw> m_lines;
	unsigned m_sz_x, m_sz_y;

	alignment m_align;
	void debug ();

public:
	go_pattern (unsigned sz_x, unsigned sz_y);
	go_pattern (const go_board &, unsigned sz_x, unsigned sz_y, unsigned offx, unsigned offy);
	go_pattern (const go_pattern &) = default;
	go_pattern (go_pattern &&) = default;
	go_pattern &operator= (const go_pattern &) = default;
	go_pattern &operator= (go_pattern &&) = default;

	int sz_x () const { return m_sz_x; }
	int sz_y () const { return m_sz_y; }

	bool operator== (const go_pattern &other) const;
	bool operator!= (const go_pattern &other) const { return !operator== (other); }

	void clear_alignment () { m_align = { false, false, false, false }; }

	template<class T> go_pattern transform () const;

	bool match (const bit_array &other_w, const bit_array &other_b, unsigned other_sz_x, unsigned other_sz_y,
		    board_rect &) const;
	std::vector<cand_match> find_cands (const bit_array &other_w, const bit_array &other_b, const bit_array &other_caps,
					    unsigned other_sz_x, unsigned other_sz_y) const;
};

class cand_match : public go_pattern
{
	std::vector<uint32_t> final_caps;
	unsigned m_xoff, m_yoff;
	unsigned m_cont_x = 0, m_cont_y = 0;
	int m_cont_col = 0;
	/* The pattern we inherit contains a bitmask of mismatches, which are counted here.
	   Stones added that match the pattern reduce the mismatches.  Stones added that do
	   not match the pattern increase them, and can also fail immediately if these stones
	   are not part of the final_caps set.  */
	int n_mismatched = 0;
	bool m_swapped = false;

public:
	enum state { unmatched, matched, continued, failed };
	enum swapped { swapped };
private:
	enum state m_state = unmatched;

public:
	cand_match (const go_pattern &pat, std::vector<uint32_t> &&caps, unsigned xoff, unsigned yoff)
		: go_pattern (pat), final_caps (caps), m_xoff (xoff), m_yoff (yoff)
	{
		for (unsigned i = 0; i < m_sz_y; i++)
			n_mismatched += popcnt (m_lines[i].bits[0]) + popcnt (m_lines[i].bits[1]);
	}
	cand_match (const go_pattern &pat, std::vector<uint32_t> &&caps, unsigned xoff, unsigned yoff, enum swapped)
		: cand_match (pat, std::move (caps), xoff, yoff)
	{
		m_swapped = true;
		for (unsigned i = 0; i < m_sz_y; i++)
			std::swap (m_lines[i].bits[0], m_lines[i].bits[1]);
	}
	cand_match (const cand_match &) = default;
	cand_match (cand_match &&) = default;

	std::tuple<unsigned, unsigned, int> extract_continuation ()
	{
		return std::tuple<unsigned, unsigned, int> { m_cont_x, m_cont_y, m_cont_col };
	}
	bool was_continued () { return m_state == continued; }
	/* Return true if the state changes: unmatched to failed or matched.  */
	void put_stone (unsigned x, unsigned y, int col_off)
	{
		if (m_state == continued || m_state == failed)
			return;
		x -= m_xoff;
		y -= m_yoff;
		if (x >= m_sz_x || y >= m_sz_y)
			return;
		uint32_t bit = 1;
		bit <<= x;
		if (m_lines[y].bits[col_off] & bit) {
			m_lines[y].bits[col_off] &= ~bit;
			n_mismatched--;
		} else if (m_state == matched && n_mismatched == 0) {
			m_state = continued;
			if (m_swapped)
				col_off ^= 1;
			m_cont_x = x;
			m_cont_y = y;
			m_cont_col = col_off;
		} else if (final_caps[y] & bit) {
			m_lines[y].bits[col_off] |= bit;
			n_mismatched++;
		} else {
			m_state = failed;
		}
	}
	void clear_stone (unsigned x, unsigned y, int col_off)
	{
		if (m_state == continued || m_state == failed)
			return;
		x -= m_xoff;
		y -= m_yoff;
		if (x >= m_sz_x || y >= m_sz_y)
			return;
		uint32_t bit = 1;
		bit <<= x;
		if (m_lines[y].bits[col_off] & bit) {
			m_lines[y].bits[col_off] &= ~bit;
			n_mismatched--;
		} else {
			m_lines[y].bits[col_off] |= bit;
			n_mismatched++;
		}
	}
	enum state end_of_node ()
	{
		if (m_state == continued || m_state == failed)
			return m_state;
		if (n_mismatched == 0)
			m_state = matched;
		return m_state;
	}
};

extern bool match_movelist (const std::vector<char> &moves, std::vector<cand_match> &cands,
			    std::vector<std::pair<int, int>> &conts, int, coord_transform);

#endif
