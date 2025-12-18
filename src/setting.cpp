/*
 * settings.cpp
 */

#include <array>
#include <QApplication>
#include <QTextStream>
#include <QStringConverter>
#include <QStandardPaths>
#include <QMessageBox>
#include <QPixmap>
#include <QTextCodec>
#include <QFile>
#include <QDir>
#include <QFont>
#include <QStringList>
#include <QString>

#include "qgo.h"
#include "setting.h"
#include "config.h"
#include "defines.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

const int default_skin_setting = 11;

/*
 *   Settings
 */

Setting::Setting()
{
	// list of parameters to be read/saved at start/end
	// true -> delete items when they are removed
	params.clear ();

  	// defaults:
	writeBoolEntry ("TIPS_STARTUP", true);

	writeBoolEntry("USE_NMATCH", true);
	writeBoolEntry("NMATCH_BLACK", true);
	writeBoolEntry("NMATCH_WHITE", true);
	writeBoolEntry("NMATCH_NIGIRI", true);
	writeIntEntry("NMATCH_HANDICAP", 9);
	writeIntEntry("NMATCH_MAIN_TIME", 100);
	writeIntEntry("NMATCH_BYO_TIME", 100);

	writeBoolEntry("SIDEBAR", true);
	writeBoolEntry("BOARD_COORDS", true);
	writeIntEntry("COORDS_SIZE", 65);
	writeBoolEntry("SGF_BOARD_COORDS", false);
	writeBoolEntry("CURSOR", true);
	writeBoolEntry("SLIDER", true);
	writeBoolEntry("FILEBAR", true);
	writeBoolEntry("TOOLBAR", true);
	writeBoolEntry("MAINTOOLBAR", true);
	writeBoolEntry("EDITBAR", true);
	writeBoolEntry("STATUSBAR", true);
	writeBoolEntry("MENUBAR", true);

	writeIntEntry("VAR_GHOSTS", 1);
	writeBoolEntry("VAR_CHILDREN", false);
	writeBoolEntry("VAR_IGNORE_DIAGS", true);
	writeIntEntry("VAR_SGF_STYLE", 2);

	writeEntry("SKIN", "1");
	writeEntry("SKIN_TABLE", "");
	writeIntEntry ("SKIN_INDEX", default_skin_setting);

	writeIntEntry("FILESEL", 1);

	writeBoolEntry("TOOLTIPS", true);
	//writeBoolEntry("STONES_SHADOW", true);
	//writeBoolEntry("STONES_SHELLS", true);

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

	writeBoolEntry("SOUND_OBSERVE", false);
	writeBoolEntry("SOUND_NORMAL", false);
	writeBoolEntry("SOUND_MATCH_BOARD", true);
	writeBoolEntry("SOUND_COMPUTER", true);

	writeIntEntry("DEFAULT_SIZE", 19);
	writeIntEntry("DEFAULT_TIME", 10);
	writeIntEntry("DEFAULT_BY", 10);
	writeIntEntry("BY_TIMER", 10);
	writeBoolEntry("TIME_WARN_DIVIDE", true);
	writeBoolEntry("TIME_WARN_BOARD", true);

	writeIntEntry("COMPUTER_SIZE", 19);
        QStringList docdirs = QStandardPaths::standardLocations (QStandardPaths::DocumentsLocation);
        qDebug () << "Docdirs: " << docdirs;
        if (!docdirs.isEmpty ()) {
		writeEntry("LAST_DIR", docdirs.first ());
		writeEntry("AUTOSAVE_PATH", docdirs.first ());
		writeBoolEntry("AUTOSAVE_TO_PATH", true);
	}

	writeIntEntry("STONES_BFLAT", 1);
	writeIntEntry("STONES_BROUND", 80);
	writeIntEntry("STONES_BHARD", 30);
	writeIntEntry("STONES_BSPEC", 50);
	writeIntEntry("STONES_WFLAT", 1);
	writeIntEntry("STONES_WROUND", 60);
	writeIntEntry("STONES_WHARD", 7);
	writeIntEntry("STONES_WSPEC", 80);
	writeIntEntry("STONES_AMBIENT", 15);
	writeIntEntry("STONES_SHADOWVAL", 100);

	writeEntry("STONES_BCOL", "#101010");
	writeEntry("STONES_WCOL", "#FFFFFF");
	writeIntEntry("STONES_LOOK", 3);
	writeBoolEntry("STONES_STRIPES", 1);

	writeEntry("CHAT_COLOR", "#000000");

	writeIntEntry("ANALYSIS_VARTYPE", 2);
	writeIntEntry("ANALYSIS_WINRATE", 2);
	writeBoolEntry("ANALYSIS_PRUNE", 1);
	writeBoolEntry("ANALYSIS_CHILDREN", 1);
	writeBoolEntry("ANALYSIS_HIDEOTHER", 1);

	writeIntEntry ("GAMETREE_SIZE", 30);
	writeBoolEntry ("GAMETREE_DIAGHIDE", 1);
	writeIntEntry ("BOARD_DIAGMODE", 1);
	writeIntEntry ("BOARD_DIAGCLEAR", 1);
	writeIntEntry ("TOROID_DUPS", 2);

	writeIntEntry ("SLIDE_LINES", 15);
	writeIntEntry ("SLIDE_X", 1920);
	writeIntEntry ("SLIDE_Y", 1080);
	writeIntEntry ("SLIDE_MARGIN", 2);

	writeIntEntry ("DB_MAX_FILESIZE", 50);
	writeBoolEntry ("DB_CACHE_MOVELIST", true);

	language = "Default";
	fontStandard = QFont();
	fontMarks = QFont();
	fontComments = QFont();
	fontLists = QFont();
	fontClocks = QFont();
	fontClocks.setPointSize (fontClocks.pointSize () * 1.5);
  	fontConsole = QFont("Fixed");

	nmatch_settings_modified = false;

	extract_frequent_settings ();
}

