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
	none, black, white, unknown
};

inline stone_color flip_color (stone_color c)
{
	switch (c) {
	case white: return black;
	case black: return white;
	default: return none;
	}
}

enum class stone_type { live, seki, dead, var };
enum class mark { none = 0, move, triangle, circle, square, plus, cross, text, num, letter, dead, seki, terr, falseeye };
typedef unsigned short mextra;

struct go_score
{
	int caps_b = 0, caps_w = 0;
	int score_b = 0, score_w = 0;
	int stones_b = 0, stones_w = 0;
};

class go_board
{
	/* In everyday conversation, we might call these "strings of stones".  That
	   terminology has a bit of a name clash in a computer program.  So we call
	   them "units".  */
	class stone_unit
	{
		friend class go_board;
		bit_array m_stones;
		short m_n_liberties;
		/* Used during Benson's algorithm.  */
		short m_n_vital;
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

	/* An anclosed area for a color C1 is obtained by a flood fill through empty spaces and stones
	   of the opposite color C2.  We also obtain the border of the area, by expanding once in each
	   direction and then removing the actual area (note that the border can contain stones in the
	   interior).  Finally, before storing the area, stones of C2 are removed from the bit set.  */
	struct enclosed_area
	{
		bit_array area;
		bit_array border;
		enclosed_area (const bit_array &a, const bit_array &b) : area (a), border (b) { }
		enclosed_area (bit_array &&a, bit_array &&b) : area (a), border (b) { }
		enclosed_area &operator= (enclosed_area other)
		{
			std::swap (other.area, area);
			std::swap (other.border, border);
			return *this;
		}
	};
	int m_sz_x, m_sz_y;
	bool m_torus_h = false, m_torus_v = false;
	const bit_array *m_masked_left, *m_masked_right, *m_column_left, *m_column_right, *m_row_top, *m_row_bottom;
	/* A mask for the board's intersections.  Null for non-variant games.
	   Owned by the game_record corresponding to this board.  */
	std::shared_ptr<const bit_array> m_mask {};
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
	bit_array m_stones_b, m_stones_w;

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
	go_board (int w, int h, bool torus_h = false, bool torus_v = false);
	go_board (int sz) : go_board (sz, sz)
	{
	}

	go_board (const go_board &other)
		: m_sz_x (other.m_sz_x), m_sz_y (other.m_sz_y),
		m_torus_h (other.m_torus_h), m_torus_v (other.m_torus_v),
		m_masked_left (other.m_masked_left), m_masked_right (other.m_masked_right),
		m_column_left (other.m_column_left), m_column_right (other.m_column_right),
		m_row_top (other.m_row_top), m_row_bottom (other.m_row_bottom),
		m_mask (other.m_mask),
		m_score_b (other.m_score_b), m_score_w (other.m_score_w),
		m_caps_b (other.m_caps_b), m_caps_w (other.m_caps_w),
		m_dead_b (other.m_dead_b), m_dead_w (other.m_dead_w),
		m_stones_b (other.m_stones_b), m_stones_w (other.m_stones_w),
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
		: m_sz_x (other.m_sz_x), m_sz_y (other.m_sz_y),
		m_torus_h (other.m_torus_h), m_torus_v (other.m_torus_v),
		m_masked_left (other.m_masked_left), m_masked_right (other.m_masked_right),
		m_column_left (other.m_column_left), m_column_right (other.m_column_right),
		m_row_top (other.m_row_top), m_row_bottom (other.m_row_bottom),
		m_mask (other.m_mask),
		m_score_b (other.m_score_b), m_score_w (other.m_score_w),
		m_caps_b (other.m_caps_b), m_caps_w (other.m_caps_w),
		m_dead_b (0), m_dead_w (0),
		m_stones_b (other.m_stones_b), m_stones_w (other.m_stones_w),
		m_units_b (other.m_units_b), m_units_w (other.m_units_w),
		m_units_t (other.m_units_t), m_units_st (other.m_units_st)
	{
	}
	/* A similar constructor to create an empty board with identical properties to another.
	   Callers should pass none to the unused stone_color argument for clarity.  */
	go_board (const go_board &other, stone_color)
		: m_sz_x (other.m_sz_x), m_sz_y (other.m_sz_y),
		m_torus_h (other.m_torus_h), m_torus_v (other.m_torus_v),
		m_masked_left (other.m_masked_left), m_masked_right (other.m_masked_right),
		m_column_left (other.m_column_left), m_column_right (other.m_column_right),
		m_row_top (other.m_row_top), m_row_bottom (other.m_row_bottom),
		m_mask (other.m_mask),
		m_stones_b (other.bitsize ()), m_stones_w (other.bitsize ())
	{
	}
	go_board &operator= (go_board other)
	{
		m_sz_x = other.m_sz_x;
		m_sz_y = other.m_sz_y;

		m_torus_h = other.m_torus_h;
		m_torus_v = other.m_torus_v;
		m_masked_left = other.m_masked_left;
		m_masked_right = other.m_masked_right;
		m_column_left = other.m_column_left;
		m_column_right = other.m_column_right;
		m_row_top = other.m_row_top;
		m_row_bottom = other.m_row_bottom;

		m_mask = other.m_mask;

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
	}
	int size_x () const
	{
		return m_sz_x;
	}
	int size_y () const
	{
		return m_sz_y;
	}
	bool torus_h () const
	{
		return m_torus_h;
	}
	bool torus_v () const
	{
		return m_torus_v;
	}
	unsigned bitsize () const
	{
		return m_sz_x * m_sz_y;
	}
	int bitpos (int x, int y) const
	{
		return x + y * m_sz_x;
	}
	void set_mask (std::shared_ptr<const bit_array> m)
	{
		m_mask = m;
	}
	void identify_units ();
	int count_liberties (const bit_array &);

