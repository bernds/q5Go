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

#include <utility>

#include "goboard.h"
#include "gogame.h"
#include "timing.h"

struct Engine;

namespace Ui {
	class NewAIGameDlgGui;
	class TwoAIGameDlgGui;
};

template<class UI>
class AIGameDlg : public QDialog
{
	void get_file_name ();

protected:
	UI *ui;

public:
	AIGameDlg (QWidget *parent);
	~AIGameDlg ();
	std::pair<int, int> board_size ();
	int handicap ();
	QString game_to_load ();
	time_settings timing ();
	go_game_ptr create_game_record ();
	virtual game_info create_game_info () = 0;

};

class NewAIGameDlg : public AIGameDlg<Ui::NewAIGameDlgGui>
{
public:
	NewAIGameDlg (QWidget* parent, bool from_position = false);

	virtual game_info create_game_info () override;
	int engine_index ();
	QString fileName ();
	bool computer_white_p ();

public slots:
	void slotCancel ();
	void slotOk ();
};

class TwoAIGameDlg : public AIGameDlg<Ui::TwoAIGameDlgGui>
{
public:
	TwoAIGameDlg (QWidget* parent);

	virtual game_info create_game_info () override;
	int engine_index (stone_color);
	QString fileName ();
	int num_games ();
	bool opening_book ();

	virtual void accept () override;
public slots:
	void slotCancel ();
	void slotOk ();
};

#endif
