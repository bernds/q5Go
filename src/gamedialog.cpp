/*
 *   gamedialog.cpp
 */

#include "config.h"
#include "misc.h"
#include "defines.h"
#include "komispinbox.h"
#include "clientwin.h"
#include "gamedialog.h"

#include "ui_newgame_gui.h"

GameDialog::GameDialog (QWidget* parent, GSName gs, const QString &name, const QString &rk,
		       const QString &opponent, const QString &opp_rk)
	: QDialog(parent), ui (std::make_unique<Ui::NewGameDialog> ()), gsname (gs), myName (name), myRk (rk), oppRk (opp_rk), buttongroup (this)
{
	ui->setupUi (this);

	setModal (true);
	have_suggestdata = false;
//	komiSpin->setValue(55);
//	ui->buttonOffer->setFocus();
	komi_request = false;
	set_is_nmatch (false);

	ui->playerOpponentEdit->setText (opponent);
	ui->playerOpponentRkEdit->setText (opp_rk);

//  boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
//  timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
//  byoTimeSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
//	cb_free->setChecked(true);

	buttongroup.addButton (ui->play_white_button);
	buttongroup.addButton (ui->play_black_button);
	buttongroup.addButton (ui->play_nigiri_button);
	buttongroup.setExclusive (true);

	void (QSpinBox::*changed_i) (int) = &QSpinBox::valueChanged;
	void (QDoubleSpinBox::*changed_d) (double) = &QDoubleSpinBox::valueChanged;
	connect (ui->komiSpin, changed_d, [this] (double) { setting_changed (); });
	connect (ui->boardSizeSpin, changed_i, [this] (int) { setting_changed (); });
	connect (ui->handicapSpin, changed_i, [this] (int) { setting_changed (); });
	connect (ui->timeSpin, changed_i, [this] (int) { setting_changed (); });
	connect (ui->byoTimeSpin, changed_i, [this] (int) { setting_changed (); });

	connect (ui->pb_suggest, &QPushButton::clicked, this, &GameDialog::slot_pbsuggest);
	connect (ui->pb_stats, &QPushButton::clicked, this, &GameDialog::slot_stats_opponent);

	connect (ui->buttonOffer, &QPushButton::toggled, this, &GameDialog::slot_offer);
	connect (ui->buttonCancel, &QPushButton::clicked, this, &GameDialog::slot_cancel);
	connect (ui->buttonDecline, &QPushButton::clicked, this, &GameDialog::slot_decline);

	clear_warnings ();
}

GameDialog::~GameDialog()
{

}

void GameDialog::clear_warnings ()
{
	ui->warn_komi->hide ();
	ui->warn_hc->hide ();
	ui->warn_size->hide ();
	ui->warn_rated->hide ();
	ui->warn_time->hide ();
	ui->warn_byo->hide ();
	ui->warn_side->hide ();
}

void GameDialog::slot_stats_opponent (bool)
{
	client_window->sendcommand ("stats " + ui->playerOpponentEdit->text(), false);
}

void GameDialog::swap_colors()
{
	qDebug("#### GameDialog::swap_colors()");
	if (ui->play_white_button->isChecked ())
		ui->play_black_button->setChecked (true);
	else
		ui->play_white_button->setChecked (true);

}

