/*
 * settings.cpp
 */

#include "setting.h"
#include "config.h"
#include "defines.h"
#include "globals.h"
#include "icons.h"
#include <qfile.h>
#include <q3textstream.h>
#include <qdir.h>
#include <qfont.h>
#include <qcolor.h>
#include <qstringlist.h>
#include <qstring.h>
//Added by qt3to4:
#include <QPixmap>

//#ifdef USE_XPM
#include ICON_APPICON
//#endif

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

#ifdef Q_WS_WIN
#include <windows.h>


QString applicationPath;
/*
 * If we are on Windows, read application path from registry, key was set
 * during installation at HKEY_LOCAL_MACHINE\Software\qGo\Location
 */
QString Setting::getApplicationPath()
{
	LONG res;
	HKEY hkey;
	QString s = NULL;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\qGo"), NULL,
	    KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		DWORD type = REG_SZ,
		size;
		char buffer[100];
		size = sizeof(buffer);
		res = RegQueryValueEx(hkey, TEXT("Location"), NULL, &type, (PBYTE)&buffer, &size);

		RegCloseKey(hkey);

		s = qt_winQString(buffer);
		qDebug("getApplicationPath(): %s", s.latin1());
	}

	return s;
}
#endif


// parameter list comprising key and txt
Parameter::Parameter(const QString &key, const QString &txt)
{ 
	k = key;
	t = txt;
}


/*
 *   Settings
 */

Setting::Setting()
{
	// list of parameters to be read/saved at start/end
	// true -> delete items when they are removed
	list.setAutoDelete(true);
	clearList();
  
  	// defaults:
	writeBoolEntry("USE_NMATCH", true);
	writeBoolEntry("NMATCH_BLACK", true);
	writeBoolEntry("NMATCH_WHITE", true);
	writeBoolEntry("NMATCH_NIGIRI", true);
	writeIntEntry("NMATCH_HANDICAP", 9);
	writeIntEntry("NMATCH_MAIN_TIME", 100);
	writeIntEntry("NMATCH_BYO_TIME", 100);

	writeBoolEntry("SIDEBAR", true);
	writeBoolEntry("BOARD_COORDS", true);
	writeBoolEntry("SGF_BOARD_COORDS", false);
	writeBoolEntry("CURSOR", true);
	writeBoolEntry("SLIDER", true);
	writeBoolEntry("FILEBAR", true);
	writeBoolEntry("TOOLBAR", true);
	writeBoolEntry("MAINTOOLBAR", true);
	writeBoolEntry("EDITBAR", true);
	writeBoolEntry("STATUSBAR", true);
	writeBoolEntry("MENUBAR", true);
	writeEntry("SKIN", "");
	writeEntry("SKIN_TABLE", "");
	writeBoolEntry("TOOLTIPS", true);
	//writeBoolEntry("STONES_SHADOW", true);
	//writeBoolEntry("STONES_SHELLS", true);
	writeBoolEntry("VAR_FONT", true);
	writeBoolEntry("SOUND_STONE", true);
	writeBoolEntry("SOUND_AUTOPLAY", true);
	writeBoolEntry("SOUND_TALK", true);
	writeBoolEntry("SOUND_MATCH", true);
	writeBoolEntry("SOUND_GAMEEND", true);
	writeBoolEntry("SOUND_PASS", true);
	writeBoolEntry("SOUND_TIME", true);
	writeBoolEntry("SOUND_SAY", true);
	writeBoolEntry("SOUND_ENTER", true);
	writeBoolEntry("SOUND_LEAVE", true);
	writeBoolEntry("SOUND_DISCONNECT", true);
	writeBoolEntry("SOUND_CONNECT", true);
	writeIntEntry("TIMER_INTERVAL", 1);
	writeBoolEntry("SGF_TIME_TAGS", true);
	writeIntEntry("DEFAULT_SIZE", 19);
	writeIntEntry("DEFAULT_TIME", 10);
	writeIntEntry("DEFAULT_BY", 10);
	writeIntEntry("BY_TIMER", 10);
	writeIntEntry("COMPUTER_SIZE", 19);
	writeEntry("LAST_DIR", QDir::homeDirPath());
//#ifdef Q_OS_MACX
	writeBoolEntry("REM_DIR", true);
	writeIntEntry("STONES_LOOK", 3);
//#endif

	// Default ASCII import charset
	charset = new ASCII_Import();
	charset->blackStone = '#';
	charset->whiteStone = 'O';
	charset->starPoint = ',';
	charset->emptyPoint = '.';
	charset->hBorder = '|';
	charset->vBorder = '-';

	qDebug() << "new charset " << charset->blackStone << charset->whiteStone << charset->starPoint << charset->emptyPoint;

	addImportAsBrother = false;
	fastLoad = false;
	language = "Default";
	fontStandard = QFont();
	fontMarks = QFont();
	fontComments = QFont();
	fontLists = QFont();
	fontClocks = QFont();
  	fontConsole= QFont("Fixed");
	colorBackground = QColor("white");
	colorAltBackground = QColor(242,242,242,QColor::Rgb);	

	// init
	qgo = 0;
	cw = 0;
	program_dir = QString();

	nmatch_settings_modified=false;

#ifdef Q_WS_WIN
	applicationPath = getApplicationPath();
#endif

//#ifdef USE_XPM
	image0 = QPixmap((const char **)Bowl_xpm);
//#else
//	image0 = QPixmap(ICON_APPICON);
//#endif
}

