/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include "globals.h"
#include "mainwindow.h"
#include "setting.h"
#include "qobject.h"
#include "defines.h"
#include <q3ptrlist.h>
#include "searchpath.h"

#ifdef Q_WS_WIN
#include <q3filedialog.h>
#endif

#ifdef Q_OS_LINUX
#include "wavplay.h"
#endif


class HelpViewer;
class QSound;
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
	void loadSound() { testSound(false); }
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
	QSound *clickSound;
//	QSound *autoplaySound;
	QSound *talkSound;
	QSound *matchSound;
	QSound *passSound;
	QSound *gameEndSound;
	QSound *timeSound;
	QSound *saySound;
	QSound *enterSound;
	QSound *leaveSound;
	QSound *connectSound;
	QSound *retrieveSound(const char *, SearchPath&);
	bool   soundsFound();
};

#endif
