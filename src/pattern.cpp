#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QThreadPool>
#include <QRunnable>
#include <QSemaphore>
#include <QMutex>

#include <string>

#include "gogame.h"
#include "pattern.h"
#include "gamedb.h"

struct coord_transform_none;
struct coord_transform_r90;
struct coord_transform_r180;
struct coord_transform_r270;
struct coord_transform_flip1;
struct coord_transform_flip2;
struct coord_transform_flip3;
struct coord_transform_flip4;

struct coord_transform_none
{
	typedef coord_transform_none inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned, unsigned)
	{
		return coord_pair { x, y };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldx, oldy };
	}
	static alignment align (alignment old)
	{
		return old;
	}
	static const bool is_identity = true;
};

struct coord_transform_r90
{
	typedef coord_transform_r270 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned sx, unsigned)
	{
		return coord_pair { y, sx - x - 1 };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldy, oldx };
	}
	static alignment align (alignment old)
	{
		return alignment { old.right, old.top, old.left, old.bot };
	}
	static const bool is_identity = false;
};

struct coord_transform_r180
{
	typedef coord_transform_r180 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned sx, unsigned sy)
	{
		return coord_pair { sx - x - 1, sy - y - 1 };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldx, oldy };
	}
	static alignment align (alignment old)
	{
		return alignment { old.bot, old.right, old.top, old.left };
	}
	static const bool is_identity = false;
};

struct coord_transform_r270
{
	typedef coord_transform_r90 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned, unsigned sy)
	{
		return coord_pair { sy - y - 1, x };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldy, oldx };
	}
	static alignment align (alignment old)
	{
		return alignment { old.left, old.bot, old.right, old.top };
	}
	static const bool is_identity = false;
};

struct coord_transform_flip1
{
	typedef coord_transform_flip1 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned, unsigned sy)
	{
		return coord_pair { x, sy - y - 1 };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldx, oldy };
	}
	static alignment align (alignment old)
	{
		return alignment { old.bot, old.left, old.top, old.right };
	}
	static const bool is_identity = false;
};

struct coord_transform_flip2
{
	typedef coord_transform_flip2 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned sx, unsigned)
	{
		return coord_pair { sx - x - 1, y };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldx, oldy };
	}
	static alignment align (alignment old)
	{
		return alignment { old.top, old.right, old.bot, old.left };
	}
	static const bool is_identity = false;
};

struct coord_transform_flip3
{
	typedef coord_transform_flip3 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned, unsigned)
	{
		return coord_pair { y, x };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldy, oldx };
	}
	static alignment align (alignment old)
	{
		return alignment { old.left, old.top, old.right, old.bot };
	}
	static const bool is_identity = false;
};

struct coord_transform_flip4
{
	typedef coord_transform_flip4 inverse;
	static coord_pair transform (unsigned x, unsigned y, unsigned sx, unsigned sy)
	{
		return coord_pair { sy - y - 1, sx - x - 1 };
	}
	static std::pair<unsigned, unsigned> new_max (unsigned oldx, unsigned oldy)
	{
		return std::pair<unsigned, unsigned> { oldy, oldx };
	}
	static alignment align (alignment old)
	{
		return alignment { old.right, old.bot, old.left, old.top };
	}
	static const bool is_identity = false;
};

go_pattern::go_pattern (unsigned sz_x, unsigned sz_y)
	: m_lines (sz_y), m_reverse (coord_transform_none::transform), m_sz_x (sz_x), m_sz_y (sz_y)
{
}

go_pattern::go_pattern (const go_board &b, unsigned sz_x, unsigned sz_y, unsigned offx, unsigned offy)
	: m_lines (sz_y), m_reverse (coord_transform_none::transform), m_sz_x (sz_x), m_sz_y (sz_y)
{
	const bit_array &sw = b.get_stones_w ();
	const bit_array &sb = b.get_stones_b ();
	for (unsigned y = 0; y < sz_y; y++) {
		m_lines[y].bits[0] = sw.extract (b.bitpos (offx, offy + y), sz_x);
		m_lines[y].bits[1] = sb.extract (b.bitpos (offx, offy + y), sz_x);
	}
	m_align.top = offy == 0;
	m_align.bot = offy + sz_y == b.size_y ();
	m_align.left = offx == 0;
	m_align.right = offx + sz_x == b.size_x ();
}

void go_pattern::debug ()
{
	for (unsigned y = 0; y < m_sz_y; y++) {
		for (unsigned x = 0; x < m_sz_x; x++)
			if (m_lines[y].bits[0] & (1 << x))
				putchar ('X');
			else if (m_lines[y].bits[1] & (1 << x))
				putchar ('O');
			else
				putchar (' ');
		putchar ('\n');
	}
}

