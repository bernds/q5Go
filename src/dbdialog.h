#ifndef DBDIALOG_H
#define DBDIALOG_H

#include <QStringList>
#include <QAbstractItemModel>

#include "ui_dbdialog_gui.h"

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

class GameDB_Data_Controller : public QObject
{
	Q_OBJECT

public:
	GameDB_Data_Controller ();
	void load ();
signals:
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

public slots:
	void slot_start_load ();
signals:
	void signal_load_complete ();
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

class DBDialog : public QDialog, public Ui::DBDialog
{
	Q_OBJECT

	go_game_ptr m_empty_game;
	go_game_ptr m_game;
	game_state *m_last_move;

	gamedb_model m_model;

	bool setPath (QString path);
	void clear_preview ();
	bool update_selection ();
	void handle_doubleclick ();
	void update_buttons ();

public slots:
	void clear_filters (bool);
	void apply_filters (bool);

signals:
	void signal_start_load ();

public:
	DBDialog (QWidget *parent);
	~DBDialog ();
	void update_prefs ();

	virtual void accept () override;

	go_game_ptr selected_record () { return m_game; }
};

extern DBDialog *db_dialog;

extern void start_db_thread ();
extern void end_db_thread ();

#endif
