#include <list>
#include <string>
#include <sstream>
#include <functional>

/* Ideally this code would be independent of Qt, but plain C++ seems to have little
   support for converting text encodings.  */
#include <QTextCodec>

#include "config.h"
#include "sgf.h"
#include "goboard.h"
#include "gogame.h"

static int coord_from_letter (char x)
{
	/* None of the other sgf readers seem to agree on how to handle boards
	   larger than 26x26.  Be conservative.  */
	if (! isalpha (x) || ! islower (x))
		throw broken_sgf ();
	x -= 'a';
	return x;
}

static void put_stones (sgf::node::property *p, int maxx, int maxy, std::function<void (int, int)> func)
{
	for (auto &v: p->values)
	{
#if 0
		/* Empty value means pass.  @@@ but this is used for AB, AW and AE only.  */
		if (v.length () == 0)
			continue;
#endif
		int x1 = coord_from_letter (v[0]);
		int y1 = coord_from_letter (v[1]);
		int x2, y2;
		if (v[2] == ':') {
			x2 = coord_from_letter (v[3]);
			y2 = coord_from_letter (v[4]);

			if (x1 > x2) {
				int t = x1;
				x1 = x2;
				x2 = t;
			}
			if (y1 > y2) {
				int t = y1;
				y1 = y2;
				y2 = t;
			}
		} else {
			x2 = x1, y2 = y1;
		}
		if (x2 >= maxx || y2 >= maxy)
			throw broken_sgf ();

		for (int x = x1; x <= x2; x++)
			for (int y = y1; y <= y2; y++)
				func (x, y);
	}
}

static bool recognized_mark (const std::string &id)
{
	return id == "MA" || id == "TR" || id == "SQ" || id == "CR" || id == "LB" || id == "TW" || id == "TB";
}

/* Look for mark properties in node N and add them to the go board.
   Return true iff we found territory markers.  */
static bool add_marks (go_board &b, sgf::node *n)
{
	bool terr = false;
	for (auto &p : n->props) {
		const std::string &id = p->ident;
		if (recognized_mark (id)) {
			p->handled = true;
			for (auto &v: p->values) {
				if (v.length () < 2)
					continue;
				int x1 = coord_from_letter (v[0]);
				int y1 = coord_from_letter (v[1]);
				mark mt = (id == "MA" ? mark::cross
					   : id == "TR" ? mark::triangle
					   : id == "SQ" ? mark::square
					   : id == "CR" ? mark::circle
					   : id == "TB" || id == "TW" ? mark::terr
					   : mark::text);
				if (mt == mark::terr)
					terr = true;
				mextra extra = id == "TB" ? 1 : 0;
				if (mt == mark::text) {
					if (v.length () < 4 || v[2] != ':')
						continue;
					std::string t = v.substr (3);
					int num = -1;
					if (!t.empty () && t.find_first_not_of ("0123456789") == std::string::npos)
						num = stoi (t);
					if (num >= 0 && num < 256 && std::to_string (num) == t) {
						mt = mark::num;
						extra = num;
					} else if (t.length () == 1 && isalpha (t[0])) {
						mt = mark::letter;
						if (isupper (t[0]))
							extra = t[0] - 'A';
						else
							extra = t[0] - 'a' + 26;
					} else {
						b.set_text_mark (x1, y1, t);
						mt = mark::none;
					}
				}
				if (mt != mark::none)
					b.set_mark (x1, y1, mt, extra);
			}
		}
	}
	return terr;
}

/* Return true if OK, false if there was a charset conversion error.  */
static bool add_comment (game_state *gs, sgf::node *n, QTextCodec *codec)
{
	sgf::node::property *comment = n->find_property ("C");
	if (comment) {
		std::string cs;
		for (auto c: comment->values) {
			cs += c;
		}
		if (codec != nullptr) {
			const char *bytes = cs.c_str ();
			QTextCodec::ConverterState state;
			QString tmp = codec->toUnicode (bytes, strlen (bytes), &state);
			if (state.invalidChars > 0)
				return false;
			cs = tmp.toStdString ();
		}
		gs->set_comment (cs);
	}
	return true;
}

