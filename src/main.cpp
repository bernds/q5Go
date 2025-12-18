/*
 *
 *  q G o   a Go Program using Trolltech's Qt
 *
 *  (C) by Peter Strempel, Johannes Mesa, Emmanuel Beranger 2001-2003
 *
 */

#include "common.h"

#include <QFileDialog>
#include <QTranslator>
#include <QTextCodec>
#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QDir>

#include "config.h"
#include "sgf.h"
#include "clientwin.h"
#include "msg_handler.h"
#include "setting.h"
#include "miscdialogs.h"
#include "ui_helpers.h"
#include "gotools.h"
#include "variantgamedlg.h"
#include "analyzedlg.h"
#include "sgfpreview.h"
#include "dbdialog.h"
#include "greeterwindow.h"
#include "newaigamedlg.h"
#include "tutorial.h"
#include "tips.h"
#include "patternsearch.h"

qGo *qgo;
QApplication *qgo_app;

Setting *setting = 0;

DBDialog *db_dialog;
PatternSearchWindow *patsearch_window;

Debug_Dialog *debug_dialog;
#ifdef OWN_DEBUG_MODE
static QFile *debug_file {};
QTextStream *debug_stream {};
QTextEdit *debug_view;
#endif

QString sec_to_time (int seconds)
{
	bool neg = seconds < 0;
	if (neg)
		seconds = -seconds;

	int h = seconds / 3600;
	seconds -= h*3600;
	int m = seconds / 60;
	int s = seconds - m*60;

	QString sec;

	// prevailling 0 for seconds
	if ((h || m) && s < 10)
		sec = "0" + QString::number(s);
	else
		sec = QString::number(s);

	if (h)
	{
		QString min;

		// prevailling 0 for minutes
		if (h && m < 10)
			min = "0" + QString::number(m);
		else
			min = QString::number(m);

		return (neg ? "-" : "") + QString::number(h) + ":" + min + ":" + sec;
	}
	else
		return (neg ? "-" : "") + QString::number(m) + ":" + sec;
}

go_game_ptr new_game_dialog (QWidget *parent)
{
	NewLocalGameDialog dlg(parent);

	if (dlg.exec() != QDialog::Accepted)
		return nullptr;

	int sz = dlg.boardSizeSpin->value();
	int hc = dlg.handicapSpin->value();
	go_board starting_pos = new_handicap_board (sz, hc);
	game_info info;
	info.name_w = dlg.playerWhiteEdit->text().toStdString ();
	info.name_b = dlg.playerBlackEdit->text().toStdString ();
	info.rank_w = dlg.playerWhiteRkEdit->text().toStdString ();
	info.rank_b = dlg.playerBlackRkEdit->text().toStdString ();
	info.komi = dlg.komiSpin->value();
	info.handicap = hc;
	go_game_ptr gr = std::make_shared<game_record> (starting_pos, hc > 1 ? white : black, info);

	return gr;
}

go_game_ptr new_variant_game_dialog (QWidget *parent)
{
	NewVariantGameDialog dlg(parent);

	if (dlg.exec() != QDialog::Accepted)
		return nullptr;

	go_game_ptr gr = dlg.create_game_record ();

	return gr;
}

static void warn_errors (go_game_ptr gr)
{
	const sgf_errors &errs = gr->errors ();
	if (errs.invalid_structure) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The file did not quite have the correct structure of an SGF file, but could otherwise be understood."));
	}
	if (errs.played_on_stone) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF file contained an invalid move that was played on top of another stone. Variations have been truncated at that point."));
	}
	if (errs.charset_error) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("One or more comments have been dropped since they contained invalid characters."));
	}
	if (errs.empty_komi) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF contained an empty value for komi. Assuming zero."));
	}
	if (errs.empty_handicap) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF contained an empty value for the handicap. Assuming zero."));
	}
	if (errs.invalid_val) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF contained an invalid value in a property related to display.  Things like move numbers might not show up correctly."));
	}
	if (errs.malformed_eval) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF contained evaluation data that could not be understood."));
	}
	if (errs.move_outside_board) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The SGF contained moves outside of the board area.  They were converted to passes."));
	}
}

