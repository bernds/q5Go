/*
* textview.cpp
*/

#include "config.h"
#include "textview.h"
#include "matrix.h"
#include <q3multilineedit.h>
#include <q3textedit.h>
#include <qmessagebox.h>
#include <q3filedialog.h>
#include <q3textstream.h>
#include <qclipboard.h> 
#include <qapplication.h>

/* 
*  Constructs a TextView which is a child of 'parent', with the 
*  name 'name' and widget flags set to 'f' 
*
*  The dialog will by default be modeless, unless you set 'modal' to
*  TRUE to construct a modal dialog.
*/
TextView::TextView(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : QDialog( parent, name, modal, fl )
{
	setupUi(this);
	textEdit->setWordWrap(Q3TextEdit::FixedColumnWidth);
	textEdit->setWrapColumnOrWidth(80);
	QFont f("fixed", 10);
	f.setStyleHint(QFont::TypeWriter);
	textEdit->setFont(f);
}

/*  
*  Destroys the object and frees any allocated resources
*/
TextView::~TextView()
{
	// no need to delete child widgets, Qt does it all for us
}

/* 
* public slot
*/
void TextView::saveMe()
{
	QString fileName(Q3FileDialog::getSaveFileName(QString::null,
		tr("Text Files (*.txt);;All Files (*)"),
		this));
	if (fileName.isEmpty())
		return;
	
	QFile file(fileName);
	
	// Confirm overwriting file.
	if (QFile(fileName).exists())
		if (QMessageBox::information(this, PACKAGE,
			tr("This file already exists. Do you want to overwrite it?"),
			tr("Yes"), tr("No"), 0, 0, 1) == 1)
			return;
		
		if (!file.open(QIODevice::WriteOnly))
		{
			QString s;
			s.sprintf(tr("Failed to write to file") + " %s", fileName.latin1());
			QMessageBox::warning(this, PACKAGE, s);
			return;
		}
		
		Q3TextStream stream(&file);
		stream << textEdit->text();
		file.close();
}

void TextView::setMatrix(Matrix *m, ASCII_Import *charset)
{
	CHECK_PTR(m);
	textEdit->setText(m->printMe(charset));
}

void TextView::toClipboard()
{
	QApplication::clipboard()->setText(textEdit->text());
}
