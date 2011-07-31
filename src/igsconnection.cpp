 /* 
 * igsconnection.cpp
 * 	
 * insprired by qigs
 *
 * authors: Peter Strempel & Tom Le Duc & Johannes Mesa
 *
 * this code is still in experimentational phase
 */


#include "igsconnection.h"
#include <qgbkcodec.h> 
#include <qtextcodec.h>
#include <qtimer.h>

IGSConnection::IGSConnection() : IGSInterface()
{ 
	authState = LOGIN;

	qsocket = new QSocket(this);
	CHECK_PTR(qsocket);

	textCodec = 0;

	username = "";
	password = "";
	
	connect(qsocket, SIGNAL(hostFound()), SLOT(OnHostFound()));
	connect(qsocket, SIGNAL(connected()), SLOT(OnConnected()));
	connect(qsocket, SIGNAL(readyRead()), SLOT(OnReadyRead()));
	connect(qsocket, SIGNAL(connectionClosed()), SLOT(OnConnectionClosed()));
	connect(qsocket, SIGNAL(delayedCloseFinished()), SLOT(OnDelayedCloseFinish()));
	connect(qsocket, SIGNAL(bytesWritten(int)), SLOT(OnBytesWritten(int)));
	connect(qsocket, SIGNAL(error(int)), SLOT(OnError(int)));
}

IGSConnection::~IGSConnection()
{
	delete qsocket;
}

// maybe callback should be callback(void)...,
// any data can be transferred after signalling with normal functions
void IGSConnection::sendTextToApp(QString txt)
{
	if (callbackFP != NULL)
		(*callbackFP)(txt);
}

// check for 'Login:' (and Password)
bool IGSConnection::checkPrompt()
{
	// prompt can be login prompt or usermode prompt
		
	if (bufferLineRest.length() < 4)
	{
		qDebug(QString("IGSConnection::checkPrompt called with string of size %1").arg(bufferLineRest.length()));

		if (authState == PASSWORD)
		{
			int b = qsocket->bytesAvailable();
			if (!b)
			{
				qsocket->waitForMore(500);
				b = qsocket->bytesAvailable();
//				if (!b)
//					return false;
			}

			while (b-- > 0)
				bufferLineRest += qsocket->getch();
		}
		else
			return false; 
	}

	switch (authState)
	{
		case LOGIN:
			if (bufferLineRest.find("Login:") != -1)
			{
				qDebug("Login: found");
				
				// tell application what to send
				sendTextToApp(bufferLineRest);
				if (!username.isEmpty())
				{
					sendTextToApp("...sending: {" + QString(username) + "}");
				
					sendTextToHost(username, true);// + '\n');       // CAREFUL : THIS SOMETIMES CHANGES on IGS
				}

				// next state
				if (password)
					authState = PASSWORD;
				else
					authState = SESSION;
				return true;
			}
			break;

		case PASSWORD:
			if ((bufferLineRest.find("Password:") != -1) || (bufferLineRest.find("1 1") != -1))
			{
				qDebug("Password or 1 1: found , strlen = %d", bufferLineRest.length());
				sendTextToApp(tr("...send password"));
				sendTextToHost(password, true);

				// next state
				authState = SESSION;
				return true;
			}
			else if (bufferLineRest.find("guest account") != -1)
			{
				authState = SESSION;
				return true;
			}
			break;

		default:
			break;
	}
	
	return false;
}


/*
 * Slots
 */

void IGSConnection::OnHostFound()
{
	qDebug("IGSConnection::OnHostFound()");
}

void IGSConnection::OnConnected()
{
	qDebug("IGSConnection::OnConnected()");

	sendTextToApp("Connected to " + qsocket->peerAddress().toString() + " " +
		  QString::number(qsocket->peerPort()));
}
// We got data to read
void IGSConnection::OnReadyRead()
{
//*
//	qDebug("OnReadyRead....");
	while (qsocket->canReadLine())
	{
		int size = qsocket->bytesAvailable() + 1;
		char *c = new char[size];
		int bytes = qsocket->readLine(c, size);
		QString x = textCodec->toUnicode(c);
		delete[] c;
		if (x.isEmpty())
		{
			qDebug("READ:NULL");
			return;
		}

		// some statistics
//		emit signal_setBytesIn(x.length());

		x.truncate(x.length()-2);

		sendTextToApp(x);

		if (authState == PASSWORD)
		{
			bufferLineRest = x;
			checkPrompt();
			qDebug("PASSWORD***");
		}
	}
	
	if (authState == LOGIN && qsocket->bytesAvailable() == 7)
	{
		QString y;
		qDebug("looking for 'Login:'");
		while (qsocket->bytesAvailable())
			y += qsocket->getch();
		
		if (y)
		{
			qDebug("Collected: " + y);
			bufferLineRest = y;
			checkPrompt();
		}
	}
//*/
//	convertBlockToLines();
}

