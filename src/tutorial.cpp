#include <QPixmap>
#include <QCheckBox>
#include <QDialog>

#include "gogame.h"
#include "board.h"
#include "tutorial.h"
#include "mainwindow.h"
#include "ui_helpers.h"

#include "ui_slideshow_gui.h"

Tutorial_Slideshow *tutorial_dialog {};

struct tutorial {
	QString title;
	const char *filename;
	int lines;
};

static std::array<tutorial, 6> tutorials =
{
	tutorial { QObject::tr ("The Rules of Go"), ":/Tutorials/sgfs/rules.sgf", 12 },
	tutorial { QObject::tr ("Life and Death"), ":/Tutorials/sgfs/lnd.sgf", 12 },
	tutorial { QObject::tr ("Basic Tactics"), ":/Tutorials/sgfs/tactics.sgf", 12 },
	tutorial { QObject::tr ("Connections"), ":/Tutorials/sgfs/connections.sgf", 12 },
	tutorial { QObject::tr ("Captures"), ":/Tutorials/sgfs/captures.sgf", 12 },
	tutorial { QObject::tr ("Openings"), ":/Tutorials/sgfs/openings.sgf", 12 }
};

Tutorial_Slideshow::Tutorial_Slideshow (QWidget *parent)
	: BaseSlideView<Ui::SlideshowDialog> (parent)
{
	m_builtin_tutorial = true;
	int i = 0;
	for (auto &p: tutorials) {
		QStringList l;
		l << p.title;
		auto item = new QTreeWidgetItem (ui->tutorialList, l);
		m_item_map[item] = i++;
	}
	ui->tutorialList->header ()->setSectionResizeMode (0, QHeaderView::ResizeToContents);

	connect (ui->goFirstButton, &QPushButton::clicked,
		 [this] (bool) { set_active (m_game->get_root ()); update_buttons (); });
	connect (ui->goLastButton, &QPushButton::clicked,
		 [this] (bool) {
			 game_state *st = m_board_exporter->displayed ();
			 for (;;) {
				 game_state *next = st->next_primary_move ();
				 if (next == nullptr)
					 break;
				 st = next;
			 }
			 set_active (st);
			 update_buttons ();
		 });
	connect (ui->goNextButton, &QPushButton::clicked,
		 [this] (bool) { set_active (m_board_exporter->displayed ()->next_primary_move ()); update_buttons (); });
	connect (ui->goPrevButton, &QPushButton::clicked,
		 [this] (bool) { set_active (m_board_exporter->displayed ()->prev_move ()); update_buttons (); });

	ui->goFirstButton->setShortcut (Qt::Key_Home);
	ui->goLastButton->setShortcut (Qt::Key_End);
	ui->goNextButton->setShortcut (Qt::Key_Right);
	ui->goPrevButton->setShortcut (Qt::Key_Left);

	connect (ui->tutorialList, &QTreeWidget::itemClicked, [this] (QTreeWidgetItem *w) {
			if (w == nullptr)
				return;
			int n = m_item_map[w];
			load_tutorial (n);
		});
	connect (ui->editButton, &QPushButton::clicked, [this] (bool) {
			MainWindow *win = new MainWindow (0, m_game, screen_key (this));
			win->show ();
		});
	load_tutorial (0);
	show ();
}

void Tutorial_Slideshow::load_tutorial (int n)
{
	QFile f (tutorials[n].filename);
	f.open (QIODevice::ReadOnly);
	go_game_ptr gr = record_from_stream (f, nullptr);
	set_game (gr);
	m_n_lines = tutorials[n].lines;
	update_buttons ();
}

void Tutorial_Slideshow::update_buttons ()
{
	const game_state *st = m_board_exporter->displayed ();
	ui->goFirstButton->setEnabled (!st->root_node_p ());
	ui->goLastButton->setEnabled (st->n_children () > 0);
	ui->goNextButton->setEnabled (st->n_children () > 0);
	ui->goPrevButton->setEnabled (!st->root_node_p ());
}
