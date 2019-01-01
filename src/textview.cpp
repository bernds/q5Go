/*
* textview.cpp
*/

#include <QFileDialog>
#include <QTextStream>
#include <QTextEdit>

#include "config.h"
#include "textview.h"
#include "matrix.h"
#include <qmessagebox.h>
#include <qclipboard.h> 
#include <qapplication.h>

/*
*  Constructs a TextView which is a child of 'parent', with the
*  name 'name' and widget flags set to 'f'
*
*  The dialog will by default be modeless, unless you set 'modal' to
*  TRUE to construct a modal dialog.
*/
TextView::TextView(QWidget* parent)
  : QDialog (parent)
{
	setupUi (this);
	setModal (true);
	textEdit->setWordWrapMode(QTextOption::WordWrap);
	textEdit->setLineWrapColumnOrWidth(80);
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
	QString fileName(QFileDialog::getSaveFileName(QString::null,
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

		QTextStream stream(&file);
		stream << textEdit->text();
		file.close();
}

void TextView::setMatrix(Matrix *m)
{
	CHECK_PTR(m);
	textEdit->setText(m->printMe());
}

void TextView::toClipboard()
{
	QApplication::clipboard()->setText(textEdit->text());
}
