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

#if 0
class GamesTableItem : public QTreeWidgetItem
{
	Game m_game;
public:
	GamesTableItem(GamesTable *parent, const Game &g);
	~GamesTableItem();
	Game get_game () { return m_game; }
	void update_game (const Game &g)
	{
		m_game = g;
		ownRepaint ();
		emitDataChanged ();
		m_game.up_to_date = true;
	}
	bool is_up_to_date ()
	{
		return m_game.up_to_date;
	}
	void clear_up_to_date ()
	{
		m_game.up_to_date = false;
	}

	void ownRepaint();
	virtual bool operator< (const QTreeWidgetItem &other) const override;
	virtual QVariant data (int column, int role) const override;
 private:
	void set_foreground (const QColor &col)
	{
		QBrush b (col);
		for (int i = 0; i < columnCount (); i++)
			setForeground (i, b);
	}

protected:
#if 0
	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);
#endif

	bool watched;
	bool its_me;
};
#endif

#endif // GAMESTABLE_H
