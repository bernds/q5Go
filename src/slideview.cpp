#include <QPixmap>
#include <QCheckBox>
#include <QDialog>

#include "gogame.h"
#include "board.h"
#include "slideview.h"

#include "ui_slideview_gui.h"

void AspectContainer::fix_aspect ()
{
	if (m_child == nullptr)
		return;
	QSize actual = size ();
	double a2 = (double)actual.width () / actual.height ();
	QSize csz;
	if (m_aspect > a2)
		csz = QSize (actual.width (), actual.width () / m_aspect);
	else
		csz = QSize (actual.height () * m_aspect, actual.height ());
	m_child->resize (csz);
	m_child->move ((actual.width () - csz.width ()) / 2,
		       (actual.height () - csz.height ()) / 2);
}


SlideView::SlideView (QWidget *parent)
	: QDialog (parent), ui (new Ui::SlideViewDialog), m_board_exporter (new FigureView)
{
	ui->setupUi (this);

	ui->aspectWidget->set_child (ui->slideView);
	m_board_exporter->hide ();

	m_scene = new QGraphicsScene (0, 0, 30, 30, this);
	ui->slideView->setScene (m_scene);

	update_prefs ();

	ui->slideXEdit->setValidator (new QIntValidator (100, 9999, this));
	ui->slideYEdit->setValidator (new QIntValidator (100, 9999, this));

	connect (ui->slideXEdit, &QLineEdit::textChanged, [this] () { inputs_changed (); });
	connect (ui->slideYEdit, &QLineEdit::textChanged, [this] () { inputs_changed (); });
	void (QSpinBox::*changed) (int) = &QSpinBox::valueChanged;
	connect (ui->slideLinesSpinBox, changed, [this] (int) { inputs_changed (); });
	connect (ui->slideMarginSpinBox, changed, [this] (int) { inputs_changed (); });
	connect (ui->slideWBCheckBox, &QCheckBox::toggled, [this] (bool) { inputs_changed (); });
	connect (ui->slideItalicCheckBox, &QCheckBox::toggled, [this] (bool) { inputs_changed (); });
	connect (ui->slideBoldCheckBox, &QCheckBox::toggled, [this] (bool) { inputs_changed (); });
	connect (ui->slideCoordsCheckBox, &QCheckBox::toggled, [this] (bool) { inputs_changed (); });

	connect (ui->toClipButton, &QPushButton::clicked, [this] ()
		 {
			 QPixmap pm = render_export ();
			 QApplication::clipboard()->setPixmap (pm);
		 });
	connect (ui->fileOpenButton, &QToolButton::clicked, this, &SlideView::choose_file);
	connect (ui->saveButton, &QPushButton::clicked, [this] () { save (); });
	connect (ui->saveAllButton, &QPushButton::clicked, [this] () { save_all (false); });
	connect (ui->saveAllMainButton, &QPushButton::clicked, [this] () { save_mainline (false); });
	connect (ui->saveComButton, &QPushButton::clicked, [this] () { save_all (true); });
	connect (ui->saveComMainButton, &QPushButton::clicked, [this] () { save_mainline (true); });
	connect (ui->saveAsButton, &QPushButton::clicked, this, &SlideView::save_as);
	connect (ui->slideView, &SizeGraphicsView::resized, this, &SlideView::view_resized);
	show ();
}

SlideView::~SlideView ()
{
	delete m_scene;
	delete m_board_exporter;
	delete ui;
}

