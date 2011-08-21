/*
 *   gamedialog.cpp
 */
//#include "setting.h"
#include "gamedialog.h"
#include "newgame_gui.h"
#include "config.h"
#include "misc.h"
#include "defines.h"
#include "komispinbox.h"

GameDialog::GameDialog(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
	: QDialog(parent, name, modal)
{
	setupUi(this);
	have_suggestdata = false;
	gsname = GS_UNKNOWN;
//	komiSpin->setValue(55);
//	buttonOffer->setFocus();
	komi_request = false;
	is_nmatch = false;
  
//  boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
//  timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
//  byoTimeSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
//	cb_free->setChecked(true);
}

GameDialog::~GameDialog()
{

}

void GameDialog::slot_stats_opponent()
{
/* 	if (playerWhiteEdit->isReadOnly())
	{
		// ok, I am white
		emit signal_sendcommand("stats " + playerBlackEdit->text(), false);
	}
	else
	{
		// ok, I am black
		emit signal_sendcommand("stats " + playerWhiteEdit->text(), false);
	}
*/
	emit signal_sendcommand("stats " + playerOpponentEdit->text(), false);


}
void GameDialog::slot_swapcolors()
{
qDebug("#### GameDialog::slot_swapcolors()");
/*	QString save = playerWhiteEdit->text();
	playerWhiteEdit->setText(playerBlackEdit->text());
	playerBlackEdit->setText(save);

	save = playerWhiteRkEdit->text();
	playerWhiteRkEdit->setText(playerBlackRkEdit->text());
	playerBlackRkEdit->setText(save);

	// swap ReadOnly field
	if (playerWhiteEdit->isReadOnly())
	{
		playerBlackEdit->setReadOnly(true);
		playerWhiteEdit->setReadOnly(false);
	}
	else
	{
		playerBlackEdit->setReadOnly(false);
		playerWhiteEdit->setReadOnly(true);
	}
*/
	if(play_white_button->isChecked())
		play_black_button->setChecked(TRUE);
	else
		play_white_button->setChecked(TRUE);

}

// button "suggest"
void GameDialog::slot_pbsuggest()
{
qDebug("#### GameDialog::slot_pbsuggest()");
	switch (gsname)
	{
		case NNGS:
		case CWS:
			// already suggested?
			if (!have_suggestdata)
			{
				// no suggestdata -> send suggest cmd
/*				if (playerWhiteEdit->isReadOnly())
					emit signal_sendcommand("suggest " + playerBlackEdit->text(), false);
				else
					emit signal_sendcommand("suggest " + playerWhiteEdit->text(), false);
*/
				emit signal_sendcommand("suggest " + playerOpponentEdit->text(), false);
				
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
			rkb = rkToKey(myRk,true).toInt();//playerBlackRkEdit->text(), true).toInt();
		
		// white is weaker than black? -> exchange
		if (rkw < rkb)
			slot_swapcolors();
		
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
	pwhite = pw;
	pblack = pb;

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
	if ((playerOpponentEdit->text() == pblack || pblack == QString(tr("you"))) &&
	    (myName == pwhite || pwhite == QString(tr("you"))))
	{
		// names are exchanged
		slot_swapcolors();
	}
	else if (playerOpponentEdit->text() != pwhite && pwhite != QString(tr("you")) ||
		 myName != pwhite && pblack != QString(tr("you")))
	{
		// wrong suggest info
		/*if (playerWhiteEdit->isReadOnly())
			emit signal_sendcommand("suggest " + playerBlackEdit->text(), false);
		else
			emit signal_sendcommand("suggest " + playerWhiteEdit->text(), false);*/
		emit signal_sendcommand("suggest " + playerOpponentEdit->text(), false);
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
/*
	// check if button pressed -> set values immediatly
	if (have_suggestdata)
	{
		slot_pbsuggest();
		//pb_suggest->toggle();
	}//*/
}

void GameDialog::slot_opponentopen(const QString &opp)
{
qDebug("#### GameDialog::slot_opponentopen()");
	if (playerOpponentEdit->text() != opp)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() != opp ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() != opp)
	    // not for me
	    return;

	QString me;
	QString opponent = playerOpponentEdit->text();;
	// send match command, send tell:
	/*if (playerWhiteEdit->isReadOnly())
	{
		// ok, I am white
		opponent = playerBlackEdit->text();
		me = playerWhiteEdit->text();
	}
	else
	{
		// ok, I am black
		opponent = playerWhiteEdit->text();
		me = playerBlackEdit->text();
	}
	*/
	// 24 *xxxx*: CLIENT: <qGo 1.9.12> match xxxx wants handicap 0, komi 0.5[, free]
	// this command is not part of server preferences "use Komi" and "auto negotiation"
	QString send = "tell " + opponent + " CLIENT: <qGo " + VERSION + "> match " +
		me + " wants handicap " + handicapSpin->text() + ", komi " +
		komiSpin->text();
	if (ComboBox_free->currentText() == QString(tr("yes")))
		send += ", free";

	emit signal_sendcommand(send, false);
}

void GameDialog::slot_offer(bool active)
{
qDebug("#### GameDialog::slot_offer()");
	// active serves for more in the future...
	if (!active)
		return;

	// if both names are identical -> teaching game
	if (playerOpponentEdit->text() == myName)
	{
		emit signal_sendcommand("teach " + boardSizeSpin->text(), false);
		// prepare for future use...
		buttonOffer->setOn(false);
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
	//{
		// ok, I am white
		if (is_nmatch)
			//<nmatch yfh2test W 3 19 60 600 25 0 0 0>
			emit signal_sendcommand("nmatch " + 
						playerOpponentEdit->text() + 
						color + //" W " + 
						handicapSpin->text() + " " +
						boardSizeSpin->text() + " " + 
						QString::number(timeSpin->value() * 60) + " " +
						QString::number(byoTimeSpin->value() * 60) + 
						" 25 0 0 0", true); // carefull : 25 stones hard coded : bad
		else 
			emit signal_sendcommand("match " + playerOpponentEdit->text() + color + boardSizeSpin->text() + " " + timeSpin->text() + " " + byoTimeSpin->text(), false);
/*	}
	else
	{
		// ok, I am black
		if (is_nmatch)
			//<nmatch yfh2test W 3 19 60 600 25 0 0 0>
			emit signal_sendcommand("nmatch " + 
						playerWhiteEdit->text() + 
						" B " + 
						handicapSpin->text() + " " +
						boardSizeSpin->text() + " " + 
						QString::number(timeSpin->value() * 60) + " " +
						QString::number(byoTimeSpin->value() * 60) + 
						" 25 0 0 0", true); // carefull : 25 stones hard coded : bad
		else 
			emit signal_sendcommand("match " + playerWhiteEdit->text() + " B " + boardSizeSpin->text() + " " + timeSpin->text() + " " + byoTimeSpin->text(), false);
	}
*/
	switch (gsname)
	{
		case NNGS:
		case CWS:
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

void GameDialog::slot_decline()
{
qDebug("#### GameDialog::slot_decline()");
	
	QString opponent = playerOpponentEdit->text();//(playerWhiteEdit->isReadOnly() ? playerBlackEdit->text():playerWhiteEdit->text());

	if (buttonOffer->isOn())
	{
		// match has been offered
		// !! there seem to be not "setOn" in the code (apart init, but this should not reach this code)
		emit signal_sendcommand("withdraw", false);
		buttonOffer->setOn(false);
		buttonOffer->setText(tr("Offer"));
	}
/*	else if (playerWhiteEdit->isReadOnly())
	{
		// ok, I am white
		emit signal_sendcommand("decline " + playerBlackEdit->text(), false);
	}
	else
	{
		// ok, I am black
		emit signal_sendcommand("decline " + playerWhiteEdit->text(), false);
	}

	//buttonDecline->setDisabled(true);
	//buttonOffer->setText(tr("Offer"));
	//buttonCancel->setEnabled(true);

  	//reject();
*/
	emit signal_sendcommand("decline " + opponent, false);
	emit signal_removeDialog(opponent);

}

void GameDialog::slot_cancel()
{

	if (is_nmatch)
		emit signal_sendcommand("nmatch _cancel", false);

	QString opponent = playerOpponentEdit->text();//(playerWhiteEdit->isReadOnly() ? playerBlackEdit->text():playerWhiteEdit->text());

	emit signal_removeDialog(opponent);
}

void GameDialog::slot_changed()
{
qDebug("#### GameDialog::slot_changed()");
	if (playerOpponentEdit->text() == myName)// playerBlackEdit->text())
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
		ComboBox_free->setCurrentItem(1);
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
	if (playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{
		// current match has been created -> send settings
		assessType kt;
		// check if komi has been requested
		if (myRk != "NR" && oppRk != "NR")
		{
			if (ComboBox_free->currentText() == QString(tr("yes")))
				kt = FREE;
			else
				kt = RATED;
		}
		else
			kt = noREQ;

		// send to qgoif
		emit signal_matchsettings(nr, handicapSpin->text(), komiSpin->text(), kt);

		// close dialog
		buttonOffer->setOn(false);
		emit accept();

		return;
	}
}

void GameDialog::slot_notopen(const QString &opponent)
{
qDebug("#### GameDialog::slot_notopen()");
	if (opponent.isNull())
	{
		// IGS: no player named -> check if offering && focus set
		if (buttonOffer->isOn())// && QWidget::hasFocus())
		{
			buttonOffer->setOn(false);
			buttonOffer->setText(tr("Offer"));
			buttonDecline->setDisabled(true);
			buttonCancel->setEnabled(true);
		}
	}
	else if (playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	         playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{
		buttonOffer->setOn(false);
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
	if (playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)
	{
//		cb_handicap->setChecked(true);
		handicapSpin->setValue(h);
//		cb_komi->setChecked(true);
		komi_request = true; //the komi checkbox has been replaced by
		komiSpin->setValue(k);
//		cb_free->setChecked(true);
		if (free)
			ComboBox_free->setCurrentItem(1);
		else
			ComboBox_free->setCurrentItem(0);

		buttonOffer->setText(tr("Accept"));
		buttonOffer->setOn(false);
		buttonCancel->setDisabled(true);
	}
	else
		buttonCancel->setEnabled(true);
}


void GameDialog::slot_dispute(const QString &opponent, const QString &line)
{
	QString val;

	if (playerOpponentEdit->text() == opponent)//(playerWhiteEdit->isReadOnly() && playerBlackEdit->text() == opponent ||	    playerBlackEdit->isReadOnly() && playerWhiteEdit->text() == opponent)

	{
		val = line.section(' ', 1, 1);
		if (handicapSpin->value() != val.toInt())
		{
			handicapSpin->setValue(val.toInt());
			handicapSpin->setPaletteBackgroundColor(QColor("cyan"));
		}
		else
			handicapSpin->unsetPalette();

		val = line.section(' ', 2, 2);
		if (boardSizeSpin->value() != val.toInt())
		{
			boardSizeSpin->setValue(val.toInt());
			boardSizeSpin->setPaletteBackgroundColor(QColor("cyan"));
		}
		else
			boardSizeSpin->unsetPalette();

		val = line.section(' ', 3, 3);
		if (timeSpin->value() != val.toInt()/60)
		{
			timeSpin->setValue(val.toInt()/60);
			timeSpin->setPaletteBackgroundColor(QColor("cyan"));
		}
		else
			timeSpin->unsetPalette();

		val = line.section(' ', 4, 4);
		if (byoTimeSpin->value() != val.toInt()/60)
		{
			byoTimeSpin->setValue(val.toInt()/60);
			byoTimeSpin->setPaletteBackgroundColor(QColor("cyan"));
		}
		else
			byoTimeSpin->unsetPalette();	

		//val = line.section(' ', 5, 5);
		//BY_label->setText(tr(" Byoyomi Time : (")+ val + tr(" stones)"));

		val = line.section(' ', 0, 0);
		if ( !(play_nigiri_button->isChecked()) && (val == "N"))
		{
			play_nigiri_button->setPaletteBackgroundColor(QColor("cyan"));
			play_white_button->setPaletteBackgroundColor(QColor("cyan"));
			play_black_button->setPaletteBackgroundColor(QColor("cyan"));
			play_nigiri_button->setChecked(true);

		}
		else if ( (play_black_button->isChecked()) && (val == "B"))
		{
			play_nigiri_button->setPaletteBackgroundColor(QColor("cyan"));
			play_white_button->setPaletteBackgroundColor(QColor("cyan"));
			play_black_button->setPaletteBackgroundColor(QColor("cyan"));
			play_white_button->setChecked(true);

		}
		else if ( (play_white_button->isChecked()) && (val == "W"))
		{
			play_nigiri_button->setPaletteBackgroundColor(QColor("cyan"));
			play_white_button->setPaletteBackgroundColor(QColor("cyan"));
			play_black_button->setPaletteBackgroundColor(QColor("cyan"));
			play_black_button->setChecked(true);

		}
		else
		{
			play_nigiri_button->unsetPalette();
			play_white_button->unsetPalette();
			play_black_button->unsetPalette();
		}

	buttonOffer->setText(tr("Accept"));
	buttonOffer->setOn(false);
	buttonDecline->setEnabled(true);
	
	}
}
