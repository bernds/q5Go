/*
* textview.h
*/

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include "textview_gui.h"
#include "globals.h"

class Matrix;

class TextView : public QDialog, public Ui::TextViewGUI
{
	Q_OBJECT

public:
	TextView(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WFlags fl = 0);
	~TextView();
	void setMatrix(Matrix *m, ASCII_Import *charset);

public slots:
	void toClipboard();
	void saveMe();
};

#endif // TEXTVIEW_H
