#ifndef FIGUREDLG_H
#define FIGUREDLG_H

#include "common.h"
#include "ui_figuredlg_gui.h"

class MainWindow;

class FigureDialog : public QDialog, public Ui::FigureDialog
{
	Q_OBJECT

	MainWindow *m_win;
	game_state *m_state;
	QIntValidator *m_intval;
	void apply_changes ();
public:
	FigureDialog (game_state *st, MainWindow* parent = 0);
	~FigureDialog ();
	virtual void accept () override;
};

#endif
