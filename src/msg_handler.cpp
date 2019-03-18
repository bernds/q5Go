/*
*   msg_handler.cpp
*/
#include <QApplication>
#include <QTextStream>

#include "msg_handler.h"
#include "qgo.h"

#ifdef OWN_DEBUG_MODE
void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
	if (debug_stream != nullptr)
		*debug_stream << msg << "\n";

	if (debug_view == nullptr || qgo_app->startingUp () || qgo_app->closingDown ())
		return;

	//QString msg2 = QString::QString(msg);

	switch (type)
	{
	case QtInfoMsg:
		debug_view->setTextColor (Qt::darkGreen);
		debug_view->append("Info: "  + msg);
		break;
	case QtDebugMsg:
		debug_view->setTextColor (Qt::black);
		debug_view->append("Debug: "  + msg);
		break;
	case QtWarningMsg:
		debug_view->setTextColor (Qt::darkYellow);
		debug_view->append((QString) "Warning: " + msg);
		break;
	case QtCriticalMsg:
		debug_view->setTextColor (Qt::darkRed);
		debug_view->append((QString) "Critical: " + msg);
		break;
	case QtFatalMsg:
		debug_view->setTextColor (Qt::red);
		debug_view->append((QString) "Fatal: " + msg);
		break;
	}
}
#endif
