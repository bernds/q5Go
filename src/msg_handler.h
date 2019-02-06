/*
 *   msg_handler.h
 */

#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#define OWN_DEBUG_MODE

#include <QTextEdit>

void myMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);

extern QTextEdit *debug_view;

#endif
