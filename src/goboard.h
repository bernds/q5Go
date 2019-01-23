#ifndef GOBOARD_H
#define GOBOARD_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <memory>

#include "bitarray.h"
#include "sgf.h"

enum stone_color
{
	none, black, white
};

enum class stone_type { live, seki, dead, var };
enum class mark { none = 0, move, triangle, circle, square, plus, cross, text, num, letter, dead, seki, terr, falseeye };
typedef unsigned char mextra;

class go_board
{
	/* In everyday conversation, we might call these "strings of stones".  That
	   terminology has a bit of a name clash in a computer program.  So we call
	   them "units".  */
	class stone_unit
	{
		friend class go_board;
		bit_array m_stones;
		int m_n_liberties;
		/* Default true, toggled by the user during scoring.  */
		bool m_alive, m_seki;
		/* Markers used during scoring.  */
		bool m_any_terr, m_real_terr, m_seki_neighbour;
	public:
		stone_unit (const bit_array &stones, int n_liberties)
			: m_stones (stones), m_n_liberties (n_liberties), m_alive (true), m_seki (false),
			m_any_terr (false), m_real_terr (false), m_seki_neighbour (false)
		{
		}
	};
	/* Used during scoring.  */
	class terr_unit
	{
		friend class go_board;
		bit_array m_terr;
		/* Set true if the area neighbours live white or black stones.  */
		bool m_nb_w, m_nb_b;
		/* Set true if the area contains dead stones.  */
		bool m_contains_dead;
	public:
		terr_unit (const bit_array &terr, bool nb_w, bool nb_b, bool contains_dead)
			: m_terr (terr), m_nb_w (nb_w), m_nb_b (nb_b), m_contains_dead (contains_dead)
		{
		}
	};

	int m_sz;
	/* The total score that has been calculated.  */
	int m_score_b = 0;
	int m_score_w = 0;
	/* Number of captures that have occurred on the board.  Note: these indicate
	   the captures made _by_ a side, so m_caps_b is the number of white stones captured.  */
	int m_caps_b = 0;
	int m_caps_w = 0;
	/* Number of stones marked as captured during the scoring phase.  Here, m_dead_b is the
	   number of dead black stones.  */
	int m_dead_b = 0;
	int m_dead_w = 0;
	bit_array *m_stones_b, *m_stones_w;

	std::vector<stone_unit> m_units_b;
	std::vector<stone_unit> m_units_w;
	/* Only holds elements while calculating scoring markers.  */
	std::vector<terr_unit> m_units_t;
	std::vector<terr_unit> m_units_st;

