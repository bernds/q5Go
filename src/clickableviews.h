#ifndef CLICKABLEVIEWS_H
#define CLICKABLEVIEWS_H

#include <QListView>
#include <QTableView>

class ClickableListView: public QListView
{
	Q_OBJECT

signals:
	void doubleclicked ();
	void current_changed ();

protected:
	virtual void mouseDoubleClickEvent (QMouseEvent *) override
	{
		emit doubleclicked ();
	}
	virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous) override
	{
		QListView::currentChanged (current, previous);
		emit current_changed ();
	}
public:
	ClickableListView (QWidget *parent) : QListView (parent)
	{
	}
};

class ClickableTableView: public QTableView
{
	Q_OBJECT

signals:
	void doubleclicked ();
protected:
	virtual void mouseDoubleClickEvent (QMouseEvent *) override
	{
		emit doubleclicked ();
	}
public:
	ClickableTableView (QWidget *parent) : QTableView (parent)
	{
	}
};

#endif
