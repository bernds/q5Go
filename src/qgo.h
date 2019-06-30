/*
 * qgo.h
 */

#ifndef QGO_H
#define QGO_H

#include <QObject>

class HelpViewer;
class QUrl;

class qGo : public QObject
{
	Q_OBJECT
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
	void playDisConnectSound (bool = false);
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