bool go_pattern::operator== (const go_pattern &other) const
{
	if (m_sz_x != other.m_sz_x || m_sz_y != other.m_sz_y)
		return false;
	if (m_align != other.m_align)
		return false;
	for (unsigned i = 0; i < m_sz_y; i++)
		if (m_lines[i].bits[0] != other.m_lines[i].bits[0] || m_lines[i].bits[1] != other.m_lines[i].bits[1])
			return false;
	return true;
}

template<class T>
go_pattern go_pattern::transform () const
{
	unsigned new_maxx, new_maxy;
	std::tie (new_maxx, new_maxy) = T::new_max (m_sz_x, m_sz_y);
	go_pattern r (new_maxx, new_maxy);
	r.m_reverse = T::inverse::transform;
	for (unsigned x = 0; x < m_sz_x; x++)
		for (unsigned y = 0; y < m_sz_y; y++) {
			unsigned r_x, r_y;
			std::tie (r_x, r_y) = T::transform (x, y, m_sz_x, m_sz_y);
			if (m_lines[y].bits[0] & (1 << x))
				r.m_lines[r_y].bits[0] |= 1 << r_x;
			if (m_lines[y].bits[1] & (1 << x))
				r.m_lines[r_y].bits[1] |= 1 << r_x;
		}
	r.m_align = T::align (m_align);
	return r;
}

bool go_pattern::match (const bit_array &other_w, const bit_array &other_b, unsigned other_sz_x, unsigned other_sz_y,
			board_rect &sel_return) const
{
	if (m_sz_x > other_sz_x || m_sz_y > other_sz_y)
		return false;
	unsigned min_offx = 0, min_offy = 0;
	unsigned max_offx = other_sz_x - m_sz_x + 1;
	unsigned max_offy = other_sz_y - m_sz_y + 1;
	/* Order matters for the following tests.  */
	if (m_align.right)
		min_offx = max_offx - 1;
	if (m_align.bot)
		min_offy = max_offy - 1;
	if (m_align.left)
		max_offx = 1;
	if (m_align.top)
		max_offy = 1;
	for (unsigned yoff = min_offy; yoff < max_offy; yoff++)
		for (unsigned xoff = min_offx; xoff < max_offx; xoff++) {
			/* A bit mask representing successful equal and opposite color comparisons.  */
			unsigned success = 3;
			for (unsigned y = 0; y < m_sz_y && success != 0; y++) {
				uint64_t row_w = other_w.extract ((yoff + y) * other_sz_x + xoff, m_sz_x);
				uint64_t row2_w = m_lines[y].bits[0];
				uint64_t row_b = other_b.extract ((yoff + y) * other_sz_x + xoff, m_sz_x);
				uint64_t row2_b = m_lines[y].bits[1];
				if (row_w != row2_w)
					success &= ~1;
				if (row_b != row2_b)
					success &= ~1;
				if (row_w != row2_b)
					success &= ~2;
				if (row_b != row2_w)
					success &= ~2;
			}
			if (success != 0) {
				sel_return.x1 = xoff;
				sel_return.y1 = yoff;
				sel_return.x2 = xoff + m_sz_x - 1;
				sel_return.y2 = yoff + m_sz_y - 1;
				return true;
			}
		}
	return false;
}

void go_pattern::find_cands (std::vector<cand_match> &result,
			     const bit_array &other_w, const bit_array &other_b, const bit_array &other_caps,
			     unsigned other_sz_x, unsigned other_sz_y) const
{
	if (m_sz_x > other_sz_x || m_sz_y > other_sz_y)
		return;

	unsigned min_offx = 0, min_offy = 0;
	unsigned max_offx = other_sz_x - m_sz_x + 1;
	unsigned max_offy = other_sz_y - m_sz_y + 1;
	/* Order matters for the following tests.  */
	if (m_align.right)
		min_offx = max_offx - 1;
	if (m_align.bot)
		min_offy = max_offy - 1;
	if (m_align.left)
		max_offx = 1;
	if (m_align.top)
		max_offy = 1;
	for (unsigned yoff = min_offy; yoff < max_offy; yoff++) {
		uint32_t valid_a = (1 << (max_offx - min_offx)) - 1;
		uint32_t valid_b = valid_a;
		for (unsigned y = 0; y < m_sz_y && (valid_a | valid_b) != 0; y++) {
			uint64_t row_w = other_w.extract ((yoff + y) * other_sz_x, other_sz_x);
			uint64_t row2_w = m_lines[y].bits[0];
			uint64_t row_b = other_b.extract ((yoff + y) * other_sz_x, other_sz_x);
			uint64_t row2_b = m_lines[y].bits[1];

			row_w >>= min_offx;
			row_b >>= min_offx;
			for (unsigned x = 0; x < max_offx - min_offx; x++, row_w >>= 1, row_b >>= 1) {
				uint32_t bit = 1 << x;
				if ((row_w & row2_w) != row2_w)
					valid_a &= ~bit;
				if ((row_b & row2_b) != row2_b)
					valid_a &= ~bit;
				if ((row_w & row2_b) != row2_b)
					valid_b &= ~bit;
				if ((row_b & row2_w) != row2_w)
					valid_b &= ~bit;
			}
		}
		if ((valid_a | valid_b) == 0)
			continue;

		for (unsigned xoff = min_offx; xoff < max_offx; xoff++, valid_a >>= 1, valid_b >>= 1) {
			if (valid_a & 1) {
				std::vector<uint32_t> caps (m_sz_y);
				for (unsigned y = 0; y < m_sz_y; y++)
					caps[y] = other_caps.extract ((yoff + y) * other_sz_x + xoff, m_sz_x);
				result.emplace_back (*this, std::move (caps), xoff, yoff);
			}
			if (valid_b & 1) {
				std::vector<uint32_t> caps (m_sz_y);
				for (unsigned y = 0; y < m_sz_y; y++)
					caps[y] = other_caps.extract ((yoff + y) * other_sz_x + xoff, m_sz_x);
				result.emplace_back (*this, std::move (caps), xoff, yoff, cand_match::swapped);
			}
		}
	}
}

