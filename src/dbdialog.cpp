#include <QPushButton>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <memory>
#include <algorithm>

#include "gogame.h"
#include "dbdialog.h"
#include "clientwin.h"

#include "ui_dbdialog_gui.h"

DBDialog::DBDialog (QWidget *parent)
	: QDialog (parent), ui (std::make_unique<Ui::DBDialog> ()), m_model (false)
{
	ui->setupUi (this);

	game_info info;
	info.name_w = tr ("White").toStdString ();
	info.name_b = tr ("Black").toStdString ();
	m_empty_game = std::make_shared<game_record> (go_board (19), black, info);
	m_game = m_empty_game;
	m_last_move = m_game->get_root ();

	clear_preview ();

	ui->gameNumLabel->setText (m_model.status_string ());
	ui->dbListView->setModel (&m_model);

	connect (&m_model, &gamedb_model::signal_changed, [this] () { ui->gameNumLabel->setText (m_model.status_string ()); });
	setWindowTitle (tr ("Open SGF file from database"));

	connect (ui->dbListView, &ClickableListView::doubleclicked, this, &DBDialog::handle_doubleclick);
	connect (ui->dbListView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [this] (const QItemSelection &, const QItemSelection &) { update_selection (); });

	connect (ui->encodingList, &QComboBox::currentTextChanged, [this] (const QString &) { update_selection (); });
	connect (ui->overwriteSGFEncoding, &QGroupBox::toggled, [this] (bool) { update_selection (); });

	connect (ui->resetButton, &QPushButton::clicked,
		 [this] (bool) { m_model.reset_filters (); ui->gameNumLabel->setText (m_model.status_string ()); });
	connect (ui->clearButton, &QPushButton::clicked, this, &DBDialog::clear_filters);
	connect (ui->applyButton, &QPushButton::clicked, this, &DBDialog::apply_filters);

	connect (ui->buttonBox->button (QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DBDialog::reject);
	QAbstractButton *open = ui->buttonBox->button (QDialogButtonBox::Open);
	connect (open, &QPushButton::clicked, this, &DBDialog::accept);
	open->setEnabled (false);

	ui->applyButton->setShortcut (Qt::Key_Return);
	ui->boardView->reset_game (m_game);
	ui->boardView->set_show_coords (false);

	connect (ui->goFirstButton, &QPushButton::clicked,
		 [this] (bool) { ui->boardView->set_displayed (m_game->get_root ()); update_buttons (); });
	connect (ui->goLastButton, &QPushButton::clicked,
		 [this] (bool) { ui->boardView->set_displayed (m_last_move); update_buttons (); });
	connect (ui->goNextButton, &QPushButton::clicked,
		 [this] (bool) { ui->boardView->set_displayed (ui->boardView->displayed ()->next_primary_move ()); update_buttons (); });
	connect (ui->goPrevButton, &QPushButton::clicked,
		 [this] (bool) { ui->boardView->set_displayed (ui->boardView->displayed ()->prev_move ()); update_buttons (); });

	connect (ui->dbConfButton, &QPushButton::clicked, [] (bool) { client_window->dlgSetPreferences (6); });
}

DBDialog::~DBDialog ()
{
}

void DBDialog::apply_filters (bool)
{
	m_model.apply_filter (ui->p1Edit->text (), ui->p2Edit->text (), ui->eventEdit->text (), ui->fromEdit->text (), ui->toEdit->text ());
	ui->gameNumLabel->setText (m_model.status_string ());
	ui->dbListView->update ();
}

void DBDialog::clear_filters (bool)
{
	ui->p1Edit->setText ("");
	ui->p2Edit->setText ("");
	ui->eventEdit->setText ("");
	ui->fromEdit->setText ("");
	ui->toEdit->setText ("");
}

void DBDialog::update_buttons ()
{
	const game_state *st = ui->boardView->displayed ();
	ui->goFirstButton->setEnabled (!st->root_node_p ());
	ui->goLastButton->setEnabled (st->n_children () > 0);
	ui->goNextButton->setEnabled (st->n_children () > 0);
	ui->goPrevButton->setEnabled (!st->root_node_p ());
}

void DBDialog::clear_preview ()
{
	ui->boardView->reset_game (m_empty_game);
	m_game = m_empty_game;
	m_last_move = m_game->get_root ();

	// ui->displayBoard->clearData ();

	ui->File_WhitePlayer->setText ("");
	ui->File_BlackPlayer->setText ("");
	ui->File_Date->setText ("");
	ui->File_Handicap->setText ("");
	ui->File_Result->setText ("");
	ui->File_Komi->setText ("");
	ui->File_Size->setText ("");
	ui->File_Event->setText ("");
	ui->File_Round->setText ("");

	ui->goFirstButton->setEnabled (false);
	ui->goLastButton->setEnabled (false);
	ui->goNextButton->setEnabled (false);
	ui->goPrevButton->setEnabled (false);
}

bool DBDialog::setPath (QString path)
{
	clear_preview ();

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
		while (st->n_children () > 0)
			st = st->next_primary_move ();
		m_last_move = st;

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

		update_buttons ();
		return true;
	} catch (...) {
	}
	return false;
}

bool DBDialog::update_selection ()
{
	QItemSelectionModel *sel = ui->dbListView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	QAbstractButton *open = ui->buttonBox->button (QDialogButtonBox::Open);
	open->setEnabled (selection);

	if (!selection)
		return false;

	QModelIndex i = selected.first ();
	const gamedb_entry &e = m_model.find (i.row ());
	QDir d (e.dirname);
	QString filename = d.filePath (e.filename);
	qDebug () << filename;
	setPath (filename);
	return true;
}

void DBDialog::handle_doubleclick ()
{
	if (!update_selection ())
		return;
	QDialog::accept ();
}

void DBDialog::accept ()
{
	QDialog::accept ();
}
