/*
 *   gamedialog.h
 */

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include <QButtonGroup>

#include "ui_newgame_gui.h"
#include "gs_globals.h"
#include "defines.h"

class GameDialog : public QDialog, public Ui::NewGameDialog
{
	Q_OBJECT

	void swap_colors ();
	void clear_warnings ();

public:
	GameDialog(QWidget* parent, GSName, const QString &);
	~GameDialog();
	void set_oppRk(QString &rk) { oppRk = rk; qDebug() << "oppRk: " << rk; }
	void set_myRk(QString &rk) { myRk = rk; qDebug() << "myRk: " << rk; }
	void set_is_nmatch (bool b) { is_nmatch = b; }

	void setting_changed ();

signals:
	void signal_matchsettings(const QString&, const QString&, const QString&, assessType);
	void signal_removeDialog(const QString&);

public slots:
	// pushbuttons
	void slot_stats_opponent (bool);
	void slot_pbsuggest (bool = false);
	void slot_offer (bool);
	void slot_decline (bool);
	void slot_cancel (bool);

	// parser
	void slot_suggest(const QString&, const QString&, const QString&, const QString&, int);
	void slot_matchcreate(const QString&, const QString&);
	void slot_notopen(const QString&);
	void slot_komirequest(const QString&, int, float, bool);
	void slot_opponentopen(const QString&);
	void slot_dispute(const QString&, const QString&);

private:
	bool have_suggestdata;
	QString h19, h13, h9;
	QString k19, k13, k9;
	GSName  gsname;
	QString oppRk;
	QString myRk;
	QString myName;
	bool komi_request;
	bool is_nmatch;
	QButtonGroup buttongroup;
};

#endif
