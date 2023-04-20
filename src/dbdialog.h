#ifndef DBDIALOG_H
#define DBDIALOG_H

#include "common.h"
#include <QStringList>
#include <QDataStream>

#include "gogame.h"
#include "gamedb.h"

namespace Ui
{
	class DBDialog;
};

class DBDialog : public QDialog
{
	Q_OBJECT

	std::unique_ptr<Ui::DBDialog> ui;
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

	virtual void accept () override;

	go_game_ptr selected_record () { return m_game; }
};

extern DBDialog *db_dialog;

extern void start_db_thread ();
extern void end_db_thread ();
extern void update_db_prefs ();

#endif