QPixmap SlideView::render_text (int w, int h)
{
	int margin = ui->slideMarginSpinBox->value ();
	int margin_px = w * margin / 100.;
	int n_lines = ui->slideLinesSpinBox->value ();
	QFontMetrics fm (m_font);
	int fh = fm.height ();
	double fh_chosen = (double)(h - 2 * margin_px) / n_lines;
	int wrap_width = fh * n_lines * (m_aspect - 1);
	double f_factor = (double)fh_chosen / fh;
	int new_ps = m_font.pointSize () * f_factor;
	QFont title_font = m_font;
	if (ui->slideItalicCheckBox->isChecked ())
		title_font.setItalic (true);
	if (ui->slideBoldCheckBox->isChecked ())
		title_font.setBold (true);
	QFont f1 = m_font;
	f1.setPointSize (new_ps);
	QFont f2 = title_font;
	f2.setPointSize (new_ps);

	QPixmap pm (w, h);
	pm.fill (ui->slideWBCheckBox->isChecked () ? Qt::black : Qt::white);
	QPainter painter;
	painter.begin (&pm);
	painter.setPen (ui->slideWBCheckBox->isChecked () ? Qt::white : Qt::black);
	int lineno = 0;
	QString comments = QString::fromStdString (m_board_exporter->displayed ()->comment ());
	QStringList paras = comments.split ("\n");
	for (auto &p: paras) {
		QStringList words = p.split (" ");
		QString line;
		QFont &this_font = lineno == 0 ? f2 : f1;
		painter.setFont (this_font);
		QFontMetrics fm_real (lineno == 0 ? title_font : m_font);
		for (auto &w: words) {
			if (lineno == n_lines)
				break;
			if (line.isEmpty ())
				line = w;
			else {
				QString trial = line + " " + w;
				QRect brect = fm_real.boundingRect (trial);
				brect = fm_real.boundingRect (brect, Qt::AlignLeft | Qt::AlignTop, trial);
				if (brect.width () > wrap_width) {
					painter.drawStaticText (margin_px, margin_px + lineno * fh_chosen,
								QStaticText (line));
					line.clear ();
					lineno++;
					if (lineno == n_lines)
						break;
					line = w;
				} else
					line = trial;
			}
		}
		if (!line.isEmpty ()) {
			painter.drawStaticText (margin_px, margin_px + lineno * fh_chosen,
						QStaticText (line));
			lineno++;
			if (lineno == n_lines)
				break;
		}
	}
	painter.end ();
	return pm;
}

void SlideView::update_text_font ()
{
	if (m_game == nullptr)
		return;
	int w = m_displayed_sz.width ();
	int h = m_displayed_sz.height ();

	QPixmap pm = render_text (w - h, h);
	delete m_text_item;
	m_text_item = m_scene->addPixmap (pm);
	m_text_item->setPos (h, 0);
}

QPixmap SlideView::render_export ()
{
	int w = ui->slideXEdit->text ().toInt ();
	int h = ui->slideYEdit->text ().toInt ();

	QPixmap pm (w, h);
	pm.fill (Qt::transparent);
	m_board_exporter->set_show_coords (ui->slideCoordsCheckBox->isChecked ());
	m_board_exporter->set_margin (0);
	m_board_exporter->resizeBoard (h, h);
	QPixmap board_pm = m_board_exporter->draw_position (0);
	QPixmap text_pm = render_text (w - h, h);
	QPainter p;
	p.begin (&pm);
	p.drawImage (QPoint (0, 0), m_board_exporter->background_image (), m_board_exporter->wood_rect ());
	p.drawPixmap (0, 0, board_pm);
	p.drawPixmap (h, 0, text_pm);
	p.end ();
	return pm;
}

void SlideView::redraw ()
{
	if (m_game == nullptr)
		return;
	update_text_font ();
	int h = m_displayed_sz.height ();
	m_board_exporter->set_show_coords (ui->slideCoordsCheckBox->isChecked ());
	m_board_exporter->set_margin (0);
	m_board_exporter->resizeBoard (h, h);
	QPixmap board_pm = m_board_exporter->draw_position (0);
	delete m_board_item;
	m_board_item = m_scene->addPixmap (board_pm);
	delete m_bg_item;
	QImage wood = m_board_exporter->background_image ().copy (m_board_exporter->wood_rect ());
	m_bg_item = m_scene->addPixmap (QPixmap::fromImage (wood));
	m_bg_item->setZValue (-1);
}

void SlideView::view_resized ()
{
	m_displayed_sz = ui->slideView->size ();
	redraw ();
}

void SlideView::update_prefs ()
{
	ui->slideLinesSpinBox->setValue (setting->readIntEntry( "SLIDE_LINES"));
	ui->slideMarginSpinBox->setValue (setting->readIntEntry( "SLIDE_MARGIN"));
	ui->slideXEdit->setText (QString::number (setting->readIntEntry ("SLIDE_X")));
	ui->slideYEdit->setText (QString::number (setting->readIntEntry ("SLIDE_Y")));
	ui->slideItalicCheckBox->setChecked (setting->readBoolEntry ("SLIDE_ITALIC"));
	ui->slideBoldCheckBox->setChecked (setting->readBoolEntry ("SLIDE_BOLD"));
	ui->slideCoordsCheckBox->setChecked (setting->readBoolEntry ("SLIDE_COORDS"));
	ui->slideWBCheckBox->setChecked (setting->readBoolEntry ("SLIDE_WB"));

	m_font = setting->fontComments;

	inputs_changed ();
}

