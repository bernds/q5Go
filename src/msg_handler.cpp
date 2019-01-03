/*
*   msg_handler.cpp
*/

#include "msg_handler.h"


#ifdef OWN_DEBUG_MODE
void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
	if (view == NULL || myapp.startingUp() || myapp.closingDown())
		return ;

	//QString msg2 = QString::QString(msg);

	switch (type)
	{
	case QtDebugMsg:
		view->append("Debug: "  + msg);
		break;
	case QtWarningMsg:
		view->append((QString) "Warning: " + msg);
		break;
	case QtFatalMsg:
		view->append((QString) "Fatal: " + msg);
		break;
	}
}
#endif
