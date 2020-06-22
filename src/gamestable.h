/*
 *   gamestable.h
 */

#ifndef GAMESTABLE_H
#define GAMESTABLE_H

#include <QVariant>
#include <QDialog>
#include <QTreeView>

#include "tables.h"

struct Game
{
	// #> [##] white name [ rk ] black name [ rk ] (Move size H Komi BY FR) (###) 
	QString nr;
	QString	wname;
	QString	wrank;
	QString	bname;
	QString	brank;
	QString	mv;
	QString Sz;
	QString H;
	QString K;
	QString By;
	QString FR;
	QString ob;
	QString sort_rk_w, sort_rk_b;

	bool operator== (const Game &other) const
	{
		return (nr == other.nr && wname == other.wname && wrank == other.wrank
			&& bname == other.bname && brank == other.brank
			&& bname == other.bname && brank == other.brank
			&& mv == other.mv && Sz == other.Sz && H == other.H && K == other.K
			&& By == other.By && FR == other.FR && ob == other.ob);
	}

	QString column (int c) const;
	QString unique_column () const;
	QVariant foreground () const;
	int compare (const Game &other, int column) const;

	static QString header_column (int c);
	static bool justify_right (int column);
	static int column_count () { return 12; }
};

class GamesTable : public QTreeView
{
	Q_OBJECT
	table_model<Game> *m_model;

public:
	GamesTable (QWidget* parent = 0);

	void set_model (table_model<Game> *m)
	{
		m_model = m;
		setModel (m);
	}
	void set_watch (QString);
	void set_mark (QString);

	void resize_columns ()
	{
		int count = m_model->columnCount ();
		for (int i = 0; i < count; i++)
			resizeColumnToContents (i);
	}

private:
	void mouseDoubleClickEvent (QMouseEvent *e);

public slots:
	virtual void slot_mouse_games (const QPoint &) {};

signals:
	void signal_doubleClicked (const Game &);
};

#endif // GAMESTABLE_H