// button "suggest"
void GameDialog::slot_pbsuggest (bool)
{
	qDebug("#### GameDialog::slot_pbsuggest()");
	switch (gsname)
	{
	case NNGS:
		// already suggested?
		if (!have_suggestdata)
		{
			// no suggestdata -> send suggest cmd
			client_window->sendcommand ("suggest " + ui->playerOpponentEdit->text(), false);
		}
		break;

	default:
		// IGS etc. don't know about suggest command
		break;
	}

	// two cases: suggest command unknown / suggest command doesn't work - you are not rated
	if (!have_suggestdata)
	{
		if (oppRk.isNull())
		{
			// cannot calculate without opponent's rank
			qWarning("*** No opponent rk given!");
			return;
		}

		// calcualte some suggest values from ranking info
		int rkw = rkToInt (ui->playerOpponentRkEdit->text ());
		int rkb;

		// don't calc for NR
		if (oppRk.contains("NR") || myRk.contains("NR"))
			rkb = rkw;
		else
			rkb = rkToInt (myRk);

		// white is weaker than black? -> exchange
		if (rkw < rkb)
			swap_colors();

		// calc handi & komi
		int diff = abs(rkw - rkb) / 100;
		if (diff > 9)
			diff = 9;

		if (diff == 0)
			ui->komiSpin->setValue(5.5);
		else if (diff == 1)
			ui->komiSpin->setValue(0.5);
		else
		{
			ui->komiSpin->setValue(0.5);
			ui->handicapSpin->setValue(diff);
		}
	}

}

// from parser
void GameDialog::slot_suggest(const QString &pw, const QString&pb, const QString &handicap, const QString &komi, int size)
{
	qDebug("#### GameDialog::slot_suggest()");

	switch (size)
	{
	case 19:
		h19 = handicap;
		k19 = komi;
		break;

	case 13:
		h13 = handicap;
		k13 = komi;
		break;

	case 9:
		h9 = handicap;
		k9 = komi;
		//have_suggestdata = true;
		break;
	}

	// check if names are ok
	if ((ui->playerOpponentEdit->text() == pb || pb == "you") && (myName == pw || pw == "you"))
	{
		if (pb == "you")
			ui->play_black_button->setChecked (true);
		else
			ui->play_white_button->setChecked (true);
	}
	else if ((ui->playerOpponentEdit->text() != pw && pw != "you") || (myName != pw && pb != "you"))
	{
		// wrong suggest info
		client_window->sendcommand ("suggest " + ui->playerOpponentEdit->text(), false);
	}

	// check if size is ok
	switch (ui->boardSizeSpin->text().toInt())
	{
	case 19:
		ui->handicapSpin->setValue(h19.toInt());
		ui->komiSpin->setValue(k19.toInt());
		break;

	case 13:
		ui->handicapSpin->setValue(h13.toInt());
		ui->komiSpin->setValue(k13.toInt());
		break;

	case 9:
		ui->handicapSpin->setValue(h13.toInt());
		ui->komiSpin->setValue(k13.toInt());
		break;

	default:
		break;
	}
}

void GameDialog::slot_opponentopen(const QString &opp)
{
	qDebug("#### GameDialog::slot_opponentopen()");
	if (ui->playerOpponentEdit->text() != opp)
	    // not for me
	    return;

	QString me;
	QString opponent = ui->playerOpponentEdit->text();;

	// send match command, send tell:

	// 24 *xxxx*: CLIENT: <qGo 1.9.12> match xxxx wants handicap 0, komi 0.5[, free]
	// this command is not part of server preferences "use Komi" and "auto negotiation"
	QString send = "tell " + opponent + " CLIENT: <" + PACKAGE + " " + VERSION + "> match " +
		me + " wants handicap " + ui->handicapSpin->text() + ", komi " +
		ui->komiSpin->text();
	if (ui->ComboBox_free->currentText() == tr("yes"))
		send += ", free";

	client_window->sendcommand (send, false);
}

