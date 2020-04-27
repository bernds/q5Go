#include <QButtonGroup>
#include <QProgressDialog>

#include "gogame.h"
#include "edit_analysis.h"
#include "mainwindow.h"

#include "ui_edit_analysis_gui.h"

EditAnalysisDialog::EditAnalysisDialog (MainWindow *parent, go_game_ptr gr, an_id_model *model)
	: ui (new Ui::EditAnalysisDialog), m_parent (parent), m_game (gr), m_model (model)
{
	ui->setupUi (this);

	ui->anIdListView->setModel (m_model);
	connect (ui->anIdListView, &ClickableListView::current_changed, this, &EditAnalysisDialog::update_buttons);
	connect (ui->delAllButton, &QPushButton::clicked, [this] (bool) { do_delete (true); });
	connect (ui->delDiagsButton, &QPushButton::clicked, [this] (bool) { do_delete (false); });
	update_buttons ();

	connect (ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

EditAnalysisDialog::~EditAnalysisDialog ()
{
	delete ui;
}

void EditAnalysisDialog::update_buttons (void)
{
	QModelIndex idx = ui->anIdListView->currentIndex ();
	bool valid = idx.isValid ();
	ui->delDiagsButton->setEnabled (valid);
	ui->delAllButton->setEnabled (valid);
	if (!valid) {
		ui->countLabel->setText (tr ("No analysis selected."));
		return;
	}
	int idnr = idx.row ();
	size_t count = 0;
	size_t shared = 0;
	auto e = m_model->entries ()[idnr];
	auto id = e.first;

	game_state *root = m_game->get_root ();
	root->walk_tree ([&count, &shared, &id] (game_state *st)
			 {
				 eval ev;
				 if (!st->has_figure ())
					 return true;
				 if (st->find_eval (id, ev)) {
					 count++;
					 if (st->eval_count () > 1)
						 shared++;
				 }
				 return true;
			 });
	QString txt = tr ("%1 diagrams found.").arg (count);
	if (shared > 0)
		txt += tr (" Warning: %1 of these have other analysis.").arg (shared);
	ui->countLabel->setText (txt);
}

void EditAnalysisDialog::do_delete (bool all)
{
	QModelIndex idx = ui->anIdListView->currentIndex ();
	bool valid = idx.isValid ();
	bool keep_multi = ui->keepMultiCheckBox->isChecked ();

	// Defensive, shouldn't happen.
	if (!valid)
		return;

	int idnr = idx.row ();
	auto e = m_model->entries ()[idnr];
	auto id = e.first;

	game_state *root = m_game->get_root ();
	std::vector<game_state *> to_delete;
	root->walk_tree ([&id, all, keep_multi, &to_delete] (game_state *st)
			 {
				 eval ev;
				 bool recurse = true;
				 if (st->has_figure () && st->find_eval (id, ev)) {
					 if (!keep_multi || st->eval_count () == 1) {
						 recurse = false;
						 to_delete.push_back (st);
					 }
				 }
				 if (all)
					 st->remove_eval (id);
				 return recurse;
			 });
	if (all)
		m_model->removeRow (idnr);
	m_parent->remove_nodes (to_delete);
	update_buttons ();
}