/* Return true if OK, false if there was a charset conversion error.  */
static bool add_figure (game_state *gs, sgf::node *n, QTextCodec *codec)
{
	sgf::node::property *figure = n->find_property ("FG");
	if (figure == nullptr)
		return true;
	if (figure->values.size () != 1)
		throw broken_sgf ();

	std::string v = *figure->values.begin ();
	if (v.empty ()) {
		/* It seems unspecified what should be used as the flags value if the
		   FG property has no value.  */
		gs->set_figure (32768, "");
		return true;
	}
	size_t sep = v.find (':');
	int flags;
	bool retval = true;
	if (sep != std::string::npos) {
		flags = stoi (v.substr (0, sep));
		v = v.substr (sep + 1);
		if (codec != nullptr) {
			const char *bytes = v.c_str ();
			QTextCodec::ConverterState state;
			QString tmp = codec->toUnicode (bytes, strlen (bytes), &state);
			if (state.invalidChars > 0) {
				retval = false;
				v = "";
			} else
				v = tmp.toStdString ();
		}
		gs->set_figure (flags, v);
	} else {
		flags = stoi (v);
		gs->set_figure (flags, "");
	}

	return retval;
}

static void add_visible (game_state *gs, sgf::node *n)
{
	sgf::node::property *vw = n->find_property ("VW");
	if (vw == nullptr)
		return;
	if (vw->values.size () == 1) {
		std::string v = *vw->values.begin ();
		if (v.length () == 0) {
			gs->set_visible (nullptr);
			return;
		}
	}
	const go_board &b = gs->get_board ();
	bit_array *a = new bit_array (b.bitsize ());
	put_stones (vw, b.size_x (), b.size_y (), [=] (int x, int y) { a->set_bit (b.bitpos (x, y)); });
	gs->set_visible (a);
}

static bool add_eval (game_state *gs, sgf::node *n)
{
	sgf::node::property *qlzv = n->find_property ("QLZV");
	if (qlzv == nullptr)
		return true;
	bool retval = true;
	for (auto &v: qlzv->values) {
		std::stringstream ss (v);
		std::vector<std::string> vals;
		for (;;) {
			std::string elt;
			if (!getline (ss, elt, ':'))
				break;
			vals.push_back (elt);
		}
		size_t sz = vals.size ();
		if (sz < 2) {
			retval = false;
			continue;
		}
		int visits;
		double winrate;
		double komi = 0;
		try {
			visits = stoi (vals[0]);
			winrate = stod (vals[1]);
			if (sz >= 3)
				komi = stod (vals[2]);
		} catch (...) {
			retval = false;
			continue;
		}
		if (sz == 2) {
			gs->set_eval_data (visits, winrate, false);
		} else {
			/* Ignore everything beyond value 3.  Those will hold things
			   like engine name and version in the future.  */
			gs->set_eval_data (visits, winrate, komi, false);
		}
	}
	return retval;
}

