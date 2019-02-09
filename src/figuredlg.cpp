#include <QPushButton>

#include "gogame.h"
#include "mainwindow.h"
#include "figuredlg.h"

FigureDialog::FigureDialog (game_state *st, MainWindow *parent) : QDialog (parent), m_win (parent), m_state (st)
{
	setupUi (this);
	setModal (true);

	m_intval = new QIntValidator (0, 10000, this);
	movenumEdit->setValidator (m_intval);

	if (st->has_figure ()) {
		const std::string &txt = st->figure_title ();
		titleEdit->setText (QString::fromStdString (txt));

		int flags = st->figure_flags ();
		if (flags & 32768)
			flags = 256;
		coordsCheckBox->setChecked (!(flags & 1));
		titleCheckBox->setChecked (!(flags & 2));
		koCheckBox->setChecked (!(flags & 4));
		remStoneCheckBox->setChecked (!(flags & 256));
		hoshiCheckBox->setChecked (!(flags & 512));
		int mp = st->print_numbering ();
		if (mp < 0)
			mp = -1;
		movenumComboBox->setCurrentIndex (mp + 1);
		int move = st->move_number ();
		int sgfmn = st->sgf_move_number ();
		moveOverrideCheckBox->setChecked (move != sgfmn);
		if (move != sgfmn)
			movenumEdit->setText (QString::number (sgfmn));
	}
	connect (buttonBox->button (QDialogButtonBox::Apply), &QPushButton::clicked, this,
		 [=] (bool) { apply_changes (); m_win->update_figure_display (); });
}

FigureDialog::~FigureDialog ()
{
	delete m_intval;
}

void FigureDialog::apply_changes ()
{
	int mp = movenumComboBox->currentIndex ();
	m_state->set_print_numbering (mp - 1);
	int flags = 0;
	flags |= coordsCheckBox->isChecked () ? 0 : 1;
	flags |= titleCheckBox->isChecked () ? 0 : 2;
	flags |= koCheckBox->isChecked () ? 0 : 4;
	flags |= remStoneCheckBox->isChecked () ? 0 : 256;
	flags |= hoshiCheckBox->isChecked () ? 0 : 512;
	m_state->set_figure (flags, titleEdit->text ().toStdString ());
	if (moveOverrideCheckBox->isChecked ())
		m_state->set_sgf_move_number (movenumEdit->text ().toInt ());
	else
		m_state->set_sgf_move_number (m_state->move_number ());
}

void FigureDialog::accept ()
{
	apply_changes ();
	QDialog::accept ();
}
