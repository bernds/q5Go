/*
 *   playertable.h
 */

#ifndef PLAYERTABLE_H
#define PLAYERTABLE_H

#include "tables.h"

#include <qvariant.h>
#include <q3listview.h>


class PlayerTable : public Q3ListView
{ 
	Q_OBJECT

public:
	PlayerTable(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0);
	~PlayerTable() {};
//	virtual void setSorting ( int column, bool ascending = TRUE );
	void showOpen(bool show);

public slots:
	virtual void slot_mouse_players(int, Q3ListViewItem*, const QPoint&, int) {};
	
	
};


class PlayerTableItem : public Q3ListViewItem
{ 
public:

	PlayerTableItem(PlayerTable *parent, const char *name = 0);
	PlayerTableItem(PlayerTableItem *parent, const char *name = 0);
	PlayerTableItem(PlayerTable *parent, QString label1, QString label2 = QString::null,
		QString label3 = QString::null, QString label4 = QString::null,
		QString label5 = QString::null, QString label6 = QString::null,
		QString label7 = QString::null, QString label8 = QString::null,
		QString label9 = QString::null, QString label10 = QString::null,
		QString label11 = QString::null, QString label12 = QString::null,
		QString label13 = QString::null);
	~PlayerTableItem();

	void ownRepaint();
	void replace() ;
	void set_nmatchSettings(Player *p);

	bool nmatch;

	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri, nmatch_settings;
	int 	nmatch_handicapMin, nmatch_handicapMax, 
		nmatch_timeMin, nmatch_timeMax, 
		nmatch_BYMin, nmatch_BYMax, 
		nmatch_stonesMin, nmatch_stonesMax,
		nmatch_KoryoMin, nmatch_KoryoMax;

	//bool isOpen() {return open;}

protected:
//	virtual QString key(int, bool) const;
	virtual int compare( Q3ListViewItem *p, int col, bool ascending ) const;
	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

	bool open;
	bool watched;
	bool exclude;
	bool its_me;
	bool seeking;

};

#endif // PLAYERTABLE_H
