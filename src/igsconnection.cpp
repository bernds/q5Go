 /* 
 * igsconnection.cpp
 * 	
 * insprired by qigs
 *
 * authors: Peter Strempel & Tom Le Duc & Johannes Mesa
 *
 * this code is still in experimentational phase
 */


#include <QTextCodec>
#include <QTimer>
#include <QWidget>
#include <QHostAddress>
#include "defines.h"
#include "igsconnection.h"

IGSConnection::IGSConnection(QWidget *lvp, QWidget *lvg) : m_lv_p (lvp), m_lv_g (lvg)
{
	authState = LOGIN;

	qsocket = new QTcpSocket(this);
	CHECK_PTR(qsocket);

	textCodec = 0;

	saved_data = 0;
	len_saved_data = 0;

	username = "";
	password = "";

	connect(qsocket, &QTcpSocket::hostFound, this, &IGSConnection::OnHostFound);
	connect(qsocket, &QTcpSocket::connected, this, &IGSConnection::OnConnected);
	connect(qsocket, &QTcpSocket::readyRead, this, &IGSConnection::OnReadyRead);
	connect(qsocket, &QTcpSocket::disconnected, this, &IGSConnection::OnConnectionClosed);
#if 0
	connect(qsocket, SIGNAL(delayedCloseFinished()), SLOT(OnDelayedCloseFinish()));
#endif
	connect(qsocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(OnError(QAbstractSocket::SocketError)));
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
bool IGSConnection::checkPrompt(const QString &line)
{
	switch (authState)
	{
		case LOGIN:
			if (line.indexOf("Login:") != -1)
			{
				qDebug("Login: found");

				// tell application what to send
				sendTextToApp(line);
				if (!username.isEmpty())
				{
					sendTextToApp("...sending: {" + QString(username) + "}");

					sendTextToHost(username, true);// + '\n');       // CAREFUL : THIS SOMETIMES CHANGES on IGS
				}

				// next state
				if (password.isNull ())
					authState = SESSION;
				else
					authState = PASSWORD;
				return true;
			}
			break;

		case PASSWORD:
			if ((line.indexOf("Password:") != -1) || (line.indexOf("1 1") != -1))
			{
				qDebug() << "Password or 1 1: found: " << line;
				sendTextToApp(tr("...send password"));
				qDebug() << "send password " << password;
				sendTextToHost(password, true);

				// next state
				authState = SESSION;
				return true;
			}
			else if (line.indexOf("guest account") != -1)
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

class Update_Locker
{
	QWidget *m_w;
public:
	Update_Locker (QWidget *w) : m_w (w)
	{
		m_w->setUpdatesEnabled (false);
	}
	~Update_Locker ()
	{
		m_w->setUpdatesEnabled (true);
	}
};

// We got data to read
void IGSConnection::OnReadyRead()
{
	qDebug("OnReadyRead....");
	Update_Locker l1 (m_lv_p);
	Update_Locker l2 (m_lv_g);
	for (;;)
	{
		int available = qsocket->bytesAvailable();
		if (available == 0)
			return;

		if (!qsocket->canReadLine()) {
			char *new_data = new char[available + len_saved_data];
			if (saved_data) {
				memcpy (new_data, saved_data, len_saved_data);
				delete[] saved_data;
			}
			int nread = qsocket->read (new_data + len_saved_data, available);
			if (nread < available)
				qDebug () << "available " << available << " but read " << nread;
			saved_data = new_data;
			len_saved_data += nread;

			if (authState == LOGIN && len_saved_data == 7)
			{
				qDebug("looking for 'Login:'");
				QString y = QString::fromLatin1(saved_data, len_saved_data);

				qDebug() << "Collected: " << y;
				if (checkPrompt(y)) {
					delete[] saved_data;
					saved_data = 0;
					len_saved_data = 0;
				}
			}
		}
		while (qsocket->canReadLine())
		{
			int size = qsocket->bytesAvailable() + 1;
			char *c = new char[size + len_saved_data];
			if (len_saved_data)
				memcpy (c, saved_data, len_saved_data);
			int bytes = qsocket->readLine(c + len_saved_data, size);
			QString x = textCodec->toUnicode(c);
			delete[] c;
			len_saved_data = 0;
			if (saved_data)
				delete[] saved_data;
			saved_data = 0;

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
				checkPrompt(x);
				qDebug("PASSWORD***");
			}
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

void IGSConnection::OnError(QAbstractSocket::SocketError i)
{
	switch (i)
	{
		case QAbstractSocket::ConnectionRefusedError: qDebug("ERROR: connection refused...");
			break;
		case QAbstractSocket::HostNotFoundError: qDebug("ERROR: host not found...");
			break;
//		case QAbstractSocket::SocketReadError: qDebug("ERROR: socket read...");
//			break;
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
	return qsocket->state() == QAbstractSocket::ConnectedState;
}

void IGSConnection::sendTextToHost(QString txt, bool ignoreCodec)
{
	/*
	*	This is intended because for weird reasons, if 'username' s given
	*	with a codec, IGS might reject it and ban the IP (!!!) for several hours.
	*	This seems to concern only windows users.
	*	Therefore, we pretend to ignore the codec when passing username or password
	*/

	qDebug () << ">> " << txt;
	if (ignoreCodec)
	{

        	int len;
		const char * txt2 = txt.toLatin1();

		if ((len = qsocket->write(txt2, strlen(txt2) * sizeof(char))) != -1)
			qsocket->write("\r\n", 2);
		else
			qWarning() << QString("*** failed sending to host: %1").arg(txt2);
	}
	else
	{
		QByteArray raw = textCodec->fromUnicode(txt);

		if (qsocket->write(raw.data(), raw.size()) != -1)
			qsocket->write("\r\n", 2);
		else
			qWarning() << QString("*** failed sending to host: %1").arg(txt);
	}
}


void IGSConnection::setTextCodec(QString codec)
{
	textCodec = QTextCodec::codecForName(codec.toLatin1 ());
	if(!textCodec)
		textCodec = QTextCodec::codecForLocale();
	CHECK_PTR(textCodec);
}



bool IGSConnection::openConnection(const QString &host, unsigned int port, const QString &user, const QString &pass)
{
	if (qsocket->state() != QAbstractSocket::UnconnectedState) {
		qDebug("Called IGSConnection::openConnection while in state %d", qsocket->state());
		return false;
	}

	//textCodec = QTextCodec::codecForName(codec);
	//if(!textCodec)
	//	textCodec = QTextCodec::codecForLocale();
	//CHECK_PTR(textCodec);

	username = user;
	password = pass;

	qDebug() << "Connecting to " << host << " " << port << " as [" << username << "], ["
		 << (password.isNull () ? NULL : "***") << "]...\n";
	sendTextToApp(tr("Trying to connect to %1 %2").arg(host).arg(port));

	qsocket->connectToHost(host, port);
	return qsocket->state() != QAbstractSocket::UnconnectedState;
}

bool IGSConnection::closeConnection()
{
	// We have no connection?
	if (qsocket->state() == QAbstractSocket::UnconnectedState)
		return false;

	qDebug("Disconnecting...");

	// Close it.
	qsocket->close();

	// Closing succeeded, return message
	if (qsocket->state() == QAbstractSocket::UnconnectedState)
	{
		authState = LOGIN;
		sendTextToApp("Connection closed.\n");
	}

	// Not yet closed? Data will be written and then slot OnDelayClosedFinish() called
	return true;
}