void GameDialog::slot_offer (bool active)
{
qDebug("#### GameDialog::slot_offer()");
	// active serves for more in the future...
	if (!active)
		return;

	clear_warnings ();

	// if both names are identical -> teaching game
	if (ui->playerOpponentEdit->text() == myName)
	{
		client_window->sendcommand ("teach " + ui->boardSizeSpin->text(), false);
		// prepare for future use...
		ui->buttonOffer->setChecked(false);
		ui->buttonDecline->setDisabled(true);
		ui->buttonCancel->setEnabled(true);
		ui->buttonOffer->setText(tr("Offer"));

		emit accept();
		return;
	}

	// send match command
	QString color = " B ";
	if (ui->play_white_button->isChecked())
		color = " W ";
	else if (ui->play_nigiri_button->isChecked() && is_nmatch)
		color = " N ";

	if (is_nmatch)
		//<nmatch yfh2test W 3 19 60 600 25 0 0 0>
		client_window->sendcommand ("nmatch " +
					ui->playerOpponentEdit->text() +
					color + //" W " +
					ui->handicapSpin->text() + " " +
					ui->boardSizeSpin->text() + " " +
					QString::number(ui->timeSpin->value() * 60) + " " +
					QString::number(ui->byoTimeSpin->value() * 60) +
					" 25 0 0 0", true); // carefull : 25 stones hard coded : bad
	else
		client_window->sendcommand ("match " + ui->playerOpponentEdit->text() + color + ui->boardSizeSpin->text() + " " + ui->timeSpin->text() + " " + ui->byoTimeSpin->text(), false);

	switch (gsname)
	{
	case NNGS:
		ui->buttonDecline->setEnabled(true);
		ui->buttonCancel->setDisabled(true);
		break;

	default:
		// IGS etc. don't support a withdraw command
		ui->buttonDecline->setDisabled(true);
		ui->buttonCancel->setEnabled(true);
		break;
	}
}

void GameDialog::slot_decline (bool)
{
	qDebug("#### GameDialog::slot_decline()");

	QString opponent = ui->playerOpponentEdit->text();

	if (ui->buttonOffer->isChecked())
	{
		// match has been offered
		// !! there seem to be not "setChecked" in the code (apart init, but this should not reach this code)
		client_window->sendcommand ("withdraw", false);
		ui->buttonOffer->setChecked(false);
		ui->buttonOffer->setText(tr("Offer"));
	}
	client_window->sendcommand ("decline " + opponent, false);
	emit signal_removeDialog(opponent);

}

void GameDialog::slot_cancel (bool)
{
	if (is_nmatch)
		client_window->sendcommand ("nmatch _cancel", false);

	QString opponent = ui->playerOpponentEdit->text();

	emit signal_removeDialog(opponent);
}

void GameDialog::setting_changed()
{
	qDebug("#### GameDialog::setting_changed()");
	if (ui->playerOpponentEdit->text() == myName)
	{
		ui->buttonOffer->setText(tr("Teaching"));
		ui->ComboBox_free->setEnabled(false);
//		cb_free->setEnabled(false);
		ui->byoTimeSpin->setEnabled(false);
		ui->timeSpin->setEnabled(false);
	}
	else
	{
		ui->buttonOffer->setText(tr("Offer"));
		ui->ComboBox_free->setEnabled(true);
		//cb_free->setEnabled(true);
		ui->byoTimeSpin->setEnabled(true);
		ui->timeSpin->setEnabled(true);
	}

	// check for free game
	if (ui->playerOpponentRkEdit->text() == "NR" || myRk == "NR" ||
		/*ui->boardSizeSpin->text() != "19" && ui->boardSizeSpin->text() != "9" ||*/
		ui->playerOpponentRkEdit->text() == myRk)
	{
//		cb_free->setChecked(true);
//		cb_free->setEnabled(false);
		ui->ComboBox_free->setCurrentIndex(1);
//		ui->ComboBox_free->setEnabled(false);
	}
	else
	{
//		cb_free->setEnabled(true);
		ui->ComboBox_free->setEnabled(true);
	}
}

void GameDialog::slot_matchcreate(const QString &nr, const QString &opponent)
{
	qDebug("#### GameDialog::slot_matchcreate()");
	if (ui->playerOpponentEdit->text() == opponent)
	{
		// current match has been created -> send settings
		assessType kt;
		// check if komi has been requested
		if (myRk != "NR" && oppRk != "NR")
		{
			if (ui->ComboBox_free->currentText() == tr("yes"))
				kt = FREE;
			else
				kt = RATED;
		}
		else
			kt = noREQ;

		// send to qgoif
		emit signal_matchsettings(nr, ui->handicapSpin->text(), ui->komiSpin->text(), kt);

		// close dialog
		ui->buttonOffer->setChecked(false);
		emit accept();

		return;
	}
}

