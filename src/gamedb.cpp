#include <QSqlDatabase>
#include <QSqlQuery>
#include <QThread>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDebug>
#include <QProgressDialog>

#include <memory>
#include <algorithm>
#include <unordered_map>

#include "config.h"
#include "gogame.h"
#include "gamedb.h"
#include "setting.h"
#include "preferences.h"

/* Besides the Kombilo database format, we also roll our own, for the following reasons:
   - Be able to create a database without worrying about interfering with Kombilo
   - Use a portable format (Kombilo writes different file formats on 32 vs 64 bit,
     which leads to problems at least on Windows)
   - Have everything in one file
   - Use more efficient storage for bitmasks
   - Allow different board sizes in one database file

   Much of the things that may look complicated in the implementation are there to guard
   against corruption issues - don't allocate a petabyte if an on-disk string length says
   that's what we should be doing, for example.  This is why we avoid the more convenient
   serializations in QDataStream.  We also avoid writing out suspiciously large data
   (strings exceeding 64k, for example).  */

const int max_db_boardsize = 52;

namespace date_re
{
	QRegularExpression full_re ("(\\d\\d\\d\\d)[^\\d](\\d\\d)[^\\d](\\d\\d)");
	QRegularExpression noday_re ("(\\d\\d\\d\\d)[^\\d](\\d\\d)");
	QRegularExpression year_re ("(\\d\\d\\d\\d)");
	/* Compatibility with old versions of qGo/q4go/q5go before we corrected
	   that little problem.  */
	QRegularExpression compat_re ("^(\\d\\d) (\\d\\d) (\\d\\d\\d\\d)$");

	QString convert (const QString &dt_in)
	{
		auto compat = compat_re.match (dt_in);
		if (compat.hasMatch ())
			return compat.captured (3) + "-" + compat.captured (2) + "-" + compat.captured (1);

		auto full = full_re.match (dt_in);
		if (full.hasMatch ()) {
			QStringList sl = full.capturedTexts ();
			sl.removeFirst ();
			return sl.join ("-");
		}

		auto noday = noday_re.match (dt_in);
		if (noday.hasMatch ()) {
			QStringList sl = noday.capturedTexts ();
			sl.removeFirst ();
			return sl.join ("-") + "-??";
		}

		auto year = year_re.match (dt_in);
		if (year.hasMatch ()) {
			return year.captured (0) + "-??" "-??";
		}

		return "0000" "-??" "-??";
	}
};


QThread *db_thread;
GameDB_Data *db_data;
GameDB_Data_Controller *db_data_controller;

void start_db_thread ()
{
	db_thread = new QThread;
	db_thread->start (QThread::LowPriority);
	db_data = new GameDB_Data;

	db_data->moveToThread (db_thread);
	QObject::connect (db_thread, &QThread::finished, db_data, &QObject::deleteLater);

	db_data_controller = new GameDB_Data_Controller;
	db_data_controller->load ();
}

void end_db_thread ()
{
	db_thread->exit ();
	db_thread->wait ();
	delete db_thread;
	delete db_data_controller;
}

GameDB_Data_Controller::GameDB_Data_Controller ()
{
	connect (this, &GameDB_Data_Controller::signal_start_load, db_data, &GameDB_Data::slot_start_load);
	connect (db_data, &GameDB_Data::signal_load_complete, this, &GameDB_Data_Controller::slot_load_complete);
}

void GameDB_Data_Controller::load ()
{
	unsigned max_mb = setting->readIntEntry ("DB_MAX_FILESIZE");
	if (max_mb < 1 || max_mb > 999)
		max_mb = 50;
	db_data->load_complete = false;
	db_data->too_large = false;
	db_data->errors_found = false;
	db_data->errors_kombilo = false;

	emit signal_prepare_load ();
	emit signal_start_load (max_mb, setting->readBoolEntry ("DB_CACHE_MOVELIST"));
}

void GameDB_Data_Controller::slot_load_complete ()
{
	QString s;
	if (db_data->too_large)
		s += tr ("<p>One of the database files was too large to be loaded.  You can adjust the limit in the settings window.</p>");
	if (db_data->errors_found)
		s += tr ("<p>One of the database files contained errors.</p>");
	if (db_data->errors_kombilo)
		s += tr ("<p>These errors could also mean that there is a 32/64-bit mismatch between this program and the version Kombilo that generated the database.</p>");
	if (!s.isEmpty ()) {
		s += tr ("<p>Databases can be regenerated from the settings window.</p>");
		QMessageBox::warning (nullptr, PACKAGE, s);
	}
}