Setting::~Setting()
{
	// write to file
//	saveSettings();
	delete charset;
}

void Setting::loadSettings()
{
	QFile file;

	// set under Linux
	const char *p = getenv("HOME");
	if (p == NULL)
		// set under Windows
		p = getenv("USERPROFILE");
	if (p == NULL)
	{
		// however...
		qDebug("HOME and/or USERPROFILE are not set");
		settingHomeDir = QDir::homeDirPath();
		file.setName(settingHomeDir + "/.qgoclientrc");
		if (file.exists())
		{
			// rename, but use it anyway
			QString oldName = ".qgoclientrc";
			QString newName = ".qgoclientrc.bak";
			QDir::home().rename(oldName, newName);
			file.setName(settingHomeDir + "/.qgoclientrc.bak");
		}
		else
			// file may be already renamed
			file.setName(settingHomeDir + "/." + PACKAGE + "rc");
	}
	else
	{
		settingHomeDir = QString (p);
		file.setName(settingHomeDir + "/." + PACKAGE + "rc");
	}

	if (!file.exists() || !file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Failed loading settings: " << file.name();

		// maybe old file available
		file.setName(QDir::homeDirPath() + "/.qgoclientrc");

		if (!file.exists() || !file.open(QIODevice::ReadOnly))
		{
			qWarning("Failed loading settings: " + file.name());
			return;
		}
	}

	qDebug() << "Use settings: " << file.name();

	// read file
	Q3TextStream txt(&file);
	QString s;
	int pos, pos1, pos2;
	while (!txt.eof())
	{
		s = txt.readLine();
		if (s.length() > 2)
		{
			// ' ' is delitmiter between key and txt
			pos = s.find(' ');
			// find first '['
			pos1 = s.find('[');
			// find last ']'
			pos2 = s.findRev(']');
			writeEntry(s.left(pos), s.mid(pos1 + 1, pos2 - pos1 - 1));
		}
	}

	file.close();

	// init fonts and colors
	updateFont(fontStandard, "FONT_MAIN");
	updateFont(fontMarks, "FONT_MARK");
	updateFont(fontComments, "FONT_COMMENT");
	updateFont(fontLists, "FONT_LIST");
	updateFont(fontClocks, "FONT_CLOCK");
	updateFont(fontConsole, "FONT_CONSOLE");
 	s = readEntry("COLOR_BK");
	if (!s.isNull())
		colorBackground = QColor(s);
	s = readEntry("COLOR_ALT_BK");
	if (!s.isNull())
		colorAltBackground = QColor(s);
	s = readEntry("CHARSET");
	// Cope with nul characters written to the config file by qGo.
	if (s.length() >= 6 && s.at(0).toAscii() != '\0') {
		charset->blackStone = s.at(0).toAscii();
		charset->emptyPoint = s.at(1).toAscii();
		charset->hBorder = s.at(2).toAscii();
		charset->starPoint = s.at(3).toAscii();
		charset->vBorder = s.at(4).toAscii();
		charset->whiteStone = s.at(5).toAscii();
	}
}

QString Setting::fontToString(QFont f)
{
#ifdef Q_WS_X11_xxx
	return f.rawName();
#else
	return f.family() + "-" +
		QString::number(f.pointSize()) + "-" +
		QString::number(f.weight()) + "-" +
		QString::number(f.italic()) + "-" +
		QString::number(f.underline()) + "-" +
		QString::number(f.strikeOut());
#endif
}

void Setting::updateFont(QFont &font, QString entry)
{
	// do font stuff
	QString s = readEntry(entry);
	if (s.isNull())
		return;
#ifdef Q_WS_X11_xxx
	font.setRawName(s);
#else
	QString fs = s.section('-', 0, 0);
	font.setFamily(fs);
//	qDebug("FAMILY: %s", fs.latin1());
	for (int i=1; i<6; i++)
	{
		int n = s.section('-', i, i).toInt();

		switch (i)
		{
			case 1:
				font.setPointSize(n);
				break;
			case 2:
				font.setWeight(n);
				break;
			case 3:
				font.setItalic(n);
				break;
			case 4:
				font.setUnderline(n);
				break;
			case 5:
				font.setStrikeOut(n);
				break;
			default:
				qWarning("Unknown font attribut!");
		}
	}
#endif
}