Setting::~Setting()
{
	delete m_wood_image;
	delete m_table_image;
}

QString wood_image_resource (int idx, const QString &filename)
{
	if (idx > 5) {
		return QString (":/BoardWindow/images/board/wood%1.jpg").arg (idx);
	} else if (idx > 0) {
		return QString (":/BoardWindow/images/board/wood%1.png").arg (idx);
	}
	return filename;
}

void Setting::obtain_skin_images ()
{
	int idx = readIntEntry ("SKIN_INDEX");
	QString filename = readEntry ("SKIN");
	filename = wood_image_resource (idx, filename);
	QPixmap *p = new QPixmap (filename);
	if (p->isNull ()) {
		QMessageBox::warning (nullptr, PACKAGE, QObject::tr ("Could not load custom board image,\nreverting to default."));

		delete p;
		p = new QPixmap (":/BoardWindow/images/board/wood1.png");
		writeIntEntry ("SKIN_INDEX", 1);
	}
	delete m_wood_image;
	m_wood_image = p;

	p = new QPixmap (readEntry ("SKIN_TABLE"));
	if (p->isNull ()) {
		delete p;
		p = new QPixmap (":/BoardWindow/images/board/table.png");
	}
	delete m_table_image;
	m_table_image = p;
}

void Setting::extract_lists ()
{
	int i = 0;
	m_dbpaths.clear ();
	for (;;) {
		QString key = "DBPATH_" + QString::number (i);
		QString v = readEntry (key);
		if (v.isNull ())
			break;
		clearEntry (key);
		m_dbpaths.append (v);
		i++;
	}

	// restore: hostlist
	i = 0;
	for (;;)
	{
		QString prefix = "HOST" + QString::number(++i);
		QString s = setting->readEntry (prefix + "a");
		if (s.isNull ())
			break;
		m_hosts.emplace_back (setting->readEntry (prefix + "a"),
				      setting->readEntry (prefix + "b"),
				      setting->readIntEntry (prefix + "c"),
				      setting->readEntry (prefix + "d"),
				      setting->readEntry (prefix + "e"),
				      setting->readEntry (prefix + "f"),
				      setting->readTriEntry (prefix + "g"));
		clearEntry (prefix + "a");
		clearEntry (prefix + "b");
		clearEntry (prefix + "c");
		clearEntry (prefix + "d");
		clearEntry (prefix + "e");
		clearEntry (prefix + "f");
		clearEntry (prefix + "g");
	}
	std::sort (m_hosts.begin (), m_hosts.end (),
		   [] (const Host &a, const Host &b) { return a.title < b.title; });

	i = 0;
	for (;;)
	{
		QString prefix = "ENGINE" + QString::number(++i);
		QString s = setting->readEntry (prefix + "a");
		if (s.isNull ())
			break;
		/* Deal with old settings files that did not have the "Restrictions" field.  */
		bool restrictions = setting->readBoolEntry (prefix + "g");
		QString size = setting->readEntry (prefix + "f");
		if (!size.isEmpty () && setting->readEntry (prefix + "g").isEmpty ())
			restrictions = true;
		m_engines.emplace_back (s, setting->readEntry (prefix + "b"),
					setting->readEntry (prefix + "c"),
					setting->readEntry (prefix + "d"),
					setting->readBoolEntry (prefix + "e"),
					size, restrictions);
		clearEntry (prefix + "a");
		clearEntry (prefix + "b");
		clearEntry (prefix + "c");
		clearEntry (prefix + "d");
		clearEntry (prefix + "e");
		clearEntry (prefix + "f");
		clearEntry (prefix + "g");
	}
	std::sort (m_engines.begin (), m_engines.end (),
		   [] (const Engine &a, const Engine &b) { return a.title < b.title; });

	bool updated = false;
	for (auto &it: m_engines)
		if (it.analysis && it.boardsize.isEmpty ()) {
			updated = true;
			it.boardsize = "19";
		}
	if (updated)
		QMessageBox::information (nullptr, PACKAGE,
					  QObject::tr("Engine configuration updated\n"
						      "Analysis engines now require a board size to be set, assuming 19 for existing entries."));
}

