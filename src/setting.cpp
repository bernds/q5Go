/*
 * settings.cpp
 */

#include <QTextStream>

#include "setting.h"
#include "config.h"
#include "defines.h"
#include "globals.h"
#include "icons.h"
#include <qfile.h>
#include <qdir.h>
#include <qfont.h>
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

/*
 *   Settings
 */

Setting::Setting()
{
	// list of parameters to be read/saved at start/end
	// true -> delete items when they are removed
	params.clear ();

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
	writeEntry("LAST_DIR", QDir::homePath());
//#ifdef Q_OS_MACX
	writeBoolEntry("REM_DIR", true);
	writeIntEntry("STONES_LOOK", 3);
//#endif

	addImportAsBrother = false;
	fastLoad = false;
	language = "Default";
	fontStandard = QFont();
	fontMarks = QFont();
	fontComments = QFont();
	fontLists = QFont();
	fontClocks = QFont();
  	fontConsole = QFont("Fixed");

	// init
	qgo = 0;
	cw = 0;

	nmatch_settings_modified=false;

//#ifdef USE_XPM
	image0 = QPixmap((const char **)Bowl_xpm);
//#else
//	image0 = QPixmap(ICON_APPICON);
//#endif
}

Setting::~Setting()
{
}

void Setting::loadSettings()
{
	settingHomeDir = QDir::homePath();
	QFile file (settingHomeDir + "/." + PACKAGE + "rc");

	if (!file.exists() || !file.open(QIODevice::ReadOnly))
		qDebug() << "Failed loading settings: " << file.fileName();
	else
		qDebug() << "Use settings: " << file.fileName();

	// read file
	QTextStream txt(&file);
	QString s;
	int pos, pos1, pos2;
	while (!txt.atEnd ())
	{
		s = txt.readLine();
		if (s.length() > 2)
		{
			// ' ' is delitmiter between key and txt
			pos = s.indexOf(' ');
			// find first '['
			pos1 = s.indexOf('[');
			// find last ']'
			pos2 = s.lastIndexOf(']');
			writeEntry(s.left(pos), s.mid(pos1 + 1, pos2 - pos1 - 1));
		}
	}

	file.close();

	// init fonts
	updateFont(fontStandard, "FONT_MAIN");
	updateFont(fontMarks, "FONT_MARK");
	updateFont(fontComments, "FONT_COMMENT");
	updateFont(fontLists, "FONT_LIST");
	updateFont(fontClocks, "FONT_CLOCK");
	updateFont(fontConsole, "FONT_CONSOLE");
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
	// add fonts
	writeEntry("FONT_MAIN", fontToString(fontStandard));
	writeEntry("FONT_MARK", fontToString(fontMarks));
	writeEntry("FONT_COMMENT", fontToString(fontComments));
	writeEntry("FONT_LIST", fontToString(fontLists));
	writeEntry("FONT_CLOCK", fontToString(fontClocks));
	writeEntry("FONT_CONSOLE", fontToString(fontConsole));

	writeIntEntry("VERSION", SETTING_VERSION);

	QFile file(settingHomeDir + "/." + PACKAGE + "rc");

	if (file.open(QIODevice::WriteOnly))
	{
		QTextStream txtfile(&file);

		// write list to file: KEY [TXT]
		QMap<QString, QString>::const_iterator i = params.constBegin();
		while (i != params.constEnd()) {
			if (!i.value().isEmpty() && !i.value().isNull())
				txtfile << i.key() << " [" << i.value() << "]" << endl;
			++i;
		}

		file.close();
	}
}

// make list entry
void Setting::writeEntry(const QString &key, const QString &txt)
{
	params[key] = txt;
}

// return a text entry indexed by key
QString Setting::readEntry(const QString &key)
{
	return params.value (key);
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

#ifdef Q_WS_WIN
	list << program_dir + "/translations"
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
			qDebug() << "Translations found in " << d.absolutePath();
			return d.absolutePath();
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