struct db_io_info : public game_info
{
	std::string filename;
	bit_array fp_w, fp_b, fp_caps;
	std::vector<unsigned char> movelist;
	unsigned sz_x, sz_y;

	db_io_info (const game_info &i, std::string f, unsigned sx, unsigned sy)
		: game_info (i), filename (f), fp_w (sx * sy), fp_b (sx * sy), fp_caps (sx * sy), sz_x (sx), sz_y (sy)
	{
	}
	db_io_info (const db_io_info &) = default;
	db_io_info (db_io_info &&) = default;
	db_io_info &operator= (const db_io_info &) = default;
	db_io_info &operator=  (db_io_info &&) = default;
};

static void collect_game_data (std::vector<db_io_info> &vec, const QString &filename, sgf *sgf)
{
	try {
		int size_x, size_y;
		std::tie (size_x, size_y) = sizes_from_sgfroot (*sgf);

		if (size_x > max_db_boardsize || size_y > max_db_boardsize)
			return;

		go_board b (size_x, size_y);
		unsigned sz = b.bitsize ();

		sgf_errors errs;
		db_io_info info (info_from_sgfroot (*sgf, nullptr, errs), filename.toStdString (), b.size_x (), b.size_y ());
		bit_array w_stones (sz);
		bit_array b_stones (sz);
		bit_array caps (sz);

		info.movelist.reserve (600);
		db_info_from_sgf (b, sgf->nodes, true, errs, info.fp_w, info.fp_b, info.fp_caps, info.movelist);
		vec.emplace_back (std::move (info));
	} catch (...) {
		return;
	}
}

class db_errors_found : public std::exception
{
public:
	db_errors_found ()
	{
	}
};

class db_too_large : public std::exception
{
public:
	db_too_large ()
	{
	}
};

static bool hash_string (std::unordered_map<std::string, unsigned> &map,
			 std::vector<const std::string *> &array,
			 const std::string &data, unsigned &id)
{
	size_t l = data.size ();
	uint16_t lu = l;
	if ((size_t)lu != l)
		return false;
	auto result = map.insert ({ data, id });
	if (result.second) {
		array.push_back (&data);
		(*result.first).second = id++;
	}
	return true;
}

static bool write_uint16 (QDataStream &ds, uint32_t val)
{
	unsigned char data[] = { (unsigned char)(val & 255), (unsigned char)(val >> 8) };
	return ds.writeRawData ((char *)data, 2) == 2;
}

static bool write_uint32 (QDataStream &ds, uint32_t val)
{
	unsigned char data[] = { (unsigned char)(val & 255), (unsigned char)(val >> 8), (unsigned char)(val >> 16), (unsigned char)(val >> 24) };
	return ds.writeRawData ((char *)data, 4) == 4;
}

template<class T>
uint32_t read_uint (QDataStream &ds)
{
	T v;
	char v_raw[sizeof (T)];
	if (ds.readRawData (v_raw, sizeof v) != sizeof v)
		throw db_errors_found ();
	memcpy (&v, v_raw, sizeof v);
	return v;
}

static bool write_string_id (QDataStream &ds, std::unordered_map<std::string, unsigned> &map, const std::string &data)
{
	auto it = map.find (data);
	/* Strings that are too large are not entered into the map.  */
	if (it == map.end ())
		it = map.find ("");
#ifdef CHECKING
	if (it == map.end ())
		abort ();
#endif
	if (map.size () < 65536)
		return write_uint16 (ds, (*it).second);
	else
		return write_uint32 (ds, (*it).second);
}

static bool write_raw_data (QDataStream &ds, const void *p, size_t len)
{
	return (size_t)ds.writeRawData ((char *)p, len) == len;
}

static sgf *read_sgf_file (const QString &filename)
{
	QFile f (filename);
	f.open (QIODevice::ReadOnly);

	try {
		sgf *sgf = load_sgf (f);
		f.close ();
		return sgf;
	} catch (...) {
		return nullptr;
	}
	f.close ();
	return nullptr;
}