	std::pair<std::string, std::string> coords_name (int x, int y, bool sgf) const
	{
		if (sgf)
			return std::make_pair (std::string (1, 'a' + x), std::string (1, 'a' + y));
		if (x > 7)
			x++;
		return std::make_pair (std::string (1, 'A' + x), std::to_string (m_sz_y - y));
	}
	bool valid_move_p (int x, int y, stone_color);
	void add_stone (int x, int y, stone_color col, bool process_captures = true);
	/* Must be followed by an identify_units call after setting all new stones.  */
	void set_stone_nounits (int x, int y, stone_color col)
	{
		int bp = bitpos (x, y);
		if (col != white)
			m_stones_w.clear_bit (bp);
		if (col != black)
			m_stones_b.clear_bit (bp);
		if (col == white)
			m_stones_w.set_bit (bp);
		else if (col == black)
			m_stones_b.set_bit (bp);
	}
	void set_stone (int x, int y, stone_color col)
	{
		set_stone_nounits (x, y, col);
		identify_units ();
	}
	stone_color stone_at (int x, int y) const
	{
		int bp = bitpos (x, y);
		if (m_stones_b.test_bit (bp))
			return black;
		else if (m_stones_w.test_bit (bp))
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
		for (unsigned i = 0; i < bitsize (); i++) {
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
		if (m_sz_x != other.m_sz_x || m_sz_y != other.m_sz_y)
			return false;
		if (m_stones_b != other.m_stones_b)
			return false;
		if (m_stones_w != other.m_stones_w)
			return false;
		return true;
	}
	bool position_empty_p () const
	{
		return m_stones_b.popcnt () == 0 && m_stones_w.popcnt () == 0;
	}
	bool operator== (const go_board &other) const
	{
		if (m_sz_x != other.m_sz_x || m_sz_y != other.m_sz_y)
			return false;
		if (m_stones_b != other.m_stones_b)
			return false;
		if (m_stones_w != other.m_stones_w)
			return false;
		if (m_marks.size () != 0 || other.m_marks.size () != 0) {
			for (unsigned i = 0; i < bitsize (); i++) {
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
		return m_stones_b;
	}
	const bit_array &get_stones_w () const
	{
		return m_stones_w;
	}
	go_score get_scores () const;
	void territory_from_markers ();
	void append_marks_sgf (std::string &) const;

private:
	void recalc_liberties ();
	void find_territory_units (const bit_array &w_stones, const bit_array &b_stones);
	bit_array init_fill (int, const bit_array &, bool);
	void flood_step (bit_array &next, const bit_array &fill);
	void flood_fill (bit_array &fill, const bit_array &boundary);
	void finish_scoring_markers (const bit_array *do_not_count);
	void scoring_flood_fill (bit_array &fill, const bit_array &w_stones, const bit_array &b_stones,
				 bool &neighbours_w, bool &neighbours_b);
	/* Two functions implementing Benson's algorithm.  */
	std::vector<enclosed_area> find_eas (const bit_array &stones, const bit_array &other_stones);
	void benson (std::vector<stone_unit> &units, const bit_array &other_stones);

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
		for (unsigned i = 0; i < bitsize (); i++)
			if (m_marks[i] == t)
				tmp.set_bit (i);
		return tmp;
	}
	bit_array collect_marks (mark t, mextra me) const
	{
		bit_array tmp (bitsize ());
		for (unsigned i = 0; i < bitsize (); i++)
			if (m_marks[i] == t && m_mark_extra[i] == me)
				tmp.set_bit (i);
		return tmp;
	}
	void append_mark_plane_sgf (std::string &, const std::string &, const bit_array &) const;
	void verify_invariants ();
};

struct board_rect
{
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	board_rect () = default;
	board_rect (const go_board &b)
	{
		x2 = b.size_x () - 1;
		y2 = b.size_y () - 1;
	}
	board_rect (int xi1, int yi1, int xi2, int yi2) : x1 (xi1), y1 (yi1), x2 (xi2), y2 (yi2)
	{
	}
	bool operator== (const board_rect &other) const
	{
		return x1 == other.x1 && x2 == other.x2 && y1 == other.y1 && y2 == other.y2;
	}
	bool operator!= (const board_rect &other) const
	{
		return !(*this == other);
	}
	int width () const { return x2 - x1 + 1; }
	int height () const { return y2 - y1 + 1; }
	bool contained (int x, int y) const
	{
		return x >= x1 && x <= x2 && y >= y1 && y <= y2;
	}
	bool aligned_left (const board_rect &other) const
	{
		return x1 == other.x1;
	}
	bool aligned_right (const board_rect &other) const
	{
		return x2 == other.x2;
	}
	bool aligned_top (const board_rect &other) const
	{
		return y1 == other.y1;
	}
	bool aligned_bottom (const board_rect &other) const
	{
		return y2 == other.y2;
	}
	bool is_square () { return width () == height (); }
};

/* Some functions to create unique bit sets of particular shapes.  These are the
   ones required elsewhere, the others are private to goboard.cc.  */
extern const bit_array *create_row_top (int w, int h);
extern const bit_array *create_column_left (int w, int h);

#endif