	std::vector<mark> m_marks;
	std::vector<mextra> m_mark_extra;
	/* For mark::text, the mark_extra field is an index into this table of strings.  */
	std::vector<std::string> m_mark_text;

public:
	go_board (int sz) : m_sz (sz), m_stones_b (new bit_array (sz * sz)), m_stones_w (new bit_array (sz * sz))
	{
	}
	go_board (const go_board &other)
		: m_sz (other.m_sz), m_score_b (other.m_score_b), m_score_w (other.m_score_w),
		m_caps_b (other.m_caps_b), m_caps_w (other.m_caps_w),
		m_dead_b (other.m_dead_b), m_dead_w (other.m_dead_w),
		m_stones_b (new bit_array (*other.m_stones_b)), m_stones_w (new bit_array (*other.m_stones_w)),
		m_units_b (other.m_units_b), m_units_w (other.m_units_w),
		m_units_t (other.m_units_t), m_units_st (other.m_units_st),
		m_marks (other.m_marks), m_mark_extra (other.m_mark_extra), m_mark_text (other.m_mark_text)
	{
	}
	/* The unused mark argument should be passed as mark::none by the callers to indicate what this
	   constructor is for: copying a board position without copying marks.
	   Note that this implies territory markers are not copied, and we do not copy m_dead_b and m_dead_w
	   which just summarize that information.  */
	go_board (const go_board &other, mark)
		: m_sz (other.m_sz), m_score_b (other.m_score_b), m_score_w (other.m_score_w),
		m_caps_b (other.m_caps_b), m_caps_w (other.m_caps_w),
		m_dead_b (0), m_dead_w (0),
		m_stones_b (new bit_array (*other.m_stones_b)), m_stones_w (new bit_array (*other.m_stones_w)),
		m_units_b (other.m_units_b), m_units_w (other.m_units_w),
		m_units_t (other.m_units_t), m_units_st (other.m_units_st)
	{
	}
	go_board &operator= (go_board other)
	{
		m_sz = other.m_sz;
		m_score_b = other.m_score_b;
		m_score_w = other.m_score_w;
		m_caps_b = other.m_caps_b;
		m_caps_w = other.m_caps_w;
		m_dead_b = other.m_dead_b;
		m_dead_w = other.m_dead_w;
		std::swap (m_stones_w, other.m_stones_w);
		std::swap (m_stones_b, other.m_stones_b);
		std::swap (m_units_w, other.m_units_w);
		std::swap (m_units_b, other.m_units_b);
		std::swap (m_marks, other.m_marks);
		std::swap (m_mark_extra, other.m_mark_extra);
		std::swap (m_mark_text, other.m_mark_text);
		return *this;
	}
	~go_board ()
	{
		delete m_stones_b;
		delete m_stones_w;
	}
	int size () const
	{
		return m_sz;
	}
	int bitsize () const
	{
		return m_sz * m_sz;
	}
	int bitpos (int x, int y) const
	{
		return x + y * m_sz;
	}
	void identify_units ();
	int count_liberties (const bit_array &);

	bool valid_move_p (int x, int y, stone_color);
	void add_stone (int x, int y, stone_color col, bool process_captures = true);
	/* Must be followed by an identify_units call after setting all new stones.  */
	void set_stone (int x, int y, stone_color col)
	{
		int bp = bitpos (x, y);
		if (col != white)
			m_stones_w->clear_bit (bp);
		if (col != black)
			m_stones_b->clear_bit (bp);
		if (col == white)
			m_stones_w->set_bit (bp);
		else if (col == black)
			m_stones_b->set_bit (bp);
	}
	stone_color stone_at (int x, int y) const
	{
		int bp = bitpos (x, y);
		if (m_stones_b->test_bit (bp))
			return black;
		else if (m_stones_w->test_bit (bp))
			return white;
		return none;
	}
	mark mark_at (int x, int y) const
	{
		if (m_marks.size () == 0)
			return mark::none;
		return m_marks[bitpos (x, y)];
	}
	mextra mark_extra_at (int x, int y) const
	{
		if (m_marks.size () == 0)
			return 0;
		return m_mark_extra[bitpos (x, y)];
	}
	const std::string mark_text_at (int x, int y) const
	{
		mark t = mark_at (x, y);
		if (t != mark::text)
			return "";
		mextra idx = mark_extra_at (x, y);
		if (idx < m_mark_text.size ())
			return m_mark_text[idx];
		return "";
	}
	bool set_mark (int x, int y, mark m, mextra extra)
	{
		init_marks (false);
		int bp = bitpos (x, y);
		if (m_marks[bp] == m
		    && (m != mark::num || m != mark::letter || m_mark_extra[bp] == extra))
			return false;

		m_marks[bp] = m;
		m_mark_extra[bp] = extra;
		return true;
	}
	void set_text_mark (int x, int y, const std::string &str)
	{
		init_marks (false);
		int bp = bitpos (x, y);
		size_t idx = m_mark_text.size ();
		if (m_marks[bp] == mark::text) {
			idx = m_mark_extra[bp];
			m_mark_text[idx] = str;
		} else
			m_mark_text.push_back (str);
		m_marks[bp] = mark::text;
		m_mark_extra[bp] = idx;
	}
	void clear_marks ()
	{
		if (m_marks.size () == 0)
			return;
		for (int i = 0; i < bitsize (); i++) {
			m_marks[i] = mark::none;
			m_mark_extra[i] = 0;
		}
		m_mark_text.clear ();
		m_mark_text.shrink_to_fit ();
	}
	void toggle_alive (int x, int y, bool flood = true);
	void toggle_seki (int x, int y);
	void calc_scoring_markers_simple ();
	void calc_scoring_markers_complex ();

