#include <QHeaderView>
#include <QMouseEvent>

#include "playertable.h"

QString Player::column (int column) const
{
	switch (column) {
	case 0: return info;
	case 1: return name;
	case 2: return rank;
	case 3: return play_str;
	case 4: return obs_str;
	case 5: return idle;
	case 6: return mark;
	case 7: return extInfo;
	case 8: return won;
	case 9: return lost;
	case 10: return country;
	case 11: return nmatch_settings;
	default: return QString ();
	}
}

QString Player::header_column (int c)
{
	switch (c) {
	case 0: return QObject::tr ("Stat");
	case 1: return QObject::tr ("Name");
	case 2: return QObject::tr ("Rk");
	case 3: return QObject::tr ("pl");
	case 4: return QObject::tr ("ob");
	case 5: return QObject::tr ("Idle");
	case 6: return QObject::tr ("X");
	case 7: return QObject::tr ("Info");
	case 8: return QObject::tr ("Won");
	case 9: return QObject::tr ("Lost");
	case 10: return QObject::tr ("Country");
	case 11: return QObject::tr ("Match prefs");
	default: return QString ();
	}
}

QVariant Player::foreground () const
{
	bool open = info.contains('X') == 0;
	bool watched = mark.contains ('W');
	bool its_me = mark.contains ('M');
	bool exclude = mark.contains ('X');

	if (its_me)
		return QBrush (Qt::blue);
	else if (watched)
		return QBrush (Qt::darkGreen);
	else if (!open)
		return QBrush (Qt::gray);
	else if (exclude)
		return QBrush (Qt::red);
	return QVariant ();
}

int Player::compare (const Player &other, int c) const
{
	/* Show watched players at the top of the list.  */
	if (c != 6) {
		int v = other.mark.contains ('W') - mark.contains ('W');
		if (v != 0)
			return v;
	}
	if (c == 2)
		return sort_rk.compare (other.sort_rk);
	else
		return column (c).compare (other.column (c));
}

bool Player::justify_right (int column)
{
	return column >= 3;
}

QString Player::unique_column () const
{
	return name;
}

PlayerTable::PlayerTable (QWidget *parent)
	: QTreeView (parent)
{
	setSortingEnabled (true);
	header ()->setSectionsMovable (false);
	setItemsExpandable (false);
	setRootIsDecorated (false);

	setFocusPolicy (Qt::NoFocus);
	setContextMenuPolicy (Qt::CustomContextMenu);

	setUniformRowHeights (true);
	setAlternatingRowColors (true);

	// set sorting order for players by rank
	setAllColumnsShowFocus (true);
}

void PlayerTable::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->button () != Qt::LeftButton)
		return;

	QModelIndex idx = indexAt (e->pos ());
	if (!idx.isValid ())
		return;
	const Player *p = m_model->find (idx);
	emit signal_doubleClicked (*p);
}
