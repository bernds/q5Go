/*
 *
 *  q G o   a Go Program using Trolltech's Qt
 *
 *  (C) by Peter Strempel, Johannes Mesa, Emmanuel Beranger 2001-2003
 *
 */

#include <QFileDialog>

#include "config.h"
#include "sgf.h"
#include "clientwin.h"
#include "msg_handler.h"
#include "setting.h"
#include "miscdialogs.h"
#include "ui_helpers.h"
#include "variantgamedlg.h"
#include "analyzedlg.h"
#include "sgfpreview.h"
#include "dbdialog.h"

#include <qtranslator.h>
#include <qtextcodec.h>
#include <qapplication.h>
#include <qdialog.h>
#include <qmessagebox.h>
#include <qdir.h>

qGo *qgo;
QApplication *qgo_app;

Setting *setting = 0;

DBDialog *db_dialog;

Debug_Dialog *debug_dialog;
#ifdef OWN_DEBUG_MODE
QTextEdit *debug_view;
#endif

std::shared_ptr<game_record> new_game_dialog (QWidget *parent)
{
	NewLocalGameDialog dlg(parent);

	if (dlg.exec() != QDialog::Accepted)
		return nullptr;

	int sz = dlg.boardSizeSpin->value();
	int hc = dlg.handicapSpin->value();
	go_board starting_pos = new_handicap_board (sz, hc);
	game_info info ("",
			dlg.playerWhiteEdit->text().toStdString (),
			dlg.playerBlackEdit->text().toStdString (),
			dlg.playerWhiteRkEdit->text().toStdString (),
			dlg.playerBlackRkEdit->text().toStdString (),
			"", dlg.komiSpin->value(), hc,
			ranked::free,
			"", "", "", "", "", "", "", "", -1);
	std::shared_ptr<game_record> gr = std::make_shared<game_record> (starting_pos, hc > 1 ? white : black, info);

	return gr;
}

std::shared_ptr<game_record> new_variant_game_dialog (QWidget *parent)
{
	NewVariantGameDialog dlg(parent);

	if (dlg.exec() != QDialog::Accepted)
		return nullptr;

	int sz_x = dlg.xSizeSpin->value();
	int sz_y = dlg.ySizeSpin->value();
	bool torus_h = dlg.hTorusCheckBox->isChecked ();
	bool torus_v = dlg.vTorusCheckBox->isChecked ();
	go_board starting_pos (sz_x, sz_y, torus_h, torus_v);
	game_info info ("",
			dlg.playerWhiteEdit->text().toStdString (),
			dlg.playerBlackEdit->text().toStdString (),
			dlg.playerWhiteRkEdit->text().toStdString (),
			dlg.playerBlackRkEdit->text().toStdString (),
			"", dlg.komiSpin->value(), 0,
			ranked::free,
			"", "", "", "", "", "", "", "", -1);
	std::shared_ptr<game_record> gr = std::make_shared<game_record> (starting_pos, black, info);

	return gr;
}

static void warn_errors (std::shared_ptr<game_record> gr)
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

std::shared_ptr<game_record> record_from_stream (QIODevice &isgf, QTextCodec* codec)
{
	try {
		sgf *sgf = load_sgf (isgf);
		std::shared_ptr<game_record> gr = sgf2record (*sgf, codec);
		warn_errors (gr);
		return gr;
	} catch (invalid_boardsize &) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Unsupported board size in SGF file."));
	} catch (broken_sgf &) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Errors found in SGF file."));
	} catch (...) {
		QMessageBox::warning (0, PACKAGE, QObject::tr ("Error while trying to load SGF file."));
	}
	return nullptr;
}

std::shared_ptr<game_record> record_from_file (const QString &filename, QTextCodec* codec)
{
	QFile f (filename);
	f.open (QIODevice::ReadOnly);

	std::shared_ptr<game_record> gr = record_from_stream (f, codec);
	if (gr != nullptr)
		gr->set_filename (filename.toStdString ());
	return gr;
}

bool open_window_from_file (const QString &filename, QTextCodec* codec)
{
	std::shared_ptr<game_record> gr = record_from_file (filename, codec);
	if (gr == nullptr)
		return false;

	MainWindow *win = new MainWindow (0, gr);
	win->show ();
	return true;
}

std::shared_ptr<game_record> open_file_dialog (QWidget *parent)
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

			std::shared_ptr<game_record> gr = file_open_dialog.selected_record ();
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

std::shared_ptr<game_record> open_db_dialog (QWidget *parent)
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
	if (result == QDialog::Accepted) {
		std::shared_ptr<game_record> gr = db_dialog->selected_record ();
		if (gr != nullptr) {
			warn_errors (gr);
			return gr;
		}
	}

	return nullptr;
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