static void add_to_game_state (game_state *gs, sgf::node *n, bool force, QTextCodec *codec, sgf_errors &errs)
{
	while (n) {
		if (!force) {
			while (n->m_siblings != nullptr)
			{
				add_to_game_state (gs, n, true, codec, errs);
				n = n->m_siblings;
			}
		}
		force = false;
		enum class im { unknown, yes, no } is_move = im::unknown;
		go_board new_board (gs->get_board (), mark::none);
		stone_color to_move = gs->to_move ();
		int move_x = -1, move_y = -1;
		bool is_pass = false;
		sgf::node::proplist unrecognized;
		for (auto &p : n->props) {
			const std::string &id = p->ident;
			if (id == "AB" || id == "B" || id == "AW" || id == "W" || id == "AE") {
#if 0
				for (int i = 0; i < gs->move_number (); i++)
					std::cerr << " ";
				std::cerr << id;
				for (auto &v: p->values)
					std::cerr << " : " << v;
				std::cerr << std::endl;
#endif
				p->handled = true;
				im thisprop_move = id.length () == 1 ? im::yes : im::no;
				if (is_move == im::unknown)
					is_move = thisprop_move;
				else if (is_move == im::yes || is_move != thisprop_move)
					/* Only one move per node allowed per the spec,
					   and all types must match.  */
					throw broken_sgf ();

				stone_color sc = id == "AB" || id == "B" ? black : id == "AW" || id == "W" ? white : none;
				if (is_move == im::yes)
				{
					if (p->values.size () != 1)
						throw broken_sgf ();

					const std::string &v = *p->values.begin ();

					if (v.length () == 0) {
						is_pass = true;
						continue;
					}
					if (v.length () != 2)
						throw broken_sgf ();

					move_x = coord_from_letter (v[0]);
					move_y = coord_from_letter (v[1]);
					if (new_board.stone_at (move_x, move_y) != none) {
						/* We'd like to throw, but Kogo's Joseki Dictionary has
						   such errors.  */
						errs.played_on_stone = true;
						return;
					}
					new_board.add_stone (move_x, move_y, sc);
					to_move = sc;
				} else {
					put_stones (p, new_board.size_x (), new_board.size_y (),
						    [&] (int x, int y) { new_board.set_stone (x, y, sc); });
				}
			}
		}
		bool terr = add_marks (new_board, n);

		if (is_move == im::no || (is_move == im::unknown && terr)) {
			/* @@@ fix up to_move.  */
			new_board.identify_units ();
			if (terr)
				new_board.territory_from_markers ();
			gs = gs->add_child_edit_nochecks (new_board, to_move, terr, false);
		} else if (is_pass) {
			gs = gs->add_child_pass_nochecks (new_board, false);
		} else
			gs = gs->add_child_move_nochecks (new_board, to_move, move_x, move_y, false);

		const std::string *pm = n->find_property_val ("PM");
		if (pm) {
			if (pm->length () != 1 || (*pm)[0] < '0' || (*pm)[0] > '2')
				errs.invalid_val = true;
			else
				gs->set_print_numbering ((*pm)[0] - '0');
		}
		const std::string *mn = n->find_property_val ("MN");
		if (mn) {
			try {
				int n = stoi (*mn);
				gs->set_sgf_move_number (n);
			} catch (...) {
				errs.invalid_val = true;
			}
		}
		const std::string *wl = n->find_property_val ("WL");
		const std::string *bl = n->find_property_val ("BL");
		const std::string *ow = n->find_property_val ("OW");
		const std::string *ob = n->find_property_val ("OB");
		if (wl)
			gs->set_time_left (white, *wl);
		if (bl)
			gs->set_time_left (black, *bl);
		if (ow)
			gs->set_stones_left (white, *ow);
		if (ob)
			gs->set_stones_left (black, *ob);
		errs.charset_error |= !add_comment (gs, n, codec);
		errs.charset_error |= !add_figure (gs, n, codec);
		errs.malformed_eval |= !add_eval (gs, n);
		add_visible (gs, n);
		for (auto p: n->props) {
			if (!p->handled)
				unrecognized.push_back (new sgf::node::property (*p));
		}
		gs->set_unrecognized (unrecognized);
		n = n->m_children;
	}
#ifdef TEST_SGF
	printf ("End of variation reached:\n");
	const go_board &b = gs->get_board ();
	b.dump_ascii ();
#endif
}

std::string translated_prop_str (const std::string *val, const QTextCodec *codec)
{
	if (!val)
		return "";
	if (codec == nullptr)
		return *val;

	const char *bytes = val->c_str ();
	QTextCodec::ConverterState state;
	QString tmp = codec->toUnicode (bytes, strlen (bytes), &state);
	if (state.invalidChars > 0)
		throw broken_sgf ();
	return std::string (tmp.toStdString ());
}

