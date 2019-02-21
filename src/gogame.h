#ifndef GOGAME_H
#define GOGAME_H

#include "goboard.h"
#include <functional>
class visual_tree
{
public:
	struct bit_rect
	{
		std::vector<bit_array> m_rep;
		int m_w, m_h;
		bit_rect (int w, int h) : m_w (w), m_h (h)
		{
			for (int i = 0; i < h; i++)
				m_rep.emplace_back (m_w);
		}
		void set_bit (int x, int y)
		{
			m_rep[y].set_bit (x);
		}
		bool test_bit (int x, int y)
		{
			return m_rep[y].test_bit (x);
		}
		void ior (const bit_rect &other, int xoff, int yoff)
		{
			for (int i = 0; i < other.m_h; i++)
				m_rep[yoff + i].ior (other.m_rep[i], xoff);
		}
		bool test_overlap (const bit_rect &other, int xoff, int yoff)
		{
			for (int y = yoff; y < m_h && y - yoff < other.m_h; y++)
				if (m_rep[y].intersect_p (other.m_rep[y - yoff], xoff))
					return true;
			return false;
		}
		void set_max_width (int x)
		{
			if (x <= m_w)
				return;
			m_w = x;
			for (auto &vec: m_rep)
				vec.grow (m_w);
		}
		void set_max_height (int h)
		{
			while (m_h < h) {
				m_rep.emplace_back (m_w);
				m_h++;
			}
		}
	};

private:
	bit_rect m_rep;
	/* The offset from the parent's box.  */
	int m_off_y = 0;
public:
	visual_tree (bool collapsed = false)
		: m_rep (collapsed ? 2 : 1, 1)
	{
		m_rep.set_bit (0, 0);
		if (m_rep.m_w == 2)
			m_rep.set_bit (1, 0);
	}
	visual_tree (visual_tree &main_var, int max_child_width);
	void add_variation (visual_tree &other);
	int width () const
	{
		return m_rep.m_w;
	}
	int height () const
	{
		return m_rep.m_h;
	}
	int y_offset () const
	{
		return m_off_y;
	}
	const bit_rect &representation ()
	{
		return m_rep;
	}
};

class game_state
{
public:
	class observer;

private:
	go_board m_board;
	/* The move number within this game tree.  Unaffected by SGF MN properties.  */
	int m_move_number;
	/* Move number as specified by SGF MN, or as above.  */
	int m_sgf_movenum;

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

	std::string m_timeleft_w = "", m_timeleft_b = "";
	std::string m_stonesleft_w = "", m_stonesleft_b = "";

	sgf::node::proplist m_unrecognized_props;

	/* Default initialized to a one-node tree, which is up-to-date when initialized without children.  */
	visual_tree m_visualized;
	/* The visualization is up-to-date iff both this variable is true, and none of the children
	   require updates.  */
	bool m_visual_ok = true;
	/* True if we should not be showing child nodes.  Always false if no children exist.  */
	bool m_visual_collapse = false;
	/* True if this node was considered visible when calculating the parent's visualization.  */
	bool m_visual_shown = false;

	/* The SGF PM property, or -1 if it wasn't set.  */
	int m_print_numbering = -1;
	sgf_figure m_figure;
	/* Win rate information.  Zero visits means it is unset.  */
	int m_eval_visits = 0;
	double m_eval_wr_black = 0.5;
	double m_eval_komi = 0;
	bool m_eval_komi_set = false;

	/* Support for SGF VW.  */
	bit_array *m_visible {};

	game_state (const go_board &b, int move, int sgf_move, game_state *parent, stone_color to_move)
		: m_board (b), m_move_number (move), m_sgf_movenum (sgf_move), m_parent (parent), m_to_move (to_move)
	{
	}

