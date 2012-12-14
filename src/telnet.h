/*
 * telnet.h
 */

#ifndef TELNET_H
#define TELNET_H

#include <QObject>

class IGSInterface;

//-----------

class TelnetInterface : public QObject
{
	Q_OBJECT

public:
	TelnetInterface();
	~TelnetInterface();
	static void callback(QString);

signals:
	void textRecieved(const QString&);
};

//-----------

class TelnetConnection : public QObject
{
	Q_OBJECT

public:
	TelnetConnection(QWidget* parent);
	~TelnetConnection();

	void sendTextFromApp(const QString&);
	void setHost(const QString, const QString, const QString, unsigned int, const QString);

public slots:
	void slotHostConnect();
	void slotHostDisconnect();
	void slotHostQuit();

private:
	QString host, loginName, password, codec;
	unsigned int port;

	IGSInterface *igsInterface;
};
 
#endif

