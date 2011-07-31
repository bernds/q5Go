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
#include <qcolor.h>
#include <qpixmap.h>
//#if (QT_VERSION < 0x030000)
//#include <qsortedlist.h>
//#else
#include <qptrlist.h>
//#endif

// delimiters for variables in settings file
// however, this character must not be used in parameters, e.g. host
#define DELIMITER ':'
#define SETTING_VERSION 1


class Setting;
extern Setting *setting;
class qGo;
class ClientWindow;
class ImageHandler;


class Parameter
{
public:
	Parameter(const QString&, const QString&);
	~Parameter() {};
	QString key() const { return k; }
	QString txt() const { return t; }
	void setPar(const QString &key, const QString &txt) { k = key; t = txt; }
	// operators <, ==
	int operator== (Parameter h)
		{ return (this->key() == h.key()); };
	int operator== (Parameter *h)
		{ return (this->key() == h->key()); };
	int operator< (Parameter h)
		{ return (this->key() < h.key()); };
	int operator< (Parameter *h)
		{ return (this->key() < h->key()); };

private:
	QString k;
	QString t;
};


class Setting : public Misc<QString>
{
public:
	Setting();
	~Setting();

	bool writeEntry(const QString&, const QString&);
	bool writeBoolEntry(const QString &s, bool b) { return writeEntry(s, (b ? "1" : "0")); }
	bool writeIntEntry(const QString &s, int i) { return writeEntry(s, QString::number(i)); }
	QString readEntry(const QString&);
	bool readBoolEntry(const QString &s)
	{
		QString e = readEntry(s);
		return (e && (e == "1"));
	}
	int  readIntEntry(const QString &s)
	{
		bool ok;
		QString e = readEntry(s);
		if (!e)
			return 0;
		else
		{
			int i = e.toInt(&ok);
			if (ok)
				return i;
			else
				return 0;
		}
	}
	void clearList(void);

	void loadSettings();
	void saveSettings();
	QString fontToString(QFont);
	void updateFont(QFont&, QString);

	ASCII_Import *charset;
	bool addImportAsBrother;
	bool fastLoad;
	QFont fontStandard, fontMarks, fontComments, fontLists, fontClocks, fontConsole;
	QColor colorBackground, colorAltBackground;
	QString language;
	QString program_dir;

	//const char* getLanguage(); //{ return readEntry("LANG") == NULL ? getenv("LANG") : readEntry("LANG").latin1(); }
	QString getLanguage();
	const QStringList getAvailableLanguages();
	QString convertNumberToLanguage(int n);
	int convertLanguageCodeToNumber();
	QString getTranslationsDirectory();
	bool getNewVersionWarning();

	bool nmatch_settings_modified;	

	// help to destroy static elements of qGo()
	qGo *qgo;
	ClientWindow *cw;

	// application icon
	QPixmap image0;

#ifdef Q_WS_WIN
	QString getApplicationPath();
#endif


protected:
//#if (QT_VERSION < 0x030000)
//	QSortedList<Parameter> list;
//#else
	QPtrList<Parameter> list;
//#endif

private:
	QString settingHomeDir;
};

#endif

