/*
 * helpviewer.cpp
 */

#include "helpviewer.h"
#include "icons.h"
#include "qgo.h"
#include "config.h"
//Added by qt3to4:
#include <QPixmap>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <qstringlist.h>
#include "globals.h"

//#ifdef USE_XPM
#include ICON_EXIT
#include ICON_HOME
//#endif

HelpViewer::HelpViewer(QWidget* parent)
    : QMainWindow(parent)
{
    resize(600, 480);

    setWindowTitle(PACKAGE " " VERSION " Manual");
    setWindowIcon(setting->image0);

    browser = new QTextBrowser(this);

    QStringList strList;

#ifdef Q_WS_WIN
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
	toolBar = new QToolBar(this);

	QPixmap iconHome, iconExit;

	//#ifdef USE_XPM
	iconHome = QPixmap(const_cast<const char**>(gohome_xpm));
	iconExit = QPixmap(const_cast<const char**>(exit_xpm));
	/*
	  #else
	  iconHome = QPixmap(ICON_HOME);
	  iconExit = QPixmap(ICON_EXIT);
	  #endif
	*/
	buttonClose = new QAction(iconExit, "Close", this);
	connect (buttonClose, SIGNAL(activated ()), this, SLOT(close()));
	buttonClose->setShortcut (Qt::Key_Escape);
	toolBar->addAction (buttonClose);

	buttonHome = new QAction(iconHome, "Home", this);
	connect (buttonHome, &QAction::triggered, [=] (bool) { browser->home (); });
	toolBar->addAction (buttonHome);
}

