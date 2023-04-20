/*
 *   gamedialog.h
 */

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

#include "common.h"
#include <QButtonGroup>

#include "gs_globals.h"
#include "defines.h"

namespace Ui
{
	class NewGameDialog;
};

class GameDialog : public QDialog
{
	Q_OBJECT

	void swap_colors ();
	void clear_warnings ();

public:
	// Public because legacy code in clientwin wants to access it.
	std::unique_ptr<Ui::NewGameDialog> ui;

	GameDialog(QWidget* parent, GSName, const QString &, const QString &, const QString &, const QString &);
	~GameDialog();
	void set_is_nmatch (bool b);

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
	QString myName;
	QString myRk;
	QString oppRk;
	bool komi_request;
	bool is_nmatch;
	QButtonGroup buttongroup;
};

#endif