/* A wrapper around sgf2record to handle exceptions with message boxes.  */

go_game_ptr record_from_stream (QIODevice &isgf, QTextCodec* codec)
{
	try {
		sgf *sgf = load_sgf (isgf);
		go_game_ptr gr = sgf2record (*sgf, codec);
		delete sgf;
		warn_errors (gr);
		return gr;
	} catch (invalid_boardsize &) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Unsupported board size in SGF file."));
	} catch (old_sgf_format &) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("The file uses an obsolete SGF format from 1993 that is no longer supported.\nIf you are using Jago, make sure the \"Pure SGF\" option is checked before saving."));
	} catch (broken_sgf &) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Errors found in SGF file."));
	} catch (...) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Error while trying to load SGF file."));
	}
	return nullptr;
}

go_game_ptr record_from_file (const QString &filename, QTextCodec* codec)
{
	QFile f (filename);
	f.open (QIODevice::ReadOnly);

	go_game_ptr gr = record_from_stream (f, codec);
	if (gr != nullptr)
		gr->set_filename (filename.toStdString ());
	return gr;
}

bool open_window_from_file (const QString &filename, QTextCodec* codec)
{
	go_game_ptr gr = record_from_file (filename, codec);
	if (gr == nullptr)
		return false;

	MainWindow *win = new MainWindow (0, gr);
	win->show ();
	return true;
}

go_game_ptr open_file_dialog (QWidget *parent)
{
	QString fileName;
	if (setting->readIntEntry ("FILESEL") == 1) {
		QString geokey = "FILESEL_GEOM_" + screen_key (parent);
		SGFPreview file_open_dialog (parent, setting->readEntry ("LAST_DIR"));
		QString saved = setting->readEntry (geokey);
		if (!saved.isEmpty ()) {
			QByteArray geometry = QByteArray::fromHex (saved.toLatin1 ());
			file_open_dialog.restoreGeometry (geometry);
		}
		int result = file_open_dialog.exec ();
		setting->writeEntry (geokey, QString::fromLatin1 (file_open_dialog.saveGeometry ().toHex ()));
		if (result == QDialog::Accepted) {
			/* If the file selector successfully loaded a preview, just use
			   that game record.  Otherwise extract the filename and try to
			   open it to show message boxes about whatever error occurs.  */

			go_game_ptr gr = file_open_dialog.selected_record ();
			if (gr != nullptr) {
				warn_errors (gr);
				return gr;
			}

			QStringList l = file_open_dialog.selected ();
			if (!l.isEmpty ())
				fileName = l.first ();
		} else
			return nullptr;
	} else {
		fileName = QFileDialog::getOpenFileName (parent, QObject::tr ("Open SGF file"),
							 setting->readEntry ("LAST_DIR"),
							 QObject::tr ("SGF Files (*.sgf *.SGF);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)"));
		if (fileName.isEmpty ())
			return nullptr;
	}
	QFileInfo fi (fileName);
	if (fi.exists ())
		setting->writeEntry ("LAST_DIR", fi.dir ().absolutePath ());
	return record_from_file (fileName, nullptr);
}

bool save_to_file (QWidget *parent, go_game_ptr gr, const QString &filename)
{
	QFile of (filename);
	if (!of.open (QIODevice::WriteOnly)) {
		QMessageBox::warning (parent, PACKAGE, QObject::tr ("Cannot open SGF file for saving."));
		return false;
	}
	std::string sgf = gr->to_sgf ();
	QByteArray bytes = QByteArray::fromStdString (sgf);
	qint64 written = of.write (bytes);
	if (written != bytes.length ()) {
		QMessageBox::warning (parent, PACKAGE, QObject::tr ("Failed to save SGF file."));
		return false;
	}
	of.close ();

	gr->set_modified (false);
	gr->clear_errors ();
	gr->set_filename (filename.toStdString ());
	return true;
}

