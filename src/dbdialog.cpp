#include <QPushButton>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <memory>
#include <algorithm>

#include "gogame.h"
#include "dbdialog.h"
#include "clientwin.h"

QThread *db_thread;
GameDB_Data *db_data;
GameDB_Data_Controller *db_data_controller;

void start_db_thread ()
{
	db_thread = new QThread;
	db_thread->start (QThread::LowPriority);
	db_data = new GameDB_Data;

	db_data->moveToThread (db_thread);
	QObject::connect (db_thread, &QThread::finished, db_data, &QObject::deleteLater);

	db_data_controller = new GameDB_Data_Controller;
	db_data_controller->load ();
}

void end_db_thread ()
{
	db_thread->exit ();
	db_thread->wait ();
	delete db_thread;
	delete db_data_controller;
}

GameDB_Data_Controller::GameDB_Data_Controller ()
{
	connect (this, &GameDB_Data_Controller::signal_start_load, db_data, &GameDB_Data::slot_start_load);
	connect (db_data, &GameDB_Data::signal_load_complete, this, &GameDB_Data_Controller::slot_load_complete);
}

void GameDB_Data_Controller::load ()
{
	int max_mb = setting->readIntEntry ("DB_MAX_FILESIZE");
	if (max_mb < 1 || max_mb > 999)
		max_mb = 50;
	db_data->load_complete = false;
	db_data->too_large = false;
	db_data->errors_found = false;

	emit signal_prepare_load ();
	emit signal_start_load (max_mb, setting->readBoolEntry ("DB_CACHE_MOVELIST"));
}

void GameDB_Data_Controller::slot_load_complete ()
{
	QString s;
	if (db_data->too_large)
		s += tr ("<p>One of the database files was too large to be loaded.  You can adjust the limit in the settings window.</p>");
	if (db_data->errors_found)
		s += tr ("<p>One of the database files contained errors.</p>");
	if (!s.isEmpty ())
		QMessageBox::warning (nullptr, PACKAGE, s);
}

class db_errors_found : public std::exception
{
public:
	db_errors_found ()
	{
	}
};


/* Read an extra kombilo file (kombilo.da to go with the kombilo.db database).  */

bool GameDB_Data::read_extra_file (QDataStream &ds, int base_id, int boardsize, int max_mb, bool cache_movelist)
{
	auto beg = std::begin (m_all_entries);
	auto end = std::end (m_all_entries);
	beg = std::lower_bound (beg, end, base_id,
				[] (const gamedb_entry &e, int val)
				{
					return e.id < val;
				});

	size_t sz;
	char sz_c[sizeof (size_t)];
	char i_c[sizeof (int)];
	/* The first section of this file is signature data, which is irrelevant to us.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();

	memcpy (&sz, sz_c, sizeof sz);
	ds.skipRawData (sz);
	size_t full_total = sz + sizeof sz;

	/* The second part contains game final positions, which we add to the gamedb_entry records.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();
	memcpy (&sz, sz_c, sizeof sz);
	size_t relevant_size = sz;

	if (relevant_size / 1024 / 1024 > (size_t)max_mb) {
		too_large = true;
		return false;
	}

	int count;
	if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
		throw db_errors_found ();
	memcpy (&count, i_c, sizeof count);

	std::vector<char> buf;
	size_t total = 0;
	for (int i = 0; i < count; i++) {
		int clen;
		int gameid;

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&gameid, i_c, sizeof gameid);
		gameid += base_id;

		auto pos = std::lower_bound (beg, end, gameid,
					     [] (const gamedb_entry &e, int val)
					     {
						     return e.id < val;
					     });
		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);

		total += sizeof gameid + sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		buf.resize (clen);
		if (ds.readRawData (&buf[0], clen) != clen)
			throw db_errors_found ();

		if (pos != end) {
#ifdef CHECKING
			if (pos->id != gameid)
				abort ();
#endif
			for (int y = 0; y < boardsize; y++) {
				for (int x = 0; x < boardsize; x++) {
					int bufpos = y / 2 + 10 * (x / 2);
					int bitpos = 2 * (x % 2 + 2 * (y % 2));
					char val = bufpos < clen ? buf[bufpos] : 0;
					if ((val & (1 << bitpos)) == 0)
						pos->finalpos_b.set_bit (x + y * boardsize);
					if ((val & (1 << (bitpos + 1))) == 0)
						pos->finalpos_w.set_bit (x + y * boardsize);
#if 0
					char c = ' ';
					if (pos->finalpos_w.test_bit (x + y * boardsize))
						c = 'O';
					if (pos->finalpos_b.test_bit (x + y * boardsize))
						c = c == ' ' ? 'X' : '#';
					putchar (c);
#endif
				}
#if 0
				putchar ('\n');
#endif
			}
		}

		if (total == sz) {
			if (i + 1 != count)
				throw db_errors_found ();
		}
	}
	full_total += sz + sizeof sz;

	/* The third part contains move list and final-position captures.  */
	if (ds.readRawData (sz_c, sizeof sz_c) != sizeof sz_c)
		throw db_errors_found ();
	memcpy (&sz, sz_c, sizeof sz);

	relevant_size += sz;
	if (relevant_size / 1024 / 1024 > (size_t)max_mb) {
		too_large = true;
		return false;
	}

	if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
		throw db_errors_found ();
	memcpy (&count, i_c, sizeof count);
	total = 0;
	for (int i = 0; i < count; i++) {
		int clen;
		int gameid;

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&gameid, i_c, sizeof gameid);
		gameid += base_id;

		auto pos = std::lower_bound (beg, end, gameid,
					     [] (const gamedb_entry &e, int val)
					     {
						     return e.id < val;
					     });
		total += sizeof gameid;
		if (pos != end) {
#ifdef CHECKING
			if (pos->movelist_off != 0)
				abort ();
#endif
			pos->movelist_off = full_total + total + sizeof sz + sizeof count + sizeof (int);
		}
		/* ??? Strange file format; it saves the length of the movelist first as
		   a plain integer, then again as part of the char array.  */
		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		int dlen;
		memcpy (&dlen, i_c, sizeof dlen);

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);
		if (clen != dlen)
			throw db_errors_found ();

		total += 2 * sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		if (pos != end && cache_movelist) {
			pos->movelist.resize (clen);
			if (ds.readRawData (&pos->movelist[0], clen) != clen)
				throw db_errors_found ();
		} else
			ds.skipRawData (clen);

		if (ds.readRawData (i_c, sizeof i_c) != sizeof i_c)
			throw db_errors_found ();
		memcpy (&clen, i_c, sizeof clen);
		buf.resize (clen);

		total += sizeof clen + clen;
		if (total > sz)
			throw db_errors_found ();

		if (ds.readRawData (&buf[0], clen) != clen)
			throw db_errors_found ();
		if (pos != end) {
			for (int y = 0; y < boardsize; y++) {
				for (int x = 0; x < boardsize; x++) {
					int bufpos = y / 4 + 5 * (x / 2);
					int bitpos = x % 2 + 2 * (y % 4);
					char val = bufpos < clen ? buf[bufpos] : 0;
					if ((val & (1 << bitpos)) != 0)
						pos->finalpos_c.set_bit (x + y * boardsize);
				}
			}
		}
	}

	return true;
}

