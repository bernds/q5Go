/*
* telnet.cpp
*/

#include "telnet.h"
#include "gs_globals.h"
#include "clientwin.h"
#include "igsconnection.h"
#include <qapplication.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qmenubar.h>

TelnetConnection::TelnetConnection(ClientWindow *parent, QWidget *lv_p, QWidget *lv_g)
{
	// Create IGSConnection instance
	igsInterface = new IGSConnection (lv_p, lv_g);
	connect (igsInterface, &IGSConnection::signal_text_to_app, parent, &ClientWindow::sendTextToApp);
}

TelnetConnection::~TelnetConnection ()
{
	slotHostQuit ();
	// qDebug("bfor deleting igsinterface addres: %x",igsInterface );
	// CHECK_PTR(igsInterface);
	delete igsInterface;
	// qDebug("after deleting igsinterface");

	qDebug("TelnetConnection::~TelnetConnection() DONE");
}

void TelnetConnection::connect_host (const Host &h)
{
	if (igsInterface->isConnected()) {
		qDebug("Already connected!");
		return;
	}

	if (!igsInterface->openConnection (h))
		qDebug("Failed to connect to host!");
	else
		qDebug("Connected");
}

void TelnetConnection::slotHostDisconnect()
{
	if (!igsInterface->closeConnection())
		qDebug("Failed to disconnect from host!");
}

// send text (from MainWindow) to host
void TelnetConnection::sendTextFromApp(const QString &txt)
{
	igsInterface->sendTextToHost(txt, false);
}

void TelnetConnection::slotHostQuit()
{
	// TODO: Timeout for connection termination, if still data has to be written
	if (igsInterface->isConnected())
		slotHostDisconnect();
}