bool PreferencesDialog::create_db_for_dir (QProgressDialog &dlg, const QString &dirname)
{
	QDir dir (dirname);
	QStringList pat;
	pat << "*.[Ss][Gg][Ff]";
	QStringList entries = dir.entryList (pat, QDir::Files);

	dlg.setMaximum (entries.size ());
	int p_count = 0;
	std::vector<db_io_info> collection;
	for (auto filename: entries) {
		if (filename.size () >= 32768)
			continue;
		qDebug () << filename;
		sgf *sgf = read_sgf_file (dir.filePath (filename));
		if (sgf != nullptr) {
			collect_game_data (collection, filename, sgf);
			dlg.setValue (++p_count);
			delete sgf;
		}
		if (dlg.wasCanceled ())
			return false;
	}

	std::unordered_map<std::string, unsigned> map;
	std::vector<const std::string *> str_array;
	unsigned id = 0;
	std::string empty = "";
	hash_string (map, str_array, empty, id);
	for (auto &d: collection) {
		bool success = true;
		success &= hash_string (map, str_array, d.name_w, id);
		success &= hash_string (map, str_array, d.rank_w, id);
		success &= hash_string (map, str_array, d.name_b, id);
		success &= hash_string (map, str_array, d.rank_b, id);
		success &= hash_string (map, str_array, d.result, id);
		success &= hash_string (map, str_array, d.date, id);
		success &= hash_string (map, str_array, d.event, id);
	}
#ifdef CHECKING
	if (id != str_array.size ())
		abort ();
#endif

	QFile f (dir.filePath ("q5go.tmp"));
	f.open (QIODevice::WriteOnly | QIODevice::Truncate);
	QDataStream ds (&f);

	bool success = true;
	char qid[] = { 'q', '5', 'd', 'b' };
	success &= write_raw_data (ds, qid, 4);

	/* A version number.  */
	success &= write_uint32 (ds, 1);
	uint32_t count = map.size ();
	success &= write_uint32 (ds, count);

	for (size_t i = 0; i < str_array.size (); i++) {
		const std::string &d = *str_array[i];
		uint16_t size = d.size ();
		if (size != d.size ())
			continue;
		success &= write_uint16 (ds, size);
		if (size > 0)
			success &= write_raw_data (ds, &d[0], size);
	}
	success &= write_uint32 (ds, collection.size ());
	for (auto &d: collection) {
		const std::string &str = d.filename;
		uint32_t size = str.size ();
		success &= write_uint16 (ds, size);
		success &= write_raw_data (ds, &str[0], size);

		success &= write_uint16 (ds, d.sz_x);
		success &= write_uint16 (ds, d.sz_y);
		success &= write_string_id (ds, map, d.name_w);
		success &= write_string_id (ds, map, d.rank_w);
		success &= write_string_id (ds, map, d.name_b);
		success &= write_string_id (ds, map, d.rank_b);
		success &= write_string_id (ds, map, d.result);
		success &= write_string_id (ds, map, d.date);
		success &= write_string_id (ds, map, d.event);
		success &= write_raw_data (ds, d.fp_w.raw_bits (), d.fp_w.raw_n_elts () * sizeof (uint64_t));
		success &= write_raw_data (ds, d.fp_b.raw_bits (), d.fp_b.raw_n_elts  () * sizeof (uint64_t));
		success &= write_raw_data (ds, d.fp_caps.raw_bits (), d.fp_caps.raw_n_elts () * sizeof (uint64_t));
		success &= write_uint32 (ds, d.movelist.size ());
		success &= write_raw_data (ds, &d.movelist[0], d.movelist.size ());
	}
	success &= f.flush ();
	f.close ();
	if (success) {
		QFile f2 (dir.filePath ("q5go.db"));
		QFile::remove (dir.filePath ("q5go.db.old"));
		f2.rename (dir.filePath ("q5go.db.old"));
		f.rename (dir.filePath ("q5go.db"));
		QFile::remove (dir.filePath ("q5go.db.old"));
	} else {
		f.remove ();
	}
	return success;
}

