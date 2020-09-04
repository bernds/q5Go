#ifndef DBDIALOG_H
#define DBDIALOG_H

#include <QStringList>
#include <QAbstractItemModel>

#include "ui_dbdialog_gui.h"
#include "gamedb.h"

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
