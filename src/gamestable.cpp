/*
 *  gamestable.cpp
 */

#include <QHeaderView>
#include <QMouseEvent>
#include "gamestable.h"
#include "misc.h"
#include "setting.h"

QString Game::column (int c) const
{
	switch (c) {
	case 0: return nr;
	case 1: return wname;
	case 2: return wrank;
	case 3: return bname;
	case 4: return brank;
	case 5: return mv;
	case 6: return Sz;
	case 7: return H;
	case 8: return K;
	case 9: return By;
	case 10: return FR;
	case 11: return ob;
	default: return QString ();
	}
}

QString Game::header_column (int c)
{
	switch (c) {
	case 0: return QObject::tr ("Id");
	case 1: return QObject::tr ("White");
	case 2: return QObject::tr ("WR");
	case 3: return QObject::tr ("Black");
	case 4: return QObject::tr ("BR");
	case 5: return QObject::tr ("Mv");
	case 6: return QObject::tr ("Sz");
	case 7: return QObject::tr ("H");
	case 8: return QObject::tr ("K");
	case 9: return QObject::tr ("By");
	case 10: return QObject::tr ("FR");
	case 11: return QObject::tr ("Ob");
	default: return QString ();
	}
}

QVariant Game::foreground () const
{
#if 0
	if (its_me)
		return QBrush (Qt::blue);
	else if (watched)
		return QBrush (Qt::red);
#endif
	return QVariant ();
}

int Game::compare (const Game &other, int c) const
{
	switch (c) {
	case 0:	case 5: case 6: case 7: case 9: case 11:
	{
		/* Numerical compare.  */
		QString a = column (c);
		QString b = other.column (c);
		int r1 = a.length () - b.length ();
		if (r1 != 0)
			return r1;
		return a.compare (b);
	}
	case 2:
		return sort_rk_w.compare (other.sort_rk_w);
	case 4:
		return sort_rk_b.compare (other.sort_rk_b);
	default:
		return column (c).compare (other.column (c));
	}
}

bool Game::justify_right (int column)
{
	return column == 0 || column >= 5;
}

QString Game::unique_column () const
{
	return nr;
}

GamesTable::GamesTable (QWidget *parent)
	: QTreeView (parent)
{
	setSortingEnabled (true);
	header ()->setSectionsMovable (false);
	setItemsExpandable (false);
	setRootIsDecorated (false);

	setFocusPolicy (Qt::NoFocus);
	setContextMenuPolicy (Qt::CustomContextMenu);
	setAllColumnsShowFocus(true);

	setUniformRowHeights (true);
	setAlternatingRowColors (true);
}

void GamesTable::mouseDoubleClickEvent (QMouseEvent *e)
{
	if (e->button () != Qt::LeftButton)
		return;

	QModelIndex idx = indexAt (e->pos ());
	if (!idx.isValid ())
		return;
	const Game *g = m_model->find (idx);
	emit signal_doubleClicked (*g);
}
