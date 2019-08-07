/*
 *   gamedialog.cpp
 */
//#include "setting.h"
#include "config.h"
#include "misc.h"
#include "defines.h"
#include "komispinbox.h"
#include "clientwin.h"
#include "gamedialog.h"

GameDialog::GameDialog(QWidget* parent, GSName gs, const QString &name)
	: QDialog(parent), gsname (gs), myName (name), buttongroup (this)
{
	setupUi(this);
	setModal (true);
	have_suggestdata = false;
//	komiSpin->setValue(55);
//	buttonOffer->setFocus();
	komi_request = false;
	is_nmatch = false;
  
//  boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
//  timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
//  byoTimeSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
//	cb_free->setChecked(true);

	buttongroup.addButton (play_white_button);
	buttongroup.addButton (play_black_button);
	buttongroup.addButton (play_nigiri_button);
	buttongroup.setExclusive (true);

	void (QSpinBox::*changed_i) (int) = &QSpinBox::valueChanged;
	void (QDoubleSpinBox::*changed_d) (double) = &QDoubleSpinBox::valueChanged;
	connect (komiSpin, changed_d, [this] (double) { setting_changed (); });
	connect (boardSizeSpin, changed_i, [this] (int) { setting_changed (); });
	connect (handicapSpin, changed_i, [this] (int) { setting_changed (); });
	connect (timeSpin, changed_i, [this] (int) { setting_changed (); });
	connect (byoTimeSpin, changed_i, [this] (int) { setting_changed (); });

	connect (pb_suggest, &QPushButton::clicked, this, &GameDialog::slot_pbsuggest);
	connect (pb_stats, &QPushButton::clicked, this, &GameDialog::slot_stats_opponent);

	connect (buttonOffer, &QPushButton::toggled, this, &GameDialog::slot_offer);
	connect (buttonCancel, &QPushButton::clicked, this, &GameDialog::slot_cancel);
	connect (buttonDecline, &QPushButton::clicked, this, &GameDialog::slot_decline);

	clear_warnings ();
}

GameDialog::~GameDialog()
{

}

void GameDialog::clear_warnings ()
{
	warn_komi->hide ();
	warn_hc->hide ();
	warn_size->hide ();
	warn_rated->hide ();
	warn_time->hide ();
	warn_byo->hide ();
	warn_side->hide ();
}

void GameDialog::slot_stats_opponent (bool)
{
	client_window->sendcommand ("stats " + playerOpponentEdit->text(), false);
}

