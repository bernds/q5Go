/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include <QObject>

#include "globals.h"
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
		void slotHelpAbout();

private:
		HelpViewer *helpViewer;
};

//-----------

class Engine
{
	QString m_title;
	QString m_path;
	QString m_args;
public:
	Engine(const QString &title, const QString &path, const QString &args)
		: m_title (title), m_path (path), m_args (args)
	{
	}
	QString title() const { return m_title; };
	QString path() const { return m_path; };
	QString args() const { return m_args; };
	int operator== (Engine h)
		{ return (this->m_title == h.m_title); };
	bool operator< (Engine h)
		{ return (this->m_title < h.m_title); };
};

extern qGo *qgo;
extern QString program_dir;

extern std::shared_ptr<game_record> new_game_dialog (QWidget *);
extern std::shared_ptr<game_record> record_from_stream (std::istream &isgf);
extern bool open_window_from_file (const std::string &filename);
extern void open_local_board (QWidget *, bool);
extern go_board new_handicap_board (int, int);
extern std::string get_candidate_filename (const std::string &dir, const game_info &);

#endif
