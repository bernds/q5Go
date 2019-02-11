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

/* @@@ check if there is a use case for this, and repair or remove as necessary.  */
void qGo::unused_quit()
{
#if 0
	emit signal_leave_qgo();
	int check = checkModified();
	if (check == 0) {
		QMessageBox::warning(0, PACKAGE,
				     tr("At least one board is modified.\n"
					"If you exit the application now, all changes will be lost!"
					"\nExit anyway?"),
				     tr("Yes"), tr("No"), QString::null,
				     1, 0);
		/* This code never did anything with the message box result.  */
	}
	qDebug() << "Program quits now...";
#endif
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

void help_about ()
{
	QString txt;
	txt = u8"<p>Copyright \u00a9 2011-2019\nBernd Schmidt &lt;bernds_cb1@t-online.de&gt;</p>";
	txt += u8"<p>Copyright \u00a9 2001-2006\nPeter Strempel &lt;pstrempel@t-online.de&gt;, Johannes Mesa &lt;frosla@gmx.at&gt;, Emmanuel B\u00E9ranger &lt;yfh2@hotmail.com&gt;</p>";
	txt += "<p>" + QObject::tr("GTP code originally from Goliath, thanks to: ") + "PALM Thomas, DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien</p>";

	txt += "<hr/><p>Visit <a href=\"https://github.com/bernds/q5go\">the Github repository</a> for new versions.</p>";
	QString translation = "<hr/><p>" + QObject::tr(u8"English translation by: Peter Strempel, Johannes Mesa, Emmanuel B\u00E9ranger", "Please set your own language and your name! Use your own language!") + "</p>";
	txt += translation;

	QMessageBox mb;
	mb.setWindowTitle (PACKAGE " " VERSION);
	mb.setTextFormat (Qt::RichText);
	mb.setText (txt);
	mb.exec ();
}

void help_new_version ()
{
	QMessageBox mb;
	mb.setWindowTitle (PACKAGE);
	mb.setTextFormat (Qt::RichText);
	mb.setText (NEWVERSIONWARNING);
	mb.exec ();
}