static int exec_db_dialog (QWidget *parent)
{
	QString fileName;
	QString geokey = "DBDIALOG_GEOM_" + screen_key (parent);
	if (db_dialog == nullptr) {
		db_dialog = new DBDialog (nullptr);

		QString saved = setting->readEntry (geokey);
		if (!saved.isEmpty ()) {
			QByteArray geometry = QByteArray::fromHex (saved.toLatin1 ());
			db_dialog->restoreGeometry (geometry);
		}
	}
	int result = db_dialog->exec ();
	setting->writeEntry (geokey, QString::fromLatin1 (db_dialog->saveGeometry ().toHex ()));
	return result;
}

go_game_ptr open_db_dialog (QWidget *parent)
{
	int result = exec_db_dialog (parent);
	if (result == QDialog::Accepted) {
		go_game_ptr gr = db_dialog->selected_record ();
		if (gr != nullptr) {
			warn_errors (gr);
			return gr;
		}
	}

	return nullptr;
}

QString open_db_filename_dialog (QWidget *parent)
{
	int result = exec_db_dialog (parent);
	if (result == QDialog::Accepted) {
		go_game_ptr gr = db_dialog->selected_record ();
		if (gr != nullptr)
			return QString::fromStdString (gr->filename ());
	}
	return QString ();
}

QString open_filename_dialog (QWidget *parent)
{
	QString fileName;
	if (setting->readIntEntry ("FILESEL") == 1) {
		QString geokey = "FILESEL_GEOM_" + screen_key (parent);
		SGFPreview file_open_dialog (parent, setting->readEntry ("LAST_DIR"));
		QString saved = setting->readEntry (geokey);
		if (!saved.isEmpty ()) {
			QByteArray geometry = QByteArray::fromHex (saved.toLatin1 ());
			file_open_dialog.restoreGeometry (geometry);
		}
		int result = file_open_dialog.exec ();
		setting->writeEntry (geokey, QString::fromLatin1 (file_open_dialog.saveGeometry ().toHex ()));
		if (result == QDialog::Accepted) {
			QStringList l = file_open_dialog.selected ();
			if (!l.isEmpty ())
				fileName = l.first ();
		} else
			return nullptr;
	} else {
		fileName = QFileDialog::getOpenFileName (parent, QObject::tr ("Open SGF file"),
							 setting->readEntry ("LAST_DIR"),
							 QObject::tr ("SGF Files (*.sgf *.SGF);;MGT Files (*.mgt);;XML Files (*.xml);;All Files (*)"));
		if (fileName.isEmpty ())
			return nullptr;
	}
	QFileInfo fi (fileName);
	if (fi.exists ())
		setting->writeEntry ("LAST_DIR", fi.dir ().absolutePath ());
	return fileName;
}

bool open_local_board (QWidget *parent, game_dialog_type type, const QString &scrkey)
{
	go_game_ptr gr;
	switch (type) {
	case game_dialog_type::normal:
		gr = new_game_dialog (parent);
		if (gr == nullptr)
			return false;
		break;
	case game_dialog_type::variant:
		gr = new_variant_game_dialog (parent);
		if (gr == nullptr)
			return false;
		break;

	case game_dialog_type::none:
	{
		go_board b (19);
		game_info info;
		info.name_w = QObject::tr ("White").toStdString ();
		info.name_b = QObject::tr ("Black").toStdString ();
		gr = std::make_shared<game_record> (b, black, info);
		break;
	}
	}
	MainWindow *win = new MainWindow (0, gr, scrkey);
	win->show ();
	return true;
}

bool play_engine (QWidget *parent)
{
	if (setting->m_engines.size () == 0)
	{
		QMessageBox::warning (parent, PACKAGE, QObject::tr ("You did not configure any engines!"));
		client_window->dlgSetPreferences (3);
		return false;
	}

	NewAIGameDlg dlg (parent);
	if (dlg.exec () != QDialog::Accepted)
		return false;

	go_game_ptr gr = dlg.create_game_record ();
	if (gr == nullptr)
		return false;

	int eidx = dlg.engine_index ();
	const Engine &engine = setting->m_engines[eidx];
	bool computer_white = dlg.computer_white_p ();
	time_settings ts = dlg.timing ();
	new MainWindow_GTP (0, gr, screen_key (parent), engine, ts, !computer_white, computer_white);
	return true;
}

