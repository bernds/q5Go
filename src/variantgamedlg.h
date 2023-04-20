#ifndef VARIANTGAMEDLG_H
#define VARIANTGAMEDLG_H

#include "common.h"
#include <QGraphicsScene>

namespace Ui {
	class NewVariantGameDialog;
};


#include "bitarray.h"

class Grid;

class NewVariantGameDialog : public QDialog
{
	Q_OBJECT

	Ui::NewVariantGameDialog *ui;
	QGraphicsScene m_canvas;

	Grid *m_grid {};
	QRect m_last_sel_rect;
	bit_array m_mask;

	void update_grid ();
	void resize_grid ();

	void update_selection (const QRect &);
	void clear_selection ();
	void add_selection ();
	void rem_selection ();
	void reset_grid ();
public:
	NewVariantGameDialog (QWidget* parent = 0);
	~NewVariantGameDialog ();

	go_game_ptr create_game_record ();
};

#endif