void open_local_board (QWidget *parent, game_dialog_type type, const QString &scrkey)
{
	std::shared_ptr<game_record> gr;
	switch (type) {
	case game_dialog_type::normal:
		gr = new_game_dialog (parent);
		if (gr == nullptr)
			return;
		break;
	case game_dialog_type::variant:
		gr = new_variant_game_dialog (parent);
		if (gr == nullptr)
			return;
		break;

	case game_dialog_type::none:
	{
		go_board b (19);
		gr = std::make_shared<game_record> (b, black, game_info (QObject::tr ("White").toStdString (),
									 QObject::tr ("Black").toStdString ()));
		break;
	}
	}
	MainWindow *win = new MainWindow (0, gr, scrkey);
	win->show ();
}

/* Create a bit array of hoshi points for a board shaped like REF.  */
bit_array calculate_hoshis (const go_board &ref)
{
	int size_x = ref.size_x ();
	int size_y = ref.size_y ();
	bit_array map (ref.bitsize ());

	int edge_dist_x = size_x > 12 ? 4 : 3;
	int edge_dist_y = size_y > 12 ? 4 : 3;
	int low_x = edge_dist_x - 1;
	int low_y = edge_dist_y - 1;
	int middle_x = size_x / 2;
	int middle_y = size_y / 2;
	int high_x = size_x - edge_dist_x;
	int high_y = size_y - edge_dist_y;
	if (size_x % 2 && size_x > 9) {
		map.set_bit (ref.bitpos (middle_x, low_y));
		map.set_bit (ref.bitpos (middle_x, high_y));
		if (size_y % 2 && size_y > 9)
			map.set_bit (ref.bitpos (middle_x, middle_y));
	}
	if (size_y % 2 && size_y > 9) {
		map.set_bit (ref.bitpos (low_x, middle_y));
		map.set_bit (ref.bitpos (high_x, middle_y));
	}

	map.set_bit (ref.bitpos (low_x, low_y));
	map.set_bit (ref.bitpos (high_x, low_y));
	map.set_bit (ref.bitpos (high_x, high_y));
	map.set_bit (ref.bitpos (low_x, high_y));
	return map;
}

