/*
 * telnet.h
 */

#ifndef TELNET_H
#define TELNET_H

#include <QObject>

class IGSConnection;
class ClientWindow;
struct Host;

//-----------

class TelnetConnection : public QObject
{
	Q_OBJECT

	IGSConnection *igsInterface;

public:
	TelnetConnection (ClientWindow *parent, QWidget *, QWidget *);
	~TelnetConnection ();

	void sendTextFromApp (const QString&);
	void connect_host (const Host &);

public slots:
	void slotHostDisconnect();
	void slotHostQuit();
};
 
#endif

