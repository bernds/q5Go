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
	TextView(QWidget* parent = 0);
	~TextView();
	void setMatrix(Matrix *m);

public slots:
	void toClipboard();
	void saveMe();
};

#endif // TEXTVIEW_H
