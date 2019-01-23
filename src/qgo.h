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

extern qGo *qgo;
extern QString program_dir;

#endif