bool play_two_engines (QWidget *parent)
{
	if (setting->m_engines.size () == 0)
	{
		QMessageBox::warning (parent, PACKAGE, QObject::tr ("You did not configure any engines!"));
		client_window->dlgSetPreferences (3);
		return false;
	}

	TwoAIGameDlg dlg (parent);
	if (dlg.exec () != QDialog::Accepted)
		return false;

	go_game_ptr gr = dlg.create_game_record ();

	if (gr == nullptr)
		return false;

	int w_eidx = dlg.engine_index (white);
	int b_eidx = dlg.engine_index (black);
	const Engine &engine_w = setting->m_engines[w_eidx];
	const Engine &engine_b = setting->m_engines[b_eidx];
	time_settings ts = dlg.timing ();
	new MainWindow_GTP (0, gr, screen_key (parent), engine_w, engine_b, ts, dlg.num_games (), dlg.opening_book ());
	return true;
}

/* Generate a candidate for the filename for this game */
QString get_candidate_filename (const QString &dir, const game_info &info)
{
	const QString pw = QString::fromStdString (info.name_w);
	const QString pb = QString::fromStdString (info.name_b);
	QString date = QDate::currentDate ().toString ("yyyy-MM-dd");
	QString cand = date + "-" + pw + "-" + pb;
	QString base = cand;
	QDir d (dir);
	int i = 1;
	while (QFile (d.filePath (cand + ".sgf")).exists ())
	{
		cand = base + "-" + QString::number (i++);
	}
	return d.filePath (cand + ".sgf");
}

void show_batch_analysis ()
{
	if (analyze_dialog == nullptr)
		analyze_dialog = new AnalyzeDialog (nullptr, QString ());
	analyze_dialog->setVisible (true);
	analyze_dialog->activateWindow ();
}

void show_tutorials ()
{
	if (tutorial_dialog == nullptr)
		tutorial_dialog = new Tutorial_Slideshow (nullptr);
	tutorial_dialog->setVisible (true);
	tutorial_dialog->activateWindow ();
}

void show_pattern_search ()
{
	if (patsearch_window == nullptr)
		patsearch_window = new PatternSearchWindow ();
	patsearch_window->setVisible (true);
	patsearch_window->activateWindow ();
}

