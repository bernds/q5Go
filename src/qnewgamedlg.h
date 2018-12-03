/***************************************************************************
                          qnewgamedlg.h  -  description
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

#ifndef QNEWGAMEDLG_H
#define QNEWGAMEDLG_H

#include <qdialog.h>
#include "qnewgamedlg_gui.h"
//#include "global.h"
//#include "gothic.h"


class QNewGameDlg : public QDialog, public Ui::QNewGameDlgGui
{
    Q_OBJECT

public:
//    QNewGameDlg( QWidget* parent = 0, const char* name = 0, bool modal = false, WFlags fl = 0 );
	QNewGameDlg( QWidget* parent = 0, const char* name = 0);
	~QNewGameDlg();

	int getSize();
	int getHandicap();
	float getKomi();
	int getLevelBlack();
	int getLevelWhite();
	QString getPlayerBlackName();
	QString getPlayerWhiteName();
	int getPlayerBlackType();
	int getPlayerWhiteType();
	int getTime();
	bool getOneColorGo();
	QString fileName;

public slots:
	void slotCancel();
	void slotOk();
  

protected:
/*
    QFrame* _NewGameWhite;
    QLabel* _WhiteLevelLabel;
    QLabel* _WhiteTypeLabel;
    QLabel* _WhiteNewGameLabel;
    QLabel* _WhitePlayerLabel;
    QLineEdit* _WhitePlayerLineEdit;
    QSpinBox* _WhiteLevelSpinBox;
    QComboBox* _WhiteTypeComboBox;
    QFrame* _Black;
    QLineEdit* _BlackPlayerLineEdit;
    QLabel* _BlackPlayerLabel;
    QLabel* _BlackLevelLabel;
    QSpinBox* _BlackLevelSpinBox;
    QLabel* _BlackTypeLabel;
    QLabel* _BlackNewGameLabel;
    QComboBox* _BlackTypeComboBox;
    QPushButton* _CancelPushButton;
    QPushButton* _OkPushButton;
    QGroupBox* _ParametersGroupBox;
//    QSpinBox* _KomiSpinBox;
    QLineEdit* _KomiLineEdit;
    QLabel* _TimeLabel;
    QLabel* _SizeLabel;
    QLabel* _HandicapLabel;
    QSpinBox* _HandicapSpinBox;
    QLabel* _KomiLabel;
    QSpinBox* _SizeSpinBox;
    QSpinBox* _TimeSpinBox;

	void initDialog();
*/
  	void init();

    bool _oneColorGo;
    int _size, _handicap, _levelBlack, _levelWhite,_time;
    int _playerWhiteType, _playerBlackType;
	float _komi;
	QString _playerWhiteName, _playerBlackName;

protected slots:
	void slotGobanSizeChanged();
    void slotHandicapChanged();
    void slotKomiChanged();
    void slotLevelBlackChanged();
    void slotLevelWhiteChanged();
    void slotPlayerBlackNameChanged();
    void slotPlayerBlackTypeChanged();
    void slotPlayerWhiteNameChanged();
    void slotPlayerWhiteTypeChanged();
    void slotTimeChanged();
    void slotGetFileName();
    void slotOneColorGoClicked();
};

/** Define player types   /
typedef enum {
	HUMAN,
 COMPUTER
} player_type;            */

#endif
