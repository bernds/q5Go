#include "goboard.h"
#include "sizegraphicsview.h"
#include "variantgamedlg.h"
#include "grid.h"

NewVariantGameDialog::NewVariantGameDialog (QWidget* parent)
	: QDialog (parent)
{
	setupUi (this);
	setModal (true);

	connect (buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect (buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	void (QSpinBox::*changed) (int) = &QSpinBox::valueChanged;
	connect (xSizeSpin, changed, [=] (int) { update_grid (); });
	connect (ySizeSpin, changed, [=] (int) { update_grid (); });
	connect (hTorusCheckBox, &QCheckBox::stateChanged, [=] (int) { update_grid (); });
	connect (vTorusCheckBox, &QCheckBox::stateChanged, [=] (int) { update_grid (); });
	connect (graphicsView, &SizeGraphicsView::resized, [=] () { resize_grid (); });
	int h = graphicsView->height ();
	graphicsView->resize (h, h);
	m_canvas.setSceneRect (0, 0, h, h);
	graphicsView->setScene (&m_canvas);
	update_grid ();
}

NewVariantGameDialog::~NewVariantGameDialog ()
{
	delete m_grid;
}

void NewVariantGameDialog::resize_grid ()
{
	if (m_grid == nullptr)
		return;
	double w = graphicsView->width ();
	double h = graphicsView->height ();
	int sz_x = xSizeSpin->value();
	int sz_y = ySizeSpin->value();
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
	int sz_x = xSizeSpin->value();
	int sz_y = ySizeSpin->value();
	bool torus_h = hTorusCheckBox->isChecked ();
	bool torus_v = vTorusCheckBox->isChecked ();

	delete m_grid;
	m_grid = nullptr;

	if (sz_x < 3 || sz_y < 3 || sz_x > 25 || sz_y > 25)
		return;
	go_board b (sz_x, sz_y, torus_h, torus_v);
	bit_array no_hoshis (b.bitsize ());
	m_grid = new Grid (&m_canvas, b, 0, 0, no_hoshis);
	resize_grid ();
}
