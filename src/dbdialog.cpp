#include <QPushButton>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>

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

	m_model.populate_list ();
	gameNumLabel->setText (m_model.status_string ());
	dbListView->setModel (&m_model);

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

const DBDialog::entry &DBDialog::db_model::find (size_t row) const
{
	return m_all_entries[m_entries[row]];
}

QVariant DBDialog::db_model::data (const QModelIndex &index, int role) const
{
	int row = index.row ();
	int col = index.column ();
	if (row < 0 || (size_t)row >= m_entries.size () || role != Qt::DisplayRole || col != 0)
		return QVariant ();
	const entry &e = find (row);
	return e.pw + "-" + e.pb + ", " + e.result + ", " + e.date;
}

QModelIndex DBDialog::db_model::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

QModelIndex DBDialog::db_model::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

int DBDialog::db_model::rowCount (const QModelIndex &) const
{
	return m_entries.size ();
}

int DBDialog::db_model::columnCount (const QModelIndex &) const
{
	return 1;
}

QVariant DBDialog::db_model::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	switch (section) {
	case 0:
		return tr ("Players");
	case 1:
		return tr ("Res.");
	case 2:
		return tr ("Date");
	}
	return QVariant ();
}

void DBDialog::db_model::populate_list ()
{
	beginResetModel ();
	m_all_entries.clear ();
	m_entries.clear ();
	for (auto &it: setting->m_dbpaths) {
		QSqlDatabase db = QSqlDatabase::addDatabase ("QSQLITE");
		QDir dbdir (it);
		if (!dbdir.exists ("kombilo.db"))
			continue;
		QString dbpath = dbdir.filePath ("kombilo.db");
		db.setDatabaseName (dbpath);
		db.open ();
		QSqlQuery q1 ("select * from db_info where rowid = 1");
		if (!q1.next () || q1.value (0) != "kombilo 0.7")
			continue;

		QSqlQuery q2 ("select filename,pw,pb,dt,re,ev from GAMES");
		while (q2.next ()) {
			m_entries.push_back (m_all_entries.size ());
			QString filename = dbdir.filePath (q2.value (0).toString ());
			QString date = q2.value (3).toString ();
			QRegExp re_full ("(\\d\\d\\d\\d)[^\\d](\\d\\d)[^\\d](\\d\\d)");
			QRegExp re_noday ("(\\d\\d\\d\\d)[^\\d](\\d\\d)");
			QRegExp re_year ("(\\d\\d\\d\\d)");
			/* Compatibility with old versions of qGo/q4go/q5go before we corrected
			   that little problem.  */
			QRegExp re_compat ("^(\\d\\d) (\\d\\d) (\\d\\d\\d\\d)$");
			if (re_compat.exactMatch (date)) {
				date = re_compat.cap (3) + "-" + re_compat.cap (2) + "-" + re_compat.cap (1);
			} else if (re_full.indexIn (date) != -1) {
				QStringList sl = re_full.capturedTexts ();
				sl.removeFirst ();
				date = sl.join ("-");
			} else if (re_noday.indexIn (date) != -1) {
				QStringList sl = re_noday.capturedTexts ();
				sl.removeFirst ();
				date = sl.join ("-") + "-??";
			} else if (re_year.indexIn (date) != -1) {
				date = re_year.cap (0) + "-??" "-??";
			} else
				date = "0000" "-??" "-??";
			m_all_entries.emplace_back (filename, q2.value (1).toString (), q2.value (2).toString (),
						    date, q2.value (4).toString (), q2.value (5).toString ());
		}
	}
	std::sort (std::begin (m_entries), std::end (m_entries),
		   [this] (size_t a, size_t b)
		   {
			   const entry &e1 = m_all_entries[a];
			   const entry &e2 = m_all_entries[b];
			   return e1.date > e2.date;
		   });
	endResetModel ();
}

void DBDialog::update_prefs ()
{
	if (!setting->dbpaths_changed)
		return;
	setting->dbpaths_changed = false;
	m_model.populate_list ();
	gameNumLabel->setText (m_model.status_string ());
}

void DBDialog::db_model::reset_filters ()
{
	beginResetModel ();
	m_entries.clear ();
	m_entries.reserve (m_all_entries.size ());
	for (size_t i = 0; i < m_all_entries.size (); i++)
		m_entries.push_back (i);

	endResetModel ();
}

QString DBDialog::db_model::status_string () const
{
	return QString::number (m_entries.size ()) + "/" + QString::number (m_all_entries.size ()) + " games";
}

void DBDialog::db_model::apply_filter (const QString &p1, const QString &p2, const QString &event,
				       const QString &dtfrom, const QString &dtto)
{
	beginResetModel ();
	m_entries.erase (std::remove_if (std::begin (m_entries), std::end (m_entries),
					 [&](size_t idx)
					 {
						 const entry &e = m_all_entries[idx];
						 if (!p1.isEmpty () && !e.pw.contains (p1) && !e.pb.contains (p1))
							 return true;
						 if (!p2.isEmpty () && !e.pw.contains (p2) && !e.pb.contains (p2))
							 return true;
						 if (!dtfrom.isEmpty () && e.date < dtfrom)
							 return true;
						 if (!dtto.isEmpty () && e.date > dtto)
							 return true;
#if 1
						 if (!event.isEmpty () && !e.event.contains (event) && !e.pb.contains (event))
							 return true;
#endif
						 return false;
					 }),
			 std::end (m_entries));
	endResetModel ();
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
	const entry &e = m_model.find (i.row ());
	QString filename = e.filename;
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
