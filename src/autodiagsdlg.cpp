#include <QIntValidator>

#include "gogame.h"
#include "mainwindow.h"
#include "autodiagsdlg.h"

AutoDiagsDialog::AutoDiagsDialog (std::shared_ptr<game_record> gr, MainWindow *parent)
	: QDialog (parent), m_game (gr)
{
	setupUi (this);
	setModal (true);

	m_intval = new QIntValidator (0, 1000, this);
	maxMovesEdit->setValidator (m_intval);
}

AutoDiagsDialog::~AutoDiagsDialog ()
{
	delete m_intval;
}

void AutoDiagsDialog::accept ()
{
	int flags = 0;
	flags |= coordsCheckBox->isChecked () ? 0 : 1;
	flags |= titleCheckBox->isChecked () ? 0 : 2;
#if 0 /* Since we don't actually implement this... */
	flags |= koCheckBox->isChecked () ? 0 : 4;
#endif
	flags |= remStoneCheckBox->isChecked () ? 0 : 256;
	flags |= hoshiCheckBox->isChecked () ? 0 : 512;

	int fig_count = 0;
	int diag_count = 1;
	game_state *r = m_game->get_root ();
	game_state *st;
	bit_array conflicts (r->get_board ().bitsize ());
	int move_count = 0;
	int max_moves = maxMovesEdit->text ().toInt ();
	game_state *last_figure = nullptr;
	int cur_move_number = 0;
	for (st = r; st; st = st->next_primary_move ()) {
		const go_board &b = st->get_board ();
		bool make_diag = st->root_node_p () || st->was_edit_p ();
		if (move_count >= max_moves)
			make_diag = true;
		else if (st->was_move_p ()) {
			int bp = b.bitpos (st->get_move_x (), st->get_move_y ());
			if (conflicts.test_bit (bp))
				make_diag = true;
			else
				conflicts.set_bit (bp);
		}
		if (make_diag) {
			if (last_figure != nullptr) {
				last_figure->set_figure (flags, tr ("Figure %1 (%2-%3)").arg (fig_count)
							 .arg (last_figure->move_number ()
							       + (last_figure->was_move_p () ? 0 : 1))
							 .arg (cur_move_number).toStdString ());
			}
			last_figure = st;
			fig_count++;
			conflicts.clear ();
			conflicts.ior (b.get_stones_w ());
			conflicts.ior (b.get_stones_b ());
			move_count = 0;
		} else {
			move_count++;
			if (clearOldCheckBox->isChecked ())
				st->clear_figure ();
		}
		if (st->was_move_p ())
			cur_move_number = st->move_number ();

		if (varDiagsCheckBox->isChecked ())
			for (auto it: st->children ()) {
				if (it == st->children ()[0])
					continue;
				it->set_figure (flags, tr ("Diagram %1").arg (diag_count).toStdString ());
				if (varOneCheckBox->isChecked ())
					it->set_sgf_move_number (1);
				diag_count++;
			}
	}
	if (last_figure != nullptr) {
		last_figure->set_figure (flags, tr ("Figure %1 (%2-%3)").arg (fig_count)
					 .arg (last_figure->move_number () + (last_figure->was_move_p () ? 0 : 1))
					 .arg (cur_move_number).toStdString ());
	}
	QDialog::accept ();
}
