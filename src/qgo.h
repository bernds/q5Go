/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include <QObject>

#include "mainwindow.h"
#include "setting.h"
#include "defines.h"
#include "audio.h"
#include "goboard.h"

class HelpViewer;

extern "C" {extern  void play(const char *Pathname); }    //SL added eb 7


class qGo : public QObject
{
	Q_OBJECT
public:

	qGo();
	~qGo();
	void openManual();
	int checkModified();
	void updateAllBoardSettings();
	void playClick();
	void playAutoPlayClick();
	void playTalkSound();
	void playMatchSound();
	void playGameEndSound();
	void playPassSound();
	void playTimeSound();
	void playSaySound();
	void playEnterSound();
	void playStoneSound();
	void playLeaveSound();
	void playDisConnectSound();
	void playConnectSound();
	void updateFont();

signals:
	void signal_leave_qgo();
	void signal_updateFont();

public slots:
	void quit();

private:
	HelpViewer *helpViewer;
};

//-----------

class Engine
{
	QString m_title;
	QString m_path;
	QString m_args;
	QString m_komi;
	bool m_analysis;

public:
	Engine (const QString &title, const QString &path, const QString &args, const QString &komi, bool analysis)
		: m_title (title), m_path (path), m_args (args), m_komi (komi), m_analysis (analysis)
	{
	}
	QString title() const { return m_title; };
	QString path() const { return m_path; };
	QString args() const { return m_args; };
	QString komi() const { return m_komi; }
	bool analysis () const { return m_analysis; }

	int operator== (Engine h)
		{ return (this->m_title == h.m_title); };
	bool operator< (Engine h)
		{ return (this->m_title < h.m_title); };
};

extern qGo *qgo;
extern QString program_dir;

#endif
