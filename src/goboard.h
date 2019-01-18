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
	int m_score_b = 0;
	int m_score_w = 0;
	int m_caps_b = 0;
	int m_caps_w = 0;
	bit_array *m_stones_b, *m_stones_w;
	int *markers = nullptr;
	std::vector<stone_unit> m_units_w;
	std::vector<stone_unit> m_units_b;
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
		m_stones_b (new bit_array (*other.m_stones_b)), m_stones_w (new bit_array (*other.m_stones_w)),
		m_marks (other.m_marks), m_mark_extra (other.m_mark_extra), m_mark_text (other.m_mark_text)
	{
		identify_units ();
	}
	go_board &operator= (go_board other)
	{
		m_sz = other.m_sz;
		m_score_b = other.m_score_b;
		m_score_w = other.m_score_w;
		m_caps_b = other.m_caps_b;
		m_caps_w = other.m_caps_w;
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
		for (int i = 0; i < m_sz * m_sz; i++) {
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
			for (int i = 0; i < m_sz * m_sz; i++) {
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
		caps_b = m_caps_b;
		caps_w = m_caps_w;
		score_b = m_score_b;
		score_w = m_score_w;
	}
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
			m_marks.resize (m_sz * m_sz);
			m_mark_extra.resize (m_sz * m_sz);
			clear = true;
		}
		if (clear)
			clear_marks ();
	}
	bit_array collect_marks (mark t) const
	{
		bit_array tmp (m_sz * m_sz);
		for (int i = 0; i < m_sz * m_sz; i++)
			if (m_marks[i] == t)
				tmp.set_bit (i);
		return tmp;
	}
	bit_array collect_marks (mark t, mextra me) const
	{
		bit_array tmp (m_sz * m_sz);
		for (int i = 0; i < m_sz * m_sz; i++)
			if (m_marks[i] == t && m_mark_extra[i] == me)
				tmp.set_bit (i);
		return tmp;
	}
	void append_mark_plane_sgf (std::string &, const std::string &, const bit_array &) const;
	void verify_invariants ()
	{
#ifdef CHECKING
		if (m_stones_b->intersect_p (*m_stones_w))
			throw std::logic_error ("white stones and black stones overlap");
#endif
	}
};

class game_state
{
public:
	class observer;

private:
	go_board m_board;
	int m_move_number;
	std::vector<game_state *> m_children;
	size_t m_active = 0;
	game_state *m_parent;
	stone_color m_to_move;
	mutable std::list<observer *> m_observers;

	int m_move_x = -1, m_move_y = -1;
	/* It seems strange to keep both a to_move and move_color. There is redundancy, but we really
	   need both: the to_move color for edit purposes, and the move color since arbitrary sgf files
	   might contain child moves by different colors, and suicide moves could make it inconvenient
	   to obtain the move color from the board.  */
	stone_color m_move_color = none;
	std::string m_comment;

	sgf::node::proplist m_unrecognized_props;

	/* A slight violation of abstractions; this is not really a property of the game, but
	   something handled by the user interface.  But the alternative is keeping some kind of
	   map of game states in the board window and making sure it gets updated whenever we delete
	   one.  The cost of simply carrying an extra boolean here is just way lower.  */
	bool m_start_count = false;

	game_state (const go_board &b, int move, game_state *parent, stone_color to_move)
		: m_board (b), m_move_number (move), m_parent (parent), m_to_move (to_move)
	{
	}

	game_state (const go_board &b, int move, game_state *parent, stone_color to_move, int x, int y, stone_color move_col)
		: m_board (b), m_move_number (move), m_parent (parent), m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col)
	{
	}

	game_state (const go_board &b, int move, game_state *parent, stone_color to_move, int x, int y, stone_color move_col, const sgf::node::proplist &unrecognized)
		: m_board (b), m_move_number (move), m_parent (parent), m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col), m_unrecognized_props (unrecognized)
	{
	}

