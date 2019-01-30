#ifndef VARIANTGAMEDLG_H
#define VARIANTGAMEDLG_H

#include "ui_newvariantgame_gui.h"

class Grid;

class NewVariantGameDialog : public QDialog, public Ui::NewVariantGameDialog
{
	Q_OBJECT

	QGraphicsScene m_canvas;

	Grid *m_grid {};
	void update_grid ();
	void resize_grid ();

public:
	NewVariantGameDialog (QWidget* parent = 0);
	~NewVariantGameDialog ();
};

#endif
