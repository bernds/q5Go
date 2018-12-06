/*
 * debugdialog.h
 */

#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H
#include "gui_dialog.h"

class Debug_Dialog : public QDialog, public Ui::Debug_Dialog
{
	Q_OBJECT

public:
	Debug_Dialog( QWidget* parent = 0, const char* name = 0, bool modal = false)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}
};

#include "gameinfo_gui.h"
class GameInfoDialog : public QDialog, public Ui::GameInfoDialog
{
	Q_OBJECT

public:
	GameInfoDialog( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}
};

#include "newgame_gui.h"

class NewGameDialog : public QDialog, public Ui::NewGameDialog
{
	Q_OBJECT

public:
	NewGameDialog( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}
};

#include "newlocalgame_gui.h"

class NewLocalGameDialog : public QDialog, public Ui::NewLocalGameDialog
{
	Q_OBJECT

public:
	NewLocalGameDialog( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}
};

#include "qnewgamedlg_gui.h"

class QNewGameDlgGui : public QDialog, public Ui::QNewGameDlgGui
{
	Q_OBJECT

public:
	QNewGameDlgGui( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}

};

#include "nthmove_gui.h"

class NthMoveDialog : public QDialog, public Ui::NthMoveDialog
{
	Q_OBJECT

public:
	NthMoveDialog( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
		setModal (modal);
	}
};

#include "textedit_gui.h"

class TextEditDialog : public QDialog, public Ui::TextEditDialog
{
	Q_OBJECT

public:
	TextEditDialog( QWidget* parent = 0, const char* name = 0, bool modal = true)
		: QDialog (parent)
	{
		setupUi(this);
	}
};

#endif // DEBUGDIALOG_H