void Setting::saveSettings()
{
//	if (readBoolEntry("REM_FONT"))
//	{
	// add fonts and colors
	writeEntry("FONT_MAIN", fontToString(fontStandard));
	writeEntry("FONT_MARK", fontToString(fontMarks));
	writeEntry("FONT_COMMENT", fontToString(fontComments));
	writeEntry("FONT_LIST", fontToString(fontLists));
	writeEntry("FONT_CLOCK", fontToString(fontClocks));
	writeEntry("FONT_CONSOLE", fontToString(fontConsole));

	QString cs = "";

	cs += QChar(charset->blackStone);
	cs += QChar(charset->emptyPoint);
	cs += QChar(charset->hBorder);
	cs += QChar(charset->starPoint);
	cs += QChar(charset->vBorder);
	cs += QChar(charset->whiteStone);

	qDebug() << "CHARSET " << cs;
	writeEntry("CHARSET", cs);

	writeEntry("COLOR_BK", colorBackground.name());
	writeEntry("COLOR_ALT_BK", colorAltBackground.name());

	writeIntEntry("VERSION", SETTING_VERSION);

	list.sort();

	QFile file(settingHomeDir + "/." + PACKAGE + "rc");
    
	if (file.open(QIODevice::WriteOnly))
	{
		Q3TextStream txtfile(&file);

		// write list to file: KEY [TXT]
		Parameter *par;
		for (par = list.first(); par != 0; par = list.next())
			if (!par->txt().isEmpty() && !par->txt().isNull())
				txtfile << par->key() << " [" << par->txt() << "]" << endl;

		file.close();
	}
}

// make list entry
bool Setting::writeEntry(const QString &key, const QString &txt)
{
	// seek key
	Parameter *par;
	for (par = list.first(); par != 0; par = list.next())
	{
		if (par->key() == key)
		{
			// if found, insert at current pos, and remove old item
			// store type value
			par->setPar(key, txt);
//			list.insert(list.at(), new Parameter(key, txt, ct));
//			list.next();
//			list.remove();

			// false -> entry already in list
			return false;
		}
	}

	list.append(new Parameter(key, txt));

	// true -> new entry
	return true;
}

// return a text entry indexed by key
QString Setting::readEntry(const QString &key)
{
	// seek key
	Parameter *par;
	for (par = list.first(); par != 0; par = list.next())
		if (par->key() == key)
			return par->txt();

	qDebug() << "Setting::readEntry(): " << key << " == 0";
	return QString::null;
}

void Setting::clearList(void)
{
	list.clear();
}

const QStringList Setting::getAvailableLanguages()
{
    // This is ugly, but I want those defines in defines.h, so all settings are in that file
    const char *available_languages[] = AVAILABLE_LANGUAGES;
    const int number_of_available_languages = NUMBER_OF_AVAILABLE_LANGUAGES;
    QStringList list;
    int i;

    for (i=0; i<number_of_available_languages; i++)
		list << available_languages[i];
	
    return list;
}

QString Setting::convertNumberToLanguage(int n)
{
    const char* language_codes[] = LANGUAGE_CODES;

    if (n == 0)
		return "Default";
    
    return language_codes[n-1];
}

int Setting::convertLanguageCodeToNumber()
{
    const char *language_codes[] = LANGUAGE_CODES;
    const int number_of_available_languages = NUMBER_OF_AVAILABLE_LANGUAGES;
    int i;

    for (i=0; i<number_of_available_languages; i++)
    {
		if (readEntry("LANG") == QString(language_codes[i]))
			return i+1;
    }

    return 0;
}

QString Setting::getTranslationsDirectory()
{
    qDebug("Checking for translations directory...");

    QStringList list;
	
	ASSERT(program_dir);

#ifdef Q_WS_WIN

	ASSERT(applicationPath);

	list << applicationPath + "/translations"
		<< program_dir + "/translations"
		<< "C:/Program Files/q4Go/translations"
		<< "D:/Program Files/q4Go/translations"
		<< "E:/Program Files/q4Go/translations"
		<< "C:/Programme/q4Go/translations"
		<< "D:/Programme/q4Go/translations"
		<< "E:/Programme/q4Go/translations"
		<< "./translations";
#elif defined(Q_OS_MACX)
	//get the bundle path and find our resources
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	list << (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())		
		+ "/Contents/Resources";
#else
	list	<< DATADIR "/translations"
		<< program_dir + "/translations"
		<< "./share/" PACKAGE "/translations"
		<< "/usr/share/" PACKAGE "/translations"
		<< "/usr/local/share/" PACKAGE "/translations"
		<< "./translations";
#endif
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
    {
		QDir d(*it);
		if (d.exists())
		{
			qDebug("Translations found in %s", d.absPath().latin1());
			return d.absPath();
		}
    }
    return "./translations";  // Hoping for the best...
}

//const char* Setting::getLanguage()
QString Setting::getLanguage()
{
	QString l = readEntry("LANG");
	
	//const char *hop[2];
	//hop[0] = char(12)  ;

	if (l.isEmpty() || l == QString("Default"))
	{
		const char *p = getenv("LANG");
	
		if (p == NULL)
			return QString::null;
		return QString(p).left(2);
	}
	else
		return l; //.latin1();
	//return readEntry("LANG") == NULL ? getenv("LANG") : readEntry("LANG").latin1(); }
}


bool Setting::getNewVersionWarning()
{
	QString l =  readEntry("NEWVERSIONWARNING");
	
	if (l == QString(VERSION))
		return false ;
		
	writeEntry("NEWVERSIONWARNING", VERSION);
	
	return true;
}



