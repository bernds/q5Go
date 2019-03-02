/*
 * miscdialogs.h
 */

#ifndef MISCDIALOGS_H
#define MISCDIALOGS_H

#include <QDialogButtonBox>

#include "ui_gameinfo_gui.h"
class GameInfoDialog : public QDialog, public Ui::GameInfoDialog
{
	Q_OBJECT

public:
	GameInfoDialog( QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
		setModal (true);

		connect (buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect (buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}
};

#include "ui_newgame_gui.h"

class NewGameDialog : public QDialog, public Ui::NewGameDialog
{
	Q_OBJECT

public:
	NewGameDialog( QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
		setModal (true);
	}
};

#include "ui_newlocalgame_gui.h"

class NewLocalGameDialog : public QDialog, public Ui::NewLocalGameDialog
{
	Q_OBJECT

public:
	NewLocalGameDialog( QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
		setModal (true);
		connect (buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect (buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	}
};

#include "ui_nthmove_gui.h"

class NthMoveDialog : public QDialog, public Ui::NthMoveDialog
{
	Q_OBJECT

public:
	NthMoveDialog( QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
		setModal (true);
	}
};

#include "ui_textedit_gui.h"

class TextEditDialog : public QDialog, public Ui::TextEditDialog
{
	Q_OBJECT

public:
	TextEditDialog( QWidget* parent = 0)
		: QDialog (parent)
	{
		setupUi (this);
		setModal (true);
	}
};

#endif // MISCDIALOGS_H