static void read_raw_data (QDataStream &ds, void *buf, int len)
{
	if (ds.readRawData ((char *)buf, len) != len)
		throw db_errors_found ();
}

static std::string read_string (QDataStream &ds, std::vector<char> &buf)
{
	std::string s;
	uint16_t len = read_uint<uint16_t> (ds);
	if (len == 0)
		return "";

	s.reserve (len);
	buf.resize (len);

	read_raw_data (ds, &buf[0], len);
	s.append (&buf[0], len);
	return s;
}

bool GameDB_Data::read_q5go_db (const QDir &dbdir, int base_id, unsigned max_mb, bool read_movelist)
{
	QFile f (dbdir.filePath ("q5go.db"));
	f.open (QIODevice::ReadOnly);
	QDataStream ds (&f);

	try {
		char id[5];
		id[4] = 0;
		if (ds.readRawData (id, 4) != 4 || strcmp (id, "q5db") != 0)
			throw db_errors_found ();
		int version = read_uint<uint32_t> (ds);
		uint32_t n_strings = read_uint<uint32_t> (ds);
		uint32_t total = 12;
		std::vector<QString> strtable;
		std::vector<char> buf;
		for (uint32_t i = 0; i < n_strings; i++) {
			std::string s = read_string (ds, buf);
			strtable.emplace_back (QString::fromStdString (s));
			total += s.size () + 2;
		}
		uint32_t (*read_str_id) (QDataStream &) = n_strings < 65536 ? read_uint<uint16_t> : read_uint<uint32_t>;
		int strid_sz = n_strings < 65536 ? 2 : 4;
		uint32_t n_games = read_uint<uint32_t> (ds);
		total += 4;
		if ((total + n_games * (strid_sz * 7 + 4)) / 1024 / 1024 > max_mb)
			throw db_too_large ();

		for (uint32_t i = 0; i < n_games; i++) {
			std::string f = read_string (ds, buf);
			QString filename = QString::fromStdString (f);
			total += f.size () + 2;
			uint32_t sx = read_uint<uint16_t> (ds);
			uint32_t sy = read_uint<uint16_t> (ds);
			if (sx > max_db_boardsize || sy > max_db_boardsize)
				throw db_errors_found ();

			uint32_t nmw_id = read_str_id (ds);
			uint32_t rkw_id = read_str_id (ds);
			uint32_t nmb_id = read_str_id (ds);
			uint32_t rkb_id = read_str_id (ds);
			uint32_t res_id = read_str_id (ds);
			uint32_t dt_id = read_str_id (ds);
			uint32_t ev_id = read_str_id (ds);
			total += strid_sz * 7 + 4;
			m_all_entries.emplace_back (base_id + i, dbdir.absolutePath (), filename,
						    strtable[nmw_id], strtable[rkw_id],
						    strtable[nmb_id], strtable[rkb_id],
						    date_re::convert (strtable[dt_id]),
						    strtable[res_id], strtable[ev_id],
						    sx, sy);
			auto &elt = m_all_entries.back ();
			uint32_t sz = elt.finalpos_w.raw_n_elts () * sizeof (uint64_t);
#ifdef CHECKING
			if (elt.finalpos_w.raw_n_elts () != elt.finalpos_b.raw_n_elts ()
			    || elt.finalpos_w.raw_n_elts () != elt.finalpos_c.raw_n_elts ())
				abort ();
#endif
			total += 3 * sz;

			read_raw_data (ds, elt.finalpos_w.raw_bits (), sz);
			read_raw_data (ds, elt.finalpos_b.raw_bits (), sz);
			read_raw_data (ds, elt.finalpos_c.raw_bits (), sz);

			elt.movelist_off = (total << 1) | 1;
			uint32_t msz = read_uint<uint32_t> (ds);
			total += 4 + msz;
			if (sz / 1024 / 1024 > max_mb || total / 1024 / 1024 > max_mb)
				throw db_too_large ();

			if (read_movelist) {
				elt.movelist.resize (msz);
				read_raw_data (ds, &elt.movelist[0], msz);
			} else
				ds.skipRawData (msz);

			/* Future-proofing.  */
			if (version > 1) {
				uint32_t skip = read_uint<uint32_t> (ds);
				ds.skipRawData (skip);
			}
		}
	} catch (...) {
		f.close ();
		throw;
	}
	f.close ();
	return true;
}