void SlideView::inputs_changed ()
{
	int w = ui->slideXEdit->text ().toInt ();
	int h = ui->slideYEdit->text ().toInt ();

	m_aspect = w * 1.0 / std::max (1, h);
	ui->aspectWidget->set_aspect (m_aspect);

	redraw ();
}

void SlideView::set_game (go_game_ptr gr)
{
	m_game = gr;
	m_board_exporter->reset_game (gr);
	redraw ();
}

void SlideView::set_active (game_state *st)
{
	m_board_exporter->set_displayed (st);
	redraw ();
}

void SlideView::save_as ()
{
	QString filename = QFileDialog::getSaveFileName (this, tr ("Export slide as"),
							 setting->readEntry ("LAST_DIR"),
							 tr("Images (*.png *.xpm *.jpg);;All Files (*)"));
	if (filename.isEmpty ())
		return;
	QPixmap pm = render_export ();
	if (!pm.save (filename)) {
		QMessageBox::warning (this, tr ("Error while saving"),
				      tr ("An error occurred while saving. The file could not be saved.\n"));
		return;
	}
}

bool SlideView::save ()
{
	QString pattern = ui->fileTemplateEdit->text ();
	if (pattern.isEmpty () || !pattern.contains ("%n")) {
		QMessageBox::warning (this, tr ("Filename pattern not set"),
				      tr ("Please enter a filename pattern which includes \"%n\" where the number should be substituted."));
		return false;
	}
	int v = ui->fileNrSpinBox->value ();
	QString v_str = QString::number (v);
	while (v_str.length () < 4)
		v_str = "0" + v_str;
	QString filename = pattern.replace (QRegExp ("%n"), v_str);
	QFile f (filename);
	if (f.exists () && !ui->overwriteCheckBox->isChecked ()) {
		QMessageBox::StandardButton choice;
		choice = QMessageBox::warning(this, tr ("File exists"),
					      tr ("A filename matching the pattern and current number already exists.  Overwrite?"),
					      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
		if (choice == QMessageBox::No)
			return false;
	}
	QPixmap pm = render_export ();
	if (!pm.save (filename)) {
		QMessageBox::warning (this, tr ("Error while saving"),
				      tr ("The file could not be saved.\nPlease verify the filename pattern is correct."));
		return false;
	}
	ui->fileNrSpinBox->setValue (v + 1);
	return true;
}

void SlideView::save_with_progress (std::vector<game_state *> &collection)
{
	QProgressDialog dlg ("Exporting diagrams...", "Abort operation", 0, collection.size (), this);
	dlg.setWindowModality(Qt::WindowModal);
	int i = 0;
	game_state *old = m_board_exporter->displayed ();
	for (auto st: collection) {
		dlg.setValue (i++);
		if (dlg.wasCanceled ())
			break;

		m_board_exporter->set_displayed (st);
		if (!save ())
			break;
	}
	m_board_exporter->set_displayed (old);
	dlg.setValue (i);
}

void SlideView::save_all (bool only_comments)
{
	game_state *r = m_game->get_root ();
	std::vector<game_state *> collection;
	std::function<bool (game_state *)> fn
		= [only_comments, &collection] (game_state *st) -> bool
		{
			if (!only_comments || st->comment ().length () > 0)
				collection.push_back (st);
			return true;
		};
	r->walk_tree (fn);
	save_with_progress (collection);
}

void SlideView::save_mainline (bool only_comments)
{
	game_state *st = m_game->get_root ();
	std::vector<game_state *> collection;
	while (st != nullptr) {
		if (!only_comments || st->comment ().length () > 0)
			collection.push_back (st);
		st = st->next_primary_move ();
	}
	save_with_progress (collection);
}

void SlideView::choose_file ()
{
	QString filename = QFileDialog::getOpenFileName (this, tr ("Choose file name to serve as template for slides"),
							 setting->readEntry ("LAST_DIR"),
							 tr("Images (*.png *.xpm *.jpg);;All Files (*)"));
	if (filename.isEmpty ())
		return;

	ui->fileTemplateEdit->setText (filename);
}