void Setting::write_lists (QTextStream &file)
{
	int i = 0;
	for (const auto &it: m_dbpaths) {
		file << "DBPATH_" + QString::number (i) << " [" << it << "]" << "\n";
		i++;
	}

	i = 0;
	for (const auto &h: m_hosts) {
		QString prefix = "HOST" + QString::number(++i);
		file << prefix << "a [" << h.title << "]" << "\n";
		file << prefix << "b [" << h.host << "]" << "\n";
		file << prefix << "c [" << h.port << "]" << "\n";
		file << prefix << "d [" << h.login_name << "]" << "\n";
		file << prefix << "e [" << h.password << "]" << "\n";
		file << prefix << "f [" << h.codec << "]" << "\n";
		if (h.quiet != -1)
			file << prefix << "g [" << h.quiet << "]" << "\n";
	}

	i = 0;
	for (const auto &e: m_engines)
	{
		QString prefix = "ENGINE" + QString::number(++i);
		file << prefix << "a [" << e.title << "]" << "\n";
		file << prefix << "b [" << e.path << "]" << "\n";
		file << prefix << "c [" << e.args << "]" << "\n";
		file << prefix << "d [" << e.komi << "]" << "\n";
		file << prefix << "e [" << (e.analysis ? "1" : "0") << "]" << "\n";
		file << prefix << "f [" << e.boardsize << "]" << "\n";
		file << prefix << "g [" << (e.restrictions ? "1" : "0") << "]" << "\n";
	}
}

void Setting::lists_to_entries ()
{
}

void Setting::loadSettings ()
{
        QString configfile = QStandardPaths::locate (QStandardPaths::AppConfigLocation, PACKAGE "rc", QStandardPaths::LocateFile);
        QFile file (configfile);

	if (!file.exists() || !file.open(QIODevice::ReadOnly))
		qDebug() << "Failed loading settings: " << file.fileName();
	else
		qDebug() << "Use settings: " << file.fileName();

	// read file
	QTextStream txt(&file);
	txt.setEncoding(QStringConverter::Utf8);
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

	extract_lists ();
	extract_frequent_settings ();

	/* Can't write this as a default in the constructor: existing preferences may
	   have a wood image for which scaling would be inappropriate.  So check here if
	   we still have the default SKIN_INDEX and no SKIN_SCALE_WOOD after loading.  */
	if (!params.contains ("SKIN_SCALE_WOOD"))
		writeBoolEntry ("SKIN_SCALE_WOOD", readIntEntry ("SKIN_INDEX") == default_skin_setting);

	// Read obsolete font entries first, and then try to replace them with more current ones.
	updateFont(fontStandard, "FONT_MAIN");
	updateFont(fontMarks, "FONT_MARK");
	updateFont(fontComments, "FONT_COMMENT");
	updateFont(fontLists, "FONT_LIST");
	updateFont(fontClocks, "FONT_CLOCK");
	updateFont(fontConsole, "FONT_CONSOLE");

	auto update2 = [=](QFont &f, const char *e) -> void
		{
			QString nm = readEntry (e);
			if (!nm.isEmpty ())
				f.fromString (nm);
		};
	update2 (fontStandard, "FONT_V2_MAIN");
	update2 (fontMarks, "FONT_V2_MARK");
	update2 (fontComments, "FONT_V2_COMMENT");
	update2 (fontLists, "FONT_V2_LIST");
	update2 (fontClocks, "FONT_V2_CLOCK");
	update2 (fontConsole, "FONT_V2_CONSOLE");
	obtain_skin_images ();
}

/* Used to be used for saving and loading with updateFont.
   Now used only for display on the font selection buttons.  */
QString Setting::fontToString(QFont f)
{
	QString nm = f.family ();
	nm += " " + QString::number (f.pointSize ());
	if (f.italic ())
		nm += " italic";
	if (f.weight () != QFont::Normal)
		nm += " Weight:" + QString::number (f.weight ());
	return nm;
}

