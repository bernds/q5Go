/*
 *   gamedialog.h
 */

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include "newgame_gui.h"
#include "gs_globals.h"
#include "defines.h"
#include "misc.h"

class GameDialog : public NewGameDialog
{ 
	Q_OBJECT

public:
	GameDialog(QWidget* parent = 0, const char* name = 0, bool modal = true, WFlags fl = 0);
	~GameDialog();
	void set_gsName(GSName g) { gsname = g; }
	void set_oppRk(QString &rk) { oppRk = rk; qDebug() << "oppRk: " << rk << std::endl; }
	void set_myRk(QString &rk) { myRk = rk; qDebug() << "myRk: " << rk << std::endl; }
	void set_myName(QString &name) { myName = name; }
	void set_is_nmatch (bool b) { is_nmatch = b; }

signals:
	void signal_sendcommand(const QString &cmd, bool localecho);
	void signal_matchsettings(const QString&, const QString&, const QString&, assessType);
	void signal_removeDialog(const QString&);

public slots:
	// pushbuttons
	virtual void slot_stats_opponent();
	virtual void slot_swapcolors();
	virtual void slot_pbsuggest();
	virtual void slot_offer(bool);
	virtual void slot_decline();
	virtual void slot_changed();
	virtual void slot_cancel();
	// parser
	void slot_suggest(const QString&, const QString&, const QString&, const QString&, int);
	void slot_matchcreate(const QString&, const QString&);
	void slot_notopen(const QString&);
	void slot_komirequest(const QString&, int, int, bool);
	void slot_opponentopen(const QString&);
	void slot_dispute(const QString&, const QString&);

private:
	bool have_suggestdata;
	QString pwhite;
	QString pblack;
	QString h19, h13, h9;
	QString k19, k13, k9;
	GSName  gsname;
	QString oppRk;
	QString myRk;
	QString myName;
	bool komi_request;
	bool is_nmatch;
};

#endif
