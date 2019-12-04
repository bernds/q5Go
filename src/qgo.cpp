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

void qGo::openManual(const QUrl &url)
{
	if (helpViewer == nullptr)
		helpViewer = new HelpViewer(0);
	helpViewer->set_url (url);
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
		it->update_settings ();
	setting->engines_changed = false;
}

void qGo::playStoneSound (bool force)
{
	static int idx = 0;

	if (!force && !setting->readBoolEntry ("SOUND_STONE"))
		return;

	switch (idx % 11) {
	default:
		QSound::play (":/sounds/stone.wav");
		break;
	case 0:
		QSound::play (":/sounds/stone2.wav");
		break;
	case 1:
		QSound::play (":/sounds/stone3.wav");
		break;
	case 2:
		QSound::play (":/sounds/stone4.wav");
		break;
	case 3:
		QSound::play (":/sounds/stone5.wav");
		break;
	case 4:
		QSound::play (":/sounds/stone6.wav");
		break;
	case 5:
		QSound::play (":/sounds/stone7.wav");
		break;
	case 6:
		QSound::play (":/sounds/stone8.wav");
		break;
	case 7:
		QSound::play (":/sounds/stone9.wav");
		break;
	case 8:
		QSound::play (":/sounds/stone10.wav");
		break;
	case 9:
		QSound::play (":/sounds/stone11.wav");
		break;
	}
	idx += 1 + rand () % 4;
}

void qGo::playTalkSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_TALK"))
		QSound::play (":/sounds/talk.wav");
}

void qGo::playMatchSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_MATCH"))
		QSound::play (":/sounds/match.wav");
}

void qGo::playPassSound (bool force)
{
        if (force || setting->readBoolEntry ("SOUND_PASS"))
		QSound::play (":/sounds/pass.wav");
}

void qGo::playGameEndSound (bool force)
{
        if (force || setting->readBoolEntry ("SOUND_GAMEEND"))
		QSound::play (":/sounds/gameend.wav");
}

void qGo::playTimeSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_TIME"))
		QSound::play (":/sounds/tictoc.wav");
}

void qGo::playSaySound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_SAY"))
		QSound::play (":/sounds/say.wav");
}

void qGo::playEnterSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_ENTER"))
		QSound::play (":/sounds/enter.wav");
}

void qGo::playLeaveSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_LEAVE"))
		QSound::play (":/sounds/leave.wav");
}

void qGo::playConnectSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_CONNECT"))
		QSound::play (":/sounds/connect.wav");
}

void qGo::playDisConnectSound (bool force)
{
	if (force || setting->readBoolEntry ("SOUND_DISCONNECT"))
		QSound::play (":/sounds/connect.wav");
}

void help_about ()
{
	QString txt;
	txt = "<p>Copyright \u00a9 2011-2019\nBernd Schmidt &lt;bernds_cb1@t-online.de&gt;</p>";
	txt += "<p>Copyright \u00a9 2001-2006\nPeter Strempel &lt;pstrempel@t-online.de&gt;, Johannes Mesa &lt;frosla@gmx.at&gt;, Emmanuel B\u00E9ranger &lt;yfh2@hotmail.com&gt;</p>";
	txt += "<p>" + QObject::tr("GTP code originally from Goliath, thanks to: ") + "PALM Thomas, DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien</p>";

	txt += "<hr/><p>Visit <a href=\"https://github.com/bernds/q5go\">the Github repository</a> for new versions.</p>";
	QString translation = "<hr/><p>" + QObject::tr("English translation by: Peter Strempel, Johannes Mesa, Emmanuel B\u00E9ranger", "Please set your own language and your name! Use your own language!") + "</p>";
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
