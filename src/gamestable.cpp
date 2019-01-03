/*
 *  gamestable.cpp
 */

#include <QHeaderView>
#include <QMouseEvent>
#include "gamestable.h"
#include "misc.h"
#include "setting.h"

GamesTable::GamesTable (QWidget *parent)
	: QTreeWidget(parent)
{
	setColumnCount (12);

	headers <<  tr ("Id") << tr ("White") << tr ("WR") << tr ("Black") << tr ("BR") << tr ("Mv") << tr ("Sz") << tr ("H") << tr ("K") << tr ("By") << tr ("FR") << tr ("Ob");
	setHeaderLabels (headers);
	for (int i = 0; i < 12; i++)
		header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);

	setFocusPolicy (Qt::NoFocus);
	setContextMenuPolicy (Qt::CustomContextMenu);
	setAllColumnsShowFocus(true);

	setUniformRowHeights (true);
	setAlternatingRowColors (true);
	//setItemMargin(2);
	sortItems (2, Qt::AscendingOrder);
}

GamesTable::~GamesTable ()
{
}

void GamesTable::mouseDoubleClickEvent (QMouseEvent *e)
{
	printf ("doubleclick\n");
	if (e->button () == Qt::LeftButton) {
		QTreeWidgetItem *item = itemAt (e->pos ());
		printf ("item %p\n", item);
		if (item)
			emit signal_doubleClicked (item);
	}
}

/*
 *   GamesTableItem
 */

GamesTableItem::GamesTableItem (GamesTable *parent, const Game &g)
	: QTreeWidgetItem(parent), m_game (g)
{
	ownRepaint ();
}

GamesTableItem::~GamesTableItem()
{
}

QVariant GamesTableItem::data (int column, int role) const
{
	if (role == Qt::ForegroundRole) {
		if (its_me)
			return QBrush (Qt::blue);
		else if (watched)
			return QBrush (Qt::red);
		return QVariant ();
	} else if (role == Qt::TextAlignmentRole) {
		return column < 5 && column != 0 ? Qt::AlignLeft : Qt::AlignRight;
	}
	if (role != Qt::DisplayRole)
		return QVariant ();
	switch (column) {
	case 0: return m_game.nr;
	case 1: return m_game.wname;
	case 2: return m_game.wrank;
	case 3: return m_game.bname;
	case 4: return m_game.brank;
	case 5: return m_game.mv;
	case 6: return m_game.Sz;
	case 7: return m_game.H;
	case 8: return m_game.K;
	case 9: return m_game.By;
	case 10: return m_game.FR;
	case 11: return m_game.ob;
	case 12: return m_game.sort_rk_w;
	case 14: return m_game.sort_rk_b;
	default: return QVariant ();
	}
}

void GamesTableItem::ownRepaint()
{
	watched = false;
	its_me = false;
	if (!m_game.sort_rk_w.isEmpty ())
	{
		its_me = m_game.sort_rk_w.at(0) == 'A';
#if 0 /* @@@ None of this works?  */
		if (text(7).isEmpty())
			watched = false;
		else
			watched = text(12).at(text(7).length()-1) == 'W';
#endif
	}
}

bool GamesTableItem::operator<(const QTreeWidgetItem &other) const
{
	int column = treeWidget()->sortColumn();
	int adj_col = column == 2 || column == 4 ? column + 10 : column;
	const QString t1 = text (adj_col).trimmed ();
	const QString t2 = other.text (adj_col).trimmed ();

	if (column == 2 || column == 4)
		return t1 < t2;

	const QString r1 = m_game.sort_rk_w;
	const QString r2 = other.text (12);

	if (column == 0 || column == 5 || column == 6 || column == 7 || column == 9 || column == 11) {
		// id, move, observe, Sz, H, By, Ob
		return t1.rightJustified (8, '0') + r1 < t2.rightJustified (8, '0') + r2;
	} else
		return t1 + r1 < t2 + r2;
}
