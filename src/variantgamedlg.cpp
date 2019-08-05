#include "goboard.h"
#include "gogame.h"
#include "sizegraphicsview.h"
#include "variantgamedlg.h"
#include "grid.h"

#include "ui_newvariantgame_gui.h"

NewVariantGameDialog::NewVariantGameDialog (QWidget* parent)
	: QDialog (parent), ui (new Ui::NewVariantGameDialog), m_mask (81)
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
	connect (ui->graphicsView, &QGraphicsView::rubberBandChanged,
		 [this] (QRect r, QPointF, QPointF) { update_selection (r); });
	connect (ui->addSelButton, &QPushButton::clicked, [this] (bool) { add_selection (); });
	connect (ui->removeSelButton, &QPushButton::clicked, [this] (bool) { rem_selection (); });
	connect (ui->clearSelButton, &QPushButton::clicked, [this] (bool) { clear_selection (); });
	connect (ui->resetButton, &QPushButton::clicked, [this] (bool) { reset_grid (); });
	int h = ui->graphicsView->height ();
	ui->graphicsView->resize (h, h);
	m_canvas.setSceneRect (0, 0, h, h);
	ui->graphicsView->setScene (&m_canvas);
	ui->graphicsView->setDragMode (QGraphicsView::RubberBandDrag);
	ui->graphicsView->setRubberBandSelectionMode (Qt::IntersectsItemBoundingRect);
	ui->addSelButton->setEnabled (false);
	ui->removeSelButton->setEnabled (false);
	ui->clearSelButton->setEnabled (false);
	update_grid ();
}

NewVariantGameDialog::~NewVariantGameDialog ()
{
	delete m_grid;
	delete ui;
}

void NewVariantGameDialog::add_selection ()
{
	bit_array s = m_grid->selected_items ();
	m_mask.andnot (s);
	m_grid->set_removed_points (m_mask);
}

void NewVariantGameDialog::rem_selection ()
{
	bit_array s = m_grid->selected_items ();
	m_mask.ior (s);
	m_grid->set_removed_points (m_mask);
}

void NewVariantGameDialog::reset_grid ()
{
	m_mask.clear ();
	m_grid->set_removed_points (m_mask);
}

void NewVariantGameDialog::update_selection (const QRect &rect)
{
	if (m_grid == nullptr)
		return;

	if (rect.isNull ()) {
		bool any_selected = m_grid->apply_selection (m_last_sel_rect);
		m_canvas.update ();
		ui->addSelButton->setEnabled (any_selected);
		ui->removeSelButton->setEnabled (any_selected);
		ui->clearSelButton->setEnabled (any_selected);
	} else
		m_last_sel_rect = rect;
}

void NewVariantGameDialog::clear_selection ()
{
	m_grid->apply_selection (QRect ());
	ui->addSelButton->setEnabled (false);
	ui->removeSelButton->setEnabled (false);
	ui->clearSelButton->setEnabled (false);
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
	m_mask = bit_array (b.bitsize ());

	bit_array no_hoshis (b.bitsize ());
	m_grid = new Grid (&m_canvas, b, no_hoshis);
	resize_grid ();
}

go_game_ptr NewVariantGameDialog::create_game_record ()
{
	int sz_x = ui->xSizeSpin->value();
	int sz_y = ui->ySizeSpin->value();
	bool torus_h = ui->hTorusCheckBox->isChecked ();
	bool torus_v = ui->vTorusCheckBox->isChecked ();
	go_board starting_pos (sz_x, sz_y, torus_h, torus_v);
	game_info info;
	info.name_w = ui->playerWhiteEdit->text().toStdString ();
	info.name_b = ui->playerBlackEdit->text().toStdString ();
	info.rank_w = ui->playerWhiteRkEdit->text().toStdString ();
	info.rank_b = ui->playerBlackRkEdit->text().toStdString ();
	info.komi = ui->komiSpin->value();

	if (m_mask.popcnt () == 0)
		return std::make_shared<game_record> (starting_pos, black, info);
	else {
		auto m = std::make_shared<const bit_array> (m_mask);
		starting_pos.set_mask (m);
		return std::make_shared<game_record> (starting_pos, black, info, m);
	}
}
