/*
 * setting.h
 */

#ifndef SETTING_H
#define SETTING_H

#include "defines.h"
#include "misc.h"
#include <stdlib.h>
#include <qstring.h>
#include <qfont.h>
#include <qpixmap.h>

// delimiters for variables in settings file
// however, this character must not be used in parameters, e.g. host
#define DELIMITER ':'
#define SETTING_VERSION 1


class Setting;
extern Setting *setting;
class qGo;
class ClientWindow;
class ImageHandler;


class Setting
{
	const QPixmap *m_wood_image {};
	const QPixmap *m_table_image {};
	QString settingHomeDir;

public:
	Setting();
	~Setting();

	void clearEntry(const QString &s) { writeEntry (s, QString ()); }
	void writeEntry(const QString&, const QString&);
	void writeBoolEntry(const QString &s, bool b) { writeEntry(s, (b ? "1" : "0")); }
	void writeIntEntry(const QString &s, int i) { writeEntry(s, QString::number(i)); }
	QString readEntry(const QString&);
	bool readBoolEntry(const QString &s)
	{
		QString e = readEntry(s);
		return e == "1";
	}
	int  readIntEntry(const QString &s)
	{
		bool ok;
		QString e = readEntry(s);
		if (e.isNull ())
			return 0;
		int i = e.toInt(&ok);
		if (ok)
			return i;
		return 0;
	}

	void loadSettings();
	void saveSettings();
	QString fontToString(QFont);

	QFont fontStandard, fontMarks, fontComments, fontLists, fontClocks, fontConsole;
	QString language;
	QString program_dir;

	//const char* getLanguage(); //{ return readEntry("LANG") == NULL ? getenv("LANG") : readEntry("LANG").latin1(); }
	QString getLanguage();
	const QStringList getAvailableLanguages();
	QString convertNumberToLanguage(int n);
	int convertLanguageCodeToNumber();
	QString getTranslationsDirectory();
	bool getNewVersionWarning();

	void obtain_skin_images ();
	const QPixmap *wood_image () { return m_wood_image; }
	const QPixmap *table_image () { return m_table_image; }
	bool nmatch_settings_modified;

	// help to destroy static elements of qGo()
	qGo *qgo;
	ClientWindow *cw;

	// application icon
	QPixmap image0;

protected:
	QMap<QString, QString> params;

private:
	void updateFont(QFont&, QString);
};

#endif

