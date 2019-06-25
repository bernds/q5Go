#include <QFileDialog>
#include <QMessageBox>

#include "qgo.h"
#include "defines.h"
#include "setting.h"
#include "newaigamedlg.h"
#include "setting.h"
#include "komispinbox.h"
#include "qgtp.h"

#include "ui_newaigamedlg_gui.h"
#include "ui_twoaigamedlg_gui.h"

template<class UI>
AIGameDlg<UI>::AIGameDlg (QWidget *parent) : QDialog (parent), ui (new UI)
{
	ui->setupUi (this);
	setModal (true);

	connect (ui->filePathButton, &QPushButton::clicked, [this] (bool) { get_file_name (); });
	connect (ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect (ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	int sz = setting->readIntEntry("COMPUTER_SIZE");
	if (sz > 3 && sz < 26)
		ui->boardsizeSpinBox->setValue (sz);
	// ui->boardsizeSpinBox->setValidator (new QIntValidator (4, 25, this));
}

template<class UI>
AIGameDlg<UI>::~AIGameDlg ()
{
	delete ui;
}

template<class UI>
QString AIGameDlg<UI>::game_to_load ()
{
	return ui->gameToLoad->text ();
}

template<class UI>
int AIGameDlg<UI>::board_size ()
{
	return ui->boardsizeSpinBox->value ();
}

template<class UI>
int AIGameDlg<UI>::handicap ()
{
	return ui->handicapSpinBox->value ();
}

template<class UI>
void AIGameDlg<UI>::get_file_name ()
{
	QString getFileName = QFileDialog::getOpenFileName (this, tr ("Choose an SGF file to load"),
							    setting->readEntry ("LAST_DIR"),
							    tr ("SGF Files (*.sgf);;All Files (*)"));
	if (getFileName.isEmpty())
		return;

	ui->gameToLoad->setText (getFileName);
}

NewAIGameDlg::NewAIGameDlg( QWidget* parent, bool from_position)
	: AIGameDlg (parent)
{
	for (auto &e: setting->m_engines)
		ui->engineComboBox->addItem (e.title);

	int hc = setting->readIntEntry ("COMPUTER_HANDICAP");
	ui->handicapSpinBox->setValue (hc);
	ui->engineColorButton->setChecked (setting->readBoolEntry ("COMPUTER_WHITE"));
	ui->humanPlayerLineEdit->setText (setting->readEntry ("HUMAN_NAME"));
	if (from_position) {
		ui->gameParamsBox->hide ();
		ui->loadBox->hide ();
		setWindowTitle (tr ("Play engine from current position"));
	}
}

int NewAIGameDlg::engine_index ()
{
	return ui->engineComboBox->currentIndex ();
}

bool NewAIGameDlg::computer_white_p ()
{
	return ui->engineColorButton->isChecked ();
}

game_info NewAIGameDlg::create_game_info ()
{
	QString human = ui->humanPlayerLineEdit->text ();
	QString computer = ui->engineComboBox->currentText ();
	bool human_is_black = ui->engineColorButton->isChecked ();
	const QString &w = human_is_black ? computer : human;
	const QString &b = human_is_black ? human : computer;
	double komi = ui->komiSpin->value ();
	int hc = handicap ();
	game_info info ("", w.toStdString (), b.toStdString (),
			"", "",
			"", komi, hc, ranked::free, "", "", "", "", "", "", "", "", -1);
	return info;
}

TwoAIGameDlg::TwoAIGameDlg( QWidget* parent)
	: AIGameDlg (parent)
{
	for (auto &e: setting->m_engines) {
		ui->engineWComboBox->addItem (e.title);
		ui->engineBComboBox->addItem (e.title);
	}
	ui->numGamesEdit->setValidator (new QIntValidator (1, 9999, this));

	ui->handicapSpinBox->setValue (0);
	// Not implemented yet
	ui->timeBox->setVisible (false);
}

game_info TwoAIGameDlg::create_game_info ()
{
	QString w = ui->engineWComboBox->currentText ();
	QString b = ui->engineBComboBox->currentText ();
	double komi = ui->komiSpin->value ();
	int hc = handicap ();
	game_info info ("", w.toStdString (), b.toStdString (),
			"", "",
			"", komi, hc, ranked::free, "", "", "", "", "", "", "", "", -1);
	return info;
}

int TwoAIGameDlg::num_games ()
{
	return ui->numGamesEdit->text ().toInt ();
}

bool TwoAIGameDlg::opening_book ()
{
	return ui->playOpeningsCheckBox->isChecked ();
}

int TwoAIGameDlg::engine_index (stone_color c)
{
	return c == white ? ui->engineWComboBox->currentIndex () : ui->engineBComboBox->currentIndex ();
}

void TwoAIGameDlg::accept ()
{
#if 0
	if (ui->playFixedRadio->isChecked ())
		if (ui->numGamesEdit->text ().isEmpty ()) {
			QMessageBox msg (tr ("Error"),
					 tr ("The number of games was not set."),
					 QMessageBox::Warning,
					 QMessageBox::Ok | QMessageBox::Default, Qt::NoButton, Qt::NoButton);
			return;
		}
#endif
	if (ui->playOpeningsCheckBox->isChecked ()) {
		if (ui->gameToLoad->text ().isEmpty ()) {
			QMessageBox msg (tr ("Error"),
					 tr ("Opening book was selected but no file name specified."),
					 QMessageBox::Warning,
					 QMessageBox::Ok | QMessageBox::Default, Qt::NoButton, Qt::NoButton);
			return;
		}
	}
	QDialog::accept ();
}

template class AIGameDlg<Ui::NewAIGameDlgGui>;
template class AIGameDlg<Ui::TwoAIGameDlgGui>;