int main(int argc, char **argv)
{
	QApplication::setAttribute (Qt::AA_EnableHighDpiScaling);

	QApplication myapp(argc, argv);
	qgo_app = &myapp;

	QCommandLineParser cmdp;
	QCommandLineOption clo_client { { "c", "client" }, QObject::tr ("Show the Go server client window (default if no other arguments)") };
	QCommandLineOption clo_board { { "b", "board" }, QObject::tr ("Start up with a board window (ignored if files are loaded).") };
	QCommandLineOption clo_analysis { { "a", "analyze" }, QObject::tr ("Start up with the computer analysis dialog to analyze <file>."), QObject::tr ("file") };
	QCommandLineOption clo_pat { { "p", "pattern-search" }, QObject::tr ("Start up with the pattern search window.") };
	QCommandLineOption clo_debug { { "d", "debug" }, QObject::tr ("Display debug messages in a window") };
	QCommandLineOption clo_debug_file { { "D", "debug-file" }, QObject::tr ("Send debug messages to <file>."), QObject::tr ("file") };
	QCommandLineOption clo_encoding { { "e", "encoding "}, QObject::tr ("Specify text <encoding> of SGF files passed by command line."), "encoding"};

	cmdp.addOption (clo_client);
	cmdp.addOption (clo_board);
	cmdp.addOption (clo_analysis);
	cmdp.addOption (clo_pat);
#ifdef OWN_DEBUG_MODE
	cmdp.addOption (clo_debug);
	cmdp.addOption (clo_debug_file);
#endif
	cmdp.addOption (clo_encoding);
	cmdp.addHelpOption ();
	cmdp.addPositionalArgument ("file", QObject::tr ("Load <file> and display it in a board window."));

	cmdp.process (myapp);
	const QStringList args = cmdp.positionalArguments ();

	bool show_client = cmdp.isSet (clo_client);

#ifdef OWN_DEBUG_MODE
	qInstallMessageHandler (myMessageHandler);
	debug_dialog = new Debug_Dialog ();
	debug_dialog->setVisible (cmdp.isSet (clo_debug));
	debug_view = debug_dialog->TextView1;
	if (cmdp.isSet (clo_debug_file)) {
		debug_file = new QFile (cmdp.value (clo_debug_file));
		debug_file->open (QIODevice::WriteOnly);
		debug_stream = new QTextStream (debug_file);
	}
#endif

	// get application path
	qDebug() << "main:qt->PROGRAM.DIRPATH = " << QApplication::applicationDirPath();

	setting = new Setting();
	setting->loadSettings();

	// Load translation
	QTranslator trans(0);
	QString lang = setting->getLanguage();
	qDebug() << "Checking for language settings..." << lang;
	QString tr_dir = setting->getTranslationsDirectory(), loc;

	if (lang.isEmpty ()) {
		QLocale locale = QLocale::system ();
		qDebug() << "No language settings found, using system locale %s" << locale.name ();
		loc = QString("qgo_") + locale.name();
	} else {
		qDebug () << "Language settings found: " + lang;
		loc = QString("qgo_") + lang;
	}

	if (trans.load(loc, tr_dir)) {
		qDebug () << "Translation loaded.";
		myapp.installTranslator(&trans);
	} else if (lang != "en" && lang != "C")  // Skip warning for en and C default.
		qWarning() << "Failed to find translation file for " << lang;

	client_window = new ClientWindow (0);

#ifdef OWN_DEBUG_MODE
	// restore size and pos
	if (client_window->getViewSize().width() > 0)
	{
		debug_dialog->resize(client_window->getViewSize());
		debug_dialog->move(client_window->getViewPos());
	}
#endif

	QObject::connect (&myapp, &QApplication::lastWindowClosed, client_window, &ClientWindow::slot_last_window_closed);

	start_db_thread ();

	client_window->setVisible (show_client);

	bool windows_open = false;
	QTextCodec* codec = nullptr;
	if (cmdp.isSet(clo_encoding)) {
		QString encoding = cmdp.value(clo_encoding);
		codec = QTextCodec::codecForName(encoding.toUtf8());
	}
	QStringList not_found;
	for (const auto &arg: args) {
		if (QFile::exists(arg))
			windows_open |= open_window_from_file (arg, codec);
		else
			not_found << arg;
	}
	if (cmdp.isSet (clo_board) && !windows_open) {
		open_local_board (client_window, game_dialog_type::none, QString ());
		windows_open = true;
	}
	windows_open |= show_client;
	windows_open |= cmdp.isSet (clo_analysis);
	windows_open |= cmdp.isSet (clo_pat);
	if (!not_found.isEmpty ()) {
		QString err = QObject::tr ("The following files could not be found:") + "\n";
		for (auto &it: not_found)
			err += "  " + it + "\n";
		if (windows_open)
			QMessageBox::warning (0, PACKAGE, err);
		else {
			QTextStream str (stderr);
			str << err;
		}
	}

	if (cmdp.isSet (clo_pat))
		show_pattern_search ();
	if (cmdp.isSet (clo_analysis)) {
		analyze_dialog = new AnalyzeDialog (nullptr, cmdp.value (clo_analysis));
	} else
		analyze_dialog = new AnalyzeDialog (nullptr, QString ());
	analyze_dialog->setVisible (cmdp.isSet (clo_analysis));

	if (setting->getNewVersionWarning ())
		help_new_version ();

	if (setting->readBoolEntry ("TIPS_STARTUP")) {
		TipsDialog tips (nullptr);
		tips.exec ();
	}

	GreeterWindow *greeter {};
	if (!windows_open) {
		greeter = new GreeterWindow (nullptr);
		greeter->show ();
	}

	auto retval = myapp.exec ();

	if (debug_stream != nullptr) {
		delete debug_stream;
		delete debug_file;
		debug_stream = nullptr;
		debug_file = nullptr;
	}

	delete db_dialog;
	end_db_thread ();

	delete greeter;
	delete client_window;
	delete analyze_dialog;
#ifdef OWN_DEBUG_MODE
	delete debug_dialog;
#endif
	delete setting;

	return retval;
}

