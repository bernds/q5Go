#include "goboard.h"
#include "gogame.h"
#include "sizegraphicsview.h"
#include "variantgamedlg.h"
#include "grid.h"

#include "ui_newvariantgame_gui.h"

NewVariantGameDialog::NewVariantGameDialog (QWidget* parent)
	: QDialog (parent), ui (new Ui::NewVariantGameDialog)
{
	ui->setupUi (this);
	setModal (true);

	connect (ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect (ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	void (QSpinBox::*changed) (int) = &QSpinBox::valueChanged;
	connect (ui->xSizeSpin, changed, [=] (int) { update_grid (); });
	connect (ui->ySizeSpin, changed, [=] (int) { update_grid (); });
	connect (ui->hTorusCheckBox, &QCheckBox::stateChanged, [=] (int) { update_grid (); });
	connect (ui->vTorusCheckBox, &QCheckBox::stateChanged, [=] (int) { update_grid (); });
	connect (ui->graphicsView, &SizeGraphicsView::resized, [=] () { resize_grid (); });
	int h = ui->graphicsView->height ();
	ui->graphicsView->resize (h, h);
	m_canvas.setSceneRect (0, 0, h, h);
	ui->graphicsView->setScene (&m_canvas);
	update_grid ();
}

NewVariantGameDialog::~NewVariantGameDialog ()
{
	delete m_grid;
	delete ui;
}

void NewVariantGameDialog::resize_grid ()
{
	if (m_grid == nullptr)
		return;
	double w = ui->graphicsView->width ();
	double h = ui->graphicsView->height ();
	int sz_x = ui->xSizeSpin->value();
	int sz_y = ui->ySizeSpin->value();
	double sqx = w * 0.95 / sz_x;
	double sqy = h * 0.95 / sz_y;
	double len = std::min (sqx, sqy);
	int offx = (w - (sz_x - 1) * len) / 2;
	int offy = (h - (sz_y - 1) * len) / 2;
	QRect r (offx, offy, sz_x * len, sz_y * len);
	m_grid->resize (r, 0, 0, len);
}

void NewVariantGameDialog::update_grid ()
{
	int sz_x = ui->xSizeSpin->value();
	int sz_y = ui->ySizeSpin->value();
	bool torus_h = ui->hTorusCheckBox->isChecked ();
	bool torus_v = ui->vTorusCheckBox->isChecked ();

	delete m_grid;
	m_grid = nullptr;

	if (sz_x < 3 || sz_y < 3 || sz_x > 25 || sz_y > 25)
		return;
	go_board b (sz_x, sz_y, torus_h, torus_v);
	bit_array no_hoshis (b.bitsize ());
	m_grid = new Grid (&m_canvas, b, 0, 0, no_hoshis);
	resize_grid ();
}

go_game_ptr NewVariantGameDialog::create_game_record ()
{
	int sz_x = ui->xSizeSpin->value();
	int sz_y = ui->ySizeSpin->value();
	bool torus_h = ui->hTorusCheckBox->isChecked ();
	bool torus_v = ui->vTorusCheckBox->isChecked ();
	go_board starting_pos (sz_x, sz_y, torus_h, torus_v);
	game_info info ("",
			ui->playerWhiteEdit->text().toStdString (),
			ui->playerBlackEdit->text().toStdString (),
			ui->playerWhiteRkEdit->text().toStdString (),
			ui->playerBlackRkEdit->text().toStdString (),
			"", ui->komiSpin->value(), 0,
			ranked::free,
			"", "", "", "", "", "", "", "", -1);
	go_game_ptr gr = std::make_shared<game_record> (starting_pos, black, info);
	return gr;
}