static const int flag_white = 32;
static const int flag_black = 64;
static const int flag_black_shift = 6;
static const int flag_delete = 128;

static const int flag_endvar = 32;
static const int flag_branch = 64;
static const int flag_node_end = 128;

bool match_movelist (const std::vector<char> &moves, std::vector<cand_match> &cands,
		     std::vector<gamedb_model::cont_bw> &conts, int cont_maxx)
{
	std::vector<cand_match *> active (cands.size ());
	for (size_t i = 0; i < cands.size (); i++)
		active[i] = &cands[i];

	size_t len = moves.size ();
	bool success = false;
	for (size_t i = 0; i + 1 < len; i += 2) {
		int x = moves[i];
		int y = moves[i + 1];

		if (x & flag_endvar)
			break;
		if (x & flag_branch)
			continue;
		bool have_move = (y & (flag_black | flag_white)) != 0;
		bool endpos = (x & flag_node_end) != 0;
		int col_off = (y >> flag_black_shift) & 1;
		x &= 31;
		y &= 31;
		if (have_move) {
			if (y & flag_delete) {
				for (auto it: active)
					it->clear_stone (x, y, col_off);
			} else {
				for (auto it: active)
					it->put_stone (x, y, col_off);
			}
		}
		if (endpos) {
			for (size_t i = 0; i < active.size (); i++) {
			again:
				auto &it = active[i];
				auto state = it->end_of_node ();
				if (state == cand_match::matched)
					success = true;
				else if (state == cand_match::failed || state == cand_match::continued) {
					it = active[active.size () - 1];
					active.pop_back ();
					if (i == active.size ())
						break;
					goto again;
				}
			}
			if (active.size () == 0)
				break;
		}
	}
	for (auto &it: cands) {
		if (it.was_continued ()) {
			unsigned x, y;
			int col;
			std::tie (x, y, col) = it.extract_continuation ();
			std::tie (x, y) = it.reverse () (x, y, it.sz_x (), it.sz_y ());
			if (col == 0)
				conts[y * cont_maxx + x].first++;
			else
				conts[y * cont_maxx + x].second++;
		}
	}
	return success;
}

bit_array match_games (const std::vector<unsigned> &cand_games, size_t first, size_t end,
		       const std::vector<go_pattern> &patterns,
		       std::vector<gamedb_model::cont_bw> &conts, int cont_maxx)
{
	bit_array result (db_data->m_all_entries.size ());
	std::vector<char> movelist;
	std::vector<cand_match> cand_matches;
	cand_matches.reserve (500);
	for (size_t j = first; j < end; j++) {
		unsigned i = cand_games[j];
		auto &entry = db_data->m_all_entries[i];
		cand_matches.clear ();
		for (const auto &pat: patterns)
			pat.find_cands (cand_matches, entry.finalpos_w, entry.finalpos_b, entry.finalpos_c,
					entry.boardsize, entry.boardsize);
		if (cand_matches.size () == 0)
			continue;
		if (entry.movelist.size () > 0) {
			if (match_movelist (entry.movelist, cand_matches, conts, cont_maxx))
				result.set_bit (i);
			continue;
		}
		QDir dbdir = entry.dirname;
		QFile f (dbdir.filePath ("kombilo.da"));
		if (f.exists ()) {
			f.open (QIODevice::ReadOnly);
			f.seek (entry.movelist_off);
			QDataStream ds (&f);
			int len;
			char i_c[sizeof len];
			if (ds.readRawData (i_c, sizeof i_c) == sizeof i_c) {
				memcpy (&len, i_c, sizeof len);
				movelist.resize (len);
				if (ds.readRawData (&movelist[0], len) == len) {
					if (match_movelist (movelist, cand_matches, conts, cont_maxx))
						result.set_bit (i);
				}
			}
			f.close ();
		}
	}
	return result;
}


