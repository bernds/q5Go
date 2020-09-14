#ifndef GAMEDB_H
#define GAMEDB_H

#include <QAbstractItemModel>
#include <QString>
#include <vector>

#include "bitarray.h"

struct gamedb_entry
{
	int id;
	QString filename;
	QString pw, pb;
	QString date, result, event;
	bit_array finalpos_w, finalpos_b;

	gamedb_entry (int i, const QString &f, const QString &w, const QString &b,
		      const QString &d, const QString &r, const QString &e, int sz)
		: id (i), filename (f), pw (w), pb (b), date (d), result (r), event (e),
		finalpos_w (sz * sz), finalpos_b (sz * sz)
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

	std::vector<size_t> m_entries;
	void default_sort ();
public:
	gamedb_model ();
	void clear_list ();
	void apply_filter (const QString &p1, const QString &p2, const QString &event,
			   const QString &dtfrom, const QString &dtto);
	void reset_filters ();
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
signals:
	void signal_prepare_load ();
	void signal_start_load ();
};

class GameDB_Data : public QObject
{
	Q_OBJECT

public:
	/* A local copy of the paths in settings.  */
	QStringList dbpaths;
	/* Computed in slot_start_load.  Once completed, we signal_load_complete,
	   after which only the main thread can access this vector.  */
	std::vector<gamedb_entry> m_all_entries;
	bool load_complete = false;

	bool read_extra_file (QDataStream &, int, int);

public slots:
	void slot_start_load ();
signals:
	void signal_load_complete ();
};

#endif