/* Function to load legacy settings.  */
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
				if (n <= 25)
				font.setWeight(QFont::Light);
				else if (n <= 50)
				font.setWeight(QFont::Normal);
				else if (n <= 63)
				font.setWeight(QFont::DemiBold);
				else if (n <= 75)
				font.setWeight(QFont::Bold);
				else
				font.setWeight(QFont::Black);
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
	writeEntry("FONT_V2_MAIN", fontStandard.toString ());
	writeEntry("FONT_V2_MARK", fontMarks.toString ());
	writeEntry("FONT_V2_COMMENT", fontComments.toString ());
	writeEntry("FONT_V2_LIST", fontLists.toString ());
	writeEntry("FONT_V2_CLOCK", fontClocks.toString ());
	writeEntry("FONT_V2_CONSOLE", fontConsole.toString ());

#if 0 /* Enable in a later version when users aren't likely to return to v0.2.  */
	clearEntry ("FONT_MAIN");
	clearEntry ("FONT_MARK");
	clearEntry ("FONT_COMMENT");
	clearEntry ("FONT_LIST");
	clearEntry ("FONT_CLOCK");
	clearEntry ("FONT_CONSOLE");
#endif

	clearEntry ("WHO_1");
	clearEntry ("WHO_2");

	writeIntEntry("VERSION", SETTING_VERSION);

        QString configfile = QStandardPaths::locate (QStandardPaths::AppConfigLocation, PACKAGE "rc");
        if (configfile.isEmpty ()) {
                QString path (QStandardPaths::writableLocation (QStandardPaths::AppConfigLocation));
                QDir dir (path);
                dir.mkpath (".");
                configfile = path + "/" + PACKAGE "rc";
        }

        QFile file (configfile);

	if (file.open(QIODevice::WriteOnly))
	{
		lists_to_entries ();

		QTextStream txtfile(&file);
		txtfile.setEncoding(QStringConverter::Utf8);
		// write list to file: KEY [TXT]
		QMap<QString, QString>::const_iterator i = params.constBegin();
		while (i != params.constEnd()) {
			if (!i.value().isEmpty() && !i.value().isNull())
				txtfile << i.key() << " [" << i.value() << "]" << "\n";
			++i;
		}

		write_lists (txtfile);

		file.close();
        } else {
                QMessageBox::warning (nullptr, PACKAGE, QObject::tr ("Unable to save settings to ") + file.fileName ());
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

void Setting::extract_frequent_settings ()
{
	values.analysis_hideother = readBoolEntry ("ANALYSIS_HIDEOTHER");
	values.analysis_children = readBoolEntry ("ANALYSIS_CHILDREN");
	values.analysis_vartype = readIntEntry ("ANALYSIS_VARTYPE");
	values.analysis_winrate = readIntEntry ("ANALYSIS_WINRATE");

	values.gametree_diaghide = readBoolEntry ("GAMETREE_DIAGHIDE");
	values.gametree_size = readIntEntry ("GAMETREE_SIZE");

	values.toroid_dups = readIntEntry ("TOROID_DUPS");

	values.clicko_delay = readBoolEntry ("ANTICLICKO");
	values.clicko_hitbox = readBoolEntry ("ANTICLICKO_HITBOX");

	values.time_warn_board = readBoolEntry ("TIME_WARN_BOARD");
	if (readBoolEntry ("CHAT_COLORED"))
		values.chat_color = QColor (readEntry ("CHAT_COLOR"));
	else
		values.chat_color = QColor ();
}

static std::array<const char *, NUMBER_OF_AVAILABLE_LANGUAGES> language_codes LANGUAGE_CODES;
static std::array<const char *, NUMBER_OF_AVAILABLE_LANGUAGES> available_languages AVAILABLE_LANGUAGES;

const QStringList Setting::getAvailableLanguages()
{
	QStringList list;

	for (const auto it: available_languages)
		list << it;

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
	QString lang = readEntry("LANG");
	for (size_t i = 0; i < language_codes.size (); i++)
		if (lang == QString (language_codes[i]))
			return i + 1;

	return 0;
}

QString Setting::getTranslationsDirectory()
{
	qDebug("Checking for translations directory...");

	QStringList list;
#if defined(Q_OS_WIN)
	list << QApplication::applicationDirPath() + "/translations";
#elif defined(Q_OS_MACX)
	//get the bundle path and find our resources
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	list << (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())
        + "/Contents/Resources/translations";
#else
	list	<< DATADIR "/translations"
		<< QApplication::applicationDirPath() + "/translations"
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

	if (l.isEmpty() || l == QString("Default"))
	{
		const char *p = getenv("LANG");
		if (p == NULL)
			return QString ();
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



