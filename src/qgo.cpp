#include <QMessageBox>
#include <QDebug>

#include <array>

#include "qgo.h"
#include "helpviewer.h"
#include "mainwindow.h"
#include "setting.h"
#include "defines.h"

#include "config.h"

qGo::qGo()
{
	stone_sounds[0].setSource (QUrl ("qrc:/sounds/stone.wav"));
	stone_sounds[1].setSource (QUrl ("qrc:/sounds/stone2.wav"));
	stone_sounds[2].setSource (QUrl ("qrc:/sounds/stone3.wav"));
	stone_sounds[3].setSource (QUrl ("qrc:/sounds/stone4.wav"));
	stone_sounds[4].setSource (QUrl ("qrc:/sounds/stone5.wav"));
	stone_sounds[5].setSource (QUrl ("qrc:/sounds/stone6.wav"));
	stone_sounds[6].setSource (QUrl ("qrc:/sounds/stone7.wav"));
	stone_sounds[7].setSource (QUrl ("qrc:/sounds/stone8.wav"));
	stone_sounds[8].setSource (QUrl ("qrc:/sounds/stone9.wav"));
	stone_sounds[9].setSource (QUrl ("qrc:/sounds/stone10.wav"));
	stone_sounds[10].setSource (QUrl ("qrc:/sounds/stone11.wav"));

	sound_talk.setSource (QUrl ("qrc:/sounds/talk.wav"));
	sound_match.setSource (QUrl ("qrc:/sounds/match.wav"));
	sound_pass.setSource (QUrl ("qrc:/sounds/pass.wav"));
	sound_gameend.setSource (QUrl ("qrc:/sounds/gameend.wav"));
	sound_time.setSource (QUrl ("qrc:/sounds/tictoc.wav"));
	sound_say.setSource (QUrl ("qrc:/sounds/say.wav"));
	sound_enter.setSource (QUrl ("qrc:/sounds/enter.wav"));
	sound_leave.setSource (QUrl ("qrc:/sounds/leave.wav"));
	sound_connect.setSource (QUrl ("qrc:/sounds/connect.wav"));
	sound_disconnect.setSource (QUrl ("qrc:/sounds/connect.wav"));
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
	static unsigned int idx = 0;

	if (!force && !setting->readBoolEntry ("SOUND_STONE"))
		return;

	stone_sounds[idx % 11].play ();
	idx += 1 + rand () % 4;
}

void qGo::playTalkSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_TALK"))
		return;
	sound_talk.play ();
}

void qGo::playMatchSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_MATCH"))
		return;
	sound_match.play ();
}

void qGo::playPassSound (bool force)
{
        if (!force && !setting->readBoolEntry ("SOUND_PASS"))
		return;
	sound_pass.play ();
}

void qGo::playGameEndSound (bool force)
{
        if (!force && !setting->readBoolEntry ("SOUND_GAMEEND"))
		return;
	sound_gameend.play ();
}

void qGo::playTimeSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_TIME"))
		return;
	sound_time.play ();
}

void qGo::playSaySound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_SAY"))
		return;
	sound_say.play ();
}

void qGo::playEnterSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_ENTER"))
		return;
	sound_enter.play ();
}

void qGo::playLeaveSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_LEAVE"))
		return;
	sound_leave.play ();
}

void qGo::playConnectSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_CONNECT"))
		return;
	sound_connect.play ();
}

void qGo::playDisconnectSound (bool force)
{
	if (!force && !setting->readBoolEntry ("SOUND_DISCONNECT"))
		return;
	sound_disconnect.play ();
}

void help_about ()
{
	QString txt;
	txt = "<p>Copyright \u00a9 2011-2020\nBernd Schmidt &lt;bernds_cb1@t-online.de&gt;</p>";
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
