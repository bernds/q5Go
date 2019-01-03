/*
* textview.h
*/

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include "ui_textview_gui.h"
#include "globals.h"

class TextView : public QDialog, public Ui::TextViewGUI
{
	Q_OBJECT

public:
	enum class type { exprt, gtp };
	TextView(QWidget* parent = 0, type m = type::exprt);
	~TextView();
	void append (const QString &);
	void set (const QString &);

public slots:
	void toClipboard();
	void saveMe();
};

#endif // TEXTVIEW_H
