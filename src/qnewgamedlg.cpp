/***************************************************************************
                          qnewgamedlg.cpp  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QFileDialog>
#include "qnewgamedlg.h"
#include "defines.h"
#include "setting.h"
#include "komispinbox.h"
#include <qmessagebox.h>
#include <stdio.h>


/* 
 *  Constructs a QNewGameDlg which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a mod#include "komispinbox.h"al dialog.
 */




QNewGameDlg::QNewGameDlg( QWidget* parent,  const char* name)
    : QDialog( parent, name, true )
{
	setupUi(this);
	init();
//	initDialog();

    // signals and slots connections
    connect( _BlackTypeComboBox, SIGNAL( activated(int) ), this, SLOT( slotPlayerBlackTypeChanged() ) );
    connect( _WhiteTypeComboBox, SIGNAL( activated(int) ), this, SLOT( slotPlayerWhiteTypeChanged() ) );
    connect( _WhitePlayerLineEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( slotPlayerWhiteNameChanged() ) );
    connect( _BlackPlayerLineEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( slotPlayerBlackNameChanged() ) );
    connect( _SizeSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( slotGobanSizeChanged() ) );
//    connect( komiSpin, SIGNAL( valueChanged() ), this, SLOT( slotKomiChanged() ) );
    connect( _HandicapSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( slotHandicapChanged() ) );
//    connect( _TimeSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( slotTimeChanged() ) );
    connect( _WhiteLevelSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( slotLevelWhiteChanged() ) );
    connect( _BlackLevelSpinBox, SIGNAL( valueChanged(int) ), this, SLOT( slotLevelBlackChanged() ) );
    connect( _OkPushButton, SIGNAL( clicked() ), this, SLOT( slotOk() ) );
    connect( _CancelPushButton, SIGNAL( clicked() ), this, SLOT( slotCancel() ) );
    connect( _oneColorGoCheckbox, SIGNAL( clicked() ), this, SLOT( slotOneColorGoClicked() ) );

}

/*  
 *  Destroys the object and frees any allocated resources
 */
QNewGameDlg::~QNewGameDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

void QNewGameDlg::slotCancel()
{
    QDialog::reject();

}

void QNewGameDlg::slotOk()
{

	if (getPlayerBlackName().simplified().isEmpty())
		_playerBlackName = getPlayerBlackType() == HUMAN ? tr("Human") : tr("Computer");
	if (getPlayerWhiteName().simplified().isEmpty())
		_playerWhiteName = getPlayerWhiteType() == HUMAN ? tr("Human") : tr("Computer");


	if (_handicap == 1)
	{
		QMessageBox msg(tr("Error"),tr("You entered an invalid Handicap (1 is not legal)"), QMessageBox::Warning,
			QMessageBox::Ok | QMessageBox::Default, Qt::NoButton, Qt::NoButton);
		msg.exec();
		QDialog::reject();
	}
	else
		QDialog::accept();
}

void QNewGameDlg::slotGobanSizeChanged()
{
	_size=_SizeSpinBox->text().toInt();
}

void QNewGameDlg::slotHandicapChanged()
{
	_handicap=_HandicapSpinBox->text().toInt();
}

void QNewGameDlg::slotKomiChanged()
{
	_komi=(float)komiSpin->value();
}

void QNewGameDlg::slotLevelBlackChanged()
{
	_levelBlack=_BlackLevelSpinBox->text().toInt();
}

void QNewGameDlg::slotLevelWhiteChanged()
{
	_levelWhite=_WhiteLevelSpinBox->text().toInt();
}

void QNewGameDlg::slotPlayerBlackNameChanged()
{
	_playerBlackName=_BlackPlayerLineEdit->text();
}

void QNewGameDlg::slotPlayerBlackTypeChanged()
{
	_playerBlackType=_BlackTypeComboBox->currentItem();
	if (_BlackTypeComboBox->currentItem()==HUMAN)
		_BlackLevelSpinBox->setDisabled(true);
	else
		_BlackLevelSpinBox->setEnabled(true);
}

void QNewGameDlg::slotPlayerWhiteNameChanged()
{
    _playerWhiteName=_WhitePlayerLineEdit->text();
}

void QNewGameDlg::slotPlayerWhiteTypeChanged()
{
    _playerWhiteType=_WhiteTypeComboBox->currentItem();
	if (_WhiteTypeComboBox->currentItem()==HUMAN)
		_WhiteLevelSpinBox->setDisabled(true);
	else
		_WhiteLevelSpinBox->setEnabled(true);
}

void QNewGameDlg::slotTimeChanged()
{
//    _time=_TimeSpinBox->text().toInt();
}

void QNewGameDlg::slotOneColorGoClicked()
{
	_oneColorGo = _oneColorGoCheckbox->isChecked();
}

int QNewGameDlg::getSize()
{
	return _size;
}

int QNewGameDlg::getHandicap()
{
	return _handicap;
}

bool QNewGameDlg::getOneColorGo()
{
	return _oneColorGo;
}

float QNewGameDlg::getKomi()
{
	return komiSpin->value();//_komi;
}

int QNewGameDlg::getLevelBlack()
{
	return _levelBlack;
}

int QNewGameDlg::getLevelWhite()
{
	return _levelWhite;
}

QString QNewGameDlg::getPlayerBlackName()
{
	return _playerBlackName;
}

QString QNewGameDlg::getPlayerWhiteName()
{
	return _playerWhiteName;
}

int QNewGameDlg::getPlayerBlackType()
{
	return _playerBlackType;
}

int QNewGameDlg::getPlayerWhiteType()
{
	return _playerWhiteType;
}

int QNewGameDlg::getTime()
{
	return _time;
}

void QNewGameDlg::init()
{
	_size = setting->readIntEntry("COMPUTER_SIZE") ;//Gothic::size;
	_komi = 5.5 ;//Gothic::komi;
	_handicap = setting->readIntEntry("COMPUTER_HANDICAP") ;//Gothic::handicap;
	_playerBlackType = (setting->readBoolEntry("COMPUTER_BLACK") ? COMPUTER : HUMAN); //Gothic::blackType;
	_playerWhiteType = (setting->readBoolEntry("COMPUTER_WHITE") ? COMPUTER : HUMAN); //Gothic::whiteType;
	_levelBlack = 10; //Gothic::levelBlack;
	_levelWhite = 10; //Gothic::levelWhite;
	_playerBlackName = "";//Gothic::playerBlackName;
	_playerWhiteName = "";//Gothic::playerWhiteName;
	_time = 0;//Gothic::time;
	_oneColorGo = false;

	_WhiteTypeComboBox->insertItem(tr("Human"));
	_WhiteTypeComboBox->insertItem(tr("Computer"));
	_WhiteTypeComboBox->setCurrentItem(_playerWhiteType);
	_BlackTypeComboBox->insertItem(tr("Human"));
	_BlackTypeComboBox->insertItem(tr("Computer"));
	_BlackTypeComboBox->setCurrentItem(_playerBlackType);
	_SizeSpinBox->setValue(_size);
	_HandicapSpinBox->setValue(_handicap);

	slotPlayerWhiteTypeChanged();
	slotPlayerBlackTypeChanged();
}

void QNewGameDlg::slotGetFileName()
{
  	QString getFileName(QFileDialog::getOpenFileName("",//setting->readEntry("LAST_DIR"),
		tr("SGF Files (*.sgf);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)"), this));
	if (getFileName.isEmpty())
		return;

	_LineEdit_GameToLoad->setText(getFileName);
	fileName = getFileName;
}