void GameDialog::slot_notopen (const QString &opponent)
{
	qDebug("#### GameDialog::slot_notopen()");
	if (opponent.isNull())
	{
		// IGS: no player named -> check if offering && focus set
		if (ui->buttonOffer->isChecked())
		{
			ui->buttonOffer->setChecked(false);
			ui->buttonOffer->setText(tr("Offer"));
			ui->buttonDecline->setDisabled(true);
			ui->buttonCancel->setEnabled(true);
		}
	}
	else if (ui->playerOpponentEdit->text() == opponent)
	{
		ui->buttonOffer->setChecked(false);
		ui->buttonOffer->setText(tr("Offer"));
		ui->buttonDecline->setDisabled(true);
		ui->buttonCancel->setEnabled(true);
		reject();
	}
}

// if opponent has requestet for handicap, komi and/or free game
void GameDialog::slot_komirequest(const QString &opponent, int h, float k, bool free)
{
	qDebug("#### GameDialog::slot_komirequest()");
	if (ui->playerOpponentEdit->text() == opponent)
	{
//		cb_handicap->setChecked(true);
		ui->handicapSpin->setValue(h);
//		cb_komi->setChecked(true);
		komi_request = true; //the komi checkbox has been replaced by
		ui->komiSpin->setValue(k);
//		cb_free->setChecked(true);
		if (free)
			ui->ComboBox_free->setCurrentIndex(1);
		else
			ui->ComboBox_free->setCurrentIndex(0);

		ui->buttonOffer->setText(tr("Accept"));
		ui->buttonOffer->setChecked(false);
		ui->buttonCancel->setDisabled(true);
	}
	else
		ui->buttonCancel->setEnabled(true);
}


void GameDialog::slot_dispute (const QString &opponent, const QString &line)
{
	if (ui->playerOpponentEdit->text () != opponent)
		return;

	clear_warnings ();

	QString val;
	val = line.section(' ', 1, 1);
	if (ui->handicapSpin->value() != val.toInt ())
	{
		ui->handicapSpin->setValue(val.toInt ());
		ui->warn_hc->show ();
	}

	val = line.section(' ', 2, 2);
	if (ui->boardSizeSpin->value() != val.toInt ())
	{
		ui->boardSizeSpin->setValue(val.toInt ());
		ui->warn_size->show ();
	}

	val = line.section(' ', 3, 3);
	if (ui->timeSpin->value() != val.toInt () / 60)
	{
		ui->timeSpin->setValue(val.toInt () / 60);
		ui->warn_time->show ();
	}

	val = line.section(' ', 4, 4);
	if (ui->byoTimeSpin->value() != val.toInt () / 60)
	{
		ui->byoTimeSpin->setValue(val.toInt () / 60);
		ui->warn_byo->show ();
	}

	//val = line.section(' ', 5, 5);
	//BY_label->setText(tr(" Byoyomi Time : (")+ val + tr(" stones)"));

	val = line.section(' ', 0, 0);
	if (!ui->play_nigiri_button->isChecked () && val == "N")
	{
		ui->play_nigiri_button->setChecked (true);
		ui->warn_side->show ();
	}
	else if (ui->play_black_button->isChecked () && val == "B")
	{
		ui->play_white_button->setChecked (true);
		ui->warn_side->show ();
	}
	else if (ui->play_white_button->isChecked () && val == "W")
	{
		ui->play_black_button->setChecked (true);
		ui->warn_side->show ();

	}

	ui->buttonOffer->setText(tr("Accept"));
	ui->buttonOffer->setChecked (false);
	ui->buttonDecline->setEnabled (true);
}

void GameDialog::set_is_nmatch (bool b)
{
	is_nmatch = b;

	// Certain options are not available with normal game requests.
	ui->handicapSpin->setEnabled (is_nmatch);
	ui->play_nigiri_button->setEnabled (is_nmatch);
}