std::shared_ptr<game_record> sgf2record (const sgf &s, QTextCodec *codec)
{
	sgf_errors errs = s.errs;

	const std::string *ff = s.nodes->find_property_val ("FF");
	const std::string *gm = s.nodes->find_property_val ("GM");
	bool our_extensions = false;
	if (ff != nullptr && *ff != "4" && *ff != "3")
		throw broken_sgf ();
	if (gm != nullptr) {
		if (*gm == "q5go-1")
			our_extensions = true;
		else if (*gm != "1") {
			throw broken_sgf ();
		}
	}
	if (codec == nullptr) {
		const std::string *ca = s.nodes->find_property_val ("CA");
		if (ca != nullptr)
			codec = QTextCodec::codecForName (ca->c_str ());
		else
			codec = QTextCodec::codecForName ("ISO-8859-1"); // default encoding of SGF by specification
	}
	const std::string *p = s.nodes->find_property_val ("SZ");
	int size_x = -1;
	int size_y = 19;
	/* GoGui writes files without SZ. Assume 19, I guess.  */
	if (p != nullptr) {
		const std::string &v = *p;
		size_y = 0;
		for (size_t i = 0; i < v.length (); i++) {
			if (v[i] == ':') {
				if (size_x != -1)
					throw broken_sgf ();
				size_x = size_y;
				size_y = 0;
			} else if (! isdigit (v[i]))
				throw broken_sgf ();
			else
				size_y = size_y * 10 + v[i] - '0';
		}
	}
	if (size_x == -1)
		size_x = size_y;
	if (size_x < 3 || size_x > 26 || size_y < 3 || size_y > 26)
		throw invalid_boardsize ();

	bool torus_h = false, torus_v = false;
	const std::string *to = our_extensions ? s.nodes->find_property_val ("TO") : nullptr;
	if (to != nullptr) {
		if (to->length () != 1 || !isdigit ((*to)[0]))
			throw broken_sgf ();
		int torus = (*to)[0] - '0';
		if (torus & 1)
			torus_h = true;
		if (torus & 2)
			torus_v = true;

	}
	const std::string *gn = s.nodes->find_property_val ("GN");

	const std::string *pw = s.nodes->find_property_val ("PW");
	const std::string *wr = s.nodes->find_property_val ("WR");
	const std::string *pb = s.nodes->find_property_val ("PB");
	const std::string *br = s.nodes->find_property_val ("BR");

	const std::string *km = s.nodes->find_property_val ("KM");
	const std::string *ru = s.nodes->find_property_val ("RU");
	const std::string *ha = s.nodes->find_property_val ("HA");
	const std::string *tm = s.nodes->find_property_val ("TM");
	const std::string *ot = s.nodes->find_property_val ("OT");
	const std::string *re = s.nodes->find_property_val ("RE");

	const std::string *dt = s.nodes->find_property_val ("DT");
	const std::string *pc = s.nodes->find_property_val ("PC");
	const std::string *ev = s.nodes->find_property_val ("EV");
	const std::string *ro = s.nodes->find_property_val ("RO");

	const std::string *cp = s.nodes->find_property_val ("CP");

	const std::string *st = s.nodes->find_property_val ("ST");

	/* Ignored, but ensure it doesn't go on the list of unrecognized properties to
	   write out later.  */
	s.nodes->find_property_val ("AP");
	if (km && km->length () == 0) {
		errs.empty_komi = true;
		km = nullptr;
	}
	if (km && km->length () > 0) {
		size_t pos = 0;
		if ((*km)[0] == '-')
			pos++;
		if (km->find_first_not_of ("0123456789.", pos) != std::string::npos)
			throw broken_sgf ();
	}
	double komi = km ? stod (*km) : 0;
	if (ha && ha->length () == 0) {
		errs.empty_handicap = true;
		ha = nullptr;
	}
	if (ha && (ha->find_first_not_of ("0123456789") != std::string::npos))
		throw broken_sgf ();
	int hc = ha ? stoi (*ha) : 0;
	if (st && (st->find_first_not_of ("0123456789") != std::string::npos))
		throw broken_sgf ();
	int style = st ? stoi (*st) : -1;
	game_info info (translated_prop_str (gn, codec),
			translated_prop_str (pw, codec), translated_prop_str (pb, codec),
			translated_prop_str (wr, codec), translated_prop_str (br, codec),
			translated_prop_str (ru, codec), komi, hc, ranked::free,
			translated_prop_str (re, codec),
			translated_prop_str (dt, codec), translated_prop_str (pc, codec),
			translated_prop_str (ev, codec), translated_prop_str (ro, codec),
			translated_prop_str (cp, codec),
			translated_prop_str (tm, codec), translated_prop_str (ot, codec),
			style);
	go_board initpos (size_x, size_y, torus_h, torus_v);
	for (auto n: s.nodes->props)
		if (n->ident == "AB") {
			n->handled = true;
			put_stones (n, size_x, size_y, [&] (int x, int y) { initpos.set_stone (x, y, black); });
		}
	for (auto n: s.nodes->props)
		if (n->ident == "AW") {
			n->handled = true;
			put_stones (n, size_x, size_y, [&] (int x, int y) { initpos.set_stone (x, y, white); });
		}
	initpos.identify_units ();
	add_marks (initpos, s.nodes);

	const std::string *pl = s.nodes->find_property_val ("PL");
	stone_color to_play = pl && *pl == "W" ? white : black;
	std::shared_ptr<game_record> game = std::make_shared<game_record> (initpos, to_play, info);

	errs.charset_error |= !add_comment (&game->m_root, s.nodes, codec);
	errs.charset_error |= !add_figure (&game->m_root, s.nodes, codec);
	errs.malformed_eval |= !add_eval (&game->m_root, s.nodes);
	add_visible (&game->m_root, s.nodes);

	sgf::node::proplist unrecognized;
	for (auto p: s.nodes->props) {
		if (!p->handled)
			unrecognized.push_back (new sgf::node::property (*p));
	}
	game->m_root.set_unrecognized (unrecognized);

	add_to_game_state (&game->m_root, s.nodes->m_children, false, codec, errs);
	game->set_errors (errs);
	if (errs.any_set ())
		game->set_modified ();

	/* Fix up situations where we have a handicap game without a PL property.
	   If it really looks like white to move, fix up the root node.  */
	if (pl == nullptr && hc > 1
	    && game->m_root.n_children () == 1
	    && game->m_root.next_move ()->was_move_p ()
	    && game->m_root.next_move ()->get_move_color () == white)
		game->m_root.set_to_move (white);

	return game;
}

