#include <QPushButton>
#include <QFileDialog>

#include "gogame.h"
#include "sgfpreview.h"
#include "ui_sgfpreview.h"

SGFPreview::SGFPreview (QWidget *parent, const QString &dir)
	: QDialog (parent), ui (std::make_unique<Ui::SGFPreview> ())
{
	ui->setupUi (this);

	game_info info;
	info.name_w = tr ("White").toStdString ();
	info.name_b = tr ("Black").toStdString ();
	m_empty_game = std::make_shared<game_record> (go_board (19), black, info);
	m_game = m_empty_game;

	QVBoxLayout *l = new QVBoxLayout (ui->dialogWidget);
	fileDialog = new QFileDialog (ui->dialogWidget, Qt::Widget);
	fileDialog->setOption (QFileDialog::DontUseNativeDialog, true);
	fileDialog->setWindowFlags (Qt::Widget);
	fileDialog->setNameFilters ({ tr ("SGF files (*.sgf *.SGF)"), tr ("All files (*)") });
	fileDialog->setDirectory (dir);

	setWindowTitle ("Open SGF file");
	l->addWidget (fileDialog);
	l->setContentsMargins (0, 0, 0, 0);
	fileDialog->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
	fileDialog->show ();
	connect (ui->encodingList, &QComboBox::currentTextChanged, this, &SGFPreview::reloadPreview);
	connect (ui->overwriteSGFEncoding, &QGroupBox::toggled, this, &SGFPreview::reloadPreview);
	connect (fileDialog, &QFileDialog::currentChanged, this, &SGFPreview::setPath);
	connect (fileDialog, &QFileDialog::accepted, this, &QDialog::accept);
	connect (fileDialog, &QFileDialog::rejected, this, &QDialog::reject);
	ui->boardView->reset_game (m_game);
	ui->boardView->set_show_coords (false);
}

SGFPreview::~SGFPreview ()
{
}

void SGFPreview::clear ()
{
	ui->boardView->reset_game (m_empty_game);
	m_game = nullptr;

	// ui->displayBoard->clearData ();

	ui->File_WhitePlayer->setText("");
	ui->File_BlackPlayer->setText("");
	ui->File_Date->setText("");
	ui->File_Handicap->setText("");
	ui->File_Result->setText("");
	ui->File_Komi->setText("");
	ui->File_Size->setText("");
	ui->File_Event->setText("");
	ui->File_Round->setText("");
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
		if (ui->overwriteSGFEncoding->isChecked ()) {
			m_game = sgf2record (*sgf, QTextCodec::codecForName (ui->encodingList->currentText ().toLatin1 ()));
		} else {
			m_game = sgf2record (*sgf, nullptr);
		}
		m_game->set_filename (path.toStdString ());

		ui->boardView->reset_game (m_game);
		game_state *st = m_game->get_root ();
		for (int i = 0; i < 20 && st->n_children () > 0; i++)
			st = st->next_primary_move ();
		ui->boardView->set_displayed (st);

		const game_info &info = m_game->info ();
		ui->File_WhitePlayer->setText (QString::fromStdString (info.name_w));
		ui->File_BlackPlayer->setText (QString::fromStdString (info.name_b));
		ui->File_Date->setText (QString::fromStdString (info.date));
		ui->File_Handicap->setText (QString::number (info.handicap));
		ui->File_Result->setText (QString::fromStdString (info.result));
		ui->File_Komi->setText (QString::number (info.komi));
		ui->File_Size->setText (QString::number (st->get_board ().size_x ()));
		ui->File_Event->setText (QString::fromStdString (info.event));
		ui->File_Round->setText (QString::fromStdString (info.round));
	} catch (...) {
	}
}

void SGFPreview::reloadPreview ()
{
	auto files = fileDialog->selectedFiles ();
	if (!files.isEmpty ())
		setPath (files.at (0));
}

void SGFPreview::accept ()
{
	QDialog::accept ();
}
