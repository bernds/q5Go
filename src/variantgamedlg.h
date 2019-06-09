#ifndef VARIANTGAMEDLG_H
#define VARIANTGAMEDLG_H

#include <memory>
#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
	class NewVariantGameDialog;
};

class game_state;
class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;

class Grid;

class NewVariantGameDialog : public QDialog
{
	Q_OBJECT

	Ui::NewVariantGameDialog *ui;
	QGraphicsScene m_canvas;

	Grid *m_grid {};
	void update_grid ();
	void resize_grid ();

public:
	NewVariantGameDialog (QWidget* parent = 0);
	~NewVariantGameDialog ();

	go_game_ptr create_game_record ();
};

#endif
