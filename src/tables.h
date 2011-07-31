/*
 *   tables.h
 */

#ifndef TABLES_H
#define TABLES_H

#include "gs_globals.h"
#include "parser.h"
#include "talk_gui.h"
#include <qstring.h>
#include <qobject.h>
#include <qlistview.h>
//#include <qmultilineedit.h>
#include <qtextedit.h>        //eb16
#include <qlineedit.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qwidget.h>
#include <qpalette.h>

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
			if (title_)
				this->title = title_;
			if (users_)
				this->users = users_;
			if (count_)
				this->count = count_;
		}
	}

	// operators <, ==
	int operator== (Channel h)
		{ return (this->get_nr() == h.get_nr()); };
	int operator== (Channel *h)
		{ return (this->get_nr() == h->get_nr()); };
	bool operator< (Channel h)
		{ return (this->get_nr() < h.get_nr()); };
	bool operator< (Channel *h)
		{ return (this->get_nr() < h->get_nr()); };
};

class ChannelList : public QPtrList<Channel>
{
public:
	ChannelList() {};
	~ChannelList() {};
	
	virtual int compareItems(Item d1, Item d2);
};

//-----------

class Talk : public TalkGui
{
	Q_OBJECT

private:
	QString        name;
//	QTextEdit      *MultiLineEdit1;     //eb16
//	QLineEdit      *LineEdit1;
//	QGridLayout    *TalkDialogWidgetLayout;
//	QPushButton    *pb_releaseTalkTab;
//	QWidget        *widget;
//	QBoxLayout     *buttonLayout;
	static int     counter;

public:
	Talk(const QString&, QWidget*, bool isplayer = true);
	~Talk();
	QTextEdit      *get_mle() const { return MultiLineEdit1; } //eb16
	QLineEdit      *get_le() const {return LineEdit1; }
	QWidget        *get_tabWidget()  { return this; }
//	QPushButton    *get_pb() const { return pb_releaseTalkTab; }
	QString        get_name() const { return name; }
	void           set_name(QString &n) { name = n; }
	void           write(const QString &text = QString()) const;
	bool           pageActive;
	void           setTalkWindowColor(QPalette pal);

public slots:
	void slot_returnPressed();
	void slot_pbRelTab();
  void slot_match();

signals:
	void signal_talkto(QString&, QString&);
	void signal_pbRelOneTab(QWidget*);
  void signal_matchrequest(const QString&,bool);
};

//-----------

class Host
{
public:
	Host(const QString&, const QString&, const unsigned int, const QString&, const QString&, const QString&);
	~Host() {};
	QString title() const { return t; };
	QString host() const { return h; };
	unsigned int port() const { return pt; };
	QString loginName() const { return lg; };
	QString password() const { return pw; };
	QString codec() const { return cdc; };
	// operators <, ==
	int operator== (Host h)
		{ return (this->title() == h.title()); };
	int operator== (Host *h)
		{ return (this->title() == h->title()); };
	bool operator< (Host h)
		{ return (this->title() < h.title()); };
	bool operator< (Host *h)
		{ return (this->title() < h->title()); };

private:
	QString t;
	QString h;
	QString lg;
	QString pw;
	unsigned int pt;
	QString cdc;
};

class HostList : public QPtrList<Host>
{
public:
	HostList() {};
	~HostList() {};
	
	virtual int compareItems(Item d1, Item d2);
};

//-----------

class Player
{
public:
	Player() {};
	~Player() {};
	// #> Info Name Idle Rank | Info Name Idle Rank 
	QString info;
	QString name;
	QString idle;
	QString rank;
	QString play_str;
	QString obs_str;
	QString extInfo;
	QString won;
	QString lost;
	QString country;
	QString nmatch_settings;
  	QString rated;
	QString address;
	int     playing;
	int     observing;
	bool 	nmatch;
	bool    online;
	// BWN 0-9 19-19 60-60 600-600 25-25 0-0 0-0 0-0
	bool nmatch_black, nmatch_white, nmatch_nigiri;
	int 	nmatch_handicapMin, nmatch_handicapMax, 
		nmatch_timeMin, nmatch_timeMax, 
		nmatch_BYMin, nmatch_BYMax, 
		nmatch_stonesMin, nmatch_stonesMax,
		nmatch_KoryoMin, nmatch_KoryoMax;

};

//-----------

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
	bool    running;
  bool oneColorGo ;
};

//-----------

class Account
{
public:
	Account(QWidget*);
	~Account();
	void set_caption();
	void set_gsname(GSName);
	void set_offline();
	void set_accname(QString&);
	void set_status(Status);
	void set_rank(QString &rk) { rank = rk; }
	QString get_rank() { return rank; }
	Status  get_status();
	GSName  get_gsname();

	GSName  gsName;
	QString svname;
	QString acc_name;
	Status  status;
	int     num_players;
	int     num_games;
	int     num_watchedplayers;
	int     num_observedgames;

private:
	QString rank;
	QString line;
	QString standard;
	QWidget *parent;
};


#endif

