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

void TextView::remember_cursor ()
{
	m_cursor_pos = textEdit->textCursor ().position ();
}

void TextView::delete_cursor_line ()
{
	QTextCursor c = textEdit->textCursor ();
	c.setPosition (m_cursor_pos);
	c.movePosition (QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	c.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
	c.removeSelectedText ();
	textEdit->moveCursor (QTextCursor::End);
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

SvgView::SvgView(QWidget* parent)
	: QDialog (parent)
{
	setupUi(this);

	m_view = new QSvgWidget (aspectWidget);
	aspectWidget->set_child (m_view);
	cb_coords->setChecked (true);
	cb_numbering->setChecked (true);
	setWindowTitle (tr ("Export to SVG"));
}

/*
 *  Destroys the object and frees any allocated resources
 */
SvgView::~SvgView()
{
	delete m_view;
}

void SvgView::set (const QByteArray &qba)
{
	m_svg = QString::fromUtf8 (qba);
	m_view->load (qba);
	QSize sz = m_view->sizeHint ();
	aspectWidget->set_aspect (sz.width () / std::max (1, sz.height ()));
}

/*
 * public slot
 */
void SvgView::saveMe()
{
	QString fileName(QFileDialog::getSaveFileName(this, tr ("Save SVG export"), QString::null,
						      tr("Svg Files (*.txt);;All Files (*)")));

	if (fileName.isEmpty())
		return;

	QFile file(fileName);

	if (!file.open(QIODevice::WriteOnly))
	{
		QString s = tr("Failed to write to file ") + fileName;
		QMessageBox::warning(this, PACKAGE, s);
		return;
	}

	QTextStream stream (&file);
	stream << m_svg;
	file.close();
}

void SvgView::toClipboard()
{
	QApplication::clipboard()->setText (m_svg);
}
