/*
 *
 *  q G o   a Go Program using Trolltech's Qt
 *
 *  (C) by Peter Strempel, Johannes Mesa, Emmanuel Beranger 2001-2003
 *
 */
 
#include "gui_dialog.h"
#include "config.h"
#include "mainwin.h"
#include "msg_handler.h"
#include "setting.h"

#include <qtranslator.h>
#include <qtextcodec.h>
#include <qwidget.h>
#include <qapplication.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <q3listview.h>
#include <qcheckbox.h>
#include <qstring.h>
#include <qobject.h>

#include <q3mainwindow.h>
#include <qaction.h>
#include <q3dragobject.h>

qGo *qgo;

// global
Setting *setting = 0;

#ifdef OWN_DEBUG_MODE
QTextEdit *view =0 ;
#endif

/*
 *   main()
 */

int main(int argc, char **argv)
{
//#ifdef OWN_DEBUG_MODE
//	qInstallMsgHandler(myMessageHandler);
//#endif

	bool found_debug = false;
	bool found_sgf = false;
	bool found_sgf_file = false;
	QString sgf_file;

	// look for arguments
	int ac = argc;
	while (ac--)
	{
		if (argv[ac] == QString("-debug"))
		{
			// view debug window
			found_debug = true;
		}
		else if (argv[ac] == QString("-sgf"))
		{
			// view board
			found_sgf = true;
		}
		else if (argv[ac] == QString("-sgf19"))
		{
			// set up board 19x19 immediately
			found_sgf = true;
			found_sgf_file = true;
			sgf_file = QString("/19/");
		}
		else if (argv[ac] == QString("-desktop"))
		{
			// set standard options
			qApp->setDesktopSettingsAware(true);
		}
		else if (ac && argv[ac][0] != '-')
		{
			// file name
			found_sgf = true;
			found_sgf_file = true;
			sgf_file = QString(argv[ac]);
		}
	}

	QApplication myapp(argc, argv);

#ifdef OWN_DEBUG_MODE
	qInstallMsgHandler(myMessageHandler);
	Debug_Dialog *nonModal = new Debug_Dialog();
	view = nonModal->TextView1;
#endif

	// get application path
	QFileInfo program(argv[0]);
	QString program_dir = program.absolutePath ();
	qDebug() << "main:qt->PROGRAM.DIRPATH = " << program_dir;

	// restore last setting
	setting = new Setting();
	setting->program_dir = program_dir;

	// load values from file
	setting->loadSettings();

	// Load translation
	QTranslator trans(0);
	QString lang = setting->getLanguage();
	//const char *lang = setting->getLanguage();
	qDebug() << "Checking for language settings..." << lang;
	QString tr_dir = setting->getTranslationsDirectory(), loc;

	if (lang.isNull ())
	{
		qDebug("No language settings found, using system locale %s", QTextCodec::locale());
		loc = QString("qgo_") + QTextCodec::locale();
		lang = QTextCodec::locale();
	}
	else
	{
		qDebug(QString("Language settings found: ")+ lang);
		loc = QString("qgo_") + lang;
	}

	if (trans.load(loc, tr_dir))
	{
		qDebug("Translation loaded.");
		myapp.installTranslator(&trans);
	}
	else if (strcmp(lang.ascii(), "en") && strcmp(lang.ascii(), "C"))  // Skip warning for en and C default.
		qWarning("Failed to find translation file for %s", lang.ascii());

	client_window = new ClientWindow(0, 0, Qt::Window);

#ifdef OWN_DEBUG_MODE
	// restore size and pos
	if (client_window->getViewSize().width() > 0)
	{
		nonModal->resize(client_window->getViewSize());
		nonModal->move(client_window->getViewPos());
	}

	if (found_debug)
		nonModal->show();
	else
		nonModal->hide();

	// for storing size
	client_window->setDebugDialog(nonModal);
#endif

	// if debugging allow enhanced output
	if (found_debug)
		client_window->DODEBUG = true;

	client_window->hide();

	// set main widget
	myapp.setMainWidget(client_window);

	QObject::connect(&myapp, SIGNAL(lastWindowClosed()), client_window, SLOT(quit()));

	if (found_sgf) {
		qGoBoard *qb = new qGoBoard();
		qb->set_id(0);
		qb->set_Mode_real (modeNormal);
		// Default value.
		qb->set_komi("6.5");
		if (found_sgf_file)
			qb->get_win()->doOpen(sgf_file, 0, false);
		qb->get_win()->setGameMode (modeNormal);
	} else {
		client_window->setWindowTitle(PACKAGE1 + QString(" V") + VERSION);
		client_window->show();
	}

	if (setting->getNewVersionWarning())
		QMessageBox::warning(client_window ,PACKAGE,NEWVERSIONWARNING);//PACKAGE, NEWVERSIONWARNING);

	return myapp.exec();
}