public:
	class observer
	{
	protected:
		observer () : m_state (nullptr) { }
		~observer () { stop_observing (); }

		void start_observing (game_state *st)
		{
			move_state (st);
		}
		void stop_observing ()
		{
			if (m_state != nullptr)
				m_state->remove_observer (this);
			m_state = nullptr;
		}
	public:
		game_state *m_state;
		virtual void observed_changed () = 0;
		void move_state (game_state *s)
		{
			if (s == m_state)
				return;
			if (m_state != nullptr)
				m_state->remove_observer (this);
			if (s != nullptr)
				s->m_observers.push_back (this);
			m_state = s;
			observed_changed ();
		}
		/* Called only when deleting a game record.  Does not need to update
		   observer lists.  */
		void set_state (game_state *s)
		{
			m_state = s;
		}
	};

	game_state (int size) : m_board (size), m_move_number (0), m_parent (0), m_to_move (black)
	{
	}
	game_state (const go_board &b, stone_color to_move)
		: m_board (b), m_move_number (0), m_parent (nullptr), m_to_move (to_move)
	{
	}
	/* Deep copy.  Don't copy observers.  */
	game_state (const game_state &other, game_state *parent)
		: game_state (other.m_board, other.m_move_number, parent, other.m_to_move,
			      other.m_move_x, other.m_move_y, other.m_move_color, other.m_unrecognized_props)
	{
		for (auto c: other.m_children) {
			game_state *new_c = new game_state (*c, this);
			m_children.push_back (new_c);
		}
		m_comment = other.m_comment;
		m_active = other.m_active;
	}
	~game_state ()
	{
		for (sgf::node::property *it: m_unrecognized_props)
			delete it;

		if (m_parent) {
			game_state *last = m_parent->m_children.back ();
			m_parent->m_children.pop_back ();
			size_t n = m_parent->m_children.size ();
			size_t i;
			for (i = 0; i < n; i++) {
				if (m_parent->m_children[i] == this) {
					m_parent->m_children[i] = last;
					break;
				}
			}
			if (i == m_parent->m_active && i > 0)
				m_parent->m_active--;
#if 0
			if (i == n && last != this)
				throw std::logic_error ("delete node not found among parent's children");
#endif
		}
		while (m_children.size () > 0) {
			game_state *c = m_children.back ();
			delete c;
		}
		for (auto &it: m_observers) {
			it->move_state (m_parent);
		}
	}
	void set_unrecognized (const sgf::node::proplist &list)
	{
		m_unrecognized_props = list;
	}
	stone_color to_move () const
	{
		return m_to_move;
	}
	void set_to_move (stone_color col)
	{
		m_to_move = col;
	}
	int move_number () const
	{
		return m_move_number;
	}
	int active_var_max () const
	{
		const game_state *st = this;
		while (st->m_children.size () > 0)
			st = st->m_children[st->m_active];
		return st->m_move_number;
	}
	const go_board &get_board () const
	{
		return m_board;
	}
	bool was_pass_p () const
	{
		return m_move_x == -1;
	}
	bool was_edit_p () const
	{
		return m_move_x == -2;
	}
	bool was_score_p () const
	{
		return m_move_x == -3;
	}
	bool was_move_p () const
	{
		return m_move_x >= 0;
	}
	int get_move_x () const
	{
		return m_move_x;
	}
	int get_move_y () const
	{
		return m_move_y;
	}
	stone_color get_move_color () const
	{
		return m_move_color;
	}
	void remove_observer (const observer *o) const
	{
		for (auto it = m_observers.begin (); it != m_observers.end (); ++it) {
			if (*it == o) {
				m_observers.erase (it);
				return;
			}
		}
	}

	void transfer_observers (game_state *other)
	{
		for (auto it: m_observers) {
			it->m_state = other;
			other->m_observers.push_back (it);
			it->observed_changed ();
		}
		m_observers.clear ();
	}

	game_state *add_child_edit (const go_board &new_board, stone_color to_move, bool scored = false, bool set_active = true)
	{
		for (auto &it: m_children)
			if (it->m_board == new_board && it->m_to_move == to_move)
				return it;
		int code = scored ? -3 : -2;
		game_state *tmp = new game_state (new_board, m_move_number + 1, this, to_move, code, code, none);
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}
	game_state *add_child_move (const go_board &new_board, stone_color to_move, int x, int y, bool set_active = true)
	{
		stone_color next_to_move = to_move == black ? white : black;
		for (auto &it: m_children)
			if (it->m_board == new_board && it->m_to_move == next_to_move)
				return it;
		game_state *tmp = new game_state (new_board, m_move_number + 1, this, next_to_move, x, y, to_move);
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}
	game_state *add_child_move (int x, int y, stone_color to_move)
	{
		if (!valid_move_p (x, y, to_move))
			return nullptr;

		go_board new_board = m_board;
		new_board.clear_marks ();
		new_board.add_stone (x, y, to_move);
		/* Check for ko.  */
		if (m_parent != nullptr) {
			if (m_parent->m_board.position_equal_p (new_board))
				return nullptr;
		}
		return add_child_move (new_board, m_to_move, x, y);
	}
	game_state *add_child_move (int x, int y)
	{
		return add_child_move (x, y, m_to_move);
	}
	game_state *add_child_pass (const go_board &new_board, bool set_active = true)
	{
		for (auto &it: m_children)
			if (it->m_board == new_board && it->was_pass_p ())
				return it;
		game_state *tmp = new game_state (new_board, m_move_number + 1, this, m_to_move == black ? white : black);
		tmp->m_move_color = m_to_move;
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}
	game_state *add_child_pass (bool set_active = true)
	{
		return add_child_pass (m_board, set_active);
	}
	bool valid_move_p (int x, int y, stone_color);
	void toggle_group_alive (int x, int y)
	{
		m_board.toggle_alive (x, y);
	}
	game_state *next_move (bool set_primary = false)
	{
		if (m_children.size () == 0)
			return nullptr;
		if (set_primary)
			m_active = 0;
		return m_children[m_active];
	}
	game_state *prev_move ()
	{
		return m_parent;
	}
	bool has_next_sibling () const
	{
		if (m_parent == nullptr)
			return false;
		return m_parent->m_children.back () != this;
	}
	bool has_prev_sibling () const
	{
		if (m_parent == nullptr)
			return false;
		return m_parent->m_children[0] != this;
	}
	game_state *next_sibling (bool set)
	{
		if (m_parent == nullptr)
			return this;
		size_t n = m_parent->m_children.size ();
		size_t ret = n - 1;
		while (n-- > 0) {
			game_state *v = m_parent->m_children[n];
			if (v == this) {
				if (set)
					m_parent->m_active = ret;
				return m_parent->m_children[ret];
			}
			ret = n;
		}
		throw std::logic_error ("variation not present in parent");
	}
	game_state *prev_sibling (bool set)
	{
		if (m_parent == nullptr)
			return this;
		size_t n = m_parent->m_children.size ();
		size_t ret = 0;
		for (size_t i = 0; i < n; i++) {
			game_state *v = m_parent->m_children[i];
			if (v == this) {
				if (set)
					m_parent->m_active = ret;
				return m_parent->m_children[ret];
			}
			ret = i;
		}
		throw std::logic_error ("variation not present in parent");
	}
	int n_siblings () const
	{
		if (m_parent == nullptr)
			return 0;
		return m_parent->m_children.size () - 1;
	}
	int var_number () const
	{
		if (m_parent == nullptr)
			return 1;
		for (size_t i = 0; i < m_parent->m_children.size (); i++)
			if (m_parent->m_children[i] == this)
				return i + 1;
		throw std::logic_error ("not a child of its parent");
	}
	int n_children () const
	{
		return m_children.size ();
	}
	game_state *find_child_move (int x, int y)
	{
		for (auto &it: m_children)
			if (it->was_move_p () && it->m_move_x == x && it->m_move_y == y)
				return it;
		return nullptr;
	}
	bool root_node_p () const
	{
		return m_parent == nullptr;
	}
	/* Collect a set of child moves to a board, for use in variation display.  */
	const go_board child_moves (const game_state *excluding) const;
	const go_board sibling_moves () const
	{
		game_state *p = m_parent;
		if (p == nullptr)
			return go_board (m_board.size ());
		return p->child_moves (this);
	}
	/* Set a mark on the current board, and return true if that made a change.  */
	bool set_mark (int x, int y, mark m, mextra extra)
	{
		return m_board.set_mark (x, y, m, extra);
	}
	void set_text_mark (int x, int y, const std::string &str)
	{
		m_board.set_text_mark (x, y, str);
	}
	void set_comment (const std::string &c)
	{
		m_comment = c;
	}
	const std::string &comment () const
	{
		return m_comment;
	}

	void append_to_sgf (std::string &) const;

	/* Should really only be used for setting handicap at the root node.  */
	void replace (const go_board &b, stone_color to_move)
	{
		m_board = b;
		m_to_move = to_move;
		for (auto it: m_observers)
			it->observed_changed ();
	}
	void set_start_count (bool on)
	{
		m_start_count = on;
	}
	bool get_start_count () const
	{
		return m_start_count;
	}
};

