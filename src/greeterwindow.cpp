#include "gogame.h"
#include "config.h"
#include "greeterwindow.h"
#include "setting.h"
#include "ui_helpers.h"
#include "clientwin.h"
#include "tutorial.h"

#include "ui_greeterwindow_gui.h"

GreeterWindow::GreeterWindow (QWidget *parent)
	: QMainWindow (parent), ui (new Ui::GreeterWindow)
{
	ui->setupUi (this);

	statusBar ()->hide ();
	setWindowTitle (PACKAGE + QString(" V") + VERSION);
	connect (ui->newButton, &QAbstractButton::clicked, [this] (bool) {
			if (open_local_board (this, game_dialog_type::none, screen_key (this)))
				hide ();
		});
	connect (ui->newVarButton, &QAbstractButton::clicked, [this] (bool) {
			if (open_local_board (this, game_dialog_type::variant, screen_key (this)))
				hide ();
		});
	connect (ui->loadButton, &QAbstractButton::clicked, [this] (bool) {
			std::shared_ptr<game_record> gr = open_file_dialog (this);
			if (gr == nullptr)
				return;

			MainWindow *win = new MainWindow (0, gr);
			win->show ();
			hide ();
		});

	connect (ui->onlineButton, &QAbstractButton::clicked, [this] (bool) {
			client_window->show ();
			hide ();
		});
	connect (ui->learnButton, &QAbstractButton::clicked, [this] (bool) {
			show_tutorials ();
			hide ();
		});
	connect (ui->enginePlayButton, &QAbstractButton::clicked, [this] (bool) {
			if (play_engine (this))
				hide ();
		});
	connect (ui->analyzeButton, &QAbstractButton::clicked, [this] (bool) {
			show_batch_analysis ();
			hide ();
		});
	connect (ui->quitButton, &QAbstractButton::clicked, [this] (bool) {
			client_window->quit ();
		});
	connect (ui->settingsButton, &QAbstractButton::clicked, [this] (bool) {
			client_window->dlgSetPreferences (0);
			enable_buttons ();
		});

	enable_buttons ();
}

GreeterWindow::~GreeterWindow ()
{
	delete ui;
}

void GreeterWindow::enable_buttons ()
{
	bool have_engines = setting->m_engines.size () > 0;
	ui->enginePlayButton->setEnabled (have_engines);
	ui->analyzeButton->setEnabled (have_engines);
}