// Connection was closed from host
void IGSConnection::OnConnectionClosed() 
{
	qDebug("CONNECTION CLOSED BY FOREIGN HOST");

	// read last data that could be in the buffer
	OnReadyRead();
	authState = LOGIN;
	sendTextToApp("Connection closed by foreign host.\n");
}

// Connection was closed from application, but delayed
void IGSConnection::OnDelayedCloseFinish()
{
	qDebug("DELAY CLOSED FINISHED");
	
	authState = LOGIN;
	sendTextToApp("Connection closed.\n");
}

void IGSConnection::OnBytesWritten(int /*nbytes*/)
{
//	qDebug("%d BYTES WRITTEN", nbytes);
//	emit signal_setBytesOut(nbytes);
}

void IGSConnection::OnError(int i)
{
	switch (i)
	{
		case QSocket::ErrConnectionRefused: qDebug("ERROR: connection refused...");
			break;
		case QSocket::ErrHostNotFound: qDebug("ERROR: host not found...");
			break;
		case QSocket::ErrSocketRead: qDebug("ERROR: socket read...");
			break;
		default: qDebug("ERROR: unknown Error...");
			break;
	}
	sendTextToApp("ERROR - Connection closed.\n");
}


/*
 * Functions called from the application
 */

bool IGSConnection::isConnected()
{
//	qDebug("IGSConnection::isConnected()");
	return qsocket->state() == QSocket::Connection;
}

void IGSConnection::sendTextToHost(QString txt, bool ignoreCodec)
{
	/*
	*	This is intended because for weird reasons, if 'username' s given
	*	with a codec, IGS might reject it and ban the IP (!!!) for several hours.
	*	This seems to concern only windows users.
	*	Therefore, we pretend to ignore the codec when passing username or password
	*/

	if (ignoreCodec)
	{

        	int len;
		const char * txt2 = txt.latin1();

        	if ((len = qsocket->writeBlock(txt2, strlen(txt2) * sizeof(char))) != -1)
			qsocket->writeBlock("\r\n", 2);
		else
			qWarning(QString("*** failed sending to host: %1").arg(txt2));
	}

	else 
	{
		QCString raw = textCodec->fromUnicode(txt);

		if (qsocket->writeBlock(raw.data(), raw.size() - 1) != -1)
			qsocket->writeBlock("\r\n", 2);
		else
			qWarning(QString("*** failed sending to host: %1").arg(txt));
	}
}


void IGSConnection::setTextCodec(QString codec)
{
	textCodec = QTextCodec::codecForName(codec);
	if(!textCodec)
		textCodec = QTextCodec::codecForLocale();
	CHECK_PTR(textCodec);
}



bool IGSConnection::openConnection(const char *host, unsigned int port, const char *user, const char *pass)
{
	if (qsocket->state() != QSocket::Idle ) {
		qDebug("Called IGSConnection::openConnection while in state %d", qsocket->state());
		return false;
	}

	//textCodec = QTextCodec::codecForName(codec);
	//if(!textCodec)
	//	textCodec = QTextCodec::codecForLocale();
	//CHECK_PTR(textCodec);

	username = user;
	password = pass;
	
	qDebug("Connecting to %s %d as [%s], [%s]...", host, port, username.latin1(), (password ? "***" : "NULL"));
	sendTextToApp(tr("Trying to connect to %1 %2").arg(host,port));

	ASSERT(host != 0);
	ASSERT(port != 0);
	int len = qstrlen(host);
	if ((len > 0) && (len < 200)) // otherwise host points to junk
		qsocket->connectToHost(host, (Q_UINT16) port);
	return qsocket->state() != QSocket::Idle;
}

bool IGSConnection::closeConnection()
{
	// We have no connection?
	if (qsocket->state() == QSocket::Idle)
		return false;

	qDebug("Disconnecting...");

	// Close it.
	qsocket->close();

	// Closing succeeded, return message
	if (qsocket->state() == QSocket::Idle)
	{
		authState = LOGIN;
		sendTextToApp("Connection closed.\n");
	}

	// Not yet closed? Data will be written and then slot OnDelayClosedFinish() called
	return true;
}

const char* IGSConnection::getUsername ()
{
	return (const char*) username;
}