/* Read an extra kombilo file (kombilo.da to go with the kombilo.db database).  */

bool GameDB_Data::read_extra_file (QDataStream &ds, int base_id, int boardsize, unsigned max_mb, bool cache_movelist)
{
	auto beg = std::begin (m_all_entries);
	auto end = std::end (m_all_entries);
	beg = std::lower_bound (beg, end, base_id,
				[] (const gamedb_entry &e, int val)
				{
					return e.id < val;
				});

	size_t sz;
	char sz_c[sizeof (size_t)];
	char i_c[sizeof (int)];
	/* The first section of this file is signature data, which is irrelevant to us.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();

	memcpy (&sz, sz_c, sizeof sz);
	ds.skipRawData (sz);
	size_t full_total = sz + sizeof sz;

	/* The second part contains game final positions, which we add to the gamedb_entry records.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();
	memcpy (&sz, sz_c, sizeof sz);
	size_t relevant_size = sz;

	if (relevant_size / 1024 / 1024 > (size_t)max_mb) {
		too_large = true;
		errors_kombilo = true;
		return false;
	}

	int count;
	if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
		throw db_errors_found ();
	memcpy (&count, i_c, sizeof count);

	std::vector<char> buf;
	size_t total = 0;
	for (int i = 0; i < count; i++) {
		int clen;
		int gameid;

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&gameid, i_c, sizeof gameid);
		gameid += base_id;

		auto pos = std::lower_bound (beg, end, gameid,
					     [] (const gamedb_entry &e, int val)
					     {
						     return e.id < val;
					     });
		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);

		total += sizeof gameid + sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		buf.resize (clen);
		if (ds.readRawData (&buf[0], clen) != clen)
			throw db_errors_found ();

		if (pos != end) {
#ifdef CHECKING
			if (pos->id != gameid)
				abort ();
#endif
			for (int y = 0; y < boardsize; y++) {
				for (int x = 0; x < boardsize; x++) {
					int bufpos = y / 2 + 10 * (x / 2);
					int bitpos = 2 * (x % 2 + 2 * (y % 2));
					char val = bufpos < clen ? buf[bufpos] : 0;
					if ((val & (1 << bitpos)) == 0)
						pos->finalpos_b.set_bit (x + y * boardsize);
					if ((val & (1 << (bitpos + 1))) == 0)
						pos->finalpos_w.set_bit (x + y * boardsize);
#if 0
					char c = ' ';
					if (pos->finalpos_w.test_bit (x + y * boardsize))
						c = 'O';
					if (pos->finalpos_b.test_bit (x + y * boardsize))
						c = c == ' ' ? 'X' : '#';
					putchar (c);
#endif
				}
#if 0
				putchar ('\n');
#endif
			}
		}

		if (total == sz) {
			if (i + 1 != count)
				throw db_errors_found ();
		}
	}
	full_total += sz + sizeof sz;

	/* The third part contains move list and final-position captures.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();
	memcpy (&sz, sz_c, sizeof sz);

	relevant_size += sz;
	if (relevant_size / 1024 / 1024 > (size_t)max_mb) {
		too_large = true;
		errors_kombilo = true;
		return false;
	}

	if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
		throw db_errors_found ();
	memcpy (&count, i_c, sizeof count);
	total = 0;
	for (int i = 0; i < count; i++) {
		int clen;
		int gameid;

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&gameid, i_c, sizeof gameid);
		gameid += base_id;

		auto pos = std::lower_bound (beg, end, gameid,
					     [] (const gamedb_entry &e, int val)
					     {
						     return e.id < val;
					     });
		total += sizeof gameid;
		if (pos != end) {
#ifdef CHECKING
			if (pos->movelist_off != 0)
				abort ();
#endif
			pos->movelist_off = full_total + total + sizeof sz + sizeof count + sizeof (int);
			pos->movelist_off <<= 1;
		}
		/* ??? Strange file format; it saves the length of the movelist first as
		   a plain integer, then again as part of the char array.  */
		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		int dlen;
		memcpy (&dlen, i_c, sizeof dlen);

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);
		if (clen != dlen)
			throw db_errors_found ();

		total += 2 * sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		if (pos != end && cache_movelist) {
			pos->movelist.resize (clen);
			if (ds.readRawData (&pos->movelist[0], clen) != clen)
				throw db_errors_found ();
		} else
			ds.skipRawData (clen);

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);
		buf.resize (clen);

		total += sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		if (ds.readRawData (&buf[0], clen) != clen)
			throw db_errors_found ();
		if (pos != end) {
			for (int y = 0; y < boardsize; y++) {
				for (int x = 0; x < boardsize; x++) {
					int bufpos = y / 4 + 5 * (x / 2);
					int bitpos = x % 2 + 2 * (y % 4);
					char val = bufpos < clen ? buf[bufpos] : 0;
					if ((val & (1 << bitpos)) != 0)
						pos->finalpos_c.set_bit (x + y * boardsize);
				}
			}
		}
	}

	return true;
}

