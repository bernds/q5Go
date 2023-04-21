/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include <QObject>
#include <array>
#include <QSoundEffect>

class HelpViewer;
class QUrl;

class qGo : public QObject
{
	Q_OBJECT

	std::array<QSoundEffect,11> stone_sounds;
	QSoundEffect sound_enter, sound_leave;
	QSoundEffect sound_connect, sound_disconnect;
	QSoundEffect sound_match, sound_pass, sound_gameend;
	QSoundEffect sound_talk, sound_say, sound_time;

public:

	qGo();
	~qGo();
	void openManual (const QUrl &);
	int checkModified ();
	void updateAllBoardSettings ();
	void playTalkSound (bool = false);
	void playMatchSound (bool = false);
	void playGameEndSound (bool = false);
	void playPassSound (bool = false);
	void playTimeSound (bool = false);
	void playSaySound (bool = false);
	void playEnterSound (bool = false);
	void playStoneSound (bool = false);
	void playLeaveSound (bool = false);
	void playDisconnectSound (bool = false);
	void playConnectSound (bool = false);

public slots:
	void unused_quit();

private:
	HelpViewer *helpViewer {};
};

class QApplication;

extern qGo *qgo;
extern QApplication *qgo_app;

#endif