void GameDialog::swap_colors()
{
	qDebug("#### GameDialog::swap_colors()");
	if (play_white_button->isChecked ())
		play_black_button->setChecked (true);
	else
		play_white_button->setChecked (true);

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
			client_window->sendcommand ("suggest " + playerOpponentEdit->text(), false);
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
		int rkw = rkToKey(playerOpponentRkEdit->text(), true).toInt();
		int rkb;

		// don't calc for NR
		if (oppRk.contains("NR") || myRk.contains("NR"))
			rkb = rkw;
		else
			rkb = rkToKey(myRk, true).toInt();

		// white is weaker than black? -> exchange
		if (rkw < rkb)
			swap_colors();

		// calc handi & komi
		int diff = abs(rkw - rkb) / 100;
		if (diff > 9)
			diff = 9;

		if (diff == 0)
			komiSpin->setValue(5.5);
		else if (diff == 1)
			komiSpin->setValue(0.5);
		else
		{
			komiSpin->setValue(0.5);
			handicapSpin->setValue(diff);
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
	if ((playerOpponentEdit->text() == pb || pb == "you") && (myName == pw || pw == "you"))
	{
		if (pb == "you")
			play_black_button->setChecked (true);
		else
			play_white_button->setChecked (true);
	}
	else if ((playerOpponentEdit->text() != pw && pw != "you") || (myName != pw && pb != "you"))
	{
		// wrong suggest info
		client_window->sendcommand ("suggest " + playerOpponentEdit->text(), false);
	}

	// check if size is ok
	switch (boardSizeSpin->text().toInt())
	{
	case 19:
		handicapSpin->setValue(h19.toInt());
		komiSpin->setValue(k19.toInt());
		break;

	case 13:
		handicapSpin->setValue(h13.toInt());
		komiSpin->setValue(k13.toInt());
		break;

	case 9:
		handicapSpin->setValue(h13.toInt());
		komiSpin->setValue(k13.toInt());
		break;

	default:
		break;
	}
}

void GameDialog::slot_opponentopen(const QString &opp)
{
	qDebug("#### GameDialog::slot_opponentopen()");
	if (playerOpponentEdit->text() != opp)
	    // not for me
	    return;

	QString me;
	QString opponent = playerOpponentEdit->text();;

	// send match command, send tell:

	// 24 *xxxx*: CLIENT: <qGo 1.9.12> match xxxx wants handicap 0, komi 0.5[, free]
	// this command is not part of server preferences "use Komi" and "auto negotiation"
	QString send = "tell " + opponent + " CLIENT: <qGo " + VERSION + "> match " +
		me + " wants handicap " + handicapSpin->text() + ", komi " +
		komiSpin->text();
	if (ComboBox_free->currentText() == tr("yes"))
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
	if (playerOpponentEdit->text() == myName)
	{
		client_window->sendcommand ("teach " + boardSizeSpin->text(), false);
		// prepare for future use...
		buttonOffer->setChecked(false);
		buttonDecline->setDisabled(true);
		buttonCancel->setEnabled(true);
		buttonOffer->setText(tr("Offer"));

		emit accept();
		return;
	}

	// send match command
	QString color = " B ";
	if (play_white_button->isChecked())
		color = " W ";
	else if (play_nigiri_button->isChecked() && is_nmatch)
		color = " N ";

	if (is_nmatch)
		//<nmatch yfh2test W 3 19 60 600 25 0 0 0>
		client_window->sendcommand ("nmatch " +
					playerOpponentEdit->text() +
					color + //" W " +
					handicapSpin->text() + " " +
					boardSizeSpin->text() + " " +
					QString::number(timeSpin->value() * 60) + " " +
					QString::number(byoTimeSpin->value() * 60) +
					" 25 0 0 0", true); // carefull : 25 stones hard coded : bad
	else
		client_window->sendcommand ("match " + playerOpponentEdit->text() + color + boardSizeSpin->text() + " " + timeSpin->text() + " " + byoTimeSpin->text(), false);

	switch (gsname)
	{
	case NNGS:
		buttonDecline->setEnabled(true);
		buttonCancel->setDisabled(true);
		break;

	default:
		// IGS etc. don't support a withdraw command
		buttonDecline->setDisabled(true);
		buttonCancel->setEnabled(true);
		break;
	}
}

void GameDialog::slot_decline (bool)
{
	qDebug("#### GameDialog::slot_decline()");

	QString opponent = playerOpponentEdit->text();

	if (buttonOffer->isChecked())
	{
		// match has been offered
		// !! there seem to be not "setChecked" in the code (apart init, but this should not reach this code)
		client_window->sendcommand ("withdraw", false);
		buttonOffer->setChecked(false);
		buttonOffer->setText(tr("Offer"));
	}
	client_window->sendcommand ("decline " + opponent, false);
	emit signal_removeDialog(opponent);

}

void GameDialog::slot_cancel (bool)
{
	if (is_nmatch)
		client_window->sendcommand ("nmatch _cancel", false);

	QString opponent = playerOpponentEdit->text();

	emit signal_removeDialog(opponent);
}

void GameDialog::setting_changed()
{
	qDebug("#### GameDialog::setting_changed()");
	if (playerOpponentEdit->text() == myName)
	{
		buttonOffer->setText(tr("Teaching"));
		ComboBox_free->setEnabled(false);
//		cb_free->setEnabled(false);
		byoTimeSpin->setEnabled(false);
		timeSpin->setEnabled(false);
	}
	else
	{
		buttonOffer->setText(tr("Offer"));
		ComboBox_free->setEnabled(true);
		//cb_free->setEnabled(true);
		byoTimeSpin->setEnabled(true);
		timeSpin->setEnabled(true);
	}

	// check for free game
	if (playerOpponentRkEdit->text() == "NR" || myRk == "NR" ||
		/*boardSizeSpin->text() != "19" && boardSizeSpin->text() != "9" ||*/
		playerOpponentRkEdit->text() == myRk)
	{
//		cb_free->setChecked(true);
//		cb_free->setEnabled(false);
		ComboBox_free->setCurrentIndex(1);
//		ComboBox_free->setEnabled(false);
	}
	else
	{
//		cb_free->setEnabled(true);
		ComboBox_free->setEnabled(true);
	}
}

void GameDialog::slot_matchcreate(const QString &nr, const QString &opponent)
{
	qDebug("#### GameDialog::slot_matchcreate()");
	if (playerOpponentEdit->text() == opponent)
	{
		// current match has been created -> send settings
		assessType kt;
		// check if komi has been requested
		if (myRk != "NR" && oppRk != "NR")
		{
			if (ComboBox_free->currentText() == tr("yes"))
				kt = FREE;
			else
				kt = RATED;
		}
		else
			kt = noREQ;

		// send to qgoif
		emit signal_matchsettings(nr, handicapSpin->text(), komiSpin->text(), kt);

		// close dialog
		buttonOffer->setChecked(false);
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
		if (buttonOffer->isChecked())
		{
			buttonOffer->setChecked(false);
			buttonOffer->setText(tr("Offer"));
			buttonDecline->setDisabled(true);
			buttonCancel->setEnabled(true);
		}
	}
	else if (playerOpponentEdit->text() == opponent)
	{
		buttonOffer->setChecked(false);
		buttonOffer->setText(tr("Offer"));
		buttonDecline->setDisabled(true);
		buttonCancel->setEnabled(true);
		reject();
	}
}

// if opponent has requestet for handicap, komi and/or free game
void GameDialog::slot_komirequest(const QString &opponent, int h, float k, bool free)
{
	qDebug("#### GameDialog::slot_komirequest()");
	if (playerOpponentEdit->text() == opponent)
	{
//		cb_handicap->setChecked(true);
		handicapSpin->setValue(h);
//		cb_komi->setChecked(true);
		komi_request = true; //the komi checkbox has been replaced by
		komiSpin->setValue(k);
//		cb_free->setChecked(true);
		if (free)
			ComboBox_free->setCurrentIndex(1);
		else
			ComboBox_free->setCurrentIndex(0);

		buttonOffer->setText(tr("Accept"));
		buttonOffer->setChecked(false);
		buttonCancel->setDisabled(true);
	}
	else
		buttonCancel->setEnabled(true);
}


void GameDialog::slot_dispute (const QString &opponent, const QString &line)
{
	if (playerOpponentEdit->text () != opponent)
		return;

	clear_warnings ();

	QString val;
	val = line.section(' ', 1, 1);
	if (handicapSpin->value() != val.toInt ())
	{
		handicapSpin->setValue(val.toInt ());
		warn_hc->show ();
	}

	val = line.section(' ', 2, 2);
	if (boardSizeSpin->value() != val.toInt ())
	{
		boardSizeSpin->setValue(val.toInt ());
		warn_size->show ();
	}

	val = line.section(' ', 3, 3);
	if (timeSpin->value() != val.toInt () / 60)
	{
		timeSpin->setValue(val.toInt () / 60);
		warn_time->show ();
	}

	val = line.section(' ', 4, 4);
	if (byoTimeSpin->value() != val.toInt () / 60)
	{
		byoTimeSpin->setValue(val.toInt () / 60);
		warn_byo->show ();
	}

	//val = line.section(' ', 5, 5);
	//BY_label->setText(tr(" Byoyomi Time : (")+ val + tr(" stones)"));

	val = line.section(' ', 0, 0);
	if (!play_nigiri_button->isChecked () && val == "N")
	{
		play_nigiri_button->setChecked (true);
		warn_side->show ();
	}
	else if (play_black_button->isChecked () && val == "B")
	{
		play_white_button->setChecked (true);
		warn_side->show ();
	}
	else if (play_white_button->isChecked () && val == "W")
	{
		play_black_button->setChecked (true);
		warn_side->show ();

	}

	buttonOffer->setText(tr("Accept"));
	buttonOffer->setChecked (false);
	buttonDecline->setEnabled (true);
}
