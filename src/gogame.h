#ifndef GOGAME_H
#define GOGAME_H

#include "goboard.h"
#include "goeval.h"

#include <functional>
#include <QString>
#include <QTextCodec>

inline std::string komi_str (double k)
{
	QString s = QString::number (k);
	if (s.contains ('.')) {
		while (s.endsWith ("0"))
			s.chop (1);
	}
	return s.toStdString ();
}

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

class game_state;

/* A memory allocator for game_state structures.  One of these is associated with every
   game_record, to make sure all game_states are deleted when the game is destroyed.  */
class game_state_manager
{
	const size_t m_n_per_chunk = 128;
	std::vector<char *> m_game_states;
	bit_array m_free = bit_array (m_n_per_chunk, false);
	int m_first_free = 0;

public:
	~game_state_manager ();

	template<typename ... ARGS> game_state *create_game_state (ARGS &&... args);
	void release_game_state (game_state *st);
	void release_state_children (game_state *st);
};

class game_state
{
private:
	game_state_manager *m_manager;
	int m_id;

	go_board m_board;
	/* The move number within this game tree.  Unaffected by SGF MN properties.  */
	int m_move_number;
	/* Move number as specified by SGF MN, or as above.  */
	int m_sgf_movenum;

	std::vector<game_state *> m_children;
	size_t m_active = 0;
	game_state *m_parent;
	stone_color m_to_move;

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

	std::vector<eval> m_evals;
	eval m_live_eval;

	/* Support for SGF VW.  */
	bit_array *m_visible {};

	/* Memory management for game_state is handled by game_state_manager.  These should never be
	   directly deleted.  */
	void operator delete (void *) { std::terminate (); }

	friend class game_state_manager;
	game_state (game_state_manager *gm, int id, const go_board &b, int move, int sgf_move, game_state *parent, stone_color to_move)
		: m_manager (gm), m_id (id), m_board (b), m_move_number (move), m_sgf_movenum (sgf_move), m_parent (parent), m_to_move (to_move)
	{
	}

	game_state (game_state_manager *gm, int id, const go_board &b, int move, int sgf_move, game_state *parent, stone_color to_move, int x, int y, stone_color move_col)
		: m_manager (gm), m_id (id), m_board (b), m_move_number (move), m_sgf_movenum (sgf_move), m_parent (parent), m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col)
	{
	}

	/* This isn't used except for delegation when doing a deep copy.  Uncertain how
	   much we should copy here vs there.  */
	game_state (game_state_manager *gm, int id, const go_board &b, int move, int sgf_move, game_state *parent,
		    stone_color to_move, int x, int y, stone_color move_col,
		    const sgf::node::proplist &unrecognized, const visual_tree &vt, bool vtok,
		    bit_array *visible)
		: m_manager (gm), m_id (id), m_board (b), m_move_number (move), m_sgf_movenum (sgf_move),
		m_parent (parent), m_to_move (to_move), m_move_x (x), m_move_y (y), m_move_color (move_col),
		m_unrecognized_props (unrecognized), m_visualized (vt), m_visual_ok (vtok),
		m_visible (visible == nullptr ? nullptr : new bit_array (*visible))
	{
	}