	bool position_equal_p (const go_board &other) const
	{
		if (m_sz != other.m_sz)
			return false;
		if (*m_stones_b != *other.m_stones_b)
			return false;
		if (*m_stones_w != *other.m_stones_w)
			return false;
		return true;
	}
	bool position_empty_p () const
	{
		return m_stones_b->popcnt () == 0 && m_stones_w->popcnt () == 0;
	}
	bool operator== (const go_board &other) const
	{
		if (m_sz != other.m_sz)
			return false;
		if (*m_stones_b != *other.m_stones_b)
			return false;
		if (*m_stones_w != *other.m_stones_w)
			return false;
		if (m_marks.size () != 0 || other.m_marks.size () != 0) {
			for (int i = 0; i < bitsize (); i++) {
				mark m1 = m_marks.size () == 0 ? mark::none : m_marks[i];
				mark m2 = other.m_marks.size () == 0 ? mark::none : other.m_marks[i];
				if (m1 != m2)
					return false;
				mextra e1 = m_marks.size () == 0 ? 0 : m_mark_extra[i];
				mextra e2 = other.m_marks.size () == 0 ? 0 : other.m_mark_extra[i];
				if (e1 != e2)
					return false;
			}
		}
		return true;
	}
	bool operator!= (const go_board &other) const { return !operator==(other); }

	void dump_ascii () const;
	void dump_bitmap (const bit_array &) const;

	const bit_array &get_stones_b () const
	{
		return *m_stones_b;
	}
	const bit_array &get_stones_w () const
	{
		return *m_stones_w;
	}
	void get_scores (int &caps_b, int &caps_w, int &score_b, int &score_w) const
	{
		caps_b = m_caps_b + m_dead_w;
		caps_w = m_caps_w + m_dead_b;
		score_b = m_score_b;
		score_w = m_score_w;
	}
	void territory_from_markers ();
	void append_marks_sgf (std::string &) const;

private:
	void recalc_liberties ();
	void find_territory_units (const bit_array &w_stones, const bit_array &b_stones,
				   const bit_array &masked_left, const bit_array &masked_right);
	void flood_step (bit_array &next, const bit_array &fill,
			 const bit_array &masked_left, const bit_array &masked_right);
	void finish_scoring_markers (const bit_array *do_not_count);
	void scoring_flood_fill (bit_array &fill, const bit_array &masked_left, const bit_array &masked_right,
				 const bit_array &w_stones, const bit_array &b_stones,
				 bool &neighbours_w, bool &neighbours_b);
	void init_marks (bool clear)
	{
		if (m_marks.size () == 0) {
			m_marks.resize (bitsize ());
			m_mark_extra.resize (bitsize ());
			clear = true;
		}
		if (clear)
			clear_marks ();
	}
	bit_array collect_marks (mark t) const
	{
		bit_array tmp (bitsize ());
		for (int i = 0; i < bitsize (); i++)
			if (m_marks[i] == t)
				tmp.set_bit (i);
		return tmp;
	}
	bit_array collect_marks (mark t, mextra me) const
	{
		bit_array tmp (bitsize ());
		for (int i = 0; i < m_sz * m_sz; i++)
			if (m_marks[i] == t && m_mark_extra[i] == me)
				tmp.set_bit (i);
		return tmp;
	}
	void append_mark_plane_sgf (std::string &, const std::string &, const bit_array &) const;
	void verify_invariants ();
};

#endif