go_board new_handicap_board (int size, int handicap)
{
	go_board b (size);

	if (size > 25 || size < 7)
	{
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
		return b;
	}

	int edge_dist = size > 12 ? 4 : 3;
	int low = edge_dist - 1;
	int middle = size / 2;
	int high = size - edge_dist;

	// extra:
	if (size == 19 && handicap > 9)
		switch (handicap)
		{
		case 13:
			b.set_stone (16, 16, black);
		case 12:
			b.set_stone (2, 2, black);
		case 11:
			b.set_stone (2, 16, black);
		case 10:
			b.set_stone (16, 2, black);
		default:
			handicap = 9;
			break;
		}

	switch (size % 2)
	{
	// odd board size
	case 1:
		switch (handicap)
		{
		case 9:
			b.set_stone (middle, middle, black);
		case 8:
		case 7:
			if (handicap >= 8)
			{
				b.set_stone (middle, low, black);
				b.set_stone (middle, high, black);
			}
			else
				b.set_stone (middle, middle, black);
		case 6:
		case 5:
			if (handicap >= 6)
			{
				b.set_stone (low, middle, black);
				b.set_stone (high, middle, black);
			}
			else
				b.set_stone (middle, middle, black);
		case 4:
			b.set_stone (high, high, black);
		case 3:
			b.set_stone (low, low, black);
		case 2:
			b.set_stone (high, low, black);
			b.set_stone (low, high, black);
		case 1:
			break;
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;

	// even board size
	case 0:
		switch (handicap)
		{
		case 4:
			b.set_stone (high, high, black);
		case 3:
			b.set_stone (low, low, black);
		case 2:
			b.set_stone (high, low, black);
			b.set_stone (low, high, black);
		case 1:
			break;

		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;

	default:
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");

	}
	b.identify_units ();
	return b;
}

board_rect find_crop (const game_state *gs)
{
	const go_board &b = gs->get_board ();
	const bit_array *col_left = create_column_left (b.size_x (), b.size_y ());
	const bit_array *row_top = create_row_top (b.size_x (), b.size_y ());
	board_rect rect (b);
	const bit_array *visible = gs->visible_inherited ();
	if (visible != nullptr && visible->popcnt () > 0) {
		for (rect.x1 = 0; rect.x1 < b.size_x (); rect.x1++)
			if (visible->intersect_p (*col_left, rect.x1))
				break;
		for (rect.x2 = b.size_x (); rect.x2-- > 0;)
			if (visible->intersect_p (*col_left, rect.x2))
				break;
		for (rect.y1 = 0; rect.y1 < b.size_y (); rect.y1++)
			if (visible->intersect_p (*row_top, rect.y1 * b.size_x ()))
				break;
		for (rect.y2 = b.size_y (); rect.y2-- > 0;)
			if (visible->intersect_p (*row_top, rect.y2 * b.size_x ()))
				break;
		return rect;
	}
	bit_array stones = b.get_stones_w ();
	stones.ior (b.get_stones_b ());
	if (stones.popcnt () == 0)
		return rect;
	for (rect.x1 = 0; rect.x1 < b.size_x (); rect.x1++)
		if (stones.intersect_p (*col_left, rect.x1)) {
			if (rect.x1 < 5)
				rect.x1 = 0;
			else
				rect.x1 = std::max (rect.x1 - 2, 0);
			break;
		}
	for (rect.x2 = b.size_x (); rect.x2-- > 0;)
		if (stones.intersect_p (*col_left, rect.x2)) {
			if (b.size_x () - 1 - rect.x2 < 5)
				rect.x2 = b.size_x () - 1;
			else
				rect.x2 = std::min (rect.x2 + 2, b.size_x () - 1);
			break;
		}
	for (rect.y1 = 0; rect.y1 < b.size_y (); rect.y1++)
		if (stones.intersect_p (*row_top, rect.y1 * b.size_x ())) {
			if (rect.y1 < 5)
				rect.y1 = 0;
			else
				rect.y1 = std::max (rect.y1 - 2, 0);
			break;
		}
	for (rect.y2 = b.size_y (); rect.y2-- > 0;)
		if (stones.intersect_p (*row_top, rect.y2 * b.size_x ())) {
			if (b.size_y () - 1 - rect.y2 < 5)
				rect.y2 = b.size_y () - 1;
			else
				rect.y2 = std::min (rect.y2 + 2, b.size_y () - 1);
			break;
		}
	return rect;
}

/* Generate a candidate for the filename for this game */
QString get_candidate_filename (const QString &dir, const game_info &info)
{
	const QString pw = QString::fromStdString (info.name_white ());
	const QString pb = QString::fromStdString (info.name_black ());
	QString date = QDate::currentDate().toString("yyyy-MM-dd");
	QString cand = date + "-" + pw + "-" + pb;
	QString base = cand;
	QDir d (dir);
	int i = 1;
	while (QFile (d.filePath (cand + ".sgf")).exists())
	{
		cand = base + "-" + QString::number(i++);
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

int main(int argc, char **argv)
{
	QApplication myapp(argc, argv);
	qgo_app = &myapp;

	myapp.setAttribute (Qt::AA_EnableHighDpiScaling);

	QCommandLineParser cmdp;
	QCommandLineOption clo_client { { "c", "client" }, QObject::tr ("Show the Go server client window (default if no other arguments)") };
	QCommandLineOption clo_board { { "b", "board" }, QObject::tr ("Start up with a board window (ignored if files are loaded).") };
	QCommandLineOption clo_analysis { { "a", "analyze" }, QObject::tr ("Start up with the computer analysis dialog to analyze <file>."), QObject::tr ("file") };
	QCommandLineOption clo_debug { { "d", "debug" }, QObject::tr ("Display debug messages in a window") };
	QCommandLineOption clo_encoding { { "e", "encoding "}, QObject::tr ("Specify text <encoding> of SGF files passed by command line."), "encoding"};

	cmdp.addOption (clo_client);
	cmdp.addOption (clo_board);
	cmdp.addOption (clo_analysis);
#ifdef OWN_DEBUG_MODE
	cmdp.addOption (clo_debug);
#endif
	cmdp.addOption (clo_encoding);
	cmdp.addHelpOption ();
	cmdp.addPositionalArgument ("file", QObject::tr ("Load <file> and display it in a board window."));

	cmdp.process (myapp);
	const QStringList args = cmdp.positionalArguments ();

	bool show_client = cmdp.isSet (clo_client) || (args.isEmpty () && !cmdp.isSet (clo_board) && !cmdp.isSet (clo_analysis));

#ifdef OWN_DEBUG_MODE
	qInstallMessageHandler (myMessageHandler);
	debug_dialog = new Debug_Dialog ();
	debug_dialog->setVisible (cmdp.isSet (clo_debug));
	debug_view = debug_dialog->TextView1;
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
		loc = QString("qgo_") + locale.language ();
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
	client_window->setWindowTitle (PACKAGE1 + QString(" V") + VERSION);

#ifdef OWN_DEBUG_MODE
	// restore size and pos
	if (client_window->getViewSize().width() > 0)
	{
		debug_dialog->resize(client_window->getViewSize());
		debug_dialog->move(client_window->getViewPos());
	}
#endif

	QObject::connect(&myapp, &QApplication::lastWindowClosed, client_window, &ClientWindow::slot_last_window_closed);

	client_window->setVisible (show_client);

	bool windows_open = false;
	QTextCodec* codec = nullptr;
	if (cmdp.isSet(clo_encoding)) {
		QString encoding = cmdp.value(clo_encoding);
		codec = QTextCodec::codecForName(encoding.toUtf8());
	}
	QStringList not_found;
	for (auto arg: args) {
		if (QFile::exists(arg) && arg.endsWith(".sgf", Qt::CaseInsensitive))
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
	if (!windows_open)
		return 1;

	if (cmdp.isSet (clo_analysis)) {
		analyze_dialog = new AnalyzeDialog (nullptr, cmdp.value (clo_analysis));
	} else
		analyze_dialog = new AnalyzeDialog (nullptr, QString ());
	analyze_dialog->setVisible (cmdp.isSet (clo_analysis));

	if (setting->getNewVersionWarning())
		help_new_version ();

	return myapp.exec();
}

