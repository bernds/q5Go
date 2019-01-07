/*
* qgo.cpp
*/

#include <QDir>
#include <QMessageBox>
#include <QLineEdit>
#include <QDebug>

#include "qgo.h"
#include "helpviewer.h"
#include "board.h"
#include "mainwindow.h"
#include "setting.h"
#include "defines.h"
#include "audio.h"

#include "config.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

qGo::qGo()
{
}

qGo::~qGo()
{
	delete helpViewer;
}

void qGo::quit()
{
	emit signal_leave_qgo();
	int check;
	if ((check = checkModified()) == 1 ||
		(!check &&
		!QMessageBox::warning(0, PACKAGE,
		tr("At least one board is modified.\n"
		"If you exit the application now, all changes will be lost!"
		"\nExit anyway?"),
		tr("Yes"), tr("No"), QString::null,
		1, 0)))
	{
		//	qApp->quit();
		qDebug() << "Program quits now...";
	}
	
	//    emit signal_leave_qgo();
}

void qGo::openManual()
{
	if (helpViewer == nullptr)
		helpViewer = new HelpViewer(0);
	helpViewer->show();
	helpViewer->raise();
}


int qGo::checkModified()
{
	for (auto it: main_window_list) {
		if (!it->checkModified(false))
			return 0;
	}
	return 1;
}

void qGo::updateAllBoardSettings()
{
	for (auto it: main_window_list)
		it->updateBoard ();
}

void qGo::updateFont()
{
	for (auto it: main_window_list)
		it->updateFont ();

	emit signal_updateFont();
}


void qGo::playClick()
{
	QSound::play (":/sounds/click.wav");
}

void qGo::playStoneSound()
{
	QSound::play (":/sounds/stone.wav");
}

void qGo::playAutoPlayClick()
{
	if (setting->readBoolEntry("SOUND_AUTOPLAY"))
	{
		QSound::play (":/sounds/click.wav");
	}
}

void qGo::playTalkSound()
{
	if (setting->readBoolEntry("SOUND_TALK"))
	{
		QSound::play (":/sounds/talk.wav");
	}
}

void qGo::playMatchSound()
{
	if (setting->readBoolEntry("SOUND_MATCH"))
	{
		QSound::play (":/sounds/match.wav");
	}
}

void qGo::playPassSound()
{
        if (setting->readBoolEntry("SOUND_PASS"))
	{
		QSound::play (":/sounds/pass.wav");
	}
}

void qGo::playGameEndSound()
{
        if (setting->readBoolEntry("SOUND_GAMEEND"))
	{
		QSound::play (":/sounds/gameend.wav");
	}
}

void qGo::playTimeSound()
{
	if (setting->readBoolEntry("SOUND_TIME"))
	{
		QSound::play (":/sounds/tictoc.wav");
	}
}

void qGo::playSaySound()
{
	if (setting->readBoolEntry("SOUND_SAY"))
	{
		QSound::play (":/sounds/say.wav");
	}
}

void qGo::playEnterSound()
{
	if (setting->readBoolEntry("SOUND_ENTER"))
	{
		QSound::play (":/sounds/enter.wav");
	}
}

void qGo::playLeaveSound()
{
	if (setting->readBoolEntry("SOUND_LEAVE"))
	{
		QSound::play (":/sounds/leave.wav");
	}
}

void qGo::playConnectSound()
{
	if (setting->readBoolEntry("SOUND_CONNECT"))
	{
		QSound::play (":/sounds/connect.wav");
	}
}

void qGo::playDisConnectSound()
{
	if (setting->readBoolEntry("SOUND_DISCONNECT"))
	{
		QSound::play (":/sounds/connect.wav");
	}
}

void qGo::slotHelpAbout()
{
	QString txt = PACKAGE " " VERSION
		"\n\nCopyright 2011-2019\nBernd Schmidt <bernds_cb1@t-online.de>"
		u8"\nCopyright (c) 2001-2006\nPeter Strempel <pstrempel@t-online.de>\nJohannes Mesa <frosla@gmx.at>\nEmmanuel B\u00E9ranger <yfh2@hotmail.com>\n\n" +
		tr("GTP code from Goliath, thanks to:") + "\nPALM Thomas\nDINTILHAC Florian\nHIVERT Anthony\nPIOC Sebastien";

	QString translation = tr(u8"English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel B\u00E9ranger", "Please set your own language and your name! Use your own language!");
	//if (translation != "English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel B\u00E9ranger")
	txt += "\n\n" + translation;

	QMessageBox::about(0, tr("About..."), txt);
}