void GameDB_Data::do_load (unsigned max_mb, bool cache_movelist)
{
	QMutexLocker lock (&db_mutex);
	m_all_entries.clear ();

	QSqlDatabase db = QSqlDatabase::database ("kombilo");

	/* Assign all games unique IDs, combining the stored ID with the current total.  */
	int base_id = 0;
	for (auto &it: dbpaths) {
		QDir dbdir (it);
		if (dbdir.exists ("q5go.db")) {
			size_t sz = m_all_entries.size ();
			try {
				read_q5go_db (dbdir, base_id, max_mb, cache_movelist);
				base_id += m_all_entries.size () - sz;
			} catch (db_errors_found &) {
				m_all_entries.erase (m_all_entries.begin () + sz, m_all_entries.end ());
				errors_found = true;
			} catch (db_too_large &) {
				m_all_entries.erase (m_all_entries.begin () + sz, m_all_entries.end ());
				too_large = true;
			}
			continue;
		}

		if (!dbdir.exists ("kombilo.db"))
			continue;
		QString dbpath = dbdir.filePath ("kombilo.db");
		db.setDatabaseName (dbpath);
		db.open ();
		QSqlQuery q1 ("select * from db_info where rowid = 1", db);
		if (!q1.next () || (q1.value (0) != "kombilo 0.7" && q1.value (0) != "kombilo 0.8")) {
			qDebug () << "skipping database with " << q1.value (0);
			continue;
		}
		QSqlQuery q1b ("select * from db_info where rowid = 3", db);
		if (!q1b.next ()) {
			qDebug () << "skipping database without boardsize";
			continue;
		}
		int boardsize = q1b.value (0).toInt ();

		QSqlQuery q2 ("select filename,pw,pb,dt,re,ev,id from GAMES order by id", db);
		int this_max = 0;
		while (q2.next ()) {
			QString filename = q2.value (0).toString ();
			QString dt_in = q2.value (3).toString ();
			QString date = date_re::convert (dt_in);
			int id = q2.value (6).toInt ();
			this_max = std::max (this_max, id);
			id += base_id;

			m_all_entries.emplace_back (id, it, filename, q2.value (1).toString (), "",
						    q2.value (2).toString (), "",
						    date, q2.value (4).toString (), q2.value (5).toString (),
						    boardsize, boardsize);
		}
		db.close ();

		/* Disabled since it creates too many problems, the main one being that there
		   might be a mismatch between 32 and 64 bit between Kombilo and this program.  */
#if 0
		/* Now read the secondary kombilo data.  */
		QFile f (dbdir.filePath ("kombilo.da"));
		if (f.exists ()) {
			f.open (QIODevice::ReadOnly);
			QDataStream ds (&f);
			try {
				read_extra_file (ds, base_id, boardsize, max_mb, cache_movelist);
			} catch (db_errors_found &) {
				errors_found = true;
				errors_kombilo = true;
			}
			f.close ();
		}
#endif
		base_id += this_max + 1;
	}

	load_complete = true;
	std::atomic_thread_fence (std::memory_order_seq_cst);
}

void GameDB_Data::slot_start_load (unsigned max_mb, bool cache_movelist)
{
	static bool init_once = false;
	if (!init_once) {
		QSqlDatabase::addDatabase ("QSQLITE", "kombilo");
		init_once = true;
	}

	setting->m_dbpath_lock.lock ();
	dbpaths = setting->m_dbpaths;
	setting->m_dbpath_lock.unlock ();

	do_load (max_mb, cache_movelist);
	emit signal_load_complete ();
}

