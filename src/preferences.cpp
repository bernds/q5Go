/*
* preferences.cpp
*/

#include <memory>

#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QProgressDialog>
#include <QWhatsThis>
#include <QIcon>
#include <QTextStream>

#include "preferences.h"
#include "mainwindow.h"
#include "qgo.h"
#include "clientwin.h"
#include "imagehandler.h"
#include "gamedb.h"

#include "ui_preferences_gui.h"
#include "ui_enginedlg_gui.h"

db_dir_entry::db_dir_entry (const QString &s)
	: title (s), qdb_found (false), kdb_found (false)
{
	QDir d (s);
	qdb_found = QFile (d.filePath ("q5go.db")).exists ();
	kdb_found = QFile (d.filePath ("kombilo.db")).exists ();
}

QVariant db_dir_entry::icon (int col) const
{
	if (col == 0) {
		return QIcon (qdb_found ? ":/images/green-check.png"
			      : kdb_found ? ":/images/yellow-exclam.png"
			      : ":/images/red-x.png");
	}
	return QVariant ();
}


void EngineDialog::init ()
{
	m_komi_vald.setDecimals (2);
	ui->engineKomi->setValidator (&m_komi_vald);
	ui->engineSize->setValidator (&m_size_vald);

	connect (ui->enginePathButton, &QPushButton::clicked, this, &EngineDialog::slot_get_path);
}

/* Create engine.  */
EngineDialog::EngineDialog (QWidget *parent)
	: QDialog (parent), ui (new Ui::EngineDialog)
{
	ui->setupUi (this);
	init ();
}

/* Duplicate or change engine.  */
EngineDialog::EngineDialog (QWidget *parent, const Engine &e, bool dup)
	: QDialog (parent), ui (new Ui::EngineDialog)
{
	ui->setupUi (this);
	if (dup) {
		setWindowTitle (tr ("Create a new engine configuration from an existing one"));
	} else {
		setWindowTitle (tr ("Change an engine configuration"));
		ui->engineName->setReadOnly (true);
	}
	ui->engineName->setText (e.title);
	ui->enginePath->setText (e.path);
	ui->engineArgs->setText (e.args);
	ui->engineKomi->setText (e.komi);
	ui->engineAnalysis->setChecked (e.analysis);
	ui->engineRestrictions->setChecked (e.restrictions);
	ui->engineSize->setText (e.boardsize);
	init ();
}

EngineDialog::~EngineDialog ()
{
	delete ui;
}

void EngineDialog::slot_get_path ()
{
	QString fileName (QFileDialog::getOpenFileName (this, tr ("Choose GTP engine path"),
							setting->readEntry ("LAST_DIR"),
							tr ("All Files (*)")));
	if (!fileName.isEmpty ())
		ui->enginePath->setText (fileName);
}

Engine EngineDialog::get_engine ()
{
	Engine e (ui->engineName->text (), ui->enginePath->text (), ui->engineArgs->text (),
		  ui->engineKomi->text (), ui->engineAnalysis->isChecked (), ui->engineSize->text (),
		  ui->engineRestrictions->isChecked ());

	return e;
}

void EngineDialog::accept ()
{
	if (ui->engineName->text ().isEmpty ()) {
		QMessageBox::warning (this, tr ("No name set for the engine."),
				      tr ("A name must be set for the engine.\nPlease enter all necessary fields before continuing"));
		return;
	}
	if (ui->enginePath->text ().isEmpty ()) {
		QMessageBox::warning (this, tr ("No path set for the engine."),
				      tr ("A path must be set for the engine.\nPlease enter all necessary fields before continuing"));
		return;
	}
	int p = 0;
	QString size = ui->engineSize->text ();
	if (!size.isEmpty () && m_size_vald.validate (size, p) != QValidator::Acceptable) {
		QMessageBox::warning (this, tr ("Invalid size specified"),
				      tr ("The value entered for the board size is invalid.\nPlease enter values between 3 and 25."));
		return;
	}
	if (ui->engineAnalysis->isChecked () && ui->engineRestrictions->isChecked () && size.isEmpty ()) {
		QMessageBox::warning (this, tr ("No boardsize specified for analysis engine."),
				      tr ("Uncheck the \"Restrictions\" field or specify a board size."));
		return;
	}
	QString komi = ui->engineKomi->text ();
	if (!komi.isEmpty () && m_komi_vald.validate (komi, p) != QValidator::Acceptable) {
		QMessageBox::warning (this, tr ("Invalid komi specified"),
				      tr ("The value entered for komi is invalid."));
		return;
	}
	QDialog::accept ();
}


template<class T>
QVariant pref_vec_model<T>::data (const QModelIndex &index, int role) const
{
	int row = index.row ();
	int col = index.column ();

	if (row < 0 || col < 0 || col >= T::n_columns ())
		return QVariant ();

	size_t r = row;
	if (r >= m_entries.size ()) {
		r -= m_entries.size ();
		if (r >= m_extra.size ())
			return QVariant ();
		if (role == Qt::BackgroundRole)
			return QBrush (Qt::lightGray);
		if (role != Qt::DisplayRole)
			return QVariant ();
		return m_extra[r].data (col);
	}
	if (role == Qt::DecorationRole)
		return m_entries[r].icon (col);

	if (role != Qt::DisplayRole)
		return QVariant ();
	const T &e = m_entries[r];
	return e.data (col);
}

template<class T>
const T *pref_vec_model<T>::find (const QModelIndex &index) const
{
	int row = index.row ();
	if (row < 0)
		return nullptr;

	size_t r = row;
	if (r >= m_entries.size ()) {
		r -= m_entries.size ();
		if (r >= m_extra.size ())
			return nullptr;
		return &m_extra[r];
	}
	return &m_entries[r];
}

template<class T>
bool pref_vec_model<T>::removeRows (int row, int count, const QModelIndex &parent)
{
	if (row < 0 || count < 1 || (size_t) (row + count) > m_entries.size ())
		return false;
	beginRemoveRows (parent, row, row + count - 1);
	m_entries.erase (m_entries.begin () + row, m_entries.begin () + (row + count));
	endRemoveRows ();
	return true;
}

template<class T>
void pref_vec_model<T>::update_entry (const QModelIndex &i, const T &v)
{
	int row = i.row ();
	if (row >= 0) {
		m_entries[row] = v;
		emit dataChanged (i, i);
	}
}

template<class T>
void pref_vec_model<T>::add_or_replace (T x)
{
	for (size_t i = 0; i < m_entries.size (); i++) {
		T &e = m_entries[i];
		if (e.title == x.title) {
			e = x;
			emit dataChanged (index (i, 0), index (i, 0));
			return;
		}
	}
	beginInsertRows (QModelIndex (), m_entries.size (), m_entries.size ());
	m_entries.push_back (x);
	endInsertRows ();
}

template<class T>
bool pref_vec_model<T>::add_no_replace (T x)
{
	for (size_t i = 0; i < m_entries.size (); i++) {
		T &e = m_entries[i];
		if (e.title == x.title)
			return false;
	}
	beginInsertRows (QModelIndex (), m_entries.size (), m_entries.size ());
	m_entries.push_back (x);
	endInsertRows ();
	return true;
}

template<class T>
QModelIndex pref_vec_model<T>::index (int row, int col, const QModelIndex &) const
{
	return createIndex (row, col);
}

template<class T>
QModelIndex pref_vec_model<T>::parent (const QModelIndex &) const
{
	return QModelIndex ();
}

template<class T>
int pref_vec_model<T>::rowCount (const QModelIndex &) const
{
	return m_entries.size () + m_extra.size ();
}

template<class T>
int pref_vec_model<T>::columnCount (const QModelIndex &) const
{
	return T::n_columns ();
}

#if 0
template<class T>
bool pref_vec_model<T>::remove (const QString &title)
{
	for (size_t i = 0; i < m_entries.size (); i++)
		if (m_entries[i].title == title) {
			beginRemoveRows (QModelIndex (), i, i);
			m_entries.erase (m_entries.begin () + i);
			endRemoveRows ();
			break;
		}
}
#endif

template<class T>
QVariant pref_vec_model<T>::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	switch (section) {
	case 0:
		return tr ("Name");
	}
	return QVariant ();
}

