/***************************************************************************
qgtp.cpp  -  description
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

#include <iostream>
//#include <unistd.h>
#include <stdlib.h>
#include <QProcess>
#include "defines.h"
#include "qgtp.h"

#ifdef Q_WS_WIN
#include <Windows.h>
#endif

QGtp *Gtp_Controller::create_gtp (const QString &prog, int size, double komi, int hc, int level)
{
	QGtp *g = new QGtp (m_parent, this, prog, size, komi, hc, level);
	return g;
}

/* Function:  open a session
* Arguments: name and path of go program
* Fails:     never
* Returns:   nothing
*/
QGtp::QGtp(QWidget *parent, Gtp_Controller *c, const QString &prog, int size, float komi, int hc, int level)
	: m_dlg (parent, TextView::type::gtp), m_controller (c), m_size (size), m_komi (komi), m_hc (hc), m_level (level)
{
	req_cnt = 1;

	m_process = new QProcess();
	m_dlg.show ();
	m_dlg.activateWindow ();

	 /* @@@ Qt5 syntax.  Since nothing uses this code at the moment, keep it commented out
	    until we've progressed to the point where we're building with Qt5.  */
#if 0
	connect (m_dlg.buttonAbort, &QPushButton::clicked, this, &QGtp::slot_abort_request);

	connect (m_process, &QProcess::started, this, &QGtp::slot_started);
	connect (m_process, &QProcess::errorOccurred, this, &QGtp::slot_error);
	// ??? Unresolved overload errors without this.
	void (QProcess::*fini)(int, QProcess::ExitStatus) = &QProcess::finished;
	connect (m_process, fini, this, &QGtp::slot_finished);
	connect (m_process, &QProcess::readyReadStandardError,
		 this, &QGtp::slot_startup_messages);
#endif

	QStringList arguments = prog.split (QRegExp ("\\s+"));
	QString filename = arguments.takeFirst();
	m_process->start (filename, arguments);

}

QGtp::~QGtp()
{
	if (m_process)
		m_process->kill();     // not very clean (should use tryTerminate)
}

void QGtp::slot_started ()
{
	connect (m_process, SIGNAL(readyReadStandardOutput()),
		 this, SLOT(slot_startup_part2()));

//	connect (m_process, SIGNAL(error()), this, SLOT(slot_later_error()) );

	send_request ("protocol_version");
	return;
}

void QGtp::slot_error (QProcess::ProcessError)
{
	m_dlg.hide ();
	m_controller->gtp_exited ();
	return;
}

void QGtp::slot_abort_request (bool)
{
	send_request ("quit");
	m_dlg.hide ();
	m_controller->gtp_exited ();
	return;
}

void QGtp::slot_startup_messages ()
{
	QString output = m_process->readAllStandardError ();
	m_dlg.append (output);

}

