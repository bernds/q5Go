/*
 *   playertable.cpp
 */

#include <QHeaderView>
#include <QMouseEvent>

#include "playertable.h"
#include "misc.h"
#include "setting.h"


PlayerTable::PlayerTable(QWidget *parent)
	: QTreeWidget (parent)
{
	setColumnCount (12);

	QStringList headers;
	headers <<  tr ("Stat") << tr ("Name") << tr ("Rk") << tr ("pl") << tr ("ob") << tr ("Idle") << tr ("X") << tr ("Info") << tr ("Won") << tr ("Lost") << tr ("Country") << tr ("Match prefs");
	setHeaderLabels (headers);
	for (int i = 0; i < 12; i++)
		header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);

	setFocusPolicy (Qt::NoFocus);
	setContextMenuPolicy (Qt::CustomContextMenu);

	setUniformRowHeights (true);
	setAlternatingRowColors (true);

	// set sorting order for players by rank
	setAllColumnsShowFocus (true);
	sortItems (2, Qt::AscendingOrder);
}

void PlayerTable::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->button () == Qt::LeftButton) {
		QTreeWidgetItem *item = itemAt (e->pos ());
		if (item)
			emit signal_doubleClicked (item);
	}
}

void PlayerTable::showOpen(bool checked)
{
	QTreeWidgetItemIterator lv(this);

	for (; *lv; lv++)
	{
		// player is not open or is playing a match
		if ((*lv)->text(0).contains('X') || !(*lv)->text(3).contains('-'))
			(*lv)->setHidden (checked);
		else
			(*lv)->setHidden (false);
	}
}

/*
 *   PlayerTableItem
 */
#if 0
PlayerTableItem::PlayerTableItem(PlayerTable *parent, const char* name)
	: QTreeWidgetItem(parent, name)
{
;
}

PlayerTableItem::PlayerTableItem(PlayerTableItem *parent, const char* name)
	: QTreeWidgetItem(parent, name)
{
;
}
#endif

PlayerTableItem::PlayerTableItem(PlayerTable *parent, const Player &p)
	: QTreeWidgetItem(parent), m_p (p)
{
	setTextAlignment(3, Qt::AlignRight);
	setTextAlignment(4, Qt::AlignRight);
	setTextAlignment(5, Qt::AlignRight);
	setTextAlignment(6, Qt::AlignRight);
	setTextAlignment(7, Qt::AlignRight);
	setTextAlignment(8, Qt::AlignRight);
	setTextAlignment(9, Qt::AlignRight);

	ownRepaint ();
	seeking = false;
}

PlayerTableItem::~PlayerTableItem()
{
}

QVariant PlayerTableItem::data (int column, int role) const
{
	if (role == Qt::ForegroundRole) {
		if (its_me)
			return QBrush (Qt::blue);
		else if (watched)
			return QBrush (Qt::darkGreen);
		else if (!open)
			return QBrush (Qt::gray);
		else if (exclude)
			return QBrush (Qt::red);
		else if (seeking)
			return QBrush (Qt::magenta);
		return QVariant ();
	} else if (role == Qt::TextAlignmentRole) {
		return column < 3 ? Qt::AlignLeft : Qt::AlignRight;
	}

	if (role != Qt::DisplayRole)
		return QVariant ();
	switch (column) {
	case 0: return m_p.info;
	case 1: return m_p.name;
	case 2: return m_p.rank;
	case 3: return m_p.play_str;
	case 4: return m_p.obs_str;
	case 5: return m_p.idle;
	case 6: return m_p.mark;
	case 7: return m_p.extInfo;
	case 8: return m_p.won;
	case 9: return m_p.lost;
	case 10: return m_p.country;
	case 11: return m_p.nmatch_settings;
	case 12: return m_p.sort_rk;
	default: return QVariant ();
	}
}

void PlayerTableItem::ownRepaint()
{
	open = m_p.info.contains('X') == 0;
	watched = m_p.mark.contains ('W');
	its_me = m_p.mark.contains ('M');
	exclude = m_p.mark.contains ('X');
}

bool PlayerTableItem::operator<(const QTreeWidgetItem &other) const
{
	int column = treeWidget()->sortColumn();
	int col_adj = column == 2 ? 12 : column;
	const QString &t1 = text (col_adj);
	const QString &t2 = other.text (col_adj);

	return t1 < t2;
}

#if 0
void PlayerTableItem::set_nmatchSettings(Player *p)
{
	nmatch = p->nmatch;

	nmatch_black = 		p->nmatch_black ;
	nmatch_white = 		p->nmatch_white;
	nmatch_nigiri = 	p->nmatch_nigiri ;
	nmatch_handicapMin = 	p->nmatch_handicapMin;
	nmatch_handicapMax  = 	p->nmatch_handicapMax;
	nmatch_timeMin = 	p->nmatch_timeMin;
	nmatch_timeMax  = 	p->nmatch_timeMax;
	nmatch_BYMin = 		p->nmatch_BYMin;
	nmatch_BYMax = 		p->nmatch_BYMax;
	nmatch_stonesMin = 	p->nmatch_stonesMin;
	nmatch_stonesMax = 	p->nmatch_stonesMax;
	nmatch_KoryoMin = 	p->nmatch_KoryoMin;
	nmatch_KoryoMax = 	p->nmatch_KoryoMax ;

	nmatch_settings =  !(p->nmatch_settings == "No match conditions");
}
#endif
