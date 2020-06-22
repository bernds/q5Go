/*
 *   playertable.h
 */

#ifndef PLAYERTABLE_H
#define PLAYERTABLE_H

#include "tables.h"

#include <QVariant>
#include <QTreeView>

struct Player
{
	// #> Info Name Idle Rank | Info Name Idle Rank 
	QString info;
	QString name;
	QString idle;
	QString rank;
	QString play_str;
	QString obs_str;
	QString extInfo;
	QString won;
	QString lost;
	QString country;
	QString nmatch_settings;
  	QString rated;
	QString address;
	QString mark;
	QString sort_rk;

	QString playing;
	QString observing;

	bool nmatch;
	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri;
	int nmatch_handicapMin, nmatch_handicapMax;
	int nmatch_timeMin, nmatch_timeMax;
	int nmatch_BYMin, nmatch_BYMax;
	int nmatch_stonesMin, nmatch_stonesMax;
	int nmatch_KoryoMin, nmatch_KoryoMax;

	bool has_nmatch_settings () { return nmatch_settings != "No match conditions"; }
	bool operator== (const Player &other) const
	{
		return (info == other.info && name == other.name && idle == other.idle
			&& rank == other.rank && play_str == other.play_str && obs_str == other.obs_str
			&& extInfo == other.extInfo && won == other.won && lost == other.lost
			&& country == other.country && nmatch_settings == other.nmatch_settings
			&& rated == other.rated && address == other.address && mark == other.mark
			&& playing == other.playing && observing == other.observing);
	}

	QString column (int c) const;
	QString unique_column () const;
	static QString header_column (int c);
	QVariant foreground () const;

	int compare (const Player &other, int column) const;
	static bool justify_right (int column);
 	static int column_count () { return 12; }
};

class PlayerTable : public QTreeView
{
	Q_OBJECT
	table_model<Player> *m_model;

public:
	PlayerTable(QWidget* parent = 0);

	void set_model (table_model<Player> *m)
	{
		m_model = m;
		setModel (m);
	}

//	virtual void setSorting ( int column, bool ascending = TRUE );
	void showOpen (bool show);

	void resize_columns ()
	{
		int count = m_model->columnCount ();
		for (int i = 0; i < count; i++)
			resizeColumnToContents (i);
	}

private:
	void mouseDoubleClickEvent (QMouseEvent *e);

public slots:
	virtual void slot_mouse_players(const QPoint&) {};

signals:
	void signal_doubleClicked (const Player &);
};

#endif // PLAYERTABLE_H