public:
	game_state (game_state_manager *gm, int id, int size)
		: m_manager (gm), m_id (id), m_board (size), m_move_number (0), m_sgf_movenum (0), m_parent (0), m_to_move (black)
	{
	}
	game_state (game_state_manager *gm, int id, const go_board &b, stone_color to_move)
		: m_manager (gm), m_id (id), m_board (b), m_move_number (0), m_sgf_movenum (0), m_parent (nullptr), m_to_move (to_move)
	{
	}
	/* Deep copy.  */
	game_state (game_state_manager *gm, int id, const game_state &other, game_state *parent)
		: game_state (gm, id, other.m_board, other.m_move_number, other.m_sgf_movenum, parent, other.m_to_move,
			      other.m_move_x, other.m_move_y, other.m_move_color, other.m_unrecognized_props,
			      other.m_visualized, other.m_visual_ok, other.m_visible)
	{
		for (auto c: other.m_children) {
			game_state *new_c = m_manager->create_game_state (*c, this);
			m_children.push_back (new_c);
		}
		m_comment = other.m_comment;
		m_active = other.m_active;
		m_figure = other.m_figure;
		m_print_numbering = other.m_print_numbering;
		m_evals = other.m_evals;

		m_timeleft_w = other.m_timeleft_w;
		m_timeleft_b = other.m_timeleft_b;
		m_stonesleft_w = other.m_stonesleft_w;
		m_stonesleft_b = other.m_stonesleft_b;
	}
	void operator delete (void *, void *) throw ()
	{
	}
	/* Returns the former position of the state in its parent's child vector.  */
	size_t disconnect ()
	{
		game_state *parent = m_parent;

		if (parent == nullptr)
			return 0;

		size_t n = parent->m_children.size ();
		size_t i;
		for (i = 0; i < n; i++)
			if (parent->m_children[i] == this)
				break;
#if 0
		if (i == n)
			throw std::logic_error ("delete node not found among parent's children");
#endif
		parent->m_children.erase (parent->m_children.begin () + i);
		if (i <= parent->m_active && parent->m_active > 0)
			parent->m_active--;

		parent->m_visual_ok = false;
		if (parent->m_children.size () == 0)
			parent->m_visual_collapse = false;
		m_parent = nullptr;
		return i;
	}
	~game_state ()
	{
		delete m_visible;
	}
	game_state *duplicate (game_state *parent)
	{
		return m_manager->create_game_state (*this, parent);
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

	/* Determines whether to make a newly added move active or not when adding it.  */
	enum class add_mode { set_active, keep_active };

private:
	game_state *insert_child (game_state *tmp, add_mode am)
	{
		m_visual_ok = false;
		m_children.push_back (tmp);
		if (am == add_mode::set_active)
			m_active = m_children.size() - 1;
		return tmp;
	}

public:
	void add_child_tree_at (game_state *c, size_t idx)
	{
		m_visual_ok = false;
		c->disconnect ();
		m_children.insert (std::begin (m_children) + idx, c);
		c->m_parent = this;
	}
	size_t find_child_idx (game_state *c)
	{
		size_t n = m_children.size ();
		for (size_t i = 0; i < n; i++)
			if (m_children[i] == c)
				return i;
		return n;
	}
	void make_child_primary (const game_state *c);
	game_state *add_child_edit_nochecks (const go_board &new_board, stone_color to_move, bool scored, add_mode am)
	{
		m_visual_ok = false;
		int code = scored ? -3 : -2;
		game_state *tmp = m_manager->create_game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
								this, to_move, code, code, none);
		return insert_child (tmp, am);
	}
	game_state *replace_child_edit (game_state *child, const go_board &new_board, stone_color to_move)
	{
		for (size_t i = 0; i < m_children.size (); i++)
			if (m_children[i] == child) {
				game_state *tmp = m_manager->create_game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
										this, to_move, -2, -2, none);
				m_children[i] = tmp;
				child->m_parent = tmp;
				tmp->insert_child (child, add_mode::set_active);
				m_visual_ok = false;
				return tmp;
			}
		return nullptr;
	}

	game_state *add_child_edit (const go_board &new_board, stone_color to_move, bool scored = false, add_mode am = add_mode::set_active)
	{
		for (const auto &it: m_children)
			if (it->m_board == new_board && it->m_to_move == to_move)
				return it;
		return add_child_edit_nochecks (new_board, to_move, scored, am);
	}

	game_state *add_child_move_nochecks (const go_board &new_board, stone_color to_move, int x, int y, add_mode am)
	{
		stone_color next_to_move = to_move == black ? white : black;
		m_visual_ok = false;
		game_state *tmp = m_manager->create_game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
								this, next_to_move, x, y, to_move);
		return insert_child (tmp, am);
	}

	game_state *add_child_move (const go_board &new_board, stone_color to_move, int x, int y, add_mode am = add_mode::set_active)
	{
		for (const auto &it: m_children)
			if (it->was_move_p () && it->m_board == new_board)
				return it;
		return add_child_move_nochecks (new_board, to_move, x, y, am);
	}

	game_state *add_child_move (int x, int y, stone_color to_move, add_mode am = add_mode::set_active, bool dup = false)
	{
		if (!valid_move_p (x, y, to_move))
			return nullptr;

		go_board new_board (m_board, mark::none);
		new_board.add_stone (x, y, to_move);
		if (!dup) {
			for (const auto &it: m_children)
				if (it->was_move_p () && it->m_board.position_equal_p (new_board))
					return it;
		}

		return add_child_move_nochecks (new_board, m_to_move, x, y, am);
	}
	game_state *add_child_move (int x, int y)
	{
		return add_child_move (x, y, m_to_move);
	}

	game_state *add_child_pass_nochecks (const go_board &new_board, add_mode am)
	{
		m_visual_ok = false;
		game_state *tmp = m_manager->create_game_state (new_board, m_move_number + 1, m_sgf_movenum + 1,
								this, m_to_move == black ? white : black);
		tmp->m_move_color = m_to_move;
		return insert_child (tmp, am);
	}
	game_state *add_child_pass (const go_board &new_board, add_mode am = add_mode::set_active)
	{
		for (const auto &it: m_children)
			if (it->m_board == new_board && it->was_pass_p ())
				return it;
		return add_child_pass_nochecks (new_board, am);
	}
	game_state *add_child_pass (add_mode am = add_mode::set_active)
	{
		return add_child_pass (m_board, am);
	}
	void add_child_tree (game_state *other)
	{
		m_children.push_back (other);
		auto callback = [] (game_state *st) -> bool {
			st->m_move_number = st->m_parent->m_move_number + 1; return true;
		};
		other->m_parent = this;
		other->walk_tree (callback);
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
	const game_state *prev_move () const
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
		for (const auto &it: m_children)
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
	std::vector<int> path_from_root ();
	game_state *follow_path (const std::vector<int> &);

	/* Set a mark on the current board, and return true if that made a change.  */
	bool set_mark (int x, int y, mark m, mextra extra)
	{
		return m_board.set_mark (x, y, m, extra);
	}
	void set_text_mark (int x, int y, const std::string &str)
	{
		m_board.set_text_mark (x, y, str);
	}
	void clear_marks ()
	{
		m_board.clear_marks ();
	}
	void set_comment (const std::string &c)
	{
		m_comment = c;
	}
	const std::string &comment () const
	{
		return m_comment;
	}
	void update_eval (const eval &);
	void update_eval (const game_state &other);
	eval best_eval ();
	eval eval_from (const analyzer_id &id, bool require);
	size_t eval_count () const
	{
		return m_evals.size ();
	}
	void remove_eval (const analyzer_id &);
	void collect_analyzers (std::function<void (const analyzer_id &, bool)> &callback)
	{
		for (const auto &it: m_evals)
			callback (it.id, it.score_stddev != 0);
	}
	void set_eval_data (int visits, double winrate_black, analyzer_id id)
	{
		eval ev;
		ev.visits = visits;
		ev.wr_black = winrate_black;
		ev.id = id;
		update_eval (ev);
	}
	void set_eval_data (int visits, double winrate_black, double scorem, double scored, analyzer_id id)
	{
		eval ev;
		ev.visits = visits;
		ev.wr_black = winrate_black;
		ev.id = id;
		ev.score_mean = scorem;
		ev.score_stddev = scored;
		update_eval (ev);
	}
	bool find_eval (const analyzer_id &id, eval &ev)
	{
		for (const auto &e: m_evals)
			if (e.id == id) {
				ev = e;
				return true;
			}
		return false;
	}
	void append_to_sgf (std::string &, bool active_only) const;

	/* Used for edits: modifying an existing edit (or root) node, or applying marks.  */
	void replace (const go_board &b, stone_color to_move)
	{
		m_board = b;
		m_to_move = to_move;
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
	typedef std::function<bool (int, int, int, game_state *)> start_run;
	void render_visualization (int, int, int, const draw_line &, bool first);
	void render_visualization (int, int, const start_run &);
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
	bool needs_visual_update ()
	{
		return !m_visual_ok;
	}
	bool has_hidden_diagrams ();
	void expand_all ();
	void collapse_nonactive (const game_state *until);

	/* Expand this node, but keep its children collapsed.
	   Does nothing if already expanded, and returns false iff that is
	   the case.  */
	bool vis_expand_one ();

	void walk_tree (const std::function<bool (game_state *)> &);
};

template<typename ... ARGS>
game_state *game_state_manager::create_game_state (ARGS &&... args)
{
	size_t n_elts = m_game_states.size () * m_n_per_chunk;
	unsigned free_elt = m_free.ffz (m_first_free);
	m_first_free = free_elt + 1;
	char *arena;
	// printf ("free elt: %u (max %u), within %u\n", free_elt, (unsigned)n_elts, free_elt % m_n_per_chunk);
	if (free_elt == n_elts) {
		arena = new char[sizeof (game_state) * m_n_per_chunk];
		m_game_states.push_back (arena);
		m_free.grow (n_elts + m_n_per_chunk);
		// printf ("new arena %p\n", arena);
	} else {
		arena = m_game_states[free_elt / m_n_per_chunk];
		// printf ("old arena %p\n", arena);
	}
	m_free.set_bit (free_elt);
	char *ptr = arena + (free_elt % m_n_per_chunk) * sizeof (game_state);
	game_state *gs = new (ptr) game_state (this, free_elt, std::forward<ARGS>(args)...);
	return gs;
}

class sgf;
class game_record;

enum class ranked { unknown, free, ranked, teaching };

enum class go_rules { unknown, old_chinese, chinese, japanese, korean, aga, bga, kgs_chinese, ogs_chinese, ing, tt, nz };

struct game_info
{
	std::string title;
	std::string name_w, name_b;
	std::string rank_w, rank_b;

	std::string rules;
	double komi = 0;
	int handicap = 0;

	std::string result;

	std::string date;
	std::string place;
	std::string event;
	std::string round;
	ranked rated = ranked::unknown;

	std::string time;
	std::string overtime;

	std::string copyright;

	/* SGF variation display style, or -1 if unset by SGF file.  */
	int style = -1;
};

class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
extern game_info info_from_sgfroot (const sgf &, QTextCodec *, sgf_errors &);
extern std::pair<int, int> sizes_from_sgfroot (const sgf &);
extern void db_info_from_sgf (go_board &b, sgf::node *n, bool is_root, sgf_errors &errs,
			      bit_array &final_w, bit_array &final_b, bit_array &final_c,
			      std::vector<unsigned char> &movelist);

extern game_state *sgf2board (sgf &);
extern go_game_ptr sgf2record (const sgf &, QTextCodec *codec);
extern std::string record2sgf (const game_record &);

class game_record : public game_state_manager
{
	friend go_game_ptr sgf2record (const sgf &s, QTextCodec *codec);
	game_info m_info;
	game_state *m_root {};

	std::string m_filename;
	bool m_modified = false;
	sgf_errors m_errors;

	/* For Go variant support, this is the mask of intersections that are not actually
	   on the board.  */
	std::shared_ptr<const bit_array> m_mask {};

public:
	game_record (int size, const game_info &info)
		: m_info (info)
	{
		m_root = create_game_state (size);
	}
	game_record (const go_board &b, stone_color to_move, const game_info &info)
		: m_info (info)
	{
		m_root = create_game_state (b, to_move);
	}
	game_record (const go_board &b, stone_color to_move, const game_info &info, const std::shared_ptr<const bit_array> &m)
		: m_info (info), m_mask (m)
	{
		m_root = create_game_state (b, to_move);
	}
	game_record (const game_record &other) : m_info (other.m_info),
		m_modified (other.m_modified), m_errors (other.m_errors), m_mask (other.m_mask)
	{
		m_root = create_game_state (*other.m_root, nullptr);
	}

	const game_info &info () { return m_info; }
	void set_info (game_info i) { m_info = std::move (i); }
	/* A few convenience functions for when we need to update only a part of the info.  */
	void set_result (const std::string s) { m_info.result = std::move (s); }
	void set_title (const std::string s) { m_info.title = std::move (s); }
	const std::string &filename () { return m_filename; }
	void set_filename (const std::string s) { m_filename = std::move (s); }
	void set_handicap (int hc) { m_info.handicap = hc; }
	void set_ranked_type (ranked r) { m_info.rated = r; }

	game_state *get_root () { return m_root; }
	bool replace_root (const go_board &b, stone_color to_move)
	{
		if (m_root->n_children () > 0)
			return false;
		m_root->replace (b, to_move);
		return true;
	}

	int boardsize () const
	{
		const go_board &b = m_root->get_board ();
		return std::max (b.size_x (), b.size_y ());
	}
	bool same_size (const game_record &other)
	{
		const go_board &b1 = m_root->get_board ();
		const go_board &b2 = other.m_root->get_board ();
		return b1.size_x () == b2.size_x () && b1.size_y () == b2.size_y ();
	}

	std::shared_ptr<const bit_array> get_board_mask ()
	{
		return m_mask;
	}
	void set_modified (bool on = true)
	{
		m_modified = on;
	}
	bool modified () const
	{
		return m_modified;
	}
	std::string to_sgf (bool active_only = false) const;
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
};

#endif
