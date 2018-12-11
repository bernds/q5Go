/*
* qgo.cpp
*/

//Added by qt3to4:
#include <Q3PtrList>
#include <QDir>
#include <QMessageBox>
#include <QLineEdit>

#include "qgo.h"
#include "helpviewer.h"
#include "board.h"
#include "mainwindow.h"
#include "setting.h"
#include "defines.h"
#include "audio.h"

#include "config.h"
#include "searchpath.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

qGo::qGo() : QObject()
{
	boardList = new Q3PtrList<MainWindow>;
	boardList->setAutoDelete(false);
	helpViewer = NULL;
	clickSound = NULL;
	talkSound = NULL;
	matchSound = NULL;
	passSound = NULL;
	gameEndSound = NULL;
	timeSound = NULL;
	saySound = NULL;
	enterSound = NULL;
	leaveSound = NULL;
	connectSound = NULL;
	loadSound ();
}

qGo::~qGo()
{
	boardList->clear();
	delete boardList;

	delete helpViewer;
	delete clickSound;

	//	settings = 0;
	boardList = 0;
	helpViewer = 0;
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

MainWindow* qGo::addBoardWindow(MainWindow *w)
{
	if (w == NULL)
	{
		qWarning("*** BOARD HAS NO PARENT");
		w = new MainWindow(0, PACKAGE);
	}
	w->show();
	boardList->append(w);
	
	return w;
}

void qGo::removeBoardWindow(MainWindow *w)
{
	if (w == NULL)
	{
		qWarning("qGo::removeBoardWindow(QWidget *w) - paramter w is 0!");
		return;
	}
	
	if (!boardList->removeRef(w))
		qWarning("Failed to remove board from list.");
}

void qGo::openManual()
{
	if (helpViewer == NULL)
		helpViewer = new HelpViewer(0);
	helpViewer->show();
	helpViewer->raise();
}


int qGo::checkModified()
{
	// Just closed the last board?
	if (boardList->isEmpty())
		return 1;
	
	// One board, same as closing a single window
	if (boardList->count() == 1)
		return ((MainWindow*)(boardList->first()))->checkModified(true);
    
	// Several boards. Check if one is modified.
	QWidget *b;
	for (b=boardList->first(); b != 0; b=boardList->next())
		if (!((MainWindow*)b)->checkModified(false))
			return 0;
		
	return 1;
}

void qGo::updateAllBoardSettings()
{
	for (QWidget *w=boardList->first(); w != 0; w=boardList->next())
		((MainWindow*)w)->updateBoard();
}

void qGo::updateFont()
{
	for (QWidget *w=boardList->first(); w != 0; w=boardList->next())
		((MainWindow*)w)->updateFont();

	emit signal_updateFont();
}

Sound * qGo::retrieveSound(const char * filename, SearchPath& sp)
{
  	QFile qfile(filename);
  	QFile * foundFile = sp.findFile(qfile);
	QString msg(filename);
  	if (! foundFile) 
	{
    		QString msg(filename);
    		msg.append(" not found");
    		qDebug() << msg;
    		return (Sound *) NULL;
  	}
	else 
	{
		msg.append(" found : " + foundFile->fileName());
		qDebug() << msg;
    		return SoundFactory::newSound(foundFile->fileName());
  	}
}

bool qGo::testSound(bool showmsg)
{
	// qDebug("qGo::testSound()");
	
	// Sound system supported?
	if (!QSound::available())
	{
		if (showmsg)
		{
#ifdef Q_WS_WIN
			QMessageBox::information(0, PACKAGE, tr("No sound available."));
			clickSound = NULL;
			return false;
#elif defined (Q_OS_MACX) 
			QMessageBox::information(0, PACKAGE, tr("No sound available. Qt on Mac uses QuickTime sound."));
#else
			QMessageBox::information(0, PACKAGE, tr("You are not running the Network Audio system.\n"
				"If you have the `au' command, run it in the background before this program. The latest release of the Network Audio System can be obtained from:\n\n"
				"ftp.ncd.com:/pub/ncd/technology/src/nas\n"
				"ftp.x.org:/contrib/audio/nas\n\n"
				"Release 1.2 of NAS is also included with the X11R6 contrib distribution. After installing NAS, you will then need to reconfigure Qt with NAS sound support.\n\n"
				"Nevertheless, if you have oss, sound should be working and directed to /dev/dsp"));
#endif
		}
	}
	else if (showmsg)
	{
		QMessageBox::information(0, PACKAGE, tr("Sound available."));
		return true;
	}
	
	//    qDebug("Sound available, checking for sound files...");

	// Sound files found?
	QStringList list;

	ASSERT(setting->program_dir);

#ifdef Q_WS_WIN
	list << applicationPath + "/sounds"
		<< setting->program_dir + "/sounds"
		<< "C:/Program Files/qGo/sounds"
		<< "D:/Program Files/qGo/sounds"
		<< "E:/Program Files/qGo/sounds"
		<< "C:/Programme/qGo/sounds"
		<< "D:/Programme/qGo/sounds"
		<< "E:/Programme/qGo/sounds"
		<< "./sounds";
#elif defined(Q_OS_MACX)
	//get the bundle path and find our resources like sounds
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	list << (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())		
		+ "/Contents/Resources";
#else
	// BUG 1165950 -- it may be better to use binreloc rather than
	// DATADIR
	list << DATADIR "/sounds"
	     << setting->program_dir + "/sounds"
		<< "./share/" PACKAGE "/sounds"
		<< "/usr/share/" PACKAGE "/sounds"
		<< "/usr/local/share/" PACKAGE "/sounds"
		<< "/sounds"
		<< "./sounds"
		<< "./src/sounds";                           //SL added eb 7
	
#endif

	SearchPath sp;
	sp << list;

	clickSound   = retrieveSound("stone.wav", sp);
	talkSound    = retrieveSound("talk.wav" , sp);
	matchSound   = retrieveSound("match.wav" , sp);
	passSound    = retrieveSound("pass.wav", sp);
	gameEndSound = retrieveSound("gameend.wav" , sp);
	timeSound    = retrieveSound("tictoc.wav" , sp);
	saySound     = retrieveSound("say.wav" , sp);
	enterSound   = retrieveSound("enter.wav", sp);
	leaveSound   = retrieveSound("leave.wav" , sp);
	connectSound = retrieveSound("connect.wav", sp);

#ifdef Q_WS_WIN
	if (soundsFound() && applicationPath.isEmpty())
	{
	  QFile qFile = QFile(connectSound->fileName()); // QQQ
	  QDir * dir = sp.findDirContainingFile(qFile); // QQQ
	  QString s = dir->dirName();
	  applicationPath = s.left(s.indexOf("/sounds"));
	  // QMessageBox::information(0, "SAVING", applicationPath);
	}
			
#endif
	if (soundsFound())
	  return true;

#ifdef Q_OS_MACX
	QMessageBox::information(0, PACKAGE, tr("No sound files in bundle, strange.\n"));
#elif ! defined(Q_WS_WIN)
	QMessageBox::information(0, PACKAGE, tr("Sound files not found.") + "\n" +
		tr("Please check for the directories") + " /usr/local/share/" + PACKAGE + "/sounds/ " + tr("or") +
		" /usr/share/" + PACKAGE + "/sounds/, " + tr("depending on your installation."));
#else

	applicationPath = setting->readEntry("PATH_SOUND");
	if (applicationPath.isEmpty())
		return testSound(false);

	QMessageBox::information(0, PACKAGE, tr("Sound files not found.") + "\n" +
			     tr("You can navigate to the main qGo directory (for example:") + " C:\\Program Files\\" + PACKAGE + " .\n" +
				 tr("If the directory was given correctly, this data will be saved and you won't"
				 "be asked\nanymore except you install qGo again into a different directory.\n"
				 "To abort this procedure, click 'Cancel' in the following dialog."));
    
	applicationPath = QFileDialog::getExistingDirectory(NULL, NULL, "appdir", tr("qGo directory"), true);
	
	if (applicationPath.isNull() || applicationPath.isEmpty())
	{
		QMessageBox::warning(0, PACKAGE, tr("No valid directory was given. Sound is not available."));
		return false;
	}

	// save path
   	setting->writeEntry("PATH_SOUND", applicationPath);

	// QMessageBox::information(0, "TRYING AGAIN", applicationPath);
	return testSound(false);
#endif
	return false;
}

bool qGo::soundsFound()
{
  // success means all sounds were found
  if (clickSound && talkSound && matchSound && passSound
      && gameEndSound && timeSound && saySound && enterSound
      && leaveSound && connectSound)
    return true;
  else
    return false;
}

void qGo::playClick()
{
	if (clickSound) //setting->readBoolEntry("SOUND_STONE") && clickSound)
	{                                                                      //added eb 7
		clickSound->play();
	}                                                                      //end add eb 7
}

void qGo::playAutoPlayClick()
{
	if (setting->readBoolEntry("SOUND_AUTOPLAY") && clickSound)
	{                                                                      //added eb 7
		clickSound->play();
	}                                                                      //end add eb 7
}

void qGo::playTalkSound()
{
	if (setting->readBoolEntry("SOUND_TALK") && talkSound)
	{                                                                      //added eb 7
		talkSound->play();
	}                                                                      //end add eb 7
}

void qGo::playMatchSound()
{
	if (setting->readBoolEntry("SOUND_MATCH") && matchSound)
	{                                                                      //added eb 7
		matchSound->play();
	}                                                                      //end add eb 7
}

void qGo::playPassSound()
{
	if (setting->readBoolEntry("SOUND_PASS") && passSound)
	{                                                                      //added eb 7
		passSound->play();
	}                                                                      //end add eb 7
}

void qGo::playGameEndSound()
{
	if (setting->readBoolEntry("SOUND_GAMEEND") && gameEndSound)
	{                                                                      //added eb 7
		gameEndSound->play();
	}                                                                      //end add eb 7
}

void qGo::playTimeSound()
{
	if (setting->readBoolEntry("SOUND_TIME") && timeSound)
	{                                                                      //added eb 7
		timeSound->play();
	}                                                                      //end add eb 7
}

void qGo::playSaySound()
{
	if (setting->readBoolEntry("SOUND_SAY") && saySound)
	{                                                                      //added eb 7
		saySound->play();
	}                                                                      //end add eb 7
}

void qGo::playEnterSound()
{
	if (setting->readBoolEntry("SOUND_ENTER") && enterSound)
	{                                                                      //added eb 7
		enterSound->play();
	}                                                                      //end add eb 7
}

void qGo::playLeaveSound()
{
	if (setting->readBoolEntry("SOUND_LEAVE") && leaveSound)
	{                                                                      //added eb 7
		leaveSound->play();
	}                                                                      //end add eb 7
}

void qGo::playConnectSound()
{
	if (setting->readBoolEntry("SOUND_CONNECT") && connectSound)
	{                                                                      //added eb 7
		connectSound->play();
	}                                                                      //end add eb 7
}

void qGo::playDisConnectSound()
{
	if (setting->readBoolEntry("SOUND_DISCONNECT") && connectSound)
	{                                                                      //added eb 7
		connectSound->play();
	}                                                                      //end add eb 7
}

void qGo::slotHelpAbout()
{
	QString txt = PACKAGE " " VERSION
		"\n\nCopyright (c) 2001-2006\nPeter Strempel <pstrempel@t-online.de>\nJohannes Mesa <frosla@gmx.at>\nEmmanuel Béranger <yfh2@hotmail.com>\n\n" +
		tr("GTP code from Goliath, thanks to:") + "\nPALM Thomas\nDINTILHAC Florian\nHIVERT Anthony\nPIOC Sebastien";
	
	QString translation = tr("English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel Beranger", "Please set your own language and your name! Use your own language!");
	//if (translation != "English translation by:\nPeter Strempel\nJohannes Mesa\nEmmanuel Béranger")
		txt += "\n\n" + translation;
	
	QMessageBox::about(0, tr("About..."), txt);
}
