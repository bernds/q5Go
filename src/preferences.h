/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "setting.h"

#include <memory>

#include <QStandardItemModel>
#include <QAbstractItemModel>
#include <QDialog>
#include <QAbstractButton>
#include <QIntValidator>
#include <QDoubleValidator>

class QIntValidator;
class MainWindow;
class ClientWindow;
class ImageHandler;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QProgressDialog;

namespace Ui
{
	class PreferencesDialogGui;
	class EngineDialog;
};

class EngineDialog : public QDialog {
	Q_OBJECT
	Ui::EngineDialog *ui;

	QIntValidator m_size_vald { 3, 25 };
	QDoubleValidator m_komi_vald;

	void init ();

public:
	EngineDialog (QWidget *parent);
	EngineDialog (QWidget *parent, const Engine &, bool dup);
	~EngineDialog ();

	Engine get_engine ();
	virtual void accept () override;

public slots:
	void slot_get_path ();
};

template<class T>
class pref_vec_model : public QAbstractItemModel
{
	std::vector<T> m_entries;
	std::vector<T> m_extra;

	void init (const QStringList &init)
	{
		m_entries.clear ();
		m_extra.clear ();
		m_entries.reserve (init.size ());
		for (const auto &s: init)
			m_entries.emplace_back (s);
	}
public:
	pref_vec_model (std::vector<T> init)
		: m_entries (init)
	{
	}
	pref_vec_model (std::vector<T> init, const std::vector<T> &extra)
		: m_entries (init), m_extra (extra)
	{
	}
	pref_vec_model (const QStringList &l)
	{
		init (l);
	}
	void reinit (const QStringList &l)
	{
		beginResetModel ();
		init (l);
		endResetModel ();
	}
	void update_entry (const QModelIndex &i, const T &v);
	void add_or_replace (T);
	bool add_no_replace (T);
	const T *find (const QModelIndex &) const;
	const std::vector<T> &entries () const { return m_entries; }
	virtual QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QModelIndex index (int row, int col, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent (const QModelIndex &index ) const override;
	int rowCount (const QModelIndex &parent = QModelIndex()) const override;
	int columnCount (const QModelIndex &parent = QModelIndex()) const override;
	bool removeRows (int row, int count, const QModelIndex &parent = QModelIndex()) override;
	QVariant headerData (int section, Qt::Orientation orientation,
			     int role = Qt::DisplayRole) const override;
};

struct db_dir_entry
{
	QString title;
	bool qdb_found;
	bool kdb_found;

	db_dir_entry (const QString &s);

	static int n_columns () { return 2; }
	QString data (int col) const
	{
		if (col == 1)
			return title;
		return QString ();
	}
	QVariant icon (int col) const;
};

class dbpath_model : public pref_vec_model<db_dir_entry>
{
	using pref_vec_model<db_dir_entry>::pref_vec_model;

	QVariant headerData (int section, Qt::Orientation orientation,
			     int role = Qt::DisplayRole) const override;
};

class PreferencesDialog : public QDialog
{
	Q_OBJECT

	Ui::PreferencesDialogGui *ui;

	bool m_changing_host = false;

	ImageHandler *m_ih;
	QGraphicsPixmapItem *m_w_stone, *m_b_stone;
	QGraphicsScene *m_stone_canvas;
	int m_stone_size;

	QString m_last_added_dbdir;

	pref_vec_model<Host> m_hosts_model;
	pref_vec_model<Engine> m_engines_model;
	dbpath_model m_dbpath_model;

	bool m_dbpaths_changed = false;
	bool m_engines_changed = false;
	bool m_hosts_changed = false;

	QColor m_chat_color { Qt::black };

	void init_from_settings ();

	bool verify ();
	bool avoid_losing_data ();
	void clear_engine ();
	void clear_host ();
	void update_board_image ();
	void update_w_stones ();
	void update_b_stones ();
	void update_stone_params ();
	void update_stone_positions ();

	void update_chat_color ();

	void update_current_host ();
	void update_current_engine ();

	void update_db_selection ();
	void update_db_labels ();

	/* Defined in gamedb.cpp.  */
	bool create_db_for_dir (QProgressDialog &dlg, const QString &dir);

public:
	PreferencesDialog (int tab, QWidget* parent = 0);
	~PreferencesDialog ();

//	static QString fontToString(QFont f);
	QColor white_color ();
	QColor black_color ();

signals:
	void signal_addHost(const QString&, const QString&, unsigned int, const QString&, const QString&);
	void signal_delHost(const QString&);


public slots:
	void on_soundButtonGroup_buttonClicked (QAbstractButton *);
	void slot_new_server ();
	void slot_add_server ();
	void slot_delete_server ();
	void slot_new_engine ();
	void slot_dup_engine ();
	void slot_change_engine ();
	void slot_delete_engine ();
	void slot_apply ();
	void startHelpMode ();
	void selectFont (QPushButton *, QFont &);
	void slot_accept ();
	void slot_reject ();
	void slot_serverChanged (const QString &);
	void slot_getGobanPicturePath ();
	void slot_getTablePicturePath ();
	void slot_main_time_changed (int);
	void slot_BY_time_changed (int);
	void select_white_color (bool);
	void select_black_color (bool);
	void select_stone_look (bool);
	void select_chat_color (bool);

	void slot_autosavedir (bool);

	void slot_dbdir (bool);
	void slot_dbcfg (bool);
	void slot_dbrem (bool);
	void slot_dbcreate (bool);

private:
	void saveSizes ();
};

#endif
