#ifndef GAMEDB_H
#define GAMEDB_H

#include <QAbstractItemModel>
#include <QString>
#include <vector>

struct gamedb_entry
{
	QString filename;
	QString pw, pb;
	QString date, result, event;

	gamedb_entry (const QString &f, const QString &w, const QString &b,
	       const QString &d, const QString &r, const QString &e)
		: filename (f), pw (w), pb (b), date (d), result (r), event (e)
	{
	}
	gamedb_entry (gamedb_entry &&other) = default;
	gamedb_entry &operator =(gamedb_entry &&other) = default;
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
	void slot_load_complete ();
};

#endif
