/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "setting.h"
#include "ui_preferences_gui.h"

#include <QStandardItemModel>
#include <QAbstractItemModel>

class QIntValidator;
class MainWindow;
class ClientWindow;
class ImageHandler;
class QGraphicsScene;
class QGraphicsPixmapItem;

template<class T>
class pref_vec_model : public QAbstractItemModel
{
	std::vector<T> m_entries;
	std::vector<T> m_extra;
public:
	pref_vec_model (std::vector<T> init)
		: m_entries (init)
	{
	}
	pref_vec_model (std::vector<T> init, const std::vector<T> &extra)
		: m_entries (init), m_extra (extra)
	{
	}

	void add_or_replace (T);
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

class PreferencesDialog : public QDialog, public Ui::PreferencesDialogGui
{
	Q_OBJECT

	bool m_changing_engine = false;
	bool m_changing_host = false;

	ImageHandler *m_ih;
	QGraphicsPixmapItem *m_w_stone, *m_b_stone;
	QGraphicsScene *m_stone_canvas;
	int m_stone_size;

	QStandardItemModel m_dbpath_model;
	QStringList m_dbpaths;
	bool m_dbpaths_changed = false;
	QString m_last_added_dbdir;

	pref_vec_model<Host> m_hosts_model;
	pref_vec_model<Engine> m_engines_model;

	bool m_engines_changed = false;
	bool m_hosts_changed = false;

	void init_from_settings ();

	bool avoid_losing_data ();
	void clear_engine ();
	void clear_host ();
	void update_board_image ();
	void update_w_stones ();
	void update_b_stones ();
	void update_stone_params ();
	void update_stone_positions ();

	void update_current_host ();
	void update_current_engine ();

	void update_db_selection ();
	void update_dbpaths (const QStringList &);
public:
	PreferencesDialog(QWidget* parent = 0);
	~PreferencesDialog();

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
	void slot_add_engine ();
	void slot_delete_engine ();
	void slot_apply ();
	void startHelpMode ();
	void selectFont (QPushButton *, QFont &);
	void slot_accept ();
	void slot_reject ();
	void slot_serverChanged (const QString &);
	void slot_engineChanged (const QString &);
	void slot_getComputerPath ();
	void slot_getGobanPicturePath ();
	void slot_getTablePicturePath ();
	void slot_main_time_changed (int);
	void slot_BY_time_changed (int);
	void select_white_color (bool);
	void select_black_color (bool);
	void select_stone_look (bool);

	void slot_dbdir (bool);
	void slot_dbcfg (bool);
	void slot_dbrem (bool);
private:
	void          saveSizes();
};

#endif
