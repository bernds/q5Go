/*
 * helpviewer.cpp
 */

#include "helpviewer.h"
#include "icons.h"
#include "setting.h"
#include <qtoolbar.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <qstringlist.h>
#include "globals.h"

//#ifdef USE_XPM
#include ICON_EXIT
#include ICON_HOME
//#endif

HelpViewer::HelpViewer(QWidget* parent, const char* name, WFlags f)
    : QMainWindow(parent, name, f)
{
    resize(600, 480);
    setCaption(PACKAGE " " VERSION " Manual");

    setIcon(setting->image0);

    browser = new QTextBrowser(this);
    
    QStringList strList;

#ifdef Q_WS_WIN
    strList << applicationPath + "/html"
	    << "C:/Program Files/qGo/html"
	    << "D:/Program Files/qGo/html"
	    << "E:/Program Files/qGo/html"
	    << "C:/Programme/qGo/html"
	    << "D:/Programme/qGo/html"
	    << "E:/Programme/qGo/html"
	    << "./html";
#else
    strList << "/usr/share/doc/" PACKAGE "/html"
	    << "/usr/doc/" PACKAGE "/html"
	    << "/usr/local/share/doc/" PACKAGE "/html"
	    << "/usr/local/doc/" PACKAGE "/html"
	    << "./html/";
#endif

    browser->mimeSourceFactory()->setFilePath(strList);
    browser->setSource("index.html");
    setCentralWidget(browser);

    initToolBar();
}

HelpViewer::~HelpViewer()
{
}

void HelpViewer::initToolBar()
{
    toolBar = new QToolBar(this, "toolbar");
       
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
    buttonClose = new QToolButton(iconExit, "Close (Escape)", "Close", this, 
SLOT(close()), toolBar);
    buttonClose->setAccel(Key_Escape);
    buttonHome = new QToolButton(iconHome, "Home", "Home", browser, 
SLOT(home()), toolBar);
}

