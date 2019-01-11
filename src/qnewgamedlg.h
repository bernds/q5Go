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
#include "ui_qnewgamedlg_gui.h"

#include "goboard.h"

class Engine;

class QNewGameDlg : public QDialog, public Ui::QNewGameDlgGui
{
	Q_OBJECT

public:
	QNewGameDlg (QWidget* parent, const QList<Engine *> engines);

	game_info create_game_info ();
	int board_size ();
	int handicap ();
	int engine_index ();
	QString fileName ();
	bool computer_white_p ();

public slots:
	void slotGetFileName ();
	void slotCancel ();
	void slotOk ();
};

#endif
