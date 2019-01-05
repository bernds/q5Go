/*
* textview.cpp
*/

#include <QFileDialog>
#include <QTextStream>
#include <QTextEdit>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>

#include "config.h"
#include "textview.h"

/*
 *  Constructs a TextView which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
TextView::TextView(QWidget* parent, type t)
	: QDialog (parent)
{
	setupUi(this);
	textEdit->setWordWrapMode(QTextOption::WordWrap);
	textEdit->setLineWrapColumnOrWidth(80);
	QFont f("fixed", 10);
	f.setStyleHint(QFont::TypeWriter);
	textEdit->setFont(f);
	if (t == type::gtp) {
		exportBox->setVisible (false);
		setWindowTitle (tr ("GTP program startup"));
	} else {
		gtpBox->setVisible (false);
		cb_coords->setChecked (true);
		cb_numbering->setChecked (true);
		setWindowTitle (tr ("Export to ASCII"));
	}
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
	QString fileName(QFileDialog::getSaveFileName(this, tr ("Save ASCII export"), QString::null,
						      tr("Text Files (*.txt);;All Files (*)")));

	if (fileName.isEmpty())
		return;

	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly))
	{
		QString s = tr("Failed to write to file ") + fileName;
		QMessageBox::warning(this, PACKAGE, s);
		return;
	}

	QTextStream stream(&file);
	stream << textEdit->toPlainText();
	file.close();
}

void TextView::append (const QString &s)
{
	textEdit->append (s);
}

void TextView::toClipboard()
{
	QApplication::clipboard()->setText(textEdit->toPlainText());
}
