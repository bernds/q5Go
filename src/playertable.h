/*
 *   playertable.h
 */

#ifndef PLAYERTABLE_H
#define PLAYERTABLE_H

#include "tables.h"

#include <QVariant>
#include <QTreeWidget>


class PlayerTable : public QTreeWidget
{
	Q_OBJECT

public:
	PlayerTable(QWidget* parent = 0);
	~PlayerTable() {};
//	virtual void setSorting ( int column, bool ascending = TRUE );
	void showOpen(bool show);

	void resize_columns ()
	{
#if 0
		for (int i = 0; i < columnCount (); i++)
			resizeColumnToContents (i);
#endif
	}

private:
	void mouseDoubleClickEvent(QMouseEvent *e);

public slots:
	virtual void slot_mouse_players(const QPoint&) {};

 signals:
	void signal_doubleClicked (QTreeWidgetItem *);
};

class Player
{
public:
	Player() {};
	~Player() {};
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

	int     playing;
	int     observing;
	bool 	nmatch;
	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri;
	int 	nmatch_handicapMin, nmatch_handicapMax, 
		nmatch_timeMin, nmatch_timeMax, 
		nmatch_BYMin, nmatch_BYMax, 
		nmatch_stonesMin, nmatch_stonesMax,
		nmatch_KoryoMin, nmatch_KoryoMax;

	bool has_nmatch_settings () { return nmatch_settings != "No match conditions"; }
};

class PlayerTableItem : public QTreeWidgetItem
{
	Player m_p;
	QString m_rk;
public:

	PlayerTableItem(PlayerTable *parent, const Player &);
	~PlayerTableItem();

	void update_player (const Player &p) { m_p = p; ownRepaint (); emitDataChanged (); }
	Player get_player () const { return m_p; }
	void ownRepaint();
	void replace() ;

	virtual QVariant data (int column, int role) const override;
	virtual bool operator< (const QTreeWidgetItem &other) const override;
private:
	void set_foreground (const QBrush &col)
	{
		QBrush b (col);
		for (int i = 0; i < columnCount (); i++)
			setForeground (i, b);
	}

protected:
//	virtual QString key(int, bool) const;
//	virtual bool operator<(const QListWidgetItem *other) const;
//	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

	bool open;
	bool watched;
	bool exclude;
	bool its_me;
	bool seeking;
};

#endif // PLAYERTABLE_H
