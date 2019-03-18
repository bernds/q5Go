/*
 *   msg_handler.h
 */

#ifndef MSG_HANDLER_H
#define MSG_HANDLER_H

#define OWN_DEBUG_MODE

#include <QTextEdit>

#include "ui_gui_dialog.h"

class Debug_Dialog : public QDialog, public Ui::Debug_Dialog
{
	Q_OBJECT

public:
	Debug_Dialog (QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
	}
};

void myMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);

extern QTextStream *debug_stream;
extern QTextEdit *debug_view;
extern Debug_Dialog *debug_dialog;

#endif
