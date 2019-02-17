#include <QPushButton>
#include <QFileDialog>

#include <fstream>

#include "gogame.h"
#include "sgfpreview.h"

SGFPreview::SGFPreview (QWidget *parent, const QString &dir)
	: QDialog (parent), m_empty_board (go_board (19), black),
	  m_empty_game (std::make_shared<game_record> (go_board (19), black, game_info ("White", "Black"))),
	  m_game (m_empty_game)
{
	setupUi (this);

	QHBoxLayout *l = new QHBoxLayout (dialogWidget);
	fileDialog = new QFileDialog (dialogWidget, Qt::Widget);
	fileDialog->setOption (QFileDialog::DontUseNativeDialog, true);
	fileDialog->setWindowFlags (Qt::Widget);
	fileDialog->setNameFilters ({ tr ("SGF files (*.sgf *.SGF)"), tr ("All files (*)") });
	fileDialog->setDirectory (dir);

	setWindowTitle ("Open SGF file");
	l->addWidget (fileDialog);
	l->setContentsMargins (0, 0, 0, 0);
	fileDialog->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
	fileDialog->show ();
	connect (fileDialog, &QFileDialog::currentChanged, this, &SGFPreview::setPath);
	connect (fileDialog, &QFileDialog::accepted, this, &QDialog::accept);
	connect (fileDialog, &QFileDialog::rejected, this, &QDialog::reject);
	boardView->reset_game (m_game);
	boardView->set_show_coords (false);
}

SGFPreview::~SGFPreview ()
{
}

void SGFPreview::clear ()
{
	boardView->reset_game (m_empty_game);
	m_game = nullptr;

	// ui->displayBoard->clearData ();

	File_WhitePlayer->setText("");
	File_BlackPlayer->setText("");
	File_Date->setText("");
	File_Handicap->setText("");
	File_Result->setText("");
	File_Komi->setText("");
	File_Size->setText("");
}

QStringList SGFPreview::selected ()
{
	return fileDialog->selectedFiles ();
}

void SGFPreview::setPath(QString path)
{
	clear ();

	try {
		QFile f (path);
		f.open (QIODevice::ReadOnly);
		// IOStreamAdapter adapter (&f);
		sgf *sgf = load_sgf (f);
		m_game = sgf2record (*sgf);
		m_game->set_filename (path.toStdString ());

		boardView->reset_game (m_game);
		game_state *st = m_game->get_root ();
		for (int i = 0; i < 20 && st->n_children () > 0; i++)
			st = st->next_primary_move ();
		boardView->set_displayed (st);

		File_WhitePlayer->setText (QString::fromStdString (m_game->name_white ()));
		File_BlackPlayer->setText (QString::fromStdString (m_game->name_black ()));
		File_Date->setText (QString::fromStdString (m_game->date ()));
		File_Handicap->setText (QString::number (m_game->handicap ()));
		File_Result->setText (QString::fromStdString (m_game->result ()));
		File_Komi->setText (QString::number (m_game->komi ()));
		File_Size->setText (QString::number (st->get_board ().size_x ()));
	} catch (...) {
	}
}

void SGFPreview::accept ()
{
	QDialog::accept ();
}