void GameDB_Data::do_load (int max_mb, bool cache_movelist)
{
	QMutexLocker lock (&db_mutex);
	m_all_entries.clear ();

	QSqlDatabase db = QSqlDatabase::database ("kombilo");

	QRegularExpression re_full ("(\\d\\d\\d\\d)[^\\d](\\d\\d)[^\\d](\\d\\d)");
	QRegularExpression re_noday ("(\\d\\d\\d\\d)[^\\d](\\d\\d)");
	QRegularExpression re_year ("(\\d\\d\\d\\d)");
	/* Compatibility with old versions of qGo/q4go/q5go before we corrected
	   that little problem.  */
	QRegularExpression re_compat ("^(\\d\\d) (\\d\\d) (\\d\\d\\d\\d)$");

	/* Assign all games unique IDs, combining the stored ID with the current total.  */
	int base_id = 0;
	for (auto &it: dbpaths) {
		QDir dbdir (it);
		if (!dbdir.exists ("kombilo.db"))
			continue;
		QString dbpath = dbdir.filePath ("kombilo.db");
		db.setDatabaseName (dbpath);
		db.open ();
		QSqlQuery q1 ("select * from db_info where rowid = 1", db);
		if (!q1.next () || (q1.value (0) != "kombilo 0.7" && q1.value (0) != "kombilo 0.8")) {
			qDebug () << "skipping database with " << q1.value (0);
			continue;
		}
		QSqlQuery q1b ("select * from db_info where rowid = 3", db);
		if (!q1b.next ()) {
			qDebug () << "skipping database without boardsize";
			continue;
		}
		int boardsize = q1b.value (0).toInt ();

		QSqlQuery q2 ("select filename,pw,pb,dt,re,ev,id from GAMES order by id", db);
		int this_max = 0;
		while (q2.next ()) {
			QString filename = q2.value (0).toString ();
			QString date = q2.value (3).toString ();
			auto compat = re_compat.match (date);
			int id = q2.value (6).toInt ();
			this_max = std::max (this_max, id);
			id += base_id;
			if (compat.hasMatch ()) {
				date = compat.captured (3) + "-" + compat.captured (2) + "-" + compat.captured (1);
			} else {
				auto full = re_full.match (date);
				if (full.hasMatch ()) {
					QStringList sl = full.capturedTexts ();
					sl.removeFirst ();
					date = sl.join ("-");
				} else {
					auto noday = re_noday.match (date);
					if (noday.hasMatch ()) {
						QStringList sl = noday.capturedTexts ();
						sl.removeFirst ();
						date = sl.join ("-") + "-??";
					} else {
						auto year = re_year.match (date);
						if (year.hasMatch ()) {
							date = year.captured (0) + "-??" "-??";
						} else
							date = "0000" "-??" "-??";
					}
				}
			}

			m_all_entries.emplace_back (id, it, filename, q2.value (1).toString (), q2.value (2).toString (),
						    date, q2.value (4).toString (), q2.value (5).toString (),
						    boardsize);
		}
		db.close ();

		/* Now read the secondary kombilo data.  */
		QFile f (dbdir.filePath ("kombilo.da"));
		if (f.exists ()) {
			f.open (QIODevice::ReadOnly);
			QDataStream ds (&f);
			try {
				read_extra_file (ds, base_id, boardsize, max_mb, cache_movelist);
			} catch (db_errors_found &) {
				errors_found = true;
			}
			f.close ();
		}
		base_id += this_max + 1;
	}

	load_complete = true;
	std::atomic_thread_fence (std::memory_order_seq_cst);
}

