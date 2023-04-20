#ifndef AUTODIAGSDLG_H
#define AUTODIAGSDLG_H

#include "common.h"
#include "ui_autodiags_gui.h"

class MainWindow;
class QIntValidator;

class AutoDiagsDialog : public QDialog, public Ui::AutoDiagsDialog
{
	Q_OBJECT

	std::shared_ptr<game_record> m_game;
	QIntValidator *m_intval;
public:
	AutoDiagsDialog (go_game_ptr, MainWindow* parent = 0);
	~AutoDiagsDialog ();
	virtual void accept () override;
};

#endif