class PartialSearch : public QRunnable
{
	std::vector<go_pattern> m_pats;
	std::vector<unsigned> *m_entries;
	QMutex *m_mutex;
	QSemaphore *m_sem;
	bit_array *m_result;
	std::vector<gamedb_model::cont_bw> *m_conts;
	std::atomic<long> *m_pcur;
	size_t m_first, m_end;
	int m_cont_sz_x;

public:
	PartialSearch (std::vector<unsigned> *e, int start, int end,
		       const std::vector<go_pattern> &p, bit_array *r,
		       std::vector<gamedb_model::cont_bw> *c, QMutex *m, QSemaphore *s,
		       std::atomic<long> *count)
		: m_pats (p), m_entries (e), m_mutex (m), m_sem (s), m_result (r), m_conts (c),
		  m_pcur (count), m_first (start), m_end (end)
	{
		m_cont_sz_x = p[0].sz_x ();
		setAutoDelete (true);
	}
	void run () override
	{
		std::vector<gamedb_model::cont_bw> continuations (m_pats[0].sz_y () * m_pats[0].sz_x ());
		auto games = match_games (*m_entries, m_first, m_end, m_pats, continuations, m_cont_sz_x);
		{
			QMutexLocker lock (m_mutex);
			m_result->ior (games);
			for (size_t i = 0; i < m_conts->size (); i++) {
				(*m_conts)[i].first += continuations[i].first;
				(*m_conts)[i].second += continuations[i].second;
			}
		}
		*m_pcur += m_end - m_first;
		m_sem->release ();
	}
};

std::vector<go_pattern> unique_symmetries (const go_pattern &p0)
{
	std::vector<go_pattern> pats = {
		p0,
		p0.transform<coord_transform_r90> (),
		p0.transform<coord_transform_r180> (),
		p0.transform<coord_transform_r270> (),
		p0.transform<coord_transform_flip1> (),
		p0.transform<coord_transform_flip2> (),
		p0.transform<coord_transform_flip3> (),
		p0.transform<coord_transform_flip4> ()
	};
	pats.erase (std::remove_if (std::begin (pats) + 1, std::end (pats),
				    [&p0] (const go_pattern &pat)
				    {
					    return p0 == pat;
				    }),
		    std::end (pats));
	return pats;
}

/* The main pattern search entry point.

   The algorithm is roughly the same as in Kombilo.  We use the "final position" bitmaps to identify candidate
   spots where the pattern could match, just based on which color stones have been on any given position during
   a game.  We then verify the candidates against the saved move list, identifying real matches and their
   continuations.
   Threading is used to search the eight symmetries in parallel.  */

std::pair <bit_array, std::vector<gamedb_model::cont_bw>>
gamedb_model::find_pattern (const go_pattern &p0, std::atomic<long> *cur, std::atomic<long> *max)
{
	bit_array result (db_data->m_all_entries.size ());
	std::vector<cont_bw> continuations (p0.sz_y () * p0.sz_x ());

	std::vector<go_pattern> pats = unique_symmetries (p0);

	QMutex result_mutex;
	QSemaphore completion_sem (0);
	QThreadPool pool;
	max->store (m_entries.size ());
	int n_started = 0;
	size_t steps = std::max ((size_t)10, m_entries.size () / 128);
	for (size_t i = 0; i < m_entries.size (); i += steps) {
		size_t end = std::min (m_entries.size (), i + steps);
		pool.start (new PartialSearch (&m_entries, i, end, pats, &result, &continuations,
					       &result_mutex, &completion_sem, cur));
		n_started++;
	}
	completion_sem.acquire (n_started);
	return std::pair <bit_array, std::vector<gamedb_model::cont_bw>> { std::move (result), std::move (continuations) };
}

game_state *find_first_match (go_game_ptr gr, const go_pattern &p0, board_rect &sel_return)
{
	std::vector<go_pattern> pats = unique_symmetries (p0);

	auto callback = [&pats, &sel_return] (game_state *st) -> bool
	{
		const go_board &b = st->get_board ();
		const bit_array &sw = b.get_stones_w ();
		const bit_array &sb = b.get_stones_b ();
		for (auto &p: pats)
			if (p.match (sw, sb, b.size_x (), b.size_y (), sel_return))
				throw st;
		return true;

	};
	try {
		gr->get_root ()->walk_tree (callback);
	} catch (game_state *st) {
		return st;
	}
	return nullptr;
}
