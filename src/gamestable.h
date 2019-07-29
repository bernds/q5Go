/*
 *   gamestable.h
 */

#ifndef GAMESTABLE_H
#define GAMESTABLE_H

#include <QVariant>
#include <QDialog>
#include <QTreeWidget>

class Game
{
public:
	Game() {};
	~Game() {};
	// #> [##] white name [ rk ] black name [ rk ] (Move size H Komi BY FR) (###) 
	QString nr;
	QString	wname;
	QString	wrank;
	QString	bname;
	QString	brank;
//	QString status;
	QString	mv;
	QString Sz;
	QString H;
	QString K;
	QString By;
	QString FR;
	QString ob;
	QString sort_rk_w, sort_rk_b;
	bool running;
	bool oneColorGo;

	bool up_to_date;
};

class GamesTable : public QTreeWidget
{
	Q_OBJECT
	QStringList headers;

public:
	GamesTable(QWidget* parent = 0);
	~GamesTable();
	void set_watch(QString);
	void set_mark(QString);


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
	virtual void slot_mouse_games(const QPoint&) {};

 signals:
	void signal_doubleClicked (QTreeWidgetItem *);
};

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

#endif // GAMESTABLE_H