class sgf;
class game_record;

enum class ranked { unknown, free, ranked, teaching };

class game_info
{
protected:
	std::string m_filename = "";

	std::string m_title = "";
	std::string m_name_w, m_name_b;
	std::string m_rank_w = "", m_rank_b = "";

	std::string m_rules = "";
	double m_komi = 0;
	int m_handicap = 0;

	std::string m_result = "";

	std::string m_date = "";
	std::string m_place = "";
	ranked m_ranked = ranked::unknown;

	std::string m_time = "";
	std::string m_overtime = "";

	std::string m_copyright = "";

	/* SGF variation display style, or -1 if unset by SGF file.  */
	int m_style = -1;
public:
	ranked ranked_type () const { return m_ranked; }
	double komi () const { return m_komi; }
	int handicap () const { return m_handicap; }

	void set_ranked_type (ranked r) { m_ranked = r; }
	void set_komi (double k) { m_komi = k; }
	void set_handicap (int h) { m_handicap = h; }

	int style () const { return m_style; }
	void set_style (int st) { m_style = st; }

	const std::string &name_white () const { return m_name_w; }
	const std::string &name_black () const { return m_name_b; }
	const std::string &rank_white () const { return m_rank_w; }
	const std::string &rank_black () const { return m_rank_b; }
	const std::string &title () const { return m_title; }
	const std::string &result () const { return m_result; }
	const std::string &date () const { return m_date; }
	const std::string &place () const { return m_place; }
	const std::string &copyright () const { return m_copyright; }
	const std::string &rules () const { return m_rules; }
	const std::string &time () const { return m_time; }
	const std::string &overtime () const { return m_overtime; }

