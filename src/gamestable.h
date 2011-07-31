/*
 *   gamestable.h
 */

#ifndef GAMESTABLE_H
#define GAMESTABLE_H

#include <qvariant.h>
#include <qdialog.h>
#include <qlistview.h>
class QVBoxLayout; 
class QHBoxLayout; 
class QGridLayout; 
class QListViewItem;
class QPushButton;

class GamesTable : public QListView
{ 
	Q_OBJECT

public:
	GamesTable(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
	~GamesTable();
	void set_watch(QString);
	void set_mark(QString);

public slots:
	virtual void slot_mouse_games(int, QListViewItem*, const QPoint&, int) {};
};

class GamesTableItem : public QListViewItem
{ 
public:

	GamesTableItem(GamesTable *parent, const char *name = 0);
	GamesTableItem(GamesTableItem *parent, const char *name = 0);
	GamesTableItem(GamesTable *parent, QString label1, QString label2 = QString::null,
		QString label3 = QString::null, QString label4 = QString::null,
		QString label5 = QString::null, QString label6 = QString::null,
		QString label7 = QString::null, QString label8 = QString::null,
		QString label9 = QString::null, QString label10 = QString::null,
		QString label11 = QString::null, QString label12 = QString::null,
		QString label13 = QString::null);
	~GamesTableItem();
	void ownRepaint();

protected:
	virtual QString key(int, bool) const;
	virtual void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment);

	bool watched;
	bool its_me;
};

#endif // GAMESTABLE_H
