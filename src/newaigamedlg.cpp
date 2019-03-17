#include <QFileDialog>
#include <QMessageBox>

#include "qgo.h"
#include "defines.h"
#include "newaigamedlg.h"
#include "setting.h"
#include "komispinbox.h"
#include "qgtp.h"

NewAIGameDlg::NewAIGameDlg( QWidget* parent, const std::vector<Engine> &engines)
	: QDialog (parent)
{
	setupUi(this);
	setModal (true);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	for (auto &e: engines)
		engineComboBox->addItem (e.title);

	int sz = setting->readIntEntry("COMPUTER_SIZE");
	if (sz > 3 && sz < 26)
		boardsizeSpinBox->setValue (sz);

	int hc = setting->readIntEntry("COMPUTER_HANDICAP");
	handicapSpinBox->setValue (hc);
	engineColorButton->setChecked (setting->readBoolEntry ("COMPUTER_WHITE"));
	humanPlayerLineEdit->setText (setting->readEntry ("HUMAN_NAME"));
}

void NewAIGameDlg::slotCancel()
{
	QDialog::reject();

}

int NewAIGameDlg::engine_index ()
{
	return engineComboBox->currentIndex ();
}

bool NewAIGameDlg::computer_white_p ()
{
	return engineColorButton->isChecked ();
}

void NewAIGameDlg::slotOk()
{
	if (handicap () == 1)
	{
		QMessageBox msg(tr("Error"),
				tr("You entered an invalid Handicap (1 is not legal)"),
				QMessageBox::Warning,
				QMessageBox::Ok | QMessageBox::Default, Qt::NoButton, Qt::NoButton);
		msg.exec();
		QDialog::reject();
	}
	else
		QDialog::accept();
}


int NewAIGameDlg::board_size()
{
	return boardsizeSpinBox->value ();
}

int NewAIGameDlg::handicap()
{
	return handicapSpinBox->value ();
}

game_info NewAIGameDlg::create_game_info ()
{
	QString human = humanPlayerLineEdit->text ();
	QString computer = engineComboBox->currentText ();
	bool human_is_black = engineColorButton->isChecked ();
	const QString &w = human_is_black ? computer : human;
	const QString &b = human_is_black ? human : computer;
	double komi = komiSpin->value ();
	int hc = handicap ();
	game_info info ("", w.toStdString (), b.toStdString (),
			"", "",
			"", komi, hc, ranked::free, "", "", "", "", "", "", "", "", -1);
	return info;
}

void NewAIGameDlg::slotGetFileName()
{
	QString getFileName(QFileDialog::getOpenFileName(this, tr ("Choose an SGF file to load"),
							 setting->readEntry("LAST_DIR"),
							 tr("SGF Files (*.sgf);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)")));
	if (getFileName.isEmpty())
		return;

	gameToLoad->setText(getFileName);
}