gamedb_model::gamedb_model (bool ps)
	: m_patternsearch (ps)
{
	connect (db_data, &GameDB_Data::signal_load_complete, this, &gamedb_model::slot_load_complete);
	connect (db_data_controller, &GameDB_Data_Controller::signal_prepare_load, this, &gamedb_model::slot_prepare_load);

	if (db_data->load_complete)
		slot_load_complete ();
}

const gamedb_entry &gamedb_model::find (size_t row) const
{
	return db_data->m_all_entries[m_entries[row]];
}

QVariant gamedb_model::data (const QModelIndex &index, int role) const
{
	int row = index.row ();
	int col = index.column ();
	if (row < 0 || (size_t)row >= m_entries.size () || role != Qt::DisplayRole || col != 0)
		return QVariant ();
	const gamedb_entry &e = find (row);
	return e.pw + " - " + e.pb + ", " + e.result + ", " + e.date;
}

QModelIndex gamedb_model::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

QModelIndex gamedb_model::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

int gamedb_model::rowCount (const QModelIndex &) const
{
	return m_entries.size ();
}

int gamedb_model::columnCount (const QModelIndex &) const
{
	return 1;
}

QVariant gamedb_model::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	switch (section) {
	case 0:
		return tr ("Players");
	case 1:
		return tr ("Res.");
	case 2:
		return tr ("Date");
	}
	return QVariant ();
}

void gamedb_model::clear_list ()
{
	beginResetModel ();
	m_entries.clear ();
	endResetModel ();
	emit signal_changed ();
}

void gamedb_model::slot_prepare_load ()
{
	clear_list ();
}

void gamedb_model::slot_load_complete ()
{
	std::atomic_thread_fence (std::memory_order_seq_cst);
	reset_filters ();
}

void gamedb_model::default_sort ()
{
	std::sort (std::begin (m_entries), std::end (m_entries),
		   [] (unsigned a, unsigned b)
		   {
			   const gamedb_entry &e1 = db_data->m_all_entries[a];
			   const gamedb_entry &e2 = db_data->m_all_entries[b];
			   return e1.date > e2.date;
		   });
}

void gamedb_model::reset_filters ()
{
	beginResetModel ();
	m_entries.clear ();
	m_entries.reserve (db_data->m_all_entries.size ());
	for (size_t i = 0; i < db_data->m_all_entries.size (); i++)
		if (db_data->m_all_entries[i].movelist_off != 0 || !m_patternsearch)
			m_entries.push_back (i);

	default_sort ();

	endResetModel ();
	emit signal_changed ();
}

QString gamedb_model::status_string () const
{
	return QString::number (m_entries.size ()) + "/" + QString::number (db_data->m_all_entries.size ()) + " games";
}

void gamedb_model::apply_game_list (std::vector<unsigned> &&games)
{
	beginResetModel ();
	m_entries = games;
	default_sort ();
	endResetModel ();
	emit signal_changed ();
}

void gamedb_model::apply_filter (const QString &p1, const QString &p2, const QString &event,
				 const QString &dtfrom, const QString &dtto)
{
	beginResetModel ();
	m_entries.erase (std::remove_if (std::begin (m_entries), std::end (m_entries),
					 [&](unsigned idx)
					 {
						 const gamedb_entry &e = db_data->m_all_entries[idx];
						 if (!p1.isEmpty () && !e.pw.contains (p1) && !e.pb.contains (p1))
							 return true;
						 if (!p2.isEmpty () && !e.pw.contains (p2) && !e.pb.contains (p2))
							 return true;
						 if (!dtfrom.isEmpty () && e.date < dtfrom)
							 return true;
						 if (!dtto.isEmpty () && e.date > dtto)
							 return true;
#if 1
						 if (!event.isEmpty () && !e.event.contains (event) && !e.pb.contains (event))
							 return true;
#endif
						 return false;
					 }),
			 std::end (m_entries));
	endResetModel ();
	emit signal_changed ();
}

void update_db_prefs ()
{
	if (!setting->dbpaths_changed)
		return;
	setting->dbpaths_changed = false;

	db_data_controller->load ();
}