void QGtp::slot_startup_part2 ()
{
	disconnect (m_process, SIGNAL(readyReadStandardOutput()), 0, 0);
	// m_dlg.hide ();

	try {
		// Get the response to the protocol version.
		QString pv = getResponse ();
		if (pv != "2")
			throw invalid_response ();

		setBoardsize (m_size);
		clearBoard ();

		if (knownCommand("level"))
			setLevel (m_level);

		setKomi (m_komi);
		fixedHandicap(m_hc);
		m_controller->gtp_startup_success ();
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::slot_receive_move ()
{
	disconnect (m_process, SIGNAL (readyReadStandardOutput()), 0, 0);
	try {
		QString response = getResponse ();
		if (response.toLower () == "resign") {
			m_controller->gtp_played_resign ();
		} else if (response.toLower () == "pass") {
			m_controller->gtp_played_pass ();
		} else {
			QChar sx = response[0];
			response.remove (0, 1);

			int i = sx.toLatin1() - 'A';
			if (i > 7)
				i--;
			int j = response.toInt();
			m_controller->gtp_played_move (i, m_size - j);
		}
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::played_move (stone_color col, int x, int y)
{
	disconnect (m_process, SIGNAL (readyReadStandardOutput()), 0, 0);
	if (x >= 8)
		x++;
	try {
		if (col == black)
			playblack ('A' + x, m_size - y);
		else
			playwhite ('A' + x, m_size - y);
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::played_move_pass (stone_color col)
{
	disconnect (m_process, SIGNAL (readyReadStandardOutput()), 0, 0);
	try {
		if (col == black)
			playblackPass ();
		else
			playwhitePass ();
	} catch (gtp_exception &e) {
		m_controller->gtp_failure (e);
		quit ();
	}
}

void QGtp::request_move (stone_color col)
{
	disconnect (m_process, SIGNAL (readyReadStandardOutput()), 0, 0);
	connect (m_process, SIGNAL (readyReadStandardOutput()),
		 this, SLOT (slot_receive_move()));

	if (col == black)
		send_request ("genmove black");
	else
		send_request ("genmove white");
}

/* Code */

// exit
void QGtp::slot_finished (int exitcode, QProcess::ExitStatus status)
{
	qDebug() << req_cnt << " quit";
	m_controller->gtp_exited ();
}


/* Function:  wait for a go response
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
QString QGtp::waitResponse()
{
	m_process->waitForReadyRead ();
	return getResponse ();
}

QString QGtp::getResponse ()
{
	QString output;
	do {
		while (m_process->canReadLine ()) {
			output = m_process->readLine ();

			m_dlg.textEdit->setTextColor (Qt::red);
			m_dlg.append (output);
			m_dlg.textEdit->setTextColor (Qt::black);
			output = output.trimmed ();
			if (!output.isEmpty ())
				break;
		}
		if (output.isEmpty ()) {
			/* @@@
			   This seems to happen every now and then with Leela Zero.
			   It's unclear why, but there's little we can do except
			   wait for another round.  */
			qDebug () << "empty output from GTP program";
			m_process->waitForReadyRead ();
		}
	} while (output.isEmpty ());

	int n_tws = 0;
	int len = output.length ();
	while (len > n_tws && output[len - n_tws - 1].isSpace ())
		n_tws++;

	qDebug() << "** QGtp::getResponse(): " << output << "\n";
;
	output.chop (n_tws);
	len -= n_tws;

#if 0
	std::cout << "R: " << output.toStdString() << "\n";
#endif
	if (output[0] != '=')
		throw invalid_response ();
	output.remove (0, 1);
	len--;
	int n_digits = 0;
	while (n_digits < len && output[n_digits].isDigit ())
		n_digits++;
	while (n_digits < len && output[n_digits].isSpace ())
		n_digits++;
	QString cmd_nr = output.left (n_digits);
	output.remove (0, n_digits);

#if 0
	std::cout << cmd_nr.toStdString () << " X " << output.toStdString () << "\n";
#endif
	if (cmd_nr.toInt () != req_cnt - 1)
		throw invalid_response ();
	while (m_process->canReadLine ()) {
		QString l = m_process->readLine ();

		m_dlg.textEdit->setTextColor (Qt::red);
		m_dlg.append (l);
		m_dlg.textEdit->setTextColor (Qt::black);
		l = l.trimmed ();
		if (!l.isEmpty ())
			throw invalid_response ();
	}
	return output;
}

/****************************
* Program identity.        *
****************************/

/* Function:  Report the name of the program.
* Arguments: none
* Fails:     never
* Returns:   program name
*/
QString QGtp::name ()
{
	send_request ("name");
	return waitResponse ();
}

/* Function:  Report protocol version.
* Arguments: none
* Fails:     never
* Returns:   protocol version number
*/
int QGtp::protocolVersion ()
{
	send_request ("protocol_version");
	return waitResponse().toInt ();
}

void QGtp::send_request(const QString &s)
{
	qDebug() << "send_request -> " << req_cnt << " " << s << "\n";
#if 1
	m_dlg.textEdit->setTextColor (Qt::blue);
	m_dlg.append (s);
	m_dlg.textEdit->setTextColor (Qt::black);
#endif
	QString req = QString::number (req_cnt) + " " + s + "\n";
	m_process->write (req.toLatin1 ());
	m_process->waitForBytesWritten();
	req_cnt++;
}

/****************************
* Administrative commands. *
****************************/

/* Function:  Quit
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
void
QGtp::quit ()
{
	send_request ("quit");
}

#if 0
/* Function:  Report the version number of the program.
* Arguments: none
* Fails:     never
* Returns:   version number
*/
QString QGtp::version ()
{
	send_request ("version");
	return waitResponse();
}
#endif

/***************************
* Setting the board size. *
***************************/

/* Function:  Set the board size to NxN.
* Arguments: integer
* Fails:     board size outside engine's limits
* Returns:   nothing
*/
void
QGtp::setBoardsize (int size)
{
	send_request (QString ("boardsize ") + QString::number (size));
	waitResponse();
}

/***********************
* Clearing the board. *
***********************/

/* Function:  Clear the board.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/

void QGtp::clearBoard ()
{
	send_request ("clear_board");
	waitResponse();
}

/***************************
* Setting.           *
***************************/

/* Function:  Set the komi.
* Arguments: float
* Fails:     incorrect argument
* Returns:   nothing
*/
void
QGtp::setKomi(float f)
{
	char komi[20];
	sprintf (komi, "komi %.2f", f);
	send_request (komi);
	waitResponse();
}

/* Function:  Set the playing level.
* Arguments: int
* Fails:     incorrect argument
* Returns:   nothing
*/
void QGtp::setLevel (int level)
{
	send_request (QString ("level ") + QString::number (level));
	waitResponse();
}
/******************
* Playing moves. *
******************/

/* Function:  Play a black stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
void
QGtp::playblack (char c , int i)
{
	char req[20];
	sprintf (req, "play black %c%d", c, i);
	send_request (req);
	waitResponse();
}

/* Function:  Black pass.
* Arguments: None
* Fails:     never
* Returns:   nothing
*/
void
QGtp::playblackPass ()
{
	send_request ("play black pass");
	waitResponse();
}

/* Function:  Play a white stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
void
QGtp::playwhite (char c, int i)
{
	char req[20];
	sprintf (req, "play white %c%d", c, i);
	send_request (req);
	waitResponse();
}

/* Function:  White pass.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
void
QGtp::playwhitePass ()
{
	send_request ("play white pass");
	waitResponse();
}

/* Function:  Set up fixed placement handicap stones.
* Arguments: number of handicap stones
* Fails:     invalid number of stones for the current boardsize
* Returns:   list of vertices with handicap stones
*/
void
QGtp::fixedHandicap (int handicap)
{
	if (handicap < 2) 
		return;

	send_request (QString ("fixed_handicap ") + QString::number (handicap));
	waitResponse();
}

#if 0
/* Function:  Undo last move
* Arguments: int
* Fails:     If move pointer is 0
* Returns:   nothing
*/
void QGtp::undo (int i)
{
	send_request (QString ("undo ") + QString::number (i));
	waitResponse();
}
#endif

/* Function:  evaluate wether a command is known
* Arguments: command word
* Fails:     never
* Returns:   true or false
*/
bool QGtp::knownCommand (QString s)
{
	send_request (QString ("known_command ") + s);
	return waitResponse() == "true";
}

