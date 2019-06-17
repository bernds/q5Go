/*
* textview.h
*/

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <QtSvg>

#include "ui_textview_gui.h"

class TextView : public QDialog, public Ui::TextViewGUI
{
	Q_OBJECT

	int m_cursor_pos {};
public:
	enum class type { exprt, gtp };
	TextView(QWidget* parent = 0, type m = type::exprt);
	~TextView();
	void append (const QString &);
	void set (const QString &);
	void remember_cursor ();
	void delete_cursor_line ();
public slots:
	void toClipboard();
	void saveMe();
};

#include "ui_svgview_gui.h"

class SvgView : public QDialog, public Ui::SvgViewGUI
{
	Q_OBJECT
	QString m_svg;
	QSvgWidget *m_view {};
public:
	SvgView(QWidget* parent = 0);
	~SvgView();
	void set (const QByteArray &);

public slots:
	void toClipboard();
	void saveMe();
};

#endif // TEXTVIEW_H