void go_board::append_mark_plane_sgf (std::string &s, const std::string &p, const bit_array &t) const
{
	if (t.popcnt () == 0)
		return;

	s += p;
	for (int x = 0; x < m_sz_x; x++)
		for (int y = 0; y < m_sz_y; y++) {
			if (!t.test_bit (bitpos (x, y)))
				continue;
			s += '[';
			s += 'a' + x;
			s += 'a' + y;
			s += ']';
		}
}

void go_board::append_marks_sgf (std::string &s) const
{
	if (m_marks.size () == 0)
		return;

	bit_array tr = collect_marks (mark::triangle);
	append_mark_plane_sgf (s, "TR", tr);
	bit_array sq = collect_marks (mark::square);
	append_mark_plane_sgf (s, "SQ", sq);
	bit_array cr = collect_marks (mark::circle);
	append_mark_plane_sgf (s, "CR", cr);
	bit_array ma = collect_marks (mark::cross);
	append_mark_plane_sgf (s, "MA", ma);
	bit_array tw = collect_marks (mark::terr, 0);
	append_mark_plane_sgf (s, "TW", tw);
	bit_array tb = collect_marks (mark::terr, 1);
	append_mark_plane_sgf (s, "TB", tb);

	bool have_lb  = false;
	for (int x = 0; x < m_sz_x; x++)
		for (int y = 0; y < m_sz_y; y++) {
			mark m = mark_at (x, y);
			mextra me = mark_extra_at (x, y);
			if (m != mark::letter && m != mark::num && m != mark::text)
				continue;
			if (!have_lb)
				s += "LB";
			have_lb = true;
			s += "[";
			s += 'a' + x;
			s += 'a' + y;
			s += ':';
			if (m == mark::letter) {
				s += me >= 26 ? 'a' + me - 26 : 'A' + me;
			} else if (m == mark::num) {
				s += std::to_string (me);
			} else {
				s += m_mark_text[me];
			}
			s += ']';
		}
}

static void maybe_add_property (std::string &s, const go_board &b, const char *name, const bit_array &arr, int *linecount)
{
	if (arr.popcnt () == 0)
		return;
	s += name;
	int szx = b.size_x ();
	int szy = b.size_y ();
	for (int x = 0; x < szx; x++)
		for (int y = 0; y < szy; y++) {
			if (arr.test_bit (b.bitpos (x, y))) {
				s += '[';
				s += 'a' + x;
				s += 'a' + y;
				s += ']';
				(*linecount)++;
			}
		}
}