	game_state (const go_board &b, int move, int sgf_move, game_state *parent, stone_color to_move, int x, int y, stone_color move_col)
		: m_board (b), m_move_number (move), m_sgf_movenum (sgf_move), m_parent (parent), m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col)
	{
	}

	/* This isn't used except for delegation when doing a deep copy.  Uncertain how
	   much we should copy here vs there.  */
	game_state (const go_board &b, int move, int sgf_move, game_state *parent,
		    stone_color to_move, int x, int y, stone_color move_col,
		    const sgf::node::proplist &unrecognized, const visual_tree &vt, bool vtok,
		    bit_array *visible)
		: m_board (b), m_move_number (move), m_sgf_movenum (sgf_move), m_parent (parent),
		m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col),
		m_unrecognized_props (unrecognized), m_visualized (vt), m_visual_ok (vtok),
		m_visible (visible == nullptr ? nullptr : new bit_array (*visible))
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
		game_state *m_state = nullptr;
		virtual void observed_changed () = 0;
		void move_state (game_state *s, bool deleting_state = false)
		{
			if (s == m_state)
				return;
			/* Avoid removing items from a list we're iterating over
			   in the game_state destructor.  */
			if (m_state != nullptr && !deleting_state)
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

	game_state (int size) : m_board (size), m_move_number (0), m_sgf_movenum (0), m_parent (0), m_to_move (black)
	{
	}
	game_state (const go_board &b, stone_color to_move)
		: m_board (b), m_move_number (0), m_sgf_movenum (0), m_parent (nullptr), m_to_move (to_move)
	{
	}
	/* Deep copy.  Don't copy observers.  */
	game_state (const game_state &other, game_state *parent)
		: game_state (other.m_board, other.m_move_number, other.m_sgf_movenum, parent, other.m_to_move,
			      other.m_move_x, other.m_move_y, other.m_move_color, other.m_unrecognized_props,
			      other.m_visualized, other.m_visual_ok, other.m_visible)
	{
		for (auto c: other.m_children) {
			game_state *new_c = new game_state (*c, this);
			m_children.push_back (new_c);
		}
		m_comment = other.m_comment;
		m_active = other.m_active;
		m_figure = other.m_figure;
		m_print_numbering = other.m_print_numbering;
		m_eval_visits = other.m_eval_visits;
		m_eval_wr_black = other.m_eval_wr_black;
		m_eval_komi = other.m_eval_komi;
		m_eval_komi_set = other.m_eval_komi_set;
	}
	void disconnect ()
	{
		game_state *parent = m_parent;

		if (parent) {
			game_state *last = parent->m_children.back ();
			parent->m_children.pop_back ();
			size_t n = parent->m_children.size ();
			size_t i;
			for (i = 0; i < n; i++) {
				if (parent->m_children[i] == this) {
					parent->m_children[i] = last;
					break;
				}
			}
			if (i == parent->m_active && i > 0)
				parent->m_active--;

			parent->m_visual_ok = false;
			if (parent->m_children.size () == 0)
				parent->m_visual_collapse = false;
#if 0
			if (i == n && last != this)
				throw std::logic_error ("delete node not found among parent's children");
#endif
		}
		m_parent = nullptr;
		for (auto &it: m_observers) {
			it->move_state (parent, true);
		}

	}
	~game_state ()
	{
		for (sgf::node::property *it: m_unrecognized_props)
			delete it;

		while (m_children.size () > 0) {
			game_state *c = m_children.back ();
			delete c;
		}
		delete m_visible;

		disconnect ();
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
	int sgf_move_number () const
	{
		return m_sgf_movenum;
	}
	void set_sgf_move_number (int n)
	{
		m_sgf_movenum = n;
	}
	int print_numbering () const
	{
		return m_print_numbering;
	}
	int print_numbering_inherited () const
	{
		const game_state *gs = this;
		while (gs != nullptr) {
			if (gs->m_print_numbering >= 0)
				return gs->m_print_numbering;
			gs = gs->m_parent;
		}
		return -1;
	}
	void set_print_numbering (int n)
	{
		if (n >= 0 && n <= 2)
			m_print_numbering = n;
		else
			m_print_numbering = -1;
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

	std::string time_left (stone_color col) const
	{
		return col == white ? m_timeleft_w : m_timeleft_b;
	}
	std::string stones_left (stone_color col) const
	{
		return col == white ? m_stonesleft_w : m_stonesleft_b;
	}
	void set_time_left (stone_color col, std::string tm)
	{
		if (col == white)
			m_timeleft_w = tm;
		else
			m_timeleft_b = tm;
	}
	void set_stones_left (stone_color col, std::string tm)
	{
		if (col == white)
			m_stonesleft_w = tm;
		else
			m_stonesleft_b = tm;
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
	void make_active ()
	{
		game_state *p = m_parent;
		game_state *prev = this;
		while (p != nullptr) {
			size_t n = p->m_children.size ();
			for (size_t i = 0; i < n; i++) {
				game_state *v = p->m_children[i];
				if (v == prev) {
					p->m_active = i;
					break;
				}
			}
			prev = p;
			p = p->m_parent;
		}
	}

	game_state *add_child_edit_nochecks (const go_board &new_board, stone_color to_move, bool scored, bool set_active)
	{
		m_visual_ok = false;
		int code = scored ? -3 : -2;
		game_state *tmp = new game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
						  this, to_move, code, code, none);
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}
	game_state *add_child_edit (const go_board &new_board, stone_color to_move, bool scored = false, bool set_active = true)
	{
		for (auto &it: m_children)
			if (it->m_board == new_board && it->m_to_move == to_move)
				return it;
		return add_child_edit_nochecks (new_board, to_move, scored, set_active);
	}

	game_state *add_child_move_nochecks (const go_board &new_board, stone_color to_move, int x, int y, bool set_active)
	{
		stone_color next_to_move = to_move == black ? white : black;
		m_visual_ok = false;
		game_state *tmp = new game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
						  this, next_to_move, x, y, to_move);
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}

	game_state *add_child_move (const go_board &new_board, stone_color to_move, int x, int y, bool set_active = true)
	{
		for (auto &it: m_children)
			if (it->was_move_p () && it->m_board == new_board)
				return it;
		return add_child_move_nochecks (new_board, to_move, x, y, set_active);
	}
	game_state *add_child_move (int x, int y, stone_color to_move, bool set_active = true)
	{
		if (!valid_move_p (x, y, to_move))
			return nullptr;

		go_board new_board (m_board, mark::none);
		new_board.add_stone (x, y, to_move);
		/* Check for ko.  */
		if (m_parent != nullptr) {
			if (m_parent->m_board.position_equal_p (new_board))
				return nullptr;
		}
		for (auto &it: m_children)
			if (it->was_move_p () && it->m_board.position_equal_p (new_board))
				return it;

		return add_child_move_nochecks (new_board, m_to_move, x, y, set_active);
	}
	game_state *add_child_move (int x, int y)
	{
		return add_child_move (x, y, m_to_move);
	}
	game_state *add_child_pass_nochecks (const go_board &new_board, bool set_active)
	{
		m_visual_ok = false;
		game_state *tmp = new game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
						  this, m_to_move == black ? white : black);
		tmp->m_move_color = m_to_move;
		m_children.push_back (tmp);
		if (set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}
	game_state *add_child_pass (const go_board &new_board, bool set_active = true)
	{
		for (auto &it: m_children)
			if (it->m_board == new_board && it->was_pass_p ())
				return it;
		return add_child_pass_nochecks (new_board, set_active);
	}
	game_state *add_child_pass (bool set_active = true)
	{
		return add_child_pass (m_board, set_active);
	}
	void add_child_tree (game_state *other)
	{
		m_children.push_back (other);
		other->m_parent = this;
		m_visual_ok = false;
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
	game_state *next_primary_move ()
	{
		if (m_children.size () == 0)
			return nullptr;
		return m_children[0];
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
	size_t n_siblings () const
	{
		if (m_parent == nullptr)
			return 0;
		return m_parent->m_children.size () - 1;
	}
	size_t var_number () const
	{
		if (m_parent == nullptr)
			return 1;
		for (size_t i = 0; i < m_parent->m_children.size (); i++)
			if (m_parent->m_children[i] == this)
				return i + 1;
		throw std::logic_error ("not a child of its parent");
	}
	size_t n_children () const
	{
		return m_children.size ();
	}
	/* I didn't really want to expose this, but avoiding it leads to contortions
	   in some places, e.g. when trying to identify figures.  */
	const std::vector<game_state *> children () const
	{
		return m_children;
	}
	std::vector<game_state *> take_children ()
	{
		std::vector<game_state *> tmp;
		std::swap (tmp, m_children);
		m_active = 0;
		m_visual_ok = false;
		for (auto it: tmp)
			it->m_parent = nullptr;
		return tmp;
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
	const go_board child_moves (const game_state *excluding, bool exclude_figs) const;
	const go_board sibling_moves (bool exclude_figs) const
	{
		game_state *p = m_parent;
		if (p == nullptr)
			/* No need to copy special properties if we're just going to use this
			   as a bit mask.  */
			return go_board (m_board.size_x (), m_board.size_y ());
		return p->child_moves (this, exclude_figs);
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
	void set_eval_data (int visits, double winrate_black, double komi, bool force_replace)
	{
		if (m_eval_visits > visits && m_eval_komi_set && m_eval_komi == komi && !force_replace)
			return;
		m_eval_visits = visits;
		m_eval_wr_black = winrate_black;
		m_eval_komi = komi;
		m_eval_komi_set = true;
	}
	void set_eval_data (int visits, double winrate_black, bool force_replace)
	{
		if ((m_eval_visits > visits || m_eval_komi_set) && !force_replace)
			return;
		m_eval_visits = visits;
		m_eval_wr_black = winrate_black;
		m_eval_komi_set = false;
	}
	void set_eval_data (const game_state &other, bool force_replace)
	{
		if (!force_replace
		    && m_eval_visits > other.m_eval_visits
		    && ((m_eval_komi_set && other.m_eval_komi_set && m_eval_komi == other.m_eval_komi)
			|| !other.m_eval_komi_set))
			return;
		m_eval_visits = other.m_eval_visits;
		m_eval_wr_black = other.m_eval_wr_black;
		m_eval_komi = other.m_eval_komi;
		m_eval_komi_set = other.m_eval_komi_set;
	}
	int eval_visits ()
	{
		return m_eval_visits;
	}
	double eval_wr_black ()
	{
		return m_eval_wr_black;
	}
	bool eval_komi_set ()
	{
		return m_eval_komi_set;
	}
	double eval_komi ()
	{
		return m_eval_komi;
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

	bool has_figure () const
	{
		return m_figure.present;
	}
	bool has_figure_recursive () const;
	const std::string &figure_title () const
	{
		return m_figure.title;
	}
	int figure_flags () const
	{
		return m_figure.flags;
	}
	void set_figure (int flags, const std::string &title)
	{
		if (!m_figure.present)
			m_visual_ok = false;
		m_figure.present = true;
		m_figure.flags = flags;
		m_figure.title = title;
	}
	void clear_figure ()
	{
		if (m_figure.present)
			m_visual_ok = false;
		m_figure.present = false;
	}
	const bit_array *visible () const
	{
		return m_visible;
	}
	const bit_array *visible_inherited () const
	{
		const game_state *gs = this;
		while (gs != nullptr) {
			if (gs->m_visible != nullptr)
				return gs->m_visible;
			gs = gs->m_parent;
		}
		return nullptr;
	}
	/* Takes ownership of the pointer.  */
	void set_visible (bit_array *v)
	{
		delete m_visible;
		m_visible = v;
	}
	/* Return true if a change was made.  */
	bool update_visualization (bool hide_figures);
	typedef std::function<void (int, int, int, int, bool)> draw_line;
	typedef std::function<void (int, int)> add_point;
	void extract_visualization (int x, int y, visual_tree::bit_rect &stones_w,
				    visual_tree::bit_rect &stones_b,
				    visual_tree::bit_rect &edits,
				    visual_tree::bit_rect &collapsed,
				    visual_tree::bit_rect &figures,
				    visual_tree::bit_rect &hidden_figs);
	void render_visualization (int, int, int, const draw_line &, bool first);
	void render_active_trace (int, int, int, const add_point &, const draw_line &);
	bool locate_visual (int, int, const game_state *active, int &, int &);
	game_state *locate_by_vis_coords (int x, int y, int off_x, int off_y);
	const visual_tree &visualization ()
	{
		return m_visualized;
	}
	void toggle_vis_collapse ()
	{
		if (m_children.size () == 0)
			return;

		m_visual_collapse = !m_visual_collapse;
		m_visual_ok = false;
	}
	bool vis_collapsed ()
	{
		return m_visual_collapse;
	}
	void expand_all ();
	void collapse_nonactive (const game_state *until);

	/* Expand this node, but keep its children collapsed.
	   Does nothing if already expanded, and returns false iff that is
	   the case.  */
	bool vis_expand_one ();
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
	std::string m_event = "";
	std::string m_round = "";
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
	const std::string &event () const { return m_event; }
	const std::string &round () const { return m_round; }
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
	void set_event (const std::string &s) { m_event = s; }
	void set_round (const std::string &s) { m_round = s; }
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
		   const std::string &dt, const std::string &pc,
		   const std::string &ev, const std::string &ro,
		   const std::string &cp,
		   const std::string &tm, const std::string &ot, int style)
		: m_title (title), m_name_w (w), m_name_b (b), m_rank_w (rw), m_rank_b (rb),
		m_rules (ru), m_komi (komi), m_handicap (hc), m_result (re), m_date (dt), m_place (pc),
		m_event (ev), m_round (ro),
		m_ranked (rt), m_time (tm), m_overtime (ot),
	  m_copyright (cp), m_style (style)
	{
	}
};

class game_record;
extern game_state *sgf2board (sgf &);
extern std::shared_ptr<game_record> sgf2record (const sgf &, QTextCodec *codec);
extern std::string record2sgf (const game_record &);

class game_record : public game_info
{
	friend std::shared_ptr<game_record> sgf2record (const sgf &s, QTextCodec *codec);
	game_state m_root;
	bool m_modified = false;
	sgf_errors m_errors;

public:
	game_record (int size, const game_info &info)
		: game_info (info), m_root (size)
	{

	}
	game_record (const go_board &b, stone_color to_move, const game_info &info)
		: game_info (info), m_root (b, to_move)
	{

	}
	game_record (const game_record &other) : game_info (other), m_root (other.m_root, nullptr),
		m_modified (other.m_modified), m_errors (other.m_errors)
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
		const go_board &b = m_root.get_board ();
		return std::max (b.size_x (), b.size_y ());
	}
	void set_modified (bool on = true)
	{
		m_modified = on;
	}
	bool modified () const
	{
		return m_modified;
	}
	std::string to_sgf () const;
	void set_errors (const sgf_errors &errs)
	{
		m_errors = errs;
	}
	void clear_errors ()
	{
		m_errors = sgf_errors ();
	}
	const sgf_errors &errors ()
	{
		return m_errors;
	}
	~game_record ()
	{
	}
};

class navigable_observer : public game_state::observer
{
protected:
	std::shared_ptr<game_record> m_game = nullptr;

public:
	void next_move ();
	void previous_move ();
	void next_figure ();
	void previous_figure ();
	void next_variation ();
	void previous_variation ();
	void next_comment ();
	void previous_comment ();
	void goto_first_move ();
	void goto_last_move ();
	void goto_main_branch ();
	void goto_var_start ();
	void goto_next_branch ();
	void goto_nth_move (int n);
	void goto_nth_move_in_var (int n);
	void find_move (int x, int y);
};

#endif
