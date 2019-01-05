/*
* telnet.cpp
*/

#include "telnet.h"
#include "gs_globals.h"
#include "mainwin.h"
#include "igsconnection.h"
#include <qapplication.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qmenubar.h>


TelnetInterface *telnetIF;


TelnetConnection::TelnetConnection(QWidget* parent, QWidget *lv_p, QWidget *lv_g)
{
	host = DEFAULT_HOST;
	port = DEFAULT_PORT;
	loginName = "guest";
	password = "";

	// Create IGSConnection instance
	igsInterface = new IGSConnection(lv_p, lv_g);
	CHECK_PTR(igsInterface);
	void (*fp)(QString);
	fp = TelnetInterface::callback;
	igsInterface->registerCallback(fp);

	// Create TelnetInterface instance
	telnetIF = new TelnetInterface();
	CHECK_PTR(telnetIF);
	connect(telnetIF,
		SIGNAL(textRecieved(const QString&)),
		parent,
		SLOT(sendTextToApp(const QString&)));
}

TelnetConnection::~TelnetConnection()
{
	// qDebug("bfor deleting igsinterface addres: %x",igsInterface );
	// CHECK_PTR(igsInterface);
	delete igsInterface;
	// qDebug("after deleting igsinterface");
	
	delete telnetIF;
	telnetIF = nullptr;
	
	qDebug("TelnetConnection::~TelnetConnection() DONE");
}

void TelnetConnection::slotHostConnect()
{
	if (igsInterface->isConnected())
	{
		qDebug("Already connected!");
		return;
	}

	igsInterface->setTextCodec(codec);


// igsInterface->openConnection(host, port);
	if (!igsInterface->openConnection(host, port, loginName, password))
	{
		qDebug("Failed to connect to host!");
	}
	else
		qDebug("Connected");
}

void TelnetConnection::slotHostDisconnect()
{
	if (!igsInterface->closeConnection())
	{
		qDebug("Failed to disconnect from host!");
	}
}

// send text (from MainWindow) to host
void TelnetConnection::sendTextFromApp(const QString &txt)
{
	igsInterface->sendTextToHost(txt, false);
}

// connect host
void TelnetConnection::setHost(const QString h, const QString lg, const QString pw, unsigned int pt, const QString cdc)
{
	// set variables for connection
	port = pt;
	host = h;
	loginName = lg;
	password = pw;
	codec = cdc;

	qDebug() << "SELECTED " << host << port << loginName << (password.isNull() ? "NULL" : "***");
}

void TelnetConnection::slotHostQuit()
{
	// TODO: Timeout for connection termination, if still data has to be written
	if (igsInterface->isConnected())
		slotHostDisconnect();
}

//------------------------------------------------------------

TelnetInterface::TelnetInterface()
{
}

TelnetInterface::~TelnetInterface()
{
}

void TelnetInterface::callback(QString s)
{
	if (telnetIF != nullptr)
		emit telnetIF->textRecieved(s);
}