	const std::string &filename () const { return m_filename; }

	void set_name_white (const std::string &s) { m_name_w = s; }
	void set_name_black (const std::string &s) { m_name_b = s; }
	void set_rank_white (const std::string &s) { m_rank_w = s; }
	void set_rank_black (const std::string &s) { m_rank_b = s; }
	void set_title (const std::string &s) { m_title = s; }
	void set_result (const std::string &s) { m_result = s; }
	void set_date (const std::string &s) { m_date = s; }
	void set_place (const std::string &s) { m_place = s; }
	void set_copyright (const std::string &s) { m_copyright = s; }
	void set_rules (const std::string &s) { m_rules = s; }
	void set_time (const std::string &s) { m_time = s; }
	void set_overtime (const std::string &s) { m_overtime = s; }

	void set_filename (const std::string &s) { m_filename = s; }

	game_info (const std::string &w, const std::string &b)
		: m_name_w (w), m_name_b (b)
	{
	}

	game_info (const std::string title, const std::string &w, const std::string &b,
		   const std::string &rw, const std::string &rb,
		   const std::string &ru, double komi, int hc, ranked rt, const std::string &re,
		   const std::string &dt, const std::string &pc, const std::string &cp,
		   const std::string &tm, const std::string &ot, int style)
		: m_title (title), m_name_w (w), m_name_b (b), m_rank_w (rw), m_rank_b (rb),
		m_rules (ru), m_komi (komi), m_handicap (hc), m_result (re), m_date (dt), m_place (pc),
		m_ranked (rt), m_time (tm), m_overtime (ot),
	  m_copyright (cp), m_style (style)
	{
	}
};

class game_record;
extern game_state *sgf2board (sgf &);
extern std::shared_ptr<game_record> sgf2record (const sgf &);
extern std::string record2sgf (const game_record &);

class game_record : public game_info
{
	friend std::shared_ptr<game_record> sgf2record (const sgf &s);
	game_state m_root;

public:
	game_record (int size, const game_info &info)
		: game_info (info), m_root (size)
	{

	}
	game_record (const go_board &b, stone_color to_move, const game_info &info)
		: game_info (info), m_root (b, to_move)
	{

	}
	game_record (const game_record &other) : game_info (other), m_root (other.m_root, nullptr)
	{
	}

	game_state *get_root () { return &m_root; }
	bool replace_root (const go_board &b, stone_color to_move)
	{
		if (m_root.n_children () > 0)
			return false;
		m_root.replace (b, to_move);
		return true;
	}

	int boardsize () const
	{
		return m_root.get_board ().size ();
	}
	std::string to_sgf () const;
	~game_record ()
	{
		// m_last_move.set_state (nullptr);
	}
};

#endif