static void encode_string (std::string &s, const char *id, std::string src, bool force = false)
{
	if (force || src.length () > 0) {
		if (id != nullptr) {
			if (src.length () > 0)
				s += "\n";
			s += id;
		}
		s += "[";
		for (size_t i = 0; i < src.length (); i++) {
			char c = src[i];
			if (c == ']' || c == '\\')
				s += '\\';
			s += c;
		}
		s += "]";
	}
}

static void write_visible (std::string &s, const game_state *gs)
{
	/* Note that the SGF standard is slightly defective here.  Points
	   in VW properties are visible, while VW[] defines the whole
	   board to be visible.  Hence there is no way to write out an
	   entirely-invisible board.  However, given that it is impossible
	   to read one and our user interface makes it impossible to set one,
	   that should not be an issue in practice.  */
	const bit_array *vis_orig = gs->visible ();
	if (vis_orig == nullptr || vis_orig->popcnt () == 0)
		return;
	s += "VW";
	bit_array vis = *vis_orig;
	const go_board &b = gs->get_board ();
	int szx = b.size_x ();
	int szy = b.size_y ();
	for (int x = 0; x < szx; x++)
		for (int y = 0; y < szy; y++) {
			int bp = b.bitpos (x, y);
			if (!vis.test_bit (bp))
				continue;
			int x2 = x;
			int y2 = y;
			while (x2 + 1 < szx || y2 + 1 < szy) {
				bool changed = false;
				if (x2 + 1 < szx) {
					int i;
					for (i = y; i <= y2; i++) {
						int tp = b.bitpos (x2 + 1, i);
						if (!vis.test_bit (tp))
							break;
					}
					if (i > y2)
						changed = true, x2++;
				}
				if (y2 + 1 < szy) {
					int i;
					for (i = x; i <= x2; i++) {
						int tp = b.bitpos (i, y2 + 1);
						if (!vis.test_bit (tp))
							break;
					}
					if (i > x2)
						changed = true, y2++;
				}
				if (!changed)
					break;
			}
			for (int i = x; i <= x2; i++)
				for (int j = y; j <= y2; j++)
					vis.clear_bit (b.bitpos (i, j));
			s += "[";
			s += 'a' + x;
			s += 'a' + y;
			if (x2 != x || y2 != y) {
				s += ':';
				s += 'a' + x2;
				s += 'a' + y2;
			}
			s += "]";
		}
}

