#include <QButtonGroup>

#include "gogame.h"
#include "board.h"
#include "multisave.h"
#include "config.h"

#include "ui_multisave_gui.h"

MultiSaveDialog::MultiSaveDialog (QWidget *parent, go_game_ptr gr, game_state *pos)
	: QDialog (parent), ui (new Ui::MultiSaveDialog), m_game (gr), m_position (pos), m_radios (new QButtonGroup)
{
	ui->setupUi (this);

	m_radios->addButton (ui->allRadioButton);
	m_radios->addButton (ui->someRadioButton);

	update_count ();
	connect (ui->buttonBox, &QDialogButtonBox::accepted, this, &MultiSaveDialog::do_save);
	connect (ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect (ui->allRadioButton, &QRadioButton::toggled, this, &MultiSaveDialog::update_count);
	connect (ui->someRadioButton, &QRadioButton::toggled, this, &MultiSaveDialog::update_count);
	ui->allRadioButton->setChecked (true);
}

MultiSaveDialog::~MultiSaveDialog ()
{
	delete m_radios;
	delete ui;
}

void MultiSaveDialog::update_count (bool)
{
	std::vector<game_state *> branches;
	bool all = ui->allRadioButton->isChecked ();
	int count = 0;
	if (all)
		branches.push_back (m_game->get_root ());
	else
		branches.push_back (m_position);
	while (!branches.empty ()) {
		game_state *st = branches.back ();
		branches.pop_back ();
		while (st->n_children () == 1)
			st = st->next_primary_move ();
		if (st->n_children () == 0) {
			count++;
			continue;
		}
		auto c = st->children ();
		branches.insert (branches.end (), c.begin (), c.end ());
	}
	ui->countLabel->setText (QString ("%1 files will be saved.").arg (count));
	m_count = count;
}

bool MultiSaveDialog::save_one (const QString &pattern, int count)
{
	QString count_str = QString::number (count);
	QString max_str = QString::number (m_count);
	while (count_str.length () < max_str.length ())
		count_str = "0" + count_str;
	QString filename = pattern;
	filename.replace (QRegExp ("%n"), count_str);
	QFile f (filename);
	if (f.exists () && !ui->overwriteCheckBox->isChecked ()) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning(this, tr ("File exists"),
					      tr ("A filename matching the pattern and current number already exists.  Overwrite?"),
					      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (choice == QMessageBox::No)
			return false;
	}
	if (!f.open (QIODevice::WriteOnly)) {
		QMessageBox::warning (this, PACKAGE, tr("Cannot open SGF file for saving."));
		return false;
	}
	std::string sgf_str = m_game->to_sgf (true);
	QByteArray bytes = QByteArray::fromStdString (sgf_str);
	qint64 written = f.write (bytes);
	if (written != bytes.length ()) {
		QMessageBox::warning (this, PACKAGE, tr("Failed to save SGF file."));
		return false;
	}
	f.close ();
	return true;
}

void MultiSaveDialog::do_save ()
{
	QString pattern = ui->fileTemplateEdit->text ();
	if (pattern.isEmpty () || !pattern.contains ("%n")) {
		QMessageBox::warning (this, tr ("Filename pattern not set"),
				      tr ("Please enter a filename pattern which includes \"%n\" where the number should be substituted."));
		return;
	}

	/* Identify the currently active branch so we can restore it later.  */
	game_state *last = m_position;
	while (last->n_children () > 0)
		last = last->next_move ();

	bool fail = false;
	std::vector<game_state *> branches;
	bool all = ui->allRadioButton->isChecked ();
	int count = 0;
	if (all)
		branches.push_back (m_game->get_root ());
	else
		branches.push_back (m_position);
	while (!branches.empty ()) {
		game_state *st = branches.back ();
		branches.pop_back ();
		st->make_active ();
		while (st->n_children () == 1)
			st = st->next_primary_move ();
		if (st->n_children () == 0) {
			if (!save_one (pattern, count++)) {
				fail = true;
				break;
			}
			continue;
		}
		auto c = st->children ();
		branches.insert (branches.end (), c.begin (), c.end ());
	}

	/* Restore the active branch.  */
	while (last) {
		last->make_active ();
		last = last->prev_move ();
	}
	if (!fail)
		QDialog::accepted ();
}

void MultiSaveDialog::choose_file ()
{
	QString filename = QFileDialog::getOpenFileName (this, tr ("Choose file name to serve as template for slides"),
							 setting->readEntry ("LAST_DIR"),
							 tr("Images (*.png *.xpm *.jpg);;All Files (*)"));
	if (filename.isEmpty ())
		return;

	ui->fileTemplateEdit->setText (filename);
}
