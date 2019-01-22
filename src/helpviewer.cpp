/*
 * helpviewer.cpp
 */

#include "helpviewer.h"
#include "qgo.h"
#include "config.h"
#include <QPixmap>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <qstringlist.h>

HelpViewer::HelpViewer(QWidget* parent)
	: QMainWindow(parent)
{
	resize(600, 480);

	setWindowTitle(PACKAGE " " VERSION " Manual");
	setWindowIcon(setting->image0);

	browser = new QTextBrowser(this);

	QStringList strList;

#ifdef Q_OS_WIN
	strList << program_dir + "/html"
		<< "C:/Program Files/qGo/html"
		<< "D:/Program Files/qGo/html"
		<< "E:/Program Files/qGo/html"
		<< "C:/Programme/qGo/html"
		<< "D:/Programme/qGo/html"
		<< "E:/Programme/qGo/html"
		<< "./html";
#else
	strList << DOCDIR "/html"
		<< "/usr/share/doc/" PACKAGE "/html"
		<< "/usr/doc/" PACKAGE "/html"
		<< "/usr/local/share/doc/" PACKAGE "/html"
		<< "/usr/local/doc/" PACKAGE "/html"
		<< "./html/";
#endif

	browser->setSearchPaths(strList);
	browser->setSource(QUrl ("index.html"));
	setCentralWidget(browser);

	initToolBar();
}

HelpViewer::~HelpViewer()
{
}

void HelpViewer::initToolBar()
{
	toolBar = addToolBar ("Navigation Bar");

	buttonClose = new QAction(QIcon (":/images/exit.png"), "Close", this);
	connect (buttonClose, &QAction::triggered, this, [=] (bool) { close (); });
	buttonClose->setShortcut (Qt::Key_Escape);
	toolBar->addAction (buttonClose);

	buttonHome = new QAction(QIcon (":/Help/images/help/gohome.png"), "Home", this);
	connect (buttonHome, &QAction::triggered, [=] (bool) { browser->home (); });
	toolBar->addAction (buttonHome);

	buttonBack = new QAction(QIcon (":/MainWidgetGui/images/mainwidget/1leftarrow.png"), "Home", this);
	connect (buttonBack, &QAction::triggered, [=] (bool) { browser->backward (); });
	toolBar->addAction (buttonBack);

	buttonForward = new QAction(QIcon (":/MainWidgetGui/images/mainwidget/1rightarrow.png"), "Home", this);
	connect (buttonForward, &QAction::triggered, [=] (bool) { browser->forward (); });
	toolBar->addAction (buttonForward);

	buttonForward->setEnabled (false);
	buttonBack->setEnabled (false);
	connect (browser, &QTextBrowser::backwardAvailable, [=] (bool set) { buttonBack->setEnabled (set); });
	connect (browser, &QTextBrowser::forwardAvailable, [=] (bool set) { buttonForward->setEnabled (set); });
}