void game_state::append_to_sgf (std::string &s) const
{
	int linecount = 0;
	const game_state *gs = this;
	while (gs) {
		const go_board &this_board = gs->m_board;
		if (gs->m_parent == nullptr || gs->was_edit_p ()) {
			go_board prev_board (this_board, none);
			if (gs->m_parent)
				prev_board = gs->m_parent->m_board;
			bit_array stones_w = this_board.get_stones_w ();
			bit_array stones_b = this_board.get_stones_b ();
			bit_array added_w = stones_w;
			added_w.andnot (prev_board.get_stones_w ());
			bit_array added_b = stones_b;
			added_b.andnot (prev_board.get_stones_b ());
			bit_array cleared = prev_board.get_stones_w ();
			cleared.ior (prev_board.get_stones_b ());
			stones_w.ior (stones_b);
			cleared.andnot (stones_w);
			maybe_add_property (s, m_board, "AW", added_w, &linecount);
			maybe_add_property (s, m_board, "AB", added_b, &linecount);
			maybe_add_property (s, m_board, "AE", cleared, &linecount);
		} else if (gs->was_move_p () || gs->was_pass_p ()) {
			stone_color col = gs->get_move_color ();
			if (col == white)
				s += "W[";
			else
				s += "B[";
			if (gs->was_move_p ()) {
				int x = gs->get_move_x ();
				int y = gs->get_move_y ();
				s += 'a' + x;
				s += 'a' + y;
				linecount++;
			}
			s += ']';
		}
		this_board.append_marks_sgf (s);
		std::string comm = gs->comment ();
		encode_string (s, "C", comm);
		if (comm.length () > 0)
			linecount = 16;
		std::string wl = gs->time_left (white);
		if (wl.length () > 0) {
			s += "WL[" + wl + "]";
			linecount++;
		}
		std::string bl = gs->time_left (black);
		if (bl.length () > 0) {
			s += "BL[" + bl + "]";
			linecount++;
		}
		std::string ow = gs->stones_left (white);
		if (ow.length () > 0) {
			s += "OW[" + ow + "]";
			linecount++;
		}
		std::string ob = gs->stones_left (black);
		if (ob.length () > 0) {
			s += "OB[" + ob + "]";
			linecount++;
		}
		if (gs->has_figure ()) {
			bool have_title = gs->m_figure.title.length () > 0;
			if (have_title)
				s += "\n";
			s += "FG[" + std::to_string (gs->m_figure.flags);
			if (have_title)
				s += ":" + gs->m_figure.title;
			s += "]";
			if (have_title)
				s += "\n", linecount = 0;
		}
		if (gs->m_print_numbering >= 0)
			s += "PM[" + std::to_string (gs->m_print_numbering);
		int prev_nr = gs->m_parent == nullptr ? -1 : gs->m_parent->m_sgf_movenum;
		if (gs->m_sgf_movenum != prev_nr + 1)
			s += "MN[" + std::to_string (gs->m_sgf_movenum) + "]";

		write_visible (s, gs);
		if (gs->m_eval_visits > 0) {
			s += "QLZV[" + std::to_string (gs->m_eval_visits) + ":" + std::to_string (gs->m_eval_wr_black);
			if (gs->m_eval_komi_set)
				s += ":" + std::to_string (gs->m_eval_komi);
			s += "]";
			linecount++;
		}
		for (auto p: gs->m_unrecognized_props) {
			s += p->ident;
			for (auto v: p->values) {
				encode_string (s, nullptr, v, true);
				linecount++;
			}
		}
		int l = gs->m_children.size ();
		if (l != 1) {
			for (int i = 0; i < l; i++) {
				game_state *c = gs->m_children[i];
				s += "\n(;";
				c->append_to_sgf (s);
				s += ")";
				linecount = 16;
			}
			break;
		}
		if (linecount > 15) {
			s += '\n';
			linecount = 0;
		}
		s += ';';
		gs = gs->m_children[0];
	}
}

std::string game_record::to_sgf () const
{
	const go_board &rootb = m_root.get_board ();
	std::string gm = "1";
	int torus = (rootb.torus_h () ? 1 : 0) | (rootb.torus_v () ? 2 : 0);

	/* There does not appear to be a standard for how to save variant Go in SGF.
	   So we intentionally pick something that other software won't accept and
	   misinterpret.  */
	if (torus != 0)
		gm = "q5go-1";
	/* UTF-8 encoding should be guaranteed, since we convert other charsets
	   when loading, and Qt uses Unicode internally and toStdString conversions
	   guarantee UTF-8.  */
	std::string s = "(;FF[4]GM[" + gm + "]CA[UTF-8]AP[" PACKAGE ":" VERSION "]";
	std::string szx = std::to_string (rootb.size_x ());
	std::string szy = std::to_string (rootb.size_y ());
	encode_string (s, "SZ", szx == szy ? szx : szx + ":" + szy);
	if (torus)
		encode_string (s, "TO", std::to_string (torus));
	encode_string (s, "GN", m_title);
	encode_string (s, "PW", m_name_w);
	encode_string (s, "PB", m_name_b);
	encode_string (s, "WR", m_rank_w);
	encode_string (s, "BR", m_rank_b);
	encode_string (s, "KM", std::to_string (m_komi));
	encode_string (s, "PC", m_place);
	encode_string (s, "DT", m_date);
	encode_string (s, "RU", m_rules);
	encode_string (s, "TM", m_time);
	encode_string (s, "EV", m_event);
	encode_string (s, "RO", m_round);
	encode_string (s, "OT", m_overtime);
	encode_string (s, "CP", m_copyright);
	if (m_handicap > 0)
		encode_string (s, "HA", std::to_string (m_handicap));
	encode_string (s, "RE", m_result);
	if (m_root.to_move () == white)
		s += "PL[W]";

	/* @@@ Could think about writing a ST property, but I dislike the idea of
	   a file telling me how I have to view it.  */

	m_root.append_to_sgf (s);
	s += ")\n";
	return s;
}
