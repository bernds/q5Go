/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NEWAIGAMEDLG_H
#define NEWAIGAMEDLG_H

#include <qdialog.h>
#include "ui_newaigamedlg_gui.h"

#include "goboard.h"
#include "gogame.h"

struct Engine;

class NewAIGameDlg : public QDialog, public Ui::NewAIGameDlgGui
{
	Q_OBJECT

public:
	NewAIGameDlg (QWidget* parent, const std::vector<Engine> &);

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
