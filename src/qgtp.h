/***************************************************************************
                          qgtp.h  -  description
                             -------------------
    begin                : Thu Nov 29 2001
    copyright            : (C) 2001 by PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGTP_H
#define QGTP_H

#include <qapplication.h>
#include <QProcess>

#include "textview.h"
//#include "global.h"

class gtp_exception : public std::exception
{
public:
	virtual QString errmsg () const = 0;
};

class process_timeout : public gtp_exception
{
 public:
  process_timeout () { }
  QString errmsg () const { return QObject::tr ("Program timed out"); }
};

class invalid_response : public gtp_exception
{
 public:
  invalid_response () { }
  QString errmsg () const { return QObject::tr ("Invalid response from program"); }
};

#define IGTP_BUFSIZE 2048    /* Size of the response buffer */
#define OK 0
#define FAIL -1

class QGtp;

/* @@@ This is here just temporarily, until the new go board code lands.  */
enum stone_color { white, black, none };

class Gtp_Controller
{
	friend class QGtp;
	QWidget *m_parent;

public:
	Gtp_Controller (QWidget *p) : m_parent (p) { }
	QGtp *create_gtp (const QString &program, int size, double komi, int hc, int level);
	virtual void gtp_played_move (int x, int y) = 0;
	virtual void gtp_played_pass () = 0;
	virtual void gtp_played_resign () = 0;
	virtual void gtp_startup_success () = 0;
	virtual void gtp_exited () = 0;
	virtual void gtp_failure (const gtp_exception &) = 0;
};

/**
  *@author PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
  */

class QGtp : public QObject
{
	Q_OBJECT

	QProcess *m_process;
	TextView m_dlg;
	Gtp_Controller *m_controller;

	int m_size;
	double m_komi;
	int m_hc;
	int m_level;

public slots:
	void slot_started ();
	void slot_finished (int exitcode, QProcess::ExitStatus status);
	void slot_error (QProcess::ProcessError);
	void slot_receive_move ();
	void slot_startup_messages ();
	void slot_startup_part2 ();
	void slot_abort_request (bool);

// void slot_stateChanged(QProcess::ProcessState);

public:
	QGtp(QWidget *parent, Gtp_Controller *c, const QString &prog, int size, float komi, int hc, int level);
	~QGtp();

	void request_move (stone_color col);
	void played_move (stone_color col, int x, int y);
	void played_move_pass (stone_color col);
	void played_move_resign (stone_color col);

	/****************************
 	* Administrative commands. *
 	****************************/
	void quit ();

	/****************************
 	* Program identity.        *
 	****************************/
	QString name ();
	int protocolVersion ();
	QString version ();

	/***************************
 	* Setting the board size. *
 	***************************/
	void setBoardsize (int size);

	/***************************
	* Clearing the board.     *
 	***************************/
	void clearBoard ();

	/************
 	* Setting. *
 	************/
	void setKomi (float k);
	void setLevel (int level);

	/******************
 	* Playing moves. *
 	******************/
	void playblack (char c, int j);
	void playblackPass ();
	void playwhite (char c, int j);
	void playwhitePass ();
	void fixedHandicap (int handicap);

	bool knownCommand(QString s);

private:

	/* Number of the request */
	int req_cnt;

	void send_request(const QString &);
	QString waitResponse();
	QString getResponse();
};




#endif
