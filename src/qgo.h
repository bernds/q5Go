/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include <QObject>
#include <Q3PtrList>
#ifdef Q_WS_WIN
#include <Q3FileDialog>
#endif

#include "globals.h"
#include "mainwindow.h"
#include "setting.h"
#include "defines.h"
#include "searchpath.h"
#include "audio.h"

class HelpViewer;

extern "C" {extern  void play(const char *Pathname); }    //SL added eb 7


class qGo : public QObject
{
	Q_OBJECT

public:

	qGo();
	~qGo();
	MainWindow* addBoardWindow(MainWindow *w=NULL);
	void removeBoardWindow(MainWindow*);
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

public:
	bool testSound(bool);

private:
	Q3PtrList<MainWindow> *boardList;
	HelpViewer *helpViewer;
	Sound *clickSound;
//	Sound *autoplaySound;
	Sound *talkSound;
	Sound *matchSound;
	Sound *passSound;
	Sound *gameEndSound;
	Sound *timeSound;
	Sound *saySound;
	Sound *enterSound;
	Sound *leaveSound;
	Sound *connectSound;
	Sound *retrieveSound(const char *, SearchPath&);
	bool   soundsFound();
	void loadSound() { testSound(false); }
};

extern qGo *qgo;

#endif
