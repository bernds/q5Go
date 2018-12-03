/*
 *   gamestable.h
 */

#ifndef GAMESTABLE_H
#define GAMESTABLE_H

#include <QVariant>
#include <QDialog>
#include <Q3ListView>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
class Q3VBoxLayout; 
class Q3HBoxLayout; 
class Q3GridLayout; 
class Q3ListViewItem;
class QPushButton;

class GamesTable : public Q3ListView
{ 
	Q_OBJECT

public:
	GamesTable(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
	~GamesTable();
	void set_watch(QString);
	void set_mark(QString);

public slots:
	virtual void slot_mouse_games(int, Q3ListViewItem*, const QPoint&, int) {};
};

class GamesTableItem : public Q3ListViewItem
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
