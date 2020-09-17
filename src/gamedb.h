#ifndef GAMEDB_H
#define GAMEDB_H

#include <QAbstractItemModel>
#include <QMutex>
#include <QString>
#include <vector>
#include <atomic>

#include "bitarray.h"
#include "coords.h"

class go_pattern;

struct gamedb_entry
{
	QString dirname, filename;
	QString pw, rkw, pb, rkb;
	QString date, result, event;
	std::vector<char> movelist;
	int id;
	int sz_x, sz_y;
	bit_array finalpos_w, finalpos_b, finalpos_c;
	/* Shifted by 1, the lowest bit indicates whether it's in the q5go.db file.  */
	size_t movelist_off = 0;

	gamedb_entry (int i, const QString &dir, const QString &f,
		      const QString &w, const QString &rw, const QString &b, const QString &rb,
		      const QString &d, const QString &r, const QString &e, int sx, int sy)
		: dirname (dir), filename (f), pw (w), rkw (rw), pb (b), rkb (rb), date (d), result (r), event (e),
		  id (i), sz_x (sx), sz_y (sy), finalpos_w (sx * sy), finalpos_b (sx * sy), finalpos_c (sx * sy)
	{
	}
	gamedb_entry (const gamedb_entry &other) = default;
	gamedb_entry (gamedb_entry &&other) = default;
	gamedb_entry &operator =(gamedb_entry &&other) = default;
	gamedb_entry &operator =(const gamedb_entry &other) = default;
};

class gamedb_model : public QAbstractItemModel
{
	Q_OBJECT

	std::vector<unsigned> m_entries;
	bool m_patternsearch;

	void default_sort ();
public:
	using cont_bw = std::pair<int, int>;
	gamedb_model (bool);
	void clear_list ();
	void apply_filter (const QString &p1, const QString &p2, const QString &event,
			   const QString &dtfrom, const QString &dtto);
	void apply_game_list (std::vector<unsigned> &&);
	void reset_filters ();
	using search_result = std::pair<std::vector<int[2]>, std::vector<cont_bw>>;
	search_result find_pattern (const go_pattern &, std::atomic<long> *, std::atomic<long> *);
	const gamedb_entry &find (size_t) const;
	QString status_string () const;

	virtual QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QModelIndex index (int row, int col, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent (const QModelIndex &index ) const override;
	int rowCount (const QModelIndex &parent = QModelIndex()) const override;
	int columnCount (const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData (int section, Qt::Orientation orientation,
			     int role = Qt::DisplayRole) const override;

	// Qt::ItemFlags flags(const QModelIndex &index) const override;
public slots:
	void slot_prepare_load ();
	void slot_load_complete ();
signals:
	void signal_changed ();
};

class GameDB_Data_Controller : public QObject
{
	Q_OBJECT

public:
	GameDB_Data_Controller ();
	void load ();
public slots:
	void slot_load_complete ();
signals:
	void signal_prepare_load ();
	void signal_start_load (unsigned, bool);
};

class QDir;
class GameDB_Data : public QObject
{
	Q_OBJECT

	void do_load (unsigned, bool);
	bool read_extra_file (QDataStream &, int, int, unsigned, bool);
	bool read_q5go_db (const QDir &, int, unsigned, bool);

public:
	/* A local copy of the paths in settings.  */
	QStringList dbpaths;
	/* Computed in slot_start_load.  Once completed, we signal_load_complete,
	   after which only the main thread can access this vector.  */
	std::vector<gamedb_entry> m_all_entries;
	QMutex db_mutex;
	bool load_complete = false;
	bool too_large = false;
	bool errors_found = false;
	bool errors_kombilo = false;

public slots:
	void slot_start_load (unsigned, bool);
signals:
	void signal_load_complete ();
};

extern GameDB_Data *db_data;
extern GameDB_Data_Controller *db_data_controller;

/* Kombilo's flags for the move list.  */
static const int db_mv_flag_white = 32;
static const int db_mv_flag_black = 64;
static const int db_mv_flag_black_shift = 6;
static const int db_mv_flag_delete = 128;

static const int db_mv_flag_endvar = 32;
static const int db_mv_flag_branch = 64;
static const int db_mv_flag_node_end = 128;

extern bit_array match_games (const std::vector<unsigned> &, const go_pattern &,
			      std::vector<gamedb_model::cont_bw> &conts, coord_transform);

#endif