QVariant dbpath_model::headerData (int section, Qt::Orientation ot, int role) const
{
	if (role == Qt::TextAlignmentRole) {
		return Qt::AlignLeft;
	}

	if (role != Qt::DisplayRole || ot != Qt::Horizontal)
		return QVariant ();
	switch (section) {
	case 0:
		return tr ("DB");
	case 1:
		return tr ("Path");
	}
	return QVariant ();
}

std::vector<Host> standard_servers {
	{ "-- IGS --", "igs.joyjoy.net", 7777, "guest", "", "SJIS", -1 },
	{ "-- LGS --", "lgs.taiwango.net", 9696, "guest", "", "Big5", -1 },
	{ "-- WING --", "wing.gr.jp", 1515, "guest", "", "", -1 } };

PreferencesDialog::PreferencesDialog (int tab, QWidget* parent)
	: QDialog (parent), ui (new Ui::PreferencesDialogGui),
	  m_hosts_model (setting->m_hosts, standard_servers), m_engines_model (setting->m_engines),
	  m_dbpath_model (setting->m_dbpaths)
{
	ui->setupUi (this);
	setModal (true);

	if (tab >= 0 && tab < ui->tabWidget->count())
		ui->tabWidget->setCurrentIndex (tab);

	if (client_window->getPrefSize().width() > 0)
	{
		resize(client_window->getPrefSize());
		move(client_window->getPrefPos());
	}

	// init random-number generator
	srand ((unsigned)time (nullptr));

	ui->LineEdit_port->setValidator (new QIntValidator (0, 9999, this));
	ui->anMaxMovesEdit->setValidator (new QIntValidator (0, 999, this));
	ui->anDepthEdit->setValidator (new QIntValidator (0, 999, this));
	ui->slideXEdit->setValidator (new QIntValidator (100, 9999, this));
	ui->slideYEdit->setValidator (new QIntValidator (100, 9999, this));

	// clear edit field
	ui->LineEdit_title->clear();

	init_from_settings ();

	// Default codec
	ui->ComboBox_codec->addItem ("");
	auto codecs = QTextCodec::availableCodecs ();
	for(auto it: codecs)
		ui->ComboBox_codec->addItem (it);

	m_ih = new ImageHandler;

	int w = ui->stoneView->width ();
	int h = ui->stoneView->height ();
	m_stone_size = std::min (w / 2, h);
	m_ih->init (5);
	m_stone_canvas = new QGraphicsScene(0, 0, w, h, ui->stoneView);
	ui->stoneView->setScene (m_stone_canvas);
	m_b_stone = new QGraphicsPixmapItem;
	m_w_stone = new QGraphicsPixmapItem;
	m_stone_canvas->addItem (m_w_stone);
	m_stone_canvas->addItem (m_b_stone);
	m_w_stone->setZValue (1);

	connect (ui->radioButtonStones_2D, &QRadioButton::toggled, this, &PreferencesDialog::select_stone_look);
	connect (ui->radioButtonStones_3D, &QRadioButton::toggled, this, &PreferencesDialog::select_stone_look);

	connect (ui->whiteColorButton, &QToolButton::clicked, this, &PreferencesDialog::select_white_color);
	connect (ui->blackColorButton, &QToolButton::clicked, this, &PreferencesDialog::select_black_color);
	connect (ui->blackRoundSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (ui->blackHardSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (ui->blackSpecSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (ui->blackFlatSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (ui->whiteRoundSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (ui->whiteHardSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (ui->whiteSpecSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (ui->whiteFlatSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (ui->ambientSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); update_b_stones (); });
	connect (ui->shadowSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); update_b_stones (); });
	connect (ui->stripesCheckBox, &QCheckBox::toggled, [=] (bool) { update_w_stones (); });

	void (QComboBox::*cic) (int) = &QComboBox::currentIndexChanged;
	connect (ui->woodComboBox, cic, [=] (int i)
		 {
			 ui->GobanPicturePathButton->setEnabled (i == 0);
			 ui->LineEdit_goban->setEnabled (i == 0);
			 if (i != 0)
				 ui->scaleWoodCheckBox->setChecked (i == 1 || i > 5);
			 update_board_image ();
		 });
	connect (ui->shadersComboBox, cic, [=] (int) { update_w_stones (); update_b_stones (); });
	connect (ui->LineEdit_goban, &QLineEdit::editingFinished, [=] () { update_board_image (); });

	update_board_image ();
	update_w_stones ();
	update_b_stones ();
	connect (ui->stoneView, &SizeGraphicsView::resized, [=] () { update_board_image (); update_w_stones (); update_b_stones (); });

	connect (ui->GobanPicturePathButton, &QToolButton::clicked, this, &PreferencesDialog::slot_getGobanPicturePath);
	connect (ui->TablePicturePathButton, &QToolButton::clicked, this, &PreferencesDialog::slot_getTablePicturePath);

	ui->dbPathsTreeView->setModel (&m_dbpath_model);
	connect (ui->dbPathsTreeView->selectionModel (), &QItemSelectionModel::selectionChanged,
		 [this] (const QItemSelection &, const QItemSelection &) { update_db_selection (); });
	ui->dbPathsTreeView->header ()->setSectionResizeMode (0, QHeaderView::ResizeToContents);

	connect (ui->dbDirsButton, &QPushButton::clicked, this, &PreferencesDialog::slot_dbdir);
	connect (ui->dbSubdirsButton, &QPushButton::clicked, this, &PreferencesDialog::slot_dbsubdir);
	connect (ui->dbCfgButton, &QPushButton::clicked, this, &PreferencesDialog::slot_dbcfg);
	connect (ui->dbRemButton, &QPushButton::clicked, this, &PreferencesDialog::slot_dbrem);
	connect (ui->dbCreateButton, &QPushButton::clicked, this, &PreferencesDialog::slot_dbcreate);

	ui->ListView_engines->setModel (&m_engines_model);
	connect (ui->ListView_engines, &ClickableListView::current_changed, [this] () { update_current_engine (); });
	ui->ListView_hosts->setModel (&m_hosts_model);
	connect (ui->ListView_hosts, &ClickableListView::current_changed, [this] () { update_current_host (); });

	update_chat_color ();
	connect (ui->chatColorButton, &QToolButton::clicked, this, &PreferencesDialog::select_chat_color);
	connect (ui->chatColorCheckBox, &QToolButton::toggled, [this] () { update_chat_color (); });

	connect (ui->fontStandardButton, &QPushButton::clicked, [this] () { selectFont (ui->fontStandardButton, setting->fontStandard); });
	connect (ui->fontMarksButton, &QPushButton::clicked, [this] () { selectFont (ui->fontMarksButton, setting->fontMarks); });
	connect (ui->fontCommentsButton, &QPushButton::clicked, [this] () { selectFont (ui->fontCommentsButton, setting->fontComments); });
	connect (ui->fontListsButton, &QPushButton::clicked, [this] () { selectFont (ui->fontListsButton, setting->fontLists); });
	connect (ui->fontClocksButton, &QPushButton::clicked, [this] () { selectFont (ui->fontClocksButton, setting->fontClocks); });
	connect (ui->fontConsoleButton, &QPushButton::clicked, [this] () { selectFont (ui->fontConsoleButton, setting->fontConsole); });

	connect (ui->pb_engine_new, &QPushButton::clicked, this, &PreferencesDialog::slot_new_engine);
	connect (ui->pb_engine_delete, &QPushButton::clicked, this, &PreferencesDialog::slot_delete_engine);
	connect (ui->pb_engine_change, &QPushButton::clicked, this, &PreferencesDialog::slot_change_engine);
	connect (ui->pb_engine_dup, &QPushButton::clicked, this, &PreferencesDialog::slot_dup_engine);

	connect (ui->autosavePathCheckBox, &QCheckBox::toggled, [this] (bool on) { ui->autosavePathWidget->setEnabled (on); });
	connect (ui->autosavePathButton, &QPushButton::clicked, this, &PreferencesDialog::slot_autosavedir);

	update_current_engine ();
	update_current_host ();
	update_db_labels ();
}

void PreferencesDialog::update_chat_color ()
{
	QPixmap p (16, 16);
	p.fill (m_chat_color);
	QIcon i (p);
	ui->chatColorButton->setIcon (i);
	ui->chatColorButton->setEnabled (ui->chatColorCheckBox->isChecked ());
}

void PreferencesDialog::update_db_selection ()
{
	QItemSelectionModel *sel = ui->dbPathsTreeView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	ui->dbRemButton->setEnabled (selection);
	ui->dbCreateButton->setEnabled (selection);
}

void PreferencesDialog::update_db_labels ()
{
	bool missing = false;
	bool kombilo = false;
	for (auto &d: m_dbpath_model.entries ()) {
		missing |= !(d.qdb_found | d.kdb_found);
		kombilo |= !d.qdb_found && d.kdb_found;
	}
	ui->dbKombiloLabel->setVisible (kombilo);
	ui->dbNotFoundLabel->setVisible (missing);
}

void PreferencesDialog::slot_dbsubdir (bool)
{
	QString dir;
	if (!m_last_added_dbdir.isEmpty ()) {
		QDir d (m_last_added_dbdir);
		d.cdUp ();
		dir = d.absolutePath ();
	}
	QString dirname = QFileDialog::getExistingDirectory (this, QObject::tr ("Add a set of database directories - choose the parent directory"),
							     dir);
	if (dirname.isEmpty ())
		return;
	QDir parent (dirname);
	int skipped = 0;
	int failed = 0;
	for (auto subdir: parent.entryList (QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name)) {
		QDir d (parent);
		if (!d.cd (subdir)) {
			failed++;
			continue;
		}
		bool skip = false;
		for (auto &it: m_dbpath_model.entries ()) {
			QDir d2 (it.title);
			if (d == d2) {
				skip = true;
				skipped++;
				break;
			}
		}
		if (!skip) {
			m_last_added_dbdir = d.absolutePath ();
			m_dbpath_model.add_no_replace (d.absolutePath ());
			m_dbpaths_changed = true;
		}
	}
	if (skipped + failed > 0) {
		QString text;
		if (skipped > 0)
			text = tr ("<p>%1 directories already exists in the list and were not added.</p>").arg (skipped);
		if (failed > 0)
			text += tr ("<p>%1 directories were not added because they could not be read.</p>").arg (failed);
		QMessageBox::warning (this, tr ("Some directories not added"), text);
	}

	update_db_labels ();
}

void PreferencesDialog::slot_dbdir (bool)
{
	QString dir;
	if (!m_last_added_dbdir.isEmpty ()) {
		QDir d (m_last_added_dbdir);
		d.cdUp ();
		dir = d.absolutePath ();
	}
	QString dirname = QFileDialog::getExistingDirectory (this, QObject::tr ("Add a database directory"),
							     dir);
	if (dirname.isEmpty ())
		return;
	QDir d (dirname);
	for (auto &it: m_dbpath_model.entries ()) {
		QDir d2 (it.title);
		if (d == d2) {
			QMessageBox::warning (this, tr ("Directory already in the list"),
					      tr ("The directory could not be added because it already exists in the list."));
			return;
		}
	}
	m_last_added_dbdir = dirname;
	m_dbpath_model.add_no_replace (dirname);
	m_dbpaths_changed = true;
	update_db_labels ();
}

void PreferencesDialog::slot_dbcfg (bool)
{
	if (m_dbpath_model.rowCount () != 0) {
		QMessageBox mb (QMessageBox::Question, tr ("Overwrite database paths"),
				tr ("This operation replaces existing database paths.\nDo you still want to import from kombilo.cfg?"),
				QMessageBox::Yes | QMessageBox::No);
		if (mb.exec () != QMessageBox::Yes)
			return;
	}
	QString filename = QFileDialog::getOpenFileName (this, QObject::tr ("Open kombilo.cfg"),
							 setting->readEntry ("LAST_DIR"),
							 QObject::tr ("CFG Files (*.cfg);;All Files (*)"));
	if (filename.isEmpty ())
		return;

	QFile f (filename);
	if (!f.open (QIODevice::ReadOnly | QIODevice::Text))
		return;

	/* We used to use QSettings here, which is almost exactly what's required.  Unfortunately,
	   it fails on Windows, since it does not handle files with backspaces.  */
	QTextStream ts (&f);
	QRegularExpression db_re ("d[0-9]* *= *([^,]*), ([^,]*), kombilo");
	while (!ts.atEnd()) {
		QString line = ts.readLine ();
		if (line == "[databases]")
			break;
	}
	QStringList l;
	while (!ts.atEnd ()) {
		QString line = ts.readLine ();
		qDebug () << line;
		auto match = db_re.match (line);
		if (match.hasMatch())
			l << match.captured (1);
		else
			break;
	}
	f.close ();
	m_dbpath_model.reinit (l);
	m_dbpaths_changed = true;
	update_db_labels ();
}

void PreferencesDialog::slot_dbrem (bool)
{
	QItemSelectionModel *sel = ui->dbPathsTreeView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	if (!selection)
		return;

	std::vector<int> rows;
	for (auto i: selected)
		rows.push_back (i.row ());
	std::sort (rows.begin (), rows.end (), [] (int r1, int r2) { return r1 > r2; });
	for (auto r: rows) {
		m_dbpath_model.removeRows (r, 1);
	}
	m_dbpaths_changed = true;
	update_db_labels ();
}

void PreferencesDialog::slot_dbcreate (bool)
{
	QItemSelectionModel *sel = ui->dbPathsTreeView->selectionModel ();
	const QModelIndexList &selected = sel->selectedRows ();
	bool selection = selected.length () != 0;

	if (!selection)
		return;

	QProgressDialog dlg (tr ("Creating database from SGF files..."), tr ("Abort operation"),
			     0, 100, this);
	dlg.setWindowModality (Qt::WindowModal);
	dlg.setMinimumDuration (0);

	for (QModelIndex i: selected) {
		const db_dir_entry *e = m_dbpath_model.find (i);
		if (e) {
			QString label = tr ("Creating database from SGF files...\nProcessing: ") + e->title;
			dlg.setLabelText (label);
			if (create_db_for_dir (dlg, e->title)) {
				db_dir_entry new_e (e->title);
				m_dbpath_model.update_entry (i, new_e);
				m_dbpaths_changed = true;
			}
		}
	}
	update_db_labels ();
}

void PreferencesDialog::slot_autosavedir (bool)
{
	QString curr = ui->autosavePathEdit->text ();
	QString dirname = QFileDialog::getExistingDirectory (this, QObject::tr ("Choose a directory for autosaves"),
							     curr);
	if (dirname.isEmpty ())
		return;
	QDir d (dirname);
	if (!d.exists ()) {
		QMessageBox::warning (this, tr ("Selected directory does not exist"),
				      tr ("The selected directory does not exist."));
		return;
	}
	ui->autosavePathEdit->setText (dirname);
}

void PreferencesDialog::update_board_image ()
{
	int w = ui->stoneView->width ();
	int h = ui->stoneView->height ();
	QImage image(w, h, QImage::Format_RGB32);
	QPixmap pm;
	QString filename = ui->LineEdit_goban->text();
	int idx = ui->woodComboBox->currentIndex ();
	filename = wood_image_resource (idx, filename);
	QPixmap p (filename);
	if (p.isNull ())
		p = QPixmap (":/BoardWindow/images/board/wood1.png");

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin(&image);
	painter.setPen(Qt::NoPen);

	/* @@@ Scaling to the narrow strip of wood in the preferences dialog won't look the
	   same as on the real board.  It's unclear what best to do here.  */
	if (0 && ui->scaleWoodCheckBox->isChecked ())
		painter.drawPixmap (0, 0, w, h, p);
	else
		painter.drawTiledPixmap (0, 0, w, h, p);
	painter.end();
	m_stone_canvas->setSceneRect (0, 0, w, h);
	m_stone_canvas->setBackgroundBrush (QBrush (image));

	update_stone_positions ();
}

void PreferencesDialog::update_stone_positions ()
{
	int w = ui->stoneView->width ();
	int h = ui->stoneView->height ();

	m_stone_size = std::min (w / 2, h);
	int real_width = m_stone_size;
	if (!ui->radioButtonStones_2D->isChecked ())
		real_width *= 9. / 8;
	int w_diff = real_width - m_stone_size;
	m_w_stone->setPos (w / 2 - real_width, 0);
	m_b_stone->setPos (w / 2 - w_diff, 0);
}

void PreferencesDialog::init_from_settings ()
{
	int idx = setting->readIntEntry("SKIN_INDEX");
	ui->GobanPicturePathButton->setEnabled (idx == 0);
	ui->LineEdit_goban->setEnabled (idx == 0);
	ui->woodComboBox->setCurrentIndex (idx);

	ui->LineEdit_goban->setText (setting->readEntry("SKIN"));
	ui->LineEdit_Table->setText (setting->readEntry("SKIN_TABLE"));
	ui->scaleWoodCheckBox->setChecked (setting->readBoolEntry("SKIN_SCALE_WOOD"));
	ui->languageComboBox->insertItems (1, setting->getAvailableLanguages());
	ui->languageComboBox->setCurrentIndex (setting->convertLanguageCodeToNumber());

	ui->fileSelComboBox->setCurrentIndex (setting->readIntEntry("FILESEL"));

	ui->radioButtonStones_2D->setChecked ((setting->readIntEntry("STONES_LOOK")==1));
	ui->radioButtonStones_3D->setChecked ((setting->readIntEntry("STONES_LOOK")==2));
	ui->radioButtonStone_real->setChecked ((setting->readIntEntry("STONES_LOOK")==3));
	ui->stripesCheckBox->setChecked (setting->readBoolEntry("STONES_STRIPES"));
	ui->whiteSpecSlider->setValue (setting->readIntEntry("STONES_WSPEC"));
	ui->blackSpecSlider->setValue (setting->readIntEntry("STONES_BSPEC"));
	ui->whiteHardSlider->setValue (setting->readIntEntry("STONES_WHARD"));
	ui->blackHardSlider->setValue (setting->readIntEntry("STONES_BHARD"));
	ui->whiteRoundSlider->setValue (setting->readIntEntry("STONES_WROUND"));
	ui->blackRoundSlider->setValue (setting->readIntEntry("STONES_BROUND"));
	ui->whiteFlatSlider->setValue (setting->readIntEntry("STONES_WFLAT"));
	ui->blackFlatSlider->setValue (setting->readIntEntry("STONES_BFLAT"));
	ui->ambientSlider->setValue (setting->readIntEntry("STONES_AMBIENT"));
	ui->shadowSlider->setValue (setting->readIntEntry("STONES_SHADOWVAL"));
	ui->shadersComboBox->setCurrentIndex (setting->readIntEntry("STONES_PRESET"));

	m_chat_color = QColor (setting->readEntry ("CHAT_COLOR"));
	ui->chatColorCheckBox->setChecked (setting->readBoolEntry ("CHAT_COLORED"));

	ui->lineScaleCheckBox->setChecked (setting->readBoolEntry("BOARD_LINESCALE"));
	ui->lineWidenCheckBox->setChecked (setting->readBoolEntry("BOARD_LINEWIDEN"));

	ui->stoneSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_STONE"));
	ui->talkSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_TALK"));
	ui->matchSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_MATCH"));
	ui->passSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_PASS"));
	ui->gameEndSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_GAMEEND"));
	ui->timeSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_TIME"));
	ui->saySoundCheckBox->setChecked (setting->readBoolEntry("SOUND_SAY"));
	ui->enterSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_ENTER"));
	ui->leaveSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_LEAVE"));
	ui->disconnectSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_DISCONNECT"));
	ui->connectSoundCheckBox->setChecked (setting->readBoolEntry("SOUND_CONNECT"));

	ui->soundMatchCheckBox->setChecked (setting->readBoolEntry("SOUND_MATCH_BOARD"));
	ui->soundObserveCheckBox->setChecked (setting->readBoolEntry("SOUND_OBSERVE"));
	ui->soundNormalCheckBox->setChecked (setting->readBoolEntry("SOUND_NORMAL"));
	ui->soundComputerCheckBox->setChecked (setting->readBoolEntry("SOUND_COMPUTER"));

	ui->variationComboBox->setCurrentIndex (setting->readIntEntry("VAR_GHOSTS"));
	ui->varChildrenComboBox->setCurrentIndex (setting->readBoolEntry("VAR_CHILDREN") != 0);
	ui->varSGFStyleComboBox->setCurrentIndex (setting->readIntEntry("VAR_SGF_STYLE"));
	ui->varNumberingComboBox->setCurrentIndex (setting->readIntEntry("VAR_NUMBERING"));
	ui->varDiagsCheckBox->setChecked (setting->readBoolEntry("VAR_IGNORE_DIAGS"));

	int coords = (setting->readBoolEntry("BOARD_COORDS")
		      ? (setting->readBoolEntry("SGF_BOARD_COORDS") ? 2 : 1)
		      : 0);
	ui->coordsComboBox->setCurrentIndex (coords);
	ui->coordSizeSlider->setValue (setting->readIntEntry("COORDS_SIZE"));
	ui->cursorCheckBox->setChecked (setting->readBoolEntry("CURSOR"));
	ui->tooltipsCheckBox->setChecked (!(setting->readBoolEntry("TOOLTIPS")));
	ui->BYTimeSpin->setValue (setting->readIntEntry("BY_TIMER"));
	ui->timeDivideCheckBox->setChecked (setting->readBoolEntry("TIME_WARN_DIVIDE"));
	ui->timeBoardCheckBox->setChecked (setting->readBoolEntry("TIME_WARN_BOARD"));

	int sidebar = setting->readBoolEntry("SIDEBAR_LEFT") ? 0 : 1;
	ui->sidebarComboBox->setCurrentIndex (sidebar);
	ui->antiClickoCheckBox->setChecked (setting->readBoolEntry("ANTICLICKO"));
	ui->hitboxCheckBox->setChecked (setting->readBoolEntry("ANTICLICKO_HITBOX"));

	ui->gameTreeSizeSlider->setValue (setting->readIntEntry("GAMETREE_SIZE"));
	ui->diagShowComboBox->setCurrentIndex (setting->readIntEntry("BOARD_DIAGMODE"));
	ui->diagClearCheckBox->setChecked (setting->readBoolEntry("BOARD_DIAGCLEAR"));
	ui->diagHideCheckBox->setChecked (setting->readBoolEntry("GAMETREE_DIAGHIDE"));

	// Client Window tab
	ui->LineEdit_watch->setText (setting->readEntry("WATCH"));
	ui->LineEdit_exclude->setText (setting->readEntry("EXCLUDE"));
//	ui->CheckBox_useNmatch->setChecked (setting->readBoolEntry("USE_NMATCH"));
	ui->checkBox_Nmatch_Black->setChecked (setting->readBoolEntry("NMATCH_BLACK"));
	ui->checkBox_Nmatch_White->setChecked (setting->readBoolEntry("NMATCH_WHITE"));
	ui->checkBox_Nmatch_Nigiri->setChecked (setting->readBoolEntry("NMATCH_NIGIRI"));
	ui->HandicapSpin_Nmatch->setValue (setting->readIntEntry("NMATCH_HANDICAP"));
	ui->timeSpin_Nmatch->setValue (setting->readIntEntry("NMATCH_MAIN_TIME"));
	ui->BYSpin_Nmatch->setValue (setting->readIntEntry("NMATCH_BYO_TIME"));

	ui->computerWhiteButton->setChecked (setting->readBoolEntry("COMPUTER_WHITE"));
	ui->computerSizeSpin->setValue (setting->readIntEntry("COMPUTER_SIZE"));
	ui->computerHandicapSpin->setValue (setting->readIntEntry("COMPUTER_HANDICAP"));
	ui->humanName->setText (setting->readEntry("HUMAN_NAME"));

	ui->anChildMovesCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_CHILDREN"));
	ui->anPruneCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_PRUNE"));
	ui->anHideCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_HIDEOTHER"));
	ui->anVarComboBox->setCurrentIndex (setting->readIntEntry ("ANALYSIS_VARTYPE"));
	ui->winrateComboBox->setCurrentIndex (setting->readIntEntry ("ANALYSIS_WINRATE"));
	ui->anDepthEdit->setText (QString::number (setting->readIntEntry ("ANALYSIS_DEPTH")));
	ui->anMaxMovesEdit->setText (QString::number (setting->readIntEntry ("ANALYSIS_MAXMOVES")));

	// Go Server tab
	ui->boardSizeSpin->setValue (setting->readIntEntry("DEFAULT_SIZE"));
	ui->timeSpin->setValue (setting->readIntEntry("DEFAULT_TIME"));
	ui->BYSpin->setValue (setting->readIntEntry("DEFAULT_BY"));
	ui->komiSpinDefault->setValue (setting->readIntEntry("DEFAULT_KOMI"));
	ui->automaticNegotiationCheckBox->setChecked (setting->readBoolEntry("DEFAULT_AUTONEGO"));
	ui->autosaveCheckBox->setChecked (setting->readBoolEntry("AUTOSAVE"));
	ui->autosavePlayedCheckBox->setChecked (setting->readBoolEntry("AUTOSAVE_PLAYED"));
	ui->autosavePathCheckBox->setChecked (setting->readBoolEntry("AUTOSAVE_TO_PATH"));
	ui->autosavePathEdit->setText (setting->readEntry("AUTOSAVE_PATH"));
	ui->autosavePathWidget->setEnabled (ui->autosavePathCheckBox->isChecked ());
	ui->observeSingleCheckBox->setChecked (setting->readBoolEntry("OBSERVE_SINGLE"));

	ui->toroidDupsSpin->setValue (setting->readIntEntry("TOROID_DUPS"));

	ui->slideLinesSpinBox->setValue (setting->readIntEntry ("SLIDE_LINES"));
	ui->slideMarginSpinBox->setValue (setting->readIntEntry ("SLIDE_MARGIN"));
	ui->slideXEdit->setText (QString::number (setting->readIntEntry ("SLIDE_X")));
	ui->slideYEdit->setText (QString::number (setting->readIntEntry ("SLIDE_Y")));
	ui->slideItalicCheckBox->setChecked (setting->readBoolEntry ("SLIDE_ITALIC"));
	ui->slideBoldCheckBox->setChecked (setting->readBoolEntry ("SLIDE_BOLD"));
	ui->slideWBCheckBox->setChecked (setting->readBoolEntry ("SLIDE_WB"));
	ui->slideCoordsCheckBox->setChecked (setting->readBoolEntry ("SLIDE_COORDS"));

	ui->titleMatchEdit->setText (setting->readEntry ("WTITLE_MATCH"));
	ui->titleObserveEdit->setText (setting->readEntry ("WTITLE_OBSERVE"));
	ui->titleClientEdit->setText (setting->readEntry ("WTITLE_CLIENT"));
	ui->titleEngineEdit->setText (setting->readEntry ("WTITLE_ENGINE"));

	ui->fontStandardButton->setText (setting->fontToString(setting->fontStandard));
	ui->fontMarksButton->setText (setting->fontToString(setting->fontMarks));
	ui->fontCommentsButton->setText (setting->fontToString(setting->fontComments));
	ui->fontListsButton->setText (setting->fontToString(setting->fontLists));
	ui->fontClocksButton->setText (setting->fontToString(setting->fontClocks));
	ui->fontConsoleButton->setText (setting->fontToString(setting->fontConsole));

	ui->fontStandardButton->setFont(setting->fontStandard);
	ui->fontMarksButton->setFont(setting->fontMarks);
	ui->fontCommentsButton->setFont(setting->fontComments);
	ui->fontListsButton->setFont(setting->fontLists);
	ui->fontClocksButton->setFont(setting->fontClocks);
	ui->fontConsoleButton->setFont(setting->fontConsole);

	ui->tipsCheckBox->setChecked (setting->readBoolEntry ("TIPS_STARTUP"));

	ui->maxDBSizeSpinBox->setValue (setting->readIntEntry ("DB_MAX_FILESIZE"));
	ui->keepMovesCheckBox->setChecked (setting->readBoolEntry ("DB_CACHE_MOVELIST"));
}

void PreferencesDialog::select_stone_look (bool)
{
	update_w_stones ();
	update_b_stones ();
	update_stone_positions ();
}

void PreferencesDialog::select_chat_color (bool)
{
	m_chat_color = QColorDialog::getColor (m_chat_color, this, tr ("Select color for own chat online"));
	update_chat_color ();
}

void PreferencesDialog::select_white_color (bool)
{
	QColor c = QColorDialog::getColor (m_ih->white_color (), this, tr ("Select white stone base color"));
	m_ih->set_white_color (c);
	update_w_stones ();
}

void PreferencesDialog::select_black_color (bool)
{
	QColor c = QColorDialog::getColor (m_ih->black_color (), this, tr ("Select black stone base color"));
	m_ih->set_black_color (c);
	update_b_stones ();
}

void PreferencesDialog::update_stone_params ()
{
	int shader_idx = ui->shadersComboBox->currentIndex ();
	if (shader_idx == 0) {
		auto vals = ImageHandler::t_params { { ui->blackRoundSlider->value (), ui->blackSpecSlider->value (), ui->blackFlatSlider->value (), ui->blackHardSlider->value () },
						     { ui->whiteRoundSlider->value (), ui->whiteSpecSlider->value (), ui->whiteFlatSlider->value (), ui->whiteHardSlider->value () },
						     ui->ambientSlider->value (), ui->stripesCheckBox->isChecked () };
		m_ih->set_stone_params (vals, ui->shadowSlider->value ());
	} else
		m_ih->set_stone_params (shader_idx - 1, ui->shadowSlider->value ());

	int look = 3;
	if (ui->radioButtonStones_2D->isChecked ())
		look = 1;
	else if (ui->radioButtonStones_3D->isChecked ())
		look = 2;
	ui->blackGroupBox->setEnabled (look == 3 && shader_idx == 0);
	ui->whiteGroupBox->setEnabled (look == 3 && shader_idx == 0);
	ui->ambientSlider->setEnabled (look == 3 && shader_idx == 0);
	ui->stripesCheckBox->setEnabled (look == 3 && shader_idx == 0);
	ui->shadersComboBox->setEnabled (look == 3);
	m_ih->set_stone_look (look);
}

void PreferencesDialog::update_w_stones ()
{
	update_stone_params ();

	int real_size = m_stone_size;
	if (!ui->radioButtonStones_2D->isChecked ())
		real_size = m_stone_size * 9. / 8;

	QPixmap pm (real_size, real_size);
	pm.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&pm);
	QImage img (m_stone_size, m_stone_size, QImage::Format_ARGB32);
	if (!ui->radioButtonStones_2D->isChecked ()) {
		m_ih->paint_shadow_stone (img, m_stone_size);
		painter.drawImage (0, real_size - m_stone_size, img);
	}
	m_ih->paint_one_stone (img, true, m_stone_size);
	painter.drawImage (real_size - m_stone_size, 0, img);
	painter.end ();
	m_w_stone->setPixmap (pm);
}

void PreferencesDialog::update_b_stones ()
{
	update_stone_params ();

	int real_size = m_stone_size;
	if (!ui->radioButtonStones_2D->isChecked ())
		real_size = m_stone_size * 9. / 8;

	QPixmap pm (real_size, real_size);
	pm.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&pm);
	QImage img (m_stone_size, m_stone_size, QImage::Format_ARGB32);
	if (!ui->radioButtonStones_2D->isChecked ()) {
		m_ih->paint_shadow_stone (img, m_stone_size);
		painter.drawImage (0, real_size - m_stone_size, img);
	}
	m_ih->paint_one_stone (img, false, m_stone_size);
	painter.drawImage (real_size - m_stone_size, 0, img);
	painter.end ();
	m_b_stone->setPixmap (pm);
}

PreferencesDialog::~PreferencesDialog()
{
	delete ui;
}

bool PreferencesDialog::verify ()
{
	bool ok;
	int slide_x = ui->slideXEdit->text ().toInt (&ok);
	if (!ok || slide_x < 100 || slide_x > 10000) {
		QMessageBox::warning (this, tr ("Invalid slide width"),
				      tr ("Please enter valid dimensions for slide export (100x100 or larger)."));
		return false;
	}
	int slide_y = ui->slideYEdit->text ().toInt (&ok);
	if (!ok || slide_y < 100 || slide_y > 10000) {
		QMessageBox::warning (this, tr ("Invalid slide height"),
				      tr ("Please enter valid dimensions for slide export (100x100 or larger)."));
		return false;
	}
	if (slide_y > slide_x) {
		QMessageBox::warning (this, tr ("Invalid slide dimensions"),
				      tr ("Slide export dimensions must be wider than they are tall."));
		return false;
	}
	if (m_dbpaths_changed) {
		bool missing = false;
		for (auto &d: m_dbpath_model.entries ())
			missing |= !(d.qdb_found | d.kdb_found);
		if (missing) {
			QMessageBox mb (QMessageBox::Question, tr ("Missing database files"),
					tr ("<p>No database files could be found in some of the directories configured for the database.</p><p>Do you want to go back and create them now?</p>"),
					QMessageBox::Yes | QMessageBox::No);

			if (mb.exec () == QMessageBox::Yes) {
				ui->tabWidget->setCurrentIndex (6);
				return false;
			}
		}
	}
	return true;
}

void PreferencesDialog::slot_apply ()
{
	if (verify ())
		do_apply ();
}

void PreferencesDialog::do_apply ()
{
	bool ok;
	int slide_x = ui->slideXEdit->text ().toInt (&ok);
	int slide_y = ui->slideYEdit->text ().toInt (&ok);

	setting->writeIntEntry ("SLIDE_X", slide_x);
	setting->writeIntEntry ("SLIDE_Y", slide_y);
	setting->writeIntEntry ("SLIDE_LINES", ui->slideLinesSpinBox->value ());
	setting->writeIntEntry ("SLIDE_MARGIN", ui->slideMarginSpinBox->value ());
	setting->writeBoolEntry ("SLIDE_ITALIC", ui->slideItalicCheckBox->isChecked ());
	setting->writeBoolEntry ("SLIDE_BOLD", ui->slideBoldCheckBox->isChecked ());
	setting->writeBoolEntry ("SLIDE_WB", ui->slideWBCheckBox->isChecked ());
	setting->writeBoolEntry ("SLIDE_COORDS", ui->slideCoordsCheckBox->isChecked ());

	setting->writeIntEntry ("SKIN_INDEX", ui->woodComboBox->currentIndex ());
	setting->writeEntry ("SKIN", ui->LineEdit_goban->text ());
	setting->writeEntry ("SKIN_TABLE", ui->LineEdit_Table->text ());
	setting->writeBoolEntry ("SKIN_SCALE_WOOD", ui->scaleWoodCheckBox->isChecked ());
	setting->obtain_skin_images ();

	setting->writeEntry ("LANG", setting->convertNumberToLanguage(ui->languageComboBox->currentIndex ()));
	setting->writeIntEntry ("FILESEL", ui->fileSelComboBox->currentIndex ());
//	setting->writeBoolEntry ("STONES_SHADOW", ui->stonesShadowCheckBox->isChecked ());
//	setting->writeBoolEntry ("STONES_SHELLS", ui->stonesShellsCheckBox->isChecked ());
	int i = 3;
	if (ui->radioButtonStones_2D->isChecked ())
		i = 1;
	else if (ui->radioButtonStones_3D->isChecked ())
		i = 2;
	setting->writeBoolEntry ("STONES_STRIPES", ui->stripesCheckBox->isChecked ());
	setting->writeIntEntry ("STONES_LOOK", i);
	setting->writeIntEntry ("STONES_WSPEC", ui->whiteSpecSlider->value ());
	setting->writeIntEntry ("STONES_BSPEC", ui->blackSpecSlider->value ());
	setting->writeIntEntry ("STONES_WHARD", ui->whiteHardSlider->value ());
	setting->writeIntEntry ("STONES_BHARD", ui->blackHardSlider->value ());
	setting->writeIntEntry ("STONES_WROUND", ui->whiteRoundSlider->value ());
	setting->writeIntEntry ("STONES_BROUND", ui->blackRoundSlider->value ());
	setting->writeIntEntry ("STONES_WFLAT", ui->whiteFlatSlider->value ());
	setting->writeIntEntry ("STONES_BFLAT", ui->blackFlatSlider->value ());
	setting->writeIntEntry ("STONES_AMBIENT", ui->ambientSlider->value ());
	setting->writeIntEntry ("STONES_SHADOWVAL", ui->shadowSlider->value ());
	setting->writeIntEntry ("STONES_PRESET", ui->shadersComboBox->currentIndex ());
	setting->writeEntry ("STONES_BCOL", black_color ().name ());
	setting->writeEntry ("STONES_WCOL", white_color ().name ());

	setting->writeEntry ("CHAT_COLOR", m_chat_color.name ());
	setting->writeBoolEntry ("CHAT_COLORED", ui->chatColorCheckBox->isChecked ());

	setting->writeBoolEntry ("BOARD_LINESCALE", ui->lineScaleCheckBox->isChecked ());
	setting->writeBoolEntry ("BOARD_LINEWIDEN", ui->lineWidenCheckBox->isChecked ());

	setting->writeBoolEntry ("SOUND_STONE", ui->stoneSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_TALK", ui->talkSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_MATCH", ui->matchSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_GAMEEND", ui->passSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_PASS", ui->gameEndSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_TIME", ui->timeSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_SAY", ui->saySoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_ENTER", ui->enterSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_LEAVE", ui->leaveSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_DISCONNECT", ui->disconnectSoundCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_CONNECT", ui->connectSoundCheckBox->isChecked ());

	setting->writeBoolEntry ("SOUND_MATCH_BOARD", ui->soundMatchCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_OBSERVE", ui->soundObserveCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_NORMAL", ui->soundNormalCheckBox->isChecked ());
	setting->writeBoolEntry ("SOUND_COMPUTER", ui->soundComputerCheckBox->isChecked ());

	setting->writeIntEntry ("VAR_GHOSTS", ui->variationComboBox->currentIndex ());
	setting->writeBoolEntry ("VAR_CHILDREN", ui->varChildrenComboBox->currentIndex () == 1);
	setting->writeIntEntry ("VAR_SGF_STYLE", ui->varSGFStyleComboBox->currentIndex ());
	setting->writeIntEntry("VAR_NUMBERING", ui->varNumberingComboBox->currentIndex ());
	setting->writeBoolEntry ("VAR_IGNORE_DIAGS", ui->varDiagsCheckBox->isChecked ());

	int coords = ui->coordsComboBox->currentIndex ();
	int sidebar = ui->sidebarComboBox->currentIndex ();
	setting->writeBoolEntry ("BOARD_COORDS", coords > 0);
	setting->writeBoolEntry ("SGF_BOARD_COORDS", coords == 2);
	setting->writeIntEntry ("COORDS_SIZE", ui->coordSizeSlider->value ());
	setting->writeBoolEntry ("SIDEBAR_LEFT", sidebar == 0);
	setting->writeBoolEntry ("CURSOR", ui->cursorCheckBox->isChecked ());
	//setting->writeBoolEntry ("SMALL_STONES", ui->smallerStonesCheckBox->isChecked ());
	setting->writeBoolEntry ("TOOLTIPS", !(ui->tooltipsCheckBox->isChecked ()));
	setting->writeIntEntry ("BY_TIMER", ui->BYTimeSpin->text ().toInt ());
	setting->writeBoolEntry ("TIME_WARN_DIVIDE", ui->timeDivideCheckBox->isChecked ());
	setting->writeBoolEntry ("TIME_WARN_BOARD", ui->timeBoardCheckBox->isChecked ());
	setting->writeBoolEntry ("ANTICLICKO", ui->antiClickoCheckBox->isChecked ());
	setting->writeBoolEntry ("ANTICLICKO_HITBOX", ui->hitboxCheckBox->isChecked ());

	// Client Window Tab
	setting->writeEntry ("WATCH", ui->LineEdit_watch->text ());
	setting->writeEntry ("EXCLUDE", ui->LineEdit_exclude->text ());
//	setting->writeBoolEntry ("USE_NMATCH", ui->CheckBox_useNmatch->isChecked ());

	//Checks wether the nmatch parameters have been modified, in order to send a new nmatchrange command
	if( 	(setting->readBoolEntry ("NMATCH_BLACK") != ui->checkBox_Nmatch_Black->isChecked ()) ||
		(setting->readBoolEntry ("NMATCH_WHITE") != ui->checkBox_Nmatch_White->isChecked ()) ||
		(setting->readBoolEntry ("NMATCH_NIGIRI") != ui->checkBox_Nmatch_Nigiri->isChecked ()) ||
		(setting->readIntEntry ("NMATCH_MAIN_TIME") != ui->timeSpin_Nmatch->text ().toInt ()) ||
		(setting->readIntEntry ("NMATCH_BYO_TIME") != ui->BYSpin_Nmatch->text ().toInt ()) ||
		(setting->readIntEntry ("NMATCH_HANDICAP") != ui->HandicapSpin_Nmatch->text ().toInt ()) ||
		(setting->readIntEntry ("DEFAULT_SIZE") != ui->boardSizeSpin->text ().toInt ()) ||
		(setting->readIntEntry ("DEFAULT_TIME") != ui->timeSpin->text ().toInt ()) ||
		(setting->readIntEntry ("DEFAULT_BY") != ui->BYSpin->text ().toInt ()) )
			setting->nmatch_settings_modified = true;

	setting->writeBoolEntry ("NMATCH_BLACK", ui->checkBox_Nmatch_Black->isChecked ());
	setting->writeBoolEntry ("NMATCH_WHITE", ui->checkBox_Nmatch_White->isChecked ());
	setting->writeBoolEntry ("NMATCH_NIGIRI", ui->checkBox_Nmatch_Nigiri->isChecked ());
	setting->writeIntEntry ("NMATCH_MAIN_TIME", ui->timeSpin_Nmatch->text ().toInt ());
	setting->writeIntEntry ("NMATCH_BYO_TIME", ui->BYSpin_Nmatch->text ().toInt ());
	setting->writeIntEntry ("NMATCH_HANDICAP", ui->HandicapSpin_Nmatch->text ().toInt ());

	setting->writeIntEntry ("DEFAULT_SIZE", ui->boardSizeSpin->text ().toInt ());
	setting->writeIntEntry ("DEFAULT_TIME", ui->timeSpin->text ().toInt ());
	setting->writeIntEntry ("DEFAULT_BY", ui->BYSpin->text ().toInt ());
	setting->writeIntEntry ("DEFAULT_KOMI", ui->komiSpinDefault->text ().toFloat());
	setting->writeBoolEntry ("DEFAULT_AUTONEGO", ui->automaticNegotiationCheckBox->isChecked ());
	setting->writeBoolEntry ("AUTOSAVE", ui->autosaveCheckBox->isChecked ());
	setting->writeBoolEntry ("AUTOSAVE_PLAYED", ui->autosavePlayedCheckBox->isChecked ());
	setting->writeBoolEntry ("AUTOSAVE_TO_PATH", ui->autosavePathCheckBox->isChecked ());
	setting->writeEntry ("AUTOSAVE_PATH", ui->autosavePathEdit->text ());
	setting->writeBoolEntry ("OBSERVE_SINGLE", ui->observeSingleCheckBox->isChecked ());

	// Computer Tab
	setting->writeBoolEntry ("COMPUTER_WHITE", ui->computerWhiteButton->isChecked ());
	setting->writeIntEntry ("COMPUTER_SIZE", ui->computerSizeSpin->text ().toInt ());
	setting->writeIntEntry ("COMPUTER_HANDICAP", ui->computerHandicapSpin->text ().toInt ());
	setting->writeEntry ("HUMAN_NAME", ui->humanName->text ());

	setting->writeBoolEntry ("ANALYSIS_CHILDREN", ui->anChildMovesCheckBox->isChecked ());
	setting->writeBoolEntry ("ANALYSIS_PRUNE", ui->anPruneCheckBox->isChecked ());
	setting->writeBoolEntry ("ANALYSIS_HIDEOTHER", ui->anHideCheckBox->isChecked ());
	setting->writeIntEntry ("ANALYSIS_VARTYPE", ui->anVarComboBox->currentIndex ());
	setting->writeIntEntry ("ANALYSIS_WINRATE", ui->winrateComboBox->currentIndex ());

	setting->writeIntEntry ("ANALYSIS_DEPTH", ui->anDepthEdit->text ().toInt ());
	setting->writeIntEntry ("ANALYSIS_MAXMOVES", ui->anMaxMovesEdit->text ().toInt ());

	setting->writeIntEntry ("GAMETREE_SIZE", ui->gameTreeSizeSlider->value ());
	setting->writeIntEntry ("BOARD_DIAGMODE", ui->diagShowComboBox->currentIndex ());
	setting->writeBoolEntry ("BOARD_DIAGCLEAR", ui->diagClearCheckBox->isChecked ());
	setting->writeBoolEntry ("GAMETREE_DIAGHIDE", ui->diagHideCheckBox->isChecked ());

	setting->writeEntry ("WTITLE_MATCH", ui->titleMatchEdit->text ());
	setting->writeEntry ("WTITLE_OBSERVE", ui->titleObserveEdit->text ());
	setting->writeEntry ("WTITLE_CLIENT", ui->titleClientEdit->text ());
	setting->writeEntry ("WTITLE_ENGINE", ui->titleEngineEdit->text ());

	setting->writeIntEntry ("TOROID_DUPS", ui->toroidDupsSpin->text ().toInt ());

	setting->writeBoolEntry ("TIPS_STARTUP", ui->tipsCheckBox->isChecked ());

	setting->writeIntEntry ("DB_MAX_FILESIZE", ui->maxDBSizeSpinBox->value ());
	setting->writeBoolEntry ("DB_CACHE_MOVELIST", ui->keepMovesCheckBox->isChecked ());

	if (m_dbpaths_changed) {
		QStringList new_paths;
		for (auto &d: m_dbpath_model.entries ())
			new_paths << d.title;
		setting->m_dbpath_lock.lock ();
		setting->m_dbpaths = new_paths;
		setting->dbpaths_changed = true;
		setting->m_dbpath_lock.unlock ();
		m_dbpaths_changed = false;
	}
	if (m_engines_changed) {
		setting->m_engines = m_engines_model.entries ();
		setting->engines_changed = true;
		m_engines_changed = false;
	}
	if (m_hosts_changed) {
		setting->m_hosts = m_hosts_model.entries ();
		setting->hosts_changed = true;
		m_hosts_changed = false;
	}
	setting->extract_frequent_settings ();

	client_window->accept_preferences ();
}

void PreferencesDialog::startHelpMode ()
{
	QWhatsThis::enterWhatsThisMode ();
}

void PreferencesDialog::selectFont (QPushButton *button, QFont &font)
{
	// Open a font dialog to select a new font
	bool ok;
	QFont f = QFontDialog::getFont(&ok, font, this);
	if (ok) {
		font = f;
		button->setText (setting->fontToString (f));
		button->setFont(f);
	}
}

bool PreferencesDialog::avoid_losing_data ()
{
	if (m_changing_host) {
		if (!ui->LineEdit_host->text ().isEmpty ()
		    || !ui->LineEdit_port->text ().isEmpty ()
		    || !ui->LineEdit_login->text ().isEmpty ())
		{
			QMessageBox mb(QMessageBox::Question, tr("Unsaved data"),
				       QString(tr("The host input fields contain\n"
						  "potentially unsaved data.\n"
						  "Really close the preferences?")),
				       QMessageBox::Yes | QMessageBox::No);
			return mb.exec() == QMessageBox::No;
		}
	}
	return false;
}

void PreferencesDialog::slot_accept()
{
	if (avoid_losing_data ())
		return;
	if (!verify ())
		return;

	saveSizes ();

	do_apply ();
	accept ();
}

void PreferencesDialog::slot_reject()
{
	if (avoid_losing_data ())
		return;

	saveSizes();
	reject();
}

void PreferencesDialog::saveSizes()
{
	// save size and position of window
	client_window->savePrefFrame(pos(), size());
}

void PreferencesDialog::slot_delete_engine ()
{
	QModelIndex idx = ui->ListView_engines->currentIndex ();
	if (!idx.isValid ())
		return;

	if ((size_t)idx.row () >= m_engines_model.entries ().size ())
		return;

	m_engines_model.removeRows (idx.row (), 1);
	m_engines_changed = true;
}

void PreferencesDialog::update_current_engine ()
{
	QModelIndex idx = ui->ListView_engines->currentIndex ();
	bool valid = idx.isValid ();

	ui->pb_engine_delete->setEnabled (valid);
	ui->pb_engine_change->setEnabled (valid);
	ui->pb_engine_dup->setEnabled (valid);
}

void PreferencesDialog::slot_new_engine ()
{
	EngineDialog dlg (this);
	for (;;) {
		if (!dlg.exec ())
			return;
		Engine new_e = dlg.get_engine ();
		bool added = m_engines_model.add_no_replace (new_e);
		if (added) {
			m_engines_changed = true;
			return;
		}
		QMessageBox::warning (this, tr ("Engine name already exists"),
				      tr ("An engine with this name already exists.\nPlease enter a new name that is not already taken."));
	}
}

void PreferencesDialog::slot_change_engine ()
{
	QModelIndex idx = ui->ListView_engines->currentIndex ();
	bool valid = idx.isValid ();
	if (!valid)
		return;

	const Engine *e = m_engines_model.find (idx);
	EngineDialog dlg (this, *e, false);
	if (dlg.exec ()) {
		Engine new_e = dlg.get_engine ();
		m_engines_model.add_or_replace (new_e);
		m_engines_changed = true;
	}
}

void PreferencesDialog::slot_dup_engine ()
{
	QModelIndex idx = ui->ListView_engines->currentIndex ();
	bool valid = idx.isValid ();
	if (!valid)
		return;

	const Engine *e = m_engines_model.find (idx);
	EngineDialog dlg (this, *e, true);
	for (;;) {
		if (!dlg.exec ())
			return;
		Engine new_e = dlg.get_engine ();
		bool added = m_engines_model.add_no_replace (new_e);
		if (added) {
			m_engines_changed = true;
			return;
		}
		QMessageBox::warning (this, tr ("Engine name already exists"),
				      tr ("An engine with this name already exists.\nPlease enter a new name that is not already taken."));
	}
}

void PreferencesDialog::clear_host ()
{
	ui->LineEdit_title->clear();
	ui->LineEdit_host->clear();
	ui->LineEdit_port->clear();
	ui->LineEdit_login->clear();
	ui->ComboBox_codec->setCurrentText("");

	m_changing_host = false;
}

void PreferencesDialog::slot_add_server()
{
	if (ui->LineEdit_title->text ().isEmpty()) {
		QMessageBox::warning (this, tr ("Title required"),
				      tr ("You need to add a title for the account that you want to add."));
		return;
	}
	if (ui->LineEdit_host->text ().isEmpty()) {
		QMessageBox::warning (this, tr ("Host required"),
				      tr ("You need to add a host name for the account that you want to add."));
		return;
	}
	if (ui->LineEdit_login->text ().isEmpty()) {
		QMessageBox::warning (this, tr ("Login name required"),
				      tr ("You need to add the login name for the account that you want to add."));
		return;
	}

	// check if title already exists
	bool check;
	unsigned int tmp = ui->LineEdit_port->text ().toUInt(&check);
	if (!check)
	{
		tmp = 0;
		QMessageBox::warning (this, tr ("Port required"),
				      tr ("You need to add the server's port number for the account that you want to add."));
		return;
	}

	Host new_h (ui->LineEdit_title->text (), ui->LineEdit_host->text (), tmp,
		    ui->LineEdit_login->text (), ui->LineEdit_pass->text (), ui->ComboBox_codec->currentText (), -1);
	m_hosts_model.add_or_replace (new_h);
	m_hosts_changed = true;

	clear_host ();
}

void PreferencesDialog::slot_delete_server()
{
	QModelIndex idx = ui->ListView_hosts->currentIndex ();
	if (!idx.isValid ())
		return;
	if ((size_t)idx.row () >= m_hosts_model.entries ().size ())
		return;

	m_hosts_model.removeRows (idx.row (), 1);
	m_hosts_changed = true;
	clear_host ();
}

void PreferencesDialog::slot_new_server()
{
	clear_host ();
}

void PreferencesDialog::update_current_host ()
{
	QModelIndex idx = ui->ListView_hosts->currentIndex ();
	bool valid = idx.isValid ();
	bool standard_host = valid && (size_t)idx.row () >= m_hosts_model.entries ().size ();

	ui->pb_server_delete->setEnabled (valid && !standard_host);

	if (!valid)
		return;

	const Host *h = m_hosts_model.find (idx);
	ui->LineEdit_title->setText (standard_host ? "" : h->title);
	ui->LineEdit_host->setText (h->host);
	ui->LineEdit_port->setText (QString::number (h->port));
	ui->LineEdit_login->setText (h->login_name);
	ui->LineEdit_pass->setText (h->password);
	ui->ComboBox_codec->setCurrentText (h->codec);
}

void PreferencesDialog::slot_serverChanged(const QString &title)
{
	for (auto &h: m_hosts_model.entries ())
		if (h.title == title) {
			m_changing_host = true;
			ui->pb_server_add->setText (tr("Change"));
			return;
		}

	m_changing_host = false;
	ui->pb_server_add->setText (tr("Add"));
}

// play the sound if check box has been clicked
void PreferencesDialog::on_soundButtonGroup_buttonClicked (QAbstractButton *cb)
{
	qDebug() << "button text = " << cb->text ();

	if (cb == ui->stoneSoundCheckBox)
		qgo->playStoneSound (true);
	else if (cb == ui->passSoundCheckBox)
		qgo->playPassSound (true);
	else if (cb == ui->timeSoundCheckBox)
		qgo->playTimeSound (true);
	else if (cb == ui->talkSoundCheckBox)
		qgo->playTalkSound (true);
	else if (cb == ui->saySoundCheckBox)
		qgo->playSaySound (true);
	else if (cb == ui->matchSoundCheckBox)
		qgo->playMatchSound (true);
	else if (cb == ui->enterSoundCheckBox)
		qgo->playEnterSound (true);
	else if (cb == ui->gameEndSoundCheckBox)
		qgo->playGameEndSound (true);
	else if (cb == ui->leaveSoundCheckBox)
		qgo->playLeaveSound (true);
	else if (cb == ui->disconnectSoundCheckBox)
		qgo->playDisconnectSound (true);
	else if (cb == ui->connectSoundCheckBox)
		qgo->playConnectSound (true);
}

void PreferencesDialog::slot_getGobanPicturePath()
{
	QString path = setting->readEntry ("LAST_DIR");
	QString old_name = ui->LineEdit_goban->text ();
	if (!old_name.isEmpty ()) {
		QFileInfo info (old_name);
		if (info.exists ()) {
			QDir dir = info.dir ();
			path = dir.absolutePath ();
		}
	}
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Select a goban wood image"), path, tr("Images (*.png *.xpm *.jpg)")));
	if (fileName.isEmpty())
		return;

	ui->LineEdit_goban->setText (fileName);
	update_board_image ();
}

void PreferencesDialog::slot_getTablePicturePath()
{
	QString path = setting->readEntry ("LAST_DIR");
	QString old_name = ui->LineEdit_Table->text ();
	if (!old_name.isEmpty ()) {
		QFileInfo info (old_name);
		if (info.exists ()) {
			QDir dir = info.dir ();
			path = dir.absolutePath ();
		}
	}
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Select a table background image"), path, tr("Images (*.png *.xpm *.jpg)")));
	if (fileName.isEmpty())
		return;

	ui->LineEdit_Table->setText (fileName);
}

void PreferencesDialog::slot_main_time_changed(int n)
{
	ui->timeSpin_Nmatch->setMinimum(n);
}


void PreferencesDialog::slot_BY_time_changed(int n)
{
	ui->BYSpin_Nmatch->setMinimum(n);
}

QColor PreferencesDialog::white_color ()
{
	return m_ih->white_color ();
}

QColor PreferencesDialog::black_color ()
{
	return m_ih->black_color ();
}
