#include <list>
#include <string>
#include <sstream>
#include <functional>

/* Ideally this code would be independent of Qt, but plain C++ seems to have little
   support for converting text encodings.  */
#include <QTextCodec>
/* Plain C++ also has no sane way to format numbers without using the locale.  */
#include <QString>

#include "config.h"
#include "sgf.h"
#include "goboard.h"
#include "gogame.h"
#include "gamedb.h"

static int coord_from_letter (char x)
{
	if (! isalpha (x))
		throw broken_sgf ();
	if (islower (x))
	    return x - 'a';
	else
	    return x - 'A' + 26;
}

static void put_stones (const sgf::node::property &p, int maxx, int maxy, std::function<void (int, int)> func)
{
	for (const auto &v: p.values)
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
	for (auto &p: n->props) {
		const std::string &id = p.ident;
		if (recognized_mark (id)) {
			p.handled = true;
			for (const auto &v: p.values) {
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
		for (const auto &c: comment->values) {
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
	put_stones (*vw, b.size_x (), b.size_y (), [=] (int x, int y) { a->set_bit (b.bitpos (x, y)); });
	gs->set_visible (a);
}

static double sane_stod (std::string s_in)
{
	QString s = QString::fromStdString (s_in);
	s.replace (QChar (','), ".");
	bool ok = false;
	double v = s.toDouble (&ok);
	if (!ok)
		throw broken_sgf ();
	return v;
}

static bool add_eval_1 (game_state *gs, sgf::node::property *propvals, bool kata)
{
	bool retval = true;
	for (const auto &v: propvals->values) {
		std::stringstream ss (v);
		std::vector<std::string> vals;
		for (;;) {
			std::string elt;
			if (!getline (ss, elt, ':'))
				break;
			vals.push_back (elt);
		}
		size_t sz = vals.size ();
		if (sz < (kata ? 5 : 2)) {
			retval = false;
			continue;
		}
		int visits;
		double winrate, scorem = 0, scored = 0;
		analyzer_id id;
		size_t komi_idx = kata ? 4 : 2;
		try {
			visits = stoi (vals[0]);
			winrate = sane_stod (vals[1]);
			if (kata) {
				scorem = sane_stod (vals[2]);
				scored = sane_stod (vals[3]);
			}
			if (sz >= komi_idx + 1) {
				id.komi_set = vals[komi_idx].length () > 0;
				if (id.komi_set)
					id.komi = sane_stod (vals[komi_idx]);
			}
			if (sz >= komi_idx + 2)
				id.engine = vals[komi_idx + 1];
		} catch (...) {
			retval = false;
			continue;
		}
		gs->set_eval_data (visits, winrate, scorem, scored, id);
	}
	return retval;
}

static bool add_eval (game_state *gs, sgf::node *n)
{
	sgf::node::property *qlzv = n->find_property ("QLZV");
	if (qlzv != nullptr)
		if (!add_eval_1 (gs, qlzv, false))
			return false;
	sgf::node::property *qkgv = n->find_property ("QKGV");
	if (qkgv != nullptr)
		if (!add_eval_1 (gs, qkgv, true))
			return false;
	return true;
}

static void encode_position_diff (std::vector<unsigned char> &movelist, bit_array &caps,
				  const go_board &from, const go_board &to, bool force)
{
	bit_array b_add = to.get_stones_b ();
	b_add.andnot (from.get_stones_b ());
	bit_array w_add = to.get_stones_w ();
	w_add.andnot (from.get_stones_w ());
	bit_array b_sub = from.get_stones_b ();
	b_sub.andnot (to.get_stones_b ());
	bit_array w_sub = from.get_stones_w ();
	w_sub.andnot (to.get_stones_w ());

	caps.ior (b_sub);
	caps.ior (w_sub);

	size_t oldsz = movelist.size ();
	for (int y = 0; y < from.size_y (); y++)
		for (int x = 0; x < from.size_x (); x++) {
			unsigned bp = from.bitpos (x, y);
			if (b_add.test_bit (bp)) {
				movelist.push_back (x);
				movelist.push_back (y | db_mv_flag_black);
			}
			if (w_add.test_bit (bp)) {
				movelist.push_back (x);
				movelist.push_back (y | db_mv_flag_white);
			}
			if (b_sub.test_bit (bp)) {
				movelist.push_back (x);
				movelist.push_back (y | db_mv_flag_black | db_mv_flag_delete);
			}
			if (w_sub.test_bit (bp)) {
				movelist.push_back (x);
				movelist.push_back (y | db_mv_flag_white | db_mv_flag_delete);
			}
		}
	size_t newsz = movelist.size ();
	if (newsz > oldsz)
		movelist[newsz - 2] |= db_mv_flag_node_end;
	else if (force) {
		movelist.push_back (db_mv_flag_node_end);
		movelist.push_back (0);
	}
}

/* Similar to add_to_game_state, but we don't create game_states or a game record, instead
   we just collect the information necessary to produce a database entry for the pattern search.  */

void db_info_from_sgf (go_board &b, sgf::node *n, bool is_root, sgf_errors &errs,
		       bit_array &final_w, bit_array &final_b, bit_array &final_c,
		       std::vector<unsigned char> &movelist)
{
	bool force = !is_root;
	while (n) {
		if (!force) {
			while (n->m_siblings != nullptr) {
				movelist.push_back (db_mv_flag_branch);
				movelist.push_back (0);
				go_board tmp_b = b;
				db_info_from_sgf (tmp_b, n, false, errs, final_w, final_b, final_c, movelist);
				movelist.push_back (db_mv_flag_endvar);
				movelist.push_back (0);
				n = n->m_siblings;
			}
		}
		force = false;
		enum class im { unknown, yes, no } is_move = im::unknown;
		go_board new_board (b, mark::none);
		int move_x = -1, move_y = -1;
		bool is_pass = false;
		sgf::node::proplist unrecognized;
		for (auto &p : n->props) {
			const std::string &id = p.ident;
			if (id == "AB" || id == "B" || id == "AW" || id == "W" || id == "AE") {
				p.handled = true;
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
					if (p.values.size () != 1)
						throw broken_sgf ();

					const std::string &v = *p.values.begin ();

					if (v.length () == 0) {
						is_pass = true;
						continue;
					}
					if (v.length () != 2)
						throw broken_sgf ();

					move_x = coord_from_letter (v[0]);
					move_y = coord_from_letter (v[1]);
					if (move_x < 0 || move_y < 0 || move_x >= new_board.size_x () || move_y >= new_board.size_y ()) {
						is_pass = true;
						/* [tt] is a backwards compatibility synonym for pass.   */
						if (move_x != 19 || move_y != 19)
							errs.move_outside_board = true;
						continue;
					}
					if (new_board.stone_at (move_x, move_y) != none) {
						/* We'd like to throw, but Kogo's Joseki Dictionary has
						   such errors.  */
						errs.played_on_stone = true;
						return;
					}
					new_board.add_stone (move_x, move_y, sc);
				} else {
					put_stones (p, new_board.size_x (), new_board.size_y (),
						    [&] (int x, int y) { new_board.set_stone_nounits (x, y, sc); });
				}
			}
		}

		if (is_move == im::no) {
			new_board.identify_units ();
		}
		if (!is_pass || is_root) {
			final_w.ior (new_board.get_stones_w ());
			final_b.ior (new_board.get_stones_b ());
			encode_position_diff (movelist, final_c, b, new_board, is_root);
		}
		b = new_board;

		n = n->m_children;
	}
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
			const std::string &id = p.ident;
			if (id == "AB" || id == "B" || id == "AW" || id == "W" || id == "AE") {
#if 0
				for (int i = 0; i < gs->move_number (); i++)
					std::cerr << " ";
				std::cerr << id;
				for (const auto &v: p.values)
					std::cerr << " : " << v;
				std::cerr << std::endl;
#endif
				p.handled = true;
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
					if (p.values.size () != 1)
						throw broken_sgf ();

					const std::string &v = *p.values.begin ();

					if (v.length () == 0) {
						is_pass = true;
						continue;
					}
					if (v.length () != 2)
						throw broken_sgf ();

					move_x = coord_from_letter (v[0]);
					move_y = coord_from_letter (v[1]);
					if (move_x < 0 || move_y < 0 || move_x >= new_board.size_x () || move_y >= new_board.size_y ()) {
						is_pass = true;
						/* [tt] is a backwards compatibility synonym for pass.   */
						if (move_x != 19 || move_y != 19)
							errs.move_outside_board = true;
						continue;
					}
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
						    [&] (int x, int y) { new_board.set_stone_nounits (x, y, sc); });
				}
			}
		}
		bool terr = add_marks (new_board, n);

		if (is_move == im::no || (is_move == im::unknown && terr)) {
			/* @@@ fix up to_move.  */
			new_board.identify_units ();
			if (terr)
				new_board.territory_from_markers ();
			gs = gs->add_child_edit_nochecks (new_board, to_move, terr, game_state::add_mode::keep_active);
		} else if (is_pass) {
			gs = gs->add_child_pass_nochecks (new_board, game_state::add_mode::keep_active);
		} else
			gs = gs->add_child_move_nochecks (new_board, to_move, move_x, move_y, game_state::add_mode::keep_active);

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
				int nr = stoi (*mn);
				gs->set_sgf_move_number (nr);
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
		for (const auto &p: n->props) {
			if (!p.handled)
				unrecognized.push_back (p);
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

game_info info_from_sgfroot (const sgf &s, QTextCodec *codec, sgf_errors &errs)
{
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
		if (km->find_first_not_of ("0123456789,.", pos) != std::string::npos)
			throw broken_sgf ();
	}
	double komi = km ? sane_stod (*km) : 0;
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
	game_info info;
	info.title = translated_prop_str (gn, codec);
	info.name_w = translated_prop_str (pw, codec);
	info.name_b = translated_prop_str (pb, codec);
	info.rank_w = translated_prop_str (wr, codec);
	info.rank_b = translated_prop_str (br, codec);
	info.rules = translated_prop_str (ru, codec);
	info.komi = komi;
	info.handicap = hc;
	info.result = translated_prop_str (re, codec);
	info.date = translated_prop_str (dt, codec);
	info.place = translated_prop_str (pc, codec);
	info.event = translated_prop_str (ev, codec);
	info.round = translated_prop_str (ro, codec);
	info.copyright = translated_prop_str (cp, codec);
	info.time = translated_prop_str (tm, codec);
	info.overtime = translated_prop_str (ot, codec);
	info.style = style;
	return info;
}

std::pair<int, int> sizes_from_sgfroot (const sgf &s)
{
	const std::string *sz = s.nodes->find_property_val ("SZ");
	int size_x = -1;
	int size_y = 19;
	/* GoGui writes files without SZ. Assume 19, I guess.  */
	if (sz != nullptr) {
		const std::string &v = *sz;
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
	if (size_x < 3 || size_x > 52 || size_y < 3 || size_y > 52)
		throw invalid_boardsize ();
	return std::pair<int, int> { size_x, size_y };
}

std::shared_ptr<game_record> sgf2record (const sgf &s, QTextCodec *codec)
{
	sgf_errors errs = s.errs;

	const std::string *ff = s.nodes->find_property_val ("FF");
	const std::string *gm = s.nodes->find_property_val ("GM");
	bool our_extensions = false;
	if (ff != nullptr) {
		if (*ff == "1" || *ff == "2")
			throw old_sgf_format ();
		if (*ff != "4" && *ff != "3")
			throw broken_sgf ();
	}
	if (gm != nullptr) {
		if (*gm == "q5go-1" || *gm == "q5go-2")
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

	int size_x = -1;
	int size_y = 19;
	std::tie (size_x, size_y) = sizes_from_sgfroot (s);

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

	sgf::node::property *mask = s.nodes->find_property ("MASK");

	go_board initpos (size_x, size_y, torus_h, torus_v);
	std::shared_ptr<bit_array> mask_array;
	if (mask != nullptr && (mask->values.size () > 1 || mask->values.begin ()->length () > 0)) {
		mask_array = std::make_shared<bit_array> (initpos.bitsize ());
		put_stones (*mask, size_x, size_y,
			    [&] (int x, int y) { mask_array->set_bit (initpos.bitpos (x, y)); });
	}
	if (mask_array != nullptr)
		initpos.set_mask (mask_array);
	for (auto &n: s.nodes->props)
		if (n.ident == "AB") {
			n.handled = true;
			put_stones (n, size_x, size_y, [&] (int x, int y) { initpos.set_stone_nounits (x, y, black); });
		}
	for (auto &n: s.nodes->props)
		if (n.ident == "AW") {
			n.handled = true;
			put_stones (n, size_x, size_y, [&] (int x, int y) { initpos.set_stone_nounits (x, y, white); });
		}
	initpos.identify_units ();
	add_marks (initpos, s.nodes);

	game_info info = info_from_sgfroot (s, codec, errs);

	const std::string *pl = s.nodes->find_property_val ("PL");
	stone_color to_play = pl && *pl == "W" ? white : black;
	std::shared_ptr<game_record> game = std::make_shared<game_record> (initpos, to_play, info, mask_array);

	errs.charset_error |= !add_comment (game->m_root, s.nodes, codec);
	errs.charset_error |= !add_figure (game->m_root, s.nodes, codec);
	errs.malformed_eval |= !add_eval (game->m_root, s.nodes);
	add_visible (game->m_root, s.nodes);

	sgf::node::proplist unrecognized;
	for (const auto &p: s.nodes->props) {
		if (!p.handled)
			unrecognized.push_back (p);
	}
	game->m_root->set_unrecognized (unrecognized);

	add_to_game_state (game->m_root, s.nodes->m_children, false, codec, errs);
	game->set_errors (errs);
	if (errs.any_set ())
		game->set_modified ();

	/* Fix up situations where we have a handicap game without a PL property.
	   If it really looks like white to move, fix up the root node.  */
	if (pl == nullptr && info.handicap > 1
	    && game->m_root->n_children () == 1
	    && game->m_root->next_move ()->was_move_p ()
	    && game->m_root->next_move ()->get_move_color () == white)
		game->m_root->set_to_move (white);

	game->m_root->walk_tree ([&] (game_state *st) -> bool
	{
		if (!st->was_score_p ())
			st->set_discard_mode ();
		return true;
	});

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

static void write_array (const bit_array *bitmap, const char *name, std::string &s, const game_state *gs)
{
	/* Note that the SGF standard is slightly defective here.  Points
	   in VW properties are visible, while VW[] defines the whole
	   board to be visible.  Hence there is no way to write out an
	   entirely-invisible board.  However, given that it is impossible
	   to read one and our user interface makes it impossible to set one,
	   that should not be an issue in practice.  */
	if (bitmap == nullptr || bitmap->popcnt () == 0)
		return;
	s += name;
	bit_array vis = *bitmap;
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

void game_state::append_to_sgf (std::string &s, bool active_only) const
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
				s += x >= 26 ? 'A' + (x - 26) : 'a' + x;
				s += y >= 26 ? 'A' + (y - 26) : 'a' + y;
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

		write_array (gs->visible (), "VW", s, gs);
		bool first = true;
		bool have_scores = false;
		for (const auto &it: gs->m_evals)
			if (it.score_stddev != 0)
				have_scores = true;
		for (const auto &it: gs->m_evals) {
			if (it.visits > 0) {
				if (first)
					s += have_scores ? "QKGV" : "QLZV";
				first = false;
				s += "[" + std::to_string (it.visits) + ":" + std::to_string (it.wr_black);
				if (have_scores)
					s += (":" + QString::number (it.score_mean) + ":" + QString::number (it.score_stddev)).toStdString ();
				if (it.id.komi_set) {
					s += ":" + QString::number (it.id.komi).toStdString ();
					if (it.id.engine.length () > 0)
						s += ":" + it.id.engine;
				} else if (it.id.engine.length () > 0)
					s += "::" + it.id.engine;
				s += "]";
				linecount++;
			}
		}
		for (const auto &p: gs->m_unrecognized_props) {
			s += p.ident;
			for (const auto &v: p.values) {
				encode_string (s, nullptr, v, true);
				linecount++;
			}
		}
		size_t l = gs->m_children.size ();
		if (l == 0)
			break;
		if (l > 1 && !active_only) {
			for (size_t i = 0; i < l; i++) {
				game_state *c = gs->m_children[i];
				s += "\n(;";
				c->append_to_sgf (s, active_only);
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
		gs = gs->m_children[active_only ? gs->m_active : 0];
	}
}

std::string game_record::to_sgf (bool active_only) const
{
	const go_board &rootb = m_root->get_board ();
	std::string gm = "1";
	int torus = (rootb.torus_h () ? 1 : 0) | (rootb.torus_v () ? 2 : 0);
	bool masked = m_mask != nullptr;

	/* There does not appear to be a standard for how to save variant Go in SGF.
	   So we intentionally pick something that other software won't accept and
	   misinterpret.  */
	if (masked)
		gm = "q5go-2";
	else if (torus != 0)
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
	if (masked)
		write_array (m_mask.get (), "MASK", s, m_root);

	encode_string (s, "GN", m_info.title);
	encode_string (s, "PW", m_info.name_w);
	encode_string (s, "PB", m_info.name_b);
	encode_string (s, "WR", m_info.rank_w);
	encode_string (s, "BR", m_info.rank_b);
	encode_string (s, "KM", QString::number (m_info.komi).toStdString ());
	encode_string (s, "PC", m_info.place);
	encode_string (s, "DT", m_info.date);
	encode_string (s, "RU", m_info.rules);
	encode_string (s, "TM", m_info.time);
	encode_string (s, "EV", m_info.event);
	encode_string (s, "RO", m_info.round);
	encode_string (s, "OT", m_info.overtime);
	encode_string (s, "CP", m_info.copyright);
	if (m_info.handicap > 0)
		encode_string (s, "HA", std::to_string (m_info.handicap));
	encode_string (s, "RE", m_info.result);
	if (m_root->to_move () == white)
		s += "PL[W]";

	/* @@@ Could think about writing a ST property, but I dislike the idea of
	   a file telling me how I have to view it.  */

	m_root->append_to_sgf (s, active_only);
	s += ")\n";
	return s;
}
