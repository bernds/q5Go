#include <QPushButton>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <memory>
#include <algorithm>

#include "gogame.h"
#include "dbdialog.h"
#include "clientwin.h"

DBDialog::DBDialog (QWidget *parent)
	: QDialog (parent)
{
	setupUi (this);

	game_info info;
	info.name_w = tr ("White").toStdString ();
	info.name_b = tr ("Black").toStdString ();
	m_empty_game = std::make_shared<game_record> (go_board (19), black, info);
	m_game = m_empty_game;
	m_last_move = m_game->get_root ();

	clear_preview ();

	gameNumLabel->setText (m_model.status_string ());
	dbListView->setModel (&m_model);

	connect (&m_model, &gamedb_model::signal_changed, [this] () { gameNumLabel->setText (m_model.status_string ()); });
	setWindowTitle (tr ("Open SGF file from database"));

	connect (dbListView, &ClickableListView::doubleclicked, this, &DBDialog::handle_doubleclick);
	connect (dbListView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [this] (const QItemSelection &, const QItemSelection &) { update_selection (); });

	connect (encodingList, &QComboBox::currentTextChanged, [this] (const QString &) { update_selection (); });
	connect (overwriteSGFEncoding, &QGroupBox::toggled, [this] (bool) { update_selection (); });

	connect (resetButton, &QPushButton::clicked,
		 [this] (bool) { m_model.reset_filters (); gameNumLabel->setText (m_model.status_string ()); });
	connect (clearButton, &QPushButton::clicked, this, &DBDialog::clear_filters);
	connect (applyButton, &QPushButton::clicked, this, &DBDialog::apply_filters);

	connect (buttonBox->button (QDialogButtonBox::Cancel), &QPushButton::clicked, this, &DBDialog::reject);
	QAbstractButton *open = buttonBox->button (QDialogButtonBox::Open);
	connect (open, &QPushButton::clicked, this, &DBDialog::accept);
	open->setEnabled (false);

	applyButton->setShortcut (Qt::Key_Return);
	boardView->reset_game (m_game);
	boardView->set_show_coords (false);

	connect (goFirstButton, &QPushButton::clicked,
		 [this] (bool) { boardView->set_displayed (m_game->get_root ()); update_buttons (); });
	connect (goLastButton, &QPushButton::clicked,
		 [this] (bool) { boardView->set_displayed (m_last_move); update_buttons (); });
	connect (goNextButton, &QPushButton::clicked,
		 [this] (bool) { boardView->set_displayed (boardView->displayed ()->next_primary_move ()); update_buttons (); });
	connect (goPrevButton, &QPushButton::clicked,
		 [this] (bool) { boardView->set_displayed (boardView->displayed ()->prev_move ()); update_buttons (); });

	connect (dbConfButton, &QPushButton::clicked, [] (bool) { client_window->dlgSetPreferences (6); });
}

DBDialog::~DBDialog ()
{
}

void DBDialog::apply_filters (bool)
{
	m_model.apply_filter (p1Edit->text (), p2Edit->text (), eventEdit->text (), fromEdit->text (), toEdit->text ());
	gameNumLabel->setText (m_model.status_string ());
	dbListView->update ();
}

void DBDialog::clear_filters (bool)
{
	p1Edit->setText ("");
	p2Edit->setText ("");
	eventEdit->setText ("");
	fromEdit->setText ("");
	toEdit->setText ("");
}

void DBDialog::update_buttons ()
{
	const game_state *st = boardView->displayed ();
	goFirstButton->setEnabled (!st->root_node_p ());
	goLastButton->setEnabled (st->n_children () > 0);
	goNextButton->setEnabled (st->n_children () > 0);
	goPrevButton->setEnabled (!st->root_node_p ());
}

void DBDialog::clear_preview ()
{
	boardView->reset_game (m_empty_game);
	m_game = m_empty_game;
	m_last_move = m_game->get_root ();

	// ui->displayBoard->clearData ();

	File_WhitePlayer->setText ("");
	File_BlackPlayer->setText ("");
	File_Date->setText ("");
	File_Handicap->setText ("");
	File_Result->setText ("");
	File_Komi->setText ("");
	File_Size->setText ("");
	File_Event->setText ("");
	File_Round->setText ("");

	goFirstButton->setEnabled (false);
	goLastButton->setEnabled (false);
	goNextButton->setEnabled (false);
	goPrevButton->setEnabled (false);
}

bool DBDialog::setPath (QString path)
{
	clear_preview ();

	try {
		QFile f (path);
		f.open (QIODevice::ReadOnly);
		// IOStreamAdapter adapter (&f);
		sgf *sgf = load_sgf (f);
		if (overwriteSGFEncoding->isChecked ()) {
			m_game = sgf2record (*sgf, QTextCodec::codecForName (encodingList->currentText ().toLatin1 ()));
		} else {
			m_game = sgf2record (*sgf, nullptr);
		}
		m_game->set_filename (path.toStdString ());

		boardView->reset_game (m_game);
		game_state *st = m_game->get_root ();
		for (int i = 0; i < 20 && st->n_children () > 0; i++)
			st = st->next_primary_move ();
		boardView->set_displayed (st);
		while (st->n_children () > 0)
			st = st->next_primary_move ();
		m_last_move = st;

		const game_info &info = m_game->info ();
		File_WhitePlayer->setText (QString::fromStdString (info.name_w));
		File_BlackPlayer->setText (QString::fromStdString (info.name_b));
		File_Date->setText (QString::fromStdString (info.date));
		File_Handicap->setText (QString::number (info.handicap));
		File_Result->setText (QString::fromStdString (info.result));
		File_Komi->setText (QString::number (info.komi));
		File_Size->setText (QString::number (st->get_board ().size_x ()));
		File_Event->setText (QString::fromStdString (info.event));
		File_Round->setText (QString::fromStdString (info.round));

		update_buttons ();
		return true;
	} catch (...) {
	}
	return false;
}

bool DBDialog::update_selection ()
{
	QItemSelectionModel *sel = dbListView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	QAbstractButton *open = buttonBox->button (QDialogButtonBox::Open);
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