void GameDB_Data::slot_start_load (int max_mb, bool cache_movelist)
{
	static bool init_once = false;
	if (!init_once) {
		QSqlDatabase::addDatabase ("QSQLITE", "kombilo");
		init_once = true;
	}

	setting->m_dbpath_lock.lock ();
	dbpaths = setting->m_dbpaths;
	setting->m_dbpath_lock.unlock ();

	do_load (max_mb, cache_movelist);
	emit signal_load_complete ();
}

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

gamedb_model::gamedb_model ()
{
	connect (db_data, &GameDB_Data::signal_load_complete, this, &gamedb_model::slot_load_complete);
	connect (db_data_controller, &GameDB_Data_Controller::signal_prepare_load, this, &gamedb_model::slot_prepare_load);

	if (db_data->load_complete)
		slot_load_complete ();
}

const gamedb_entry &gamedb_model::find (size_t row) const
{
	return db_data->m_all_entries[m_entries[row]];
}

QVariant gamedb_model::data (const QModelIndex &index, int role) const
{
	int row = index.row ();
	int col = index.column ();
	if (row < 0 || (size_t)row >= m_entries.size () || role != Qt::DisplayRole || col != 0)
		return QVariant ();
	const gamedb_entry &e = find (row);
	return e.pw + " - " + e.pb + ", " + e.result + ", " + e.date;
}

QModelIndex gamedb_model::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

QModelIndex gamedb_model::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

int gamedb_model::rowCount (const QModelIndex &) const
{
	return m_entries.size ();
}

int gamedb_model::columnCount (const QModelIndex &) const
{
	return 1;
}

QVariant gamedb_model::headerData (int section, Qt::Orientation ot, int role) const
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

void gamedb_model::clear_list ()
{
	beginResetModel ();
	m_entries.clear ();
	endResetModel ();
	emit signal_changed ();
}

void gamedb_model::slot_prepare_load ()
{
	clear_list ();
}

void gamedb_model::slot_load_complete ()
{
	std::atomic_thread_fence (std::memory_order_seq_cst);
	reset_filters ();
}

void gamedb_model::default_sort ()
{
	std::sort (std::begin (m_entries), std::end (m_entries),
		   [this] (unsigned a, unsigned b)
		   {
			   const gamedb_entry &e1 = db_data->m_all_entries[a];
			   const gamedb_entry &e2 = db_data->m_all_entries[b];
			   return e1.date > e2.date;
		   });
}

void gamedb_model::reset_filters ()
{
	beginResetModel ();
	m_entries.clear ();
	m_entries.reserve (db_data->m_all_entries.size ());
	for (size_t i = 0; i < db_data->m_all_entries.size (); i++)
		m_entries.push_back (i);

	default_sort ();

	endResetModel ();
	emit signal_changed ();
}

QString gamedb_model::status_string () const
{
	return QString::number (m_entries.size ()) + "/" + QString::number (db_data->m_all_entries.size ()) + " games";
}

void gamedb_model::apply_game_list (std::vector<unsigned> &&games)
{
	beginResetModel ();
	m_entries = games;
	default_sort ();
	endResetModel ();
	emit signal_changed ();
}

void gamedb_model::apply_filter (const QString &p1, const QString &p2, const QString &event,
				 const QString &dtfrom, const QString &dtto)
{
	beginResetModel ();
	m_entries.erase (std::remove_if (std::begin (m_entries), std::end (m_entries),
					 [&](unsigned idx)
					 {
						 const gamedb_entry &e = db_data->m_all_entries[idx];
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
	emit signal_changed ();
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

void update_db_prefs ()
{
	if (!setting->dbpaths_changed)
		return;
	setting->dbpaths_changed = false;

	db_data_controller->load ();
}
