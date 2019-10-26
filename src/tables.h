/*
 *   tables.h
 */

#ifndef TABLES_H
#define TABLES_H

#include "gs_globals.h"
#include "parser.h"
#include "ui_talk_gui.h"
#include <QString>
#include <QObject>

//-----------

class Channel
{
private:
	int     nr;
	QString title;
	QString users;
	int     count;

public:
	Channel(int nr_, const QString &title_=QString(), const QString &users_=QString(), int count_=0)
	{
		nr = nr_;
		title = title_;
		users = users_;
		count = count_;
	}
	~Channel() {};
	int     get_nr() const { return nr; }
	QString get_title() const { return title; }
	QString get_users() const { return users; }
	int     get_count() const { return count; }
	void    set_channel(int nr_, const QString &title_, const QString &users_=QString(), int count_=0)
	{
		if (this->nr == nr_)
		{
			if (!title_.isNull ())
				this->title = title_;
			if (!users_.isNull ())
				this->users = users_;
			if (count_)
				this->count = count_;
		}
	}

	// operators <, ==
	int operator== (Channel h)
		{ return (this->get_nr() == h.get_nr()); };
	bool operator< (Channel h)
		{ return (this->get_nr() < h.get_nr()); };
};

typedef QList<Channel *> ChannelList;

//-----------

class Talk : public QDialog, public Ui::TalkGui
{
	Q_OBJECT

	QString m_name;

public:
	Talk (const QString&, QWidget*, bool isplayer = true);

	QString get_name () const { return m_name; }
	void set_name (QString &n) { m_name = n; }
	void write (const QString &text) const;

	bool lineedit_has_focus ();
	void append_to_mle (const QString &);

public slots:
	void slot_returnPressed ();
	void slot_match ();

signals:
	void signal_talkto(QString&, QString&);
	void signal_pbRelOneTab(QWidget*);
};

//-----------

class Update_Locker
{
	QWidget *m_w;
public:
	Update_Locker (QWidget *w) : m_w (w)
	{
		m_w->setUpdatesEnabled (false);
	}
	~Update_Locker ()
	{
		m_w->setUpdatesEnabled (true);
	}
};

#endif

