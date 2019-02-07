/*
* preferences.cpp
*/

#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QWhatsThis>

#include "preferences.h"
#include "mainwindow.h"
#include "qgo.h"
#include "clientwin.h"
#include "imagehandler.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

PreferencesDialog::PreferencesDialog(QWidget* parent)
	: QDialog (parent)
{
	setupUi (this);
	setModal (true);
	// pointer to ClientWindow
	parent_cw = setting->cw;
	CHECK_PTR(parent_cw);

	if (parent_cw->getPrefSize().width() > 0)
	{
		resize(parent_cw->getPrefSize());
		move(parent_cw->getPrefPos());
	}
	int engine_w = enginelabel_1->width ();
	engine_w = std::max (engine_w, enginelabel_2->width ());
	engine_w = std::max (engine_w, enginelabel_3->width ());
	engine_w = std::max (engine_w, enginelabel_4->width ());
	enginelabel_1->setMinimumWidth (engine_w);
	enginelabel_2->setMinimumWidth (engine_w);
	enginelabel_3->setMinimumWidth (engine_w);
	enginelabel_4->setMinimumWidth (engine_w);

	// init random-number generator
	srand ((unsigned)time (nullptr));

	LineEdit_port->setValidator (new QIntValidator (0, 9999, this));
	anMaxMovesEdit->setValidator (new QIntValidator (0, 999, this));
	anDepthEdit->setValidator (new QIntValidator (0, 999, this));

	// clear edit field
	LineEdit_title->clear();

	init_from_settings ();

	insertStandardHosts();

	// Default codec
	ComboBox_codec->addItem("");
	auto codecs = QTextCodec::availableCodecs ();
	for(auto it: codecs)
		ComboBox_codec->addItem(it);

	m_ih = new ImageHandler;

	int w = stoneView->width ();
	int h = stoneView->height ();
	m_stone_size = std::min (w / 2, h);
	m_ih->init (5);
	m_stone_canvas = new QGraphicsScene(0, 0, w, h, stoneView);
	stoneView->setScene (m_stone_canvas);
	m_b_stone = new QGraphicsPixmapItem;
	m_w_stone = new QGraphicsPixmapItem;
	m_stone_canvas->addItem (m_w_stone);
	m_stone_canvas->addItem (m_b_stone);

	connect (radioButtonStones_2D, &QRadioButton::toggled, this, &PreferencesDialog::select_stone_look);
	connect (radioButtonStones_3D, &QRadioButton::toggled, this, &PreferencesDialog::select_stone_look);

	connect (whiteColorButton, &QToolButton::clicked, this, &PreferencesDialog::select_white_color);
	connect (blackColorButton, &QToolButton::clicked, this, &PreferencesDialog::select_black_color);
	connect (blackRoundSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (blackHardSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (blackSpecSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (blackFlatSlider, &QSlider::valueChanged, [=] (int) { update_b_stones (); });
	connect (whiteRoundSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (whiteHardSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (whiteSpecSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (whiteFlatSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); });
	connect (ambientSlider, &QSlider::valueChanged, [=] (int) { update_w_stones (); update_b_stones (); });
	connect (stripesCheckBox, &QCheckBox::toggled, [=] (int) { update_w_stones (); });

	void (QComboBox::*cic) (int) = &QComboBox::currentIndexChanged;
	connect (woodComboBox, cic, [=] (int i) { GobanPicturePathButton->setEnabled (i == 0); LineEdit_goban->setEnabled (i == 0); update_board_image (); });
	connect (LineEdit_goban, &QLineEdit::editingFinished, [=] () { update_board_image (); });

	update_board_image ();
	update_w_stones ();
	update_b_stones ();
	connect (stoneView, &SizeGraphicsView::resized, [=] () { update_board_image (); update_w_stones (); update_b_stones (); });
}

void PreferencesDialog::update_board_image ()
{
	int w = stoneView->width ();
	int h = stoneView->height ();
	QImage image(w, h, QImage::Format_RGB32);
	QPixmap pm;
	QString filename = LineEdit_goban->text();
	int idx = woodComboBox->currentIndex ();
	if (idx > 0)
		filename = QString (":/BoardWindow/images/board/wood%1.png").arg (idx);
	QPixmap p (filename);
	if (p.isNull ())
		p = QPixmap (":/BoardWindow/images/board/wood1.png");

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin(&image);
	painter.setPen(Qt::NoPen);

	painter.drawTiledPixmap (0, 0, w, h, p);
	painter.end();
	m_stone_canvas->setSceneRect (0, 0, w, h);
	m_stone_canvas->setBackgroundBrush (QBrush (image));

	m_stone_size = std::min (w / 2, h);
	m_w_stone->setPos (w / 2 - m_stone_size, 0);
	m_b_stone->setPos (w / 2, 0);
}

void PreferencesDialog::init_from_settings ()
{
	for (auto h: parent_cw->hostlist)
		new QListWidgetItem (h->title(), ListView_hosts);
	for (auto h: parent_cw->m_engines)
		new QListWidgetItem (h->title(), ListView_engines);

	int idx = setting->readIntEntry("SKIN_INDEX");
	GobanPicturePathButton->setEnabled (idx == 0);
	LineEdit_goban->setEnabled (idx == 0);
	woodComboBox->setCurrentIndex (idx);

	LineEdit_goban->setText(setting->readEntry("SKIN"));
	LineEdit_Table->setText(setting->readEntry("SKIN_TABLE"));
	languageComboBox->insertItems(1, setting->getAvailableLanguages());
	languageComboBox->setCurrentIndex(setting->convertLanguageCodeToNumber());

	radioButtonStones_2D->setChecked((setting->readIntEntry("STONES_LOOK")==1));
	radioButtonStones_3D->setChecked((setting->readIntEntry("STONES_LOOK")==2));
	radioButtonStone_real->setChecked((setting->readIntEntry("STONES_LOOK")==3));
	stripesCheckBox->setChecked (setting->readBoolEntry("STONES_STRIPES"));
	whiteSpecSlider->setValue (setting->readIntEntry("STONES_WSPEC"));
	blackSpecSlider->setValue (setting->readIntEntry("STONES_BSPEC"));
	whiteHardSlider->setValue(setting->readIntEntry("STONES_WHARD"));
	blackHardSlider->setValue(setting->readIntEntry("STONES_BHARD"));
	whiteRoundSlider->setValue(setting->readIntEntry("STONES_WROUND"));
	blackRoundSlider->setValue(setting->readIntEntry("STONES_BROUND"));
	whiteFlatSlider->setValue(setting->readIntEntry("STONES_WFLAT"));
	blackFlatSlider->setValue(setting->readIntEntry("STONES_BFLAT"));
	ambientSlider->setValue (setting->readIntEntry("STONES_AMBIENT"));

	lineScaleCheckBox->setChecked (setting->readBoolEntry("BOARD_LINESCALE"));
	lineWidenCheckBox->setChecked (setting->readBoolEntry("BOARD_LINEWIDEN"));

	stoneSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_STONE"));
	autoplaySoundCheckBox->setChecked(setting->readBoolEntry("SOUND_AUTOPLAY"));
	talkSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_TALK"));
	matchSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_MATCH"));
	passSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_PASS"));
	gameEndSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_GAMEEND"));
	timeSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_TIME"));
	saySoundCheckBox->setChecked(setting->readBoolEntry("SOUND_SAY"));
	enterSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_ENTER"));
	leaveSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_LEAVE"));
	disConnectSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_DISCONNECT"));
	connectSoundCheckBox->setChecked(setting->readBoolEntry("SOUND_CONNECT"));

	soundMatchCheckBox->setChecked(setting->readBoolEntry("SOUND_MATCH_BOARD"));
	soundObserveCheckBox->setChecked(setting->readBoolEntry("SOUND_OBSERVE"));
	soundNormalCheckBox->setChecked(setting->readBoolEntry("SOUND_NORMAL"));
	soundComputerCheckBox->setChecked(setting->readBoolEntry("SOUND_COMPUTER"));

	variationComboBox->setCurrentIndex(setting->readIntEntry("VAR_GHOSTS"));
	varChildrenComboBox->setCurrentIndex(setting->readBoolEntry("VAR_CHILDREN") != 0);
	varSGFStyleComboBox->setCurrentIndex(setting->readIntEntry("VAR_SGF_STYLE"));

	int coords = (setting->readBoolEntry("BOARD_COORDS")
		      ? (setting->readBoolEntry("SGF_BOARD_COORDS") ? 2 : 1)
		      : 0);
	coordsComboBox->setCurrentIndex(coords);
	cursorCheckBox->setChecked(setting->readBoolEntry("CURSOR"));
	tooltipsCheckBox->setChecked(!(setting->readBoolEntry("TOOLTIPS")));
	timerComboBox->setCurrentIndex(setting->readIntEntry("TIMER_INTERVAL"));
	BYTimeSpin->setValue(setting->readIntEntry("BY_TIMER"));
	sgfTimeTagsCheckBox->setChecked(setting->readBoolEntry("SGF_TIME_TAGS"));
	sliderCheckBox->setChecked(setting->readBoolEntry("SLIDER"));
	int sidebar = (setting->readBoolEntry("SIDEBAR")
		       ? (setting->readBoolEntry("SIDEBAR_LEFT") ? 1 : 2)
		       : 0);
	sidebarComboBox->setCurrentIndex(sidebar);
	antiClickoCheckBox->setChecked(setting->readBoolEntry("ANTICLICKO"));

	gameTreeSizeSlider->setValue(setting->readIntEntry("GAMETREE_SIZE"));
	diagShowComboBox->setCurrentIndex(setting->readIntEntry("BOARD_DIAGMODE"));
	diagClearCheckBox->setChecked(setting->readBoolEntry("BOARD_DIAGCLEAR"));
	diagHideCheckBox->setChecked(setting->readBoolEntry("GAMETREE_DIAGHIDE"));

	// Client Window tab
	LineEdit_watch->setText(setting->readEntry("WATCH"));
	LineEdit_exclude->setText(setting->readEntry("EXCLUDE"));
	CheckBox_extUserInfo->setChecked(setting->readBoolEntry("EXTUSERINFO"));
//	CheckBox_useNmatch->setChecked(setting->readBoolEntry("USE_NMATCH"));
	checkBox_Nmatch_Black->setChecked(setting->readBoolEntry("NMATCH_BLACK"));
	checkBox_Nmatch_White->setChecked(setting->readBoolEntry("NMATCH_WHITE"));
	checkBox_Nmatch_Nigiri->setChecked(setting->readBoolEntry("NMATCH_NIGIRI"));
	HandicapSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_HANDICAP"));
	timeSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_MAIN_TIME"));
	BYSpin_Nmatch->setValue(setting->readIntEntry("NMATCH_BYO_TIME"));

	computerWhiteButton->setChecked(setting->readBoolEntry("COMPUTER_WHITE"));
	computerSizeSpin->setValue(setting->readIntEntry("COMPUTER_SIZE"));
	computerHandicapSpin->setValue(setting->readIntEntry("COMPUTER_HANDICAP"));
	humanName->setText(setting->readEntry("HUMAN_NAME"));

	anChildMovesCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_CHILDREN"));
	anPruneCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_PRUNE"));
	anHideCheckBox->setChecked (setting->readBoolEntry ("ANALYSIS_HIDEOTHER"));
	anVarComboBox->setCurrentIndex (setting->readIntEntry ("ANALYSIS_VARTYPE"));
	winrateComboBox->setCurrentIndex (setting->readIntEntry ("ANALYSIS_WINRATE"));
	anDepthEdit->setText (QString::number (setting->readIntEntry ("ANALYSIS_DEPTH")));
	anMaxMovesEdit->setText (QString::number (setting->readIntEntry ("ANALYSIS_MAXMOVES")));

	// Go Server tab
	boardSizeSpin->setValue(setting->readIntEntry("DEFAULT_SIZE"));
	timeSpin->setValue(setting->readIntEntry("DEFAULT_TIME"));
	BYSpin->setValue(setting->readIntEntry("DEFAULT_BY"));
	komiSpinDefault->setValue(setting->readIntEntry("DEFAULT_KOMI"));
	automaticNegotiationCheckBox->setChecked(setting->readBoolEntry("DEFAULT_AUTONEGO"));
	CheckBox_autoSave->setChecked(setting->readBoolEntry("AUTOSAVE"));
	CheckBox_autoSave_Played->setChecked(setting->readBoolEntry("AUTOSAVE_PLAYED"));

	toroidDupsSpin->setValue (setting->readIntEntry("TOROID_DUPS"));

	fontStandardButton->setText(setting->fontToString(setting->fontStandard));
	fontMarksButton->setText(setting->fontToString(setting->fontMarks));
	fontCommentsButton->setText(setting->fontToString(setting->fontComments));
	fontListsButton->setText(setting->fontToString(setting->fontLists));
	fontClocksButton->setText(setting->fontToString(setting->fontClocks));
	fontConsoleButton->setText(setting->fontToString(setting->fontConsole));

	fontStandardButton->setFont(setting->fontStandard);
	fontMarksButton->setFont(setting->fontMarks);
	fontCommentsButton->setFont(setting->fontComments);
	fontListsButton->setFont(setting->fontLists);
	fontClocksButton->setFont(setting->fontClocks);
	fontConsoleButton->setFont(setting->fontConsole);
}

void PreferencesDialog::select_stone_look (bool)
{
	update_w_stones ();
	update_b_stones ();
}

void PreferencesDialog::select_white_color (bool)
{
	QColor c = QColorDialog::getColor (m_ih->white_color (), this, tr ("Select white stone base color"));
	m_ih->set_white_color (c);
	update_w_stones ();
}

void PreferencesDialog::select_black_color (bool)
{
	QColor c = QColorDialog::getColor (m_ih->black_color (), this, tr ("Select black stone base color"));
	m_ih->set_black_color (c);
	update_b_stones ();
}

void PreferencesDialog::update_stone_params ()
{
	double br = 2.05 + (100 - blackRoundSlider->value ()) / 30.0;
	double wr = 2.05 + (100 - whiteRoundSlider->value ()) / 30.0;
	double bs = blackSpecSlider->value () / 100.0;
	double ws = whiteSpecSlider->value () / 100.0;
	double bf = blackFlatSlider->value ();
	double wf = whiteFlatSlider->value ();
	double bh = 1 + blackHardSlider->value () / 10.0;
	double wh = 1 + whiteHardSlider->value () / 10.0;
	double ambient = ambientSlider->value () / 100.0;
	bool clamshell = stripesCheckBox->isChecked ();
	int look = 3;
	if (radioButtonStones_2D->isChecked ())
		look = 1;
	else if (radioButtonStones_3D->isChecked ())
		look = 2;
	blackGroupBox->setEnabled (look == 3);
	whiteGroupBox->setEnabled (look == 3);
	m_ih->set_stone_params (wh, bh, ws, bs, wr, br, wf, bf, ambient, look, clamshell);
}

void PreferencesDialog::update_w_stones ()
{
	update_stone_params ();
	QImage img (m_stone_size, m_stone_size, QImage::Format_ARGB32);
	m_ih->paint_one_stone (img, true, m_stone_size);
	QPixmap pm = QPixmap::fromImage (img);
	m_w_stone->setPixmap (pm);
}

void PreferencesDialog::update_b_stones ()
{
	update_stone_params ();
	QImage img (m_stone_size, m_stone_size, QImage::Format_ARGB32);
	m_ih->paint_one_stone (img, false, m_stone_size);
	QPixmap pm = QPixmap::fromImage (img);
	m_b_stone->setPixmap (pm);
}


/*  
*  Destroys the object and frees any allocated resources
*/
PreferencesDialog::~PreferencesDialog()
{
	// no need to delete child widgets, Qt does it all for us
}


/*****************************************************************
*
*               Overwritten own implementations
*
*****************************************************************/

void PreferencesDialog::slot_apply()
{
	qDebug() << "onApply";

	setting->writeIntEntry("SKIN_INDEX", woodComboBox->currentIndex ());
	setting->writeEntry("SKIN", LineEdit_goban->text());
	setting->writeEntry("SKIN_TABLE", LineEdit_Table->text());
	setting->obtain_skin_images ();

	setting->writeEntry("LANG", setting->convertNumberToLanguage(languageComboBox->currentIndex()));
//	setting->writeBoolEntry("STONES_SHADOW", stonesShadowCheckBox->isChecked());
//	setting->writeBoolEntry("STONES_SHELLS", stonesShellsCheckBox->isChecked());
	int i = 3;
	if ( radioButtonStones_2D->isChecked())
		i=1;
	else if ( radioButtonStones_3D->isChecked())
		i=2;
	setting->writeBoolEntry("STONES_STRIPES", stripesCheckBox->isChecked());
	setting->writeIntEntry("STONES_LOOK", i);
	setting->writeIntEntry("STONES_WSPEC", whiteSpecSlider->value());
	setting->writeIntEntry("STONES_BSPEC", blackSpecSlider->value());
	setting->writeIntEntry("STONES_WHARD", whiteHardSlider->value());
	setting->writeIntEntry("STONES_BHARD", blackHardSlider->value());
	setting->writeIntEntry("STONES_WROUND", whiteRoundSlider->value());
	setting->writeIntEntry("STONES_BROUND", blackRoundSlider->value());
	setting->writeIntEntry("STONES_WFLAT", whiteFlatSlider->value());
	setting->writeIntEntry("STONES_BFLAT", blackFlatSlider->value());
	setting->writeIntEntry("STONES_AMBIENT", ambientSlider->value());
	setting->writeEntry("STONES_BCOL", black_color().name());
	setting->writeEntry("STONES_WCOL", white_color().name());

	setting->writeBoolEntry("BOARD_LINESCALE", lineScaleCheckBox->isChecked ());
	setting->writeBoolEntry("BOARD_LINEWIDEN", lineWidenCheckBox->isChecked ());

	setting->writeBoolEntry("SOUND_STONE", stoneSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_AUTOPLAY", autoplaySoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_TALK", talkSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_MATCH", matchSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_GAMEEND", passSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_PASS", gameEndSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_TIME", timeSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_SAY", saySoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_ENTER", enterSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_LEAVE", leaveSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_DISCONNECT", disConnectSoundCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_CONNECT", connectSoundCheckBox->isChecked());

	setting->writeBoolEntry("SOUND_MATCH_BOARD", soundMatchCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_OBSERVE", soundObserveCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_NORMAL", soundNormalCheckBox->isChecked());
	setting->writeBoolEntry("SOUND_COMPUTER", soundComputerCheckBox->isChecked());

	setting->writeIntEntry("VAR_GHOSTS", variationComboBox->currentIndex());
	setting->writeBoolEntry("VAR_CHILDREN", varChildrenComboBox->currentIndex() == 1);
	setting->writeIntEntry("VAR_SGF_STYLE", varSGFStyleComboBox->currentIndex());

	int coords = coordsComboBox->currentIndex();
	int sidebar = sidebarComboBox->currentIndex();
	setting->writeBoolEntry("BOARD_COORDS", coords > 0);
	setting->writeBoolEntry("SGF_BOARD_COORDS", coords == 2);
	setting->writeBoolEntry("SLIDER", sliderCheckBox->isChecked());
	setting->writeBoolEntry("SIDEBAR", sidebar > 0);
	setting->writeBoolEntry("SIDEBAR_LEFT", sidebar == 1);
	setting->writeBoolEntry("CURSOR", cursorCheckBox->isChecked());
	//setting->writeBoolEntry("SMALL_STONES", smallerStonesCheckBox->isChecked());
	setting->writeBoolEntry("TOOLTIPS", !(tooltipsCheckBox->isChecked()));
	setting->writeIntEntry("BY_TIMER", BYTimeSpin->text().toInt());
	setting->writeIntEntry("TIMER_INTERVAL", timerComboBox->currentIndex());
	setting->writeBoolEntry("SGF_TIME_TAGS", sgfTimeTagsCheckBox->isChecked());
	setting->writeBoolEntry("ANTICLICKO", antiClickoCheckBox->isChecked());

	// Client Window Tab
	setting->writeEntry("WATCH", LineEdit_watch->text());
	setting->writeEntry("EXCLUDE", LineEdit_exclude->text());
	setting->writeBoolEntry("EXTUSERINFO", CheckBox_extUserInfo->isChecked());
//	setting->writeBoolEntry("USE_NMATCH", CheckBox_useNmatch->isChecked());

	//Checks wether the nmatch parameters have been modified, in order to send a new nmatchrange command
	if( 	(setting->readBoolEntry("NMATCH_BLACK") != checkBox_Nmatch_Black->isChecked()) ||
		(setting->readBoolEntry("NMATCH_WHITE") != checkBox_Nmatch_White->isChecked()) ||
		(setting->readBoolEntry("NMATCH_NIGIRI") != checkBox_Nmatch_Nigiri->isChecked()) ||
		(setting->readIntEntry("NMATCH_MAIN_TIME") != timeSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("NMATCH_BYO_TIME") != BYSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("NMATCH_HANDICAP") != HandicapSpin_Nmatch->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_SIZE") != boardSizeSpin->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_TIME") != timeSpin->text().toInt()) ||
		(setting->readIntEntry("DEFAULT_BY") != BYSpin->text().toInt()) )
			setting->nmatch_settings_modified = true;

	setting->writeBoolEntry("NMATCH_BLACK", checkBox_Nmatch_Black->isChecked());
	setting->writeBoolEntry("NMATCH_WHITE", checkBox_Nmatch_White->isChecked());
	setting->writeBoolEntry("NMATCH_NIGIRI", checkBox_Nmatch_Nigiri->isChecked());
	setting->writeIntEntry("NMATCH_MAIN_TIME", timeSpin_Nmatch->text().toInt());
	setting->writeIntEntry("NMATCH_BYO_TIME", BYSpin_Nmatch->text().toInt());
	setting->writeIntEntry("NMATCH_HANDICAP", HandicapSpin_Nmatch->text().toInt());

	setting->writeIntEntry("DEFAULT_SIZE", boardSizeSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_TIME", timeSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_BY", BYSpin->text().toInt());
	setting->writeIntEntry("DEFAULT_KOMI", komiSpinDefault->text().toFloat());
	setting->writeBoolEntry("DEFAULT_AUTONEGO", automaticNegotiationCheckBox->isChecked());
	setting->writeBoolEntry("AUTOSAVE", CheckBox_autoSave->isChecked());
	setting->writeBoolEntry("AUTOSAVE_PLAYED", CheckBox_autoSave_Played->isChecked());

	// Computer Tab
	setting->writeBoolEntry("COMPUTER_WHITE", computerWhiteButton->isChecked());
	setting->writeIntEntry("COMPUTER_SIZE", computerSizeSpin->text().toInt());
	setting->writeIntEntry("COMPUTER_HANDICAP", computerHandicapSpin->text().toInt());
	setting->writeEntry("HUMAN_NAME", humanName->text());

	setting->writeBoolEntry ("ANALYSIS_CHILDREN", anChildMovesCheckBox->isChecked ());
	setting->writeBoolEntry ("ANALYSIS_PRUNE", anPruneCheckBox->isChecked ());
	setting->writeBoolEntry ("ANALYSIS_HIDEOTHER", anHideCheckBox->isChecked ());
	setting->writeIntEntry ("ANALYSIS_VARTYPE", anVarComboBox->currentIndex ());
	setting->writeIntEntry ("ANALYSIS_WINRATE", winrateComboBox->currentIndex ());

	setting->writeIntEntry ("ANALYSIS_DEPTH", anDepthEdit->text().toInt());
	setting->writeIntEntry ("ANALYSIS_MAXMOVES", anMaxMovesEdit->text().toInt());

	setting->writeIntEntry("GAMETREE_SIZE", gameTreeSizeSlider->value());
	setting->writeIntEntry("BOARD_DIAGMODE", diagShowComboBox->currentIndex());
	setting->writeBoolEntry("BOARD_DIAGCLEAR", diagClearCheckBox->isChecked());
	setting->writeBoolEntry("GAMETREE_DIAGHIDE", diagHideCheckBox->isChecked());

	setting->writeIntEntry("TOROID_DUPS", toroidDupsSpin->text().toInt());

	setting->extract_frequent_settings ();

	client_window->preferencesAccept();
}

void PreferencesDialog::startHelpMode()
{
	QWhatsThis::enterWhatsThisMode();
}

void PreferencesDialog::selectFont(int selector)
{
	// Open a font dialog to select a new font
	bool ok;
	QFont f;
	switch (selector)
	{
	case 0:
		f = QFontDialog::getFont(&ok, setting->fontStandard, this);
		if (ok)  // Accepted
		{
			setting->fontStandard = f;
			fontStandardButton->setText(setting->fontToString(f));
			fontStandardButton->setFont(f);
		}
		break;

	case 1:
		f = QFontDialog::getFont(&ok, setting->fontMarks, this);
		if (ok)  // Accepted
		{
			setting->fontMarks = f;
			fontMarksButton->setText(setting->fontToString(f));
			fontMarksButton->setFont(f);
		}
		break;

	case 2:
		f = QFontDialog::getFont(&ok, setting->fontComments, this);
		if (ok)  // Accepted
		{
			setting->fontComments = f;
			fontCommentsButton->setText(setting->fontToString(f));
			fontCommentsButton->setFont(f);
		}
		break;

	case 3:
		f = QFontDialog::getFont(&ok, setting->fontLists, this);
		if (ok)  // Accepted
		{
			setting->fontLists = f;
			fontListsButton->setText(setting->fontToString(f));
			fontListsButton->setFont(f);
		}
		break;

	case 4:
		f = QFontDialog::getFont(&ok, setting->fontClocks, this);
		if (ok)  // Accepted
		{
			setting->fontClocks = f;
			fontClocksButton->setText(setting->fontToString(f));
			fontClocksButton->setFont(f);
		}
		break;

	case 5:
		f = QFontDialog::getFont(&ok, setting->fontConsole, this);
		if (ok)  // Accepted
		{
			setting->fontConsole = f;
			fontConsoleButton->setText(setting->fontToString(f));
			fontConsoleButton->setFont(f);
		}
		break;

	default:
		break;
	}
}

bool PreferencesDialog::avoid_losing_data ()
{
	if (m_changing_engine) {
		if (!enginePath->text ().isEmpty () || !engineArgs->text ().isEmpty ()
		    || !engineKomi->text ().isEmpty ())
		{
			QMessageBox mb(QMessageBox::Question, tr("Unsaved data"),
				       QString(tr("The engine input fields contain\n"
						  "potentially unsaved data.\n"
						  "Really close the preferences?")),
				       QMessageBox::Yes | QMessageBox::No);
			return mb.exec() == QMessageBox::No;
		}
	}
	if (m_changing_host) {
		if (!LineEdit_host->text ().isEmpty ()
		    || !LineEdit_port->text ().isEmpty ()
		    || !LineEdit_login->text ().isEmpty ())
		{
			QMessageBox mb(QMessageBox::Question, tr("Unsaved data"),
				       QString(tr("The host input fields contain\n"
						  "potentially unsaved data.\n"
						  "Really close the preferences?")),
				       QMessageBox::Yes | QMessageBox::No);
			return mb.exec() == QMessageBox::No;
		}
	}
	return false;
}

void PreferencesDialog::slot_accept()
{
	if (avoid_losing_data ())
		return;
	saveSizes();

	slot_apply ();
	accept();
}

void PreferencesDialog::slot_reject()
{
	if (avoid_losing_data ())
		return;

	saveSizes();
	reject();
}

void PreferencesDialog::saveSizes()
{
	// save size and position of window
	parent_cw->savePrefFrame(pos(), size());

	// update hosts
	parent_cw->slot_cbconnect(QString());
}

void PreferencesDialog::slot_add_engine()
{
	QString komi = engineKomi->text ();
	if (!komi.isEmpty ()) {
		QDoubleValidator kv;
		kv.setDecimals (2);
		int pos = 0;
		if (kv.validate (komi, pos) != QValidator::Acceptable) {
			QMessageBox::warning (this, tr ("Invalid komi entered"),
					      tr ("Please enter a valid komi before adding the engine."));
			return;
		}
	}
	const QString name = engineName->text();
	// check if at least title and path are set
	if (!name.isEmpty() && !enginePath->text().isEmpty())
	{
		// check if title already exists
		bool found = false;

		for (auto h: parent_cw->m_engines)
			if (h->title() == name)
			{
				found = true;
				// if found, insert at current pos, and remove old item
				parent_cw->m_engines.removeOne (h);
				break;
			}

		parent_cw->m_engines.append(new Engine(engineName->text(),
						       enginePath->text(), engineArgs->text (),
						       engineKomi->text(),
						       engineAnalysis->isChecked ()));
		std::sort (parent_cw->m_engines.begin (), parent_cw->m_engines.end (),
			   [] (Engine *a, Engine *b) { return *a < *b; });

		// create entry in listview
		if (!found)
			new QListWidgetItem(name, ListView_engines);
		else
		{
			ListView_engines->currentItem()->setText(name);
		}
		ListView_engines->repaint();
	}

	clear_engine ();
}

void PreferencesDialog::clear_engine ()
{
	engineName->clear ();
	enginePath->clear ();
	engineArgs->clear ();
	engineKomi->clear ();
	engineAnalysis->setChecked (false);
	m_changing_engine = false;
}

void PreferencesDialog::slot_delete_engine()
{
	for (auto h: parent_cw->m_engines) {
		if (h->title() == LineEdit_title->text())
		{
			// if found, delete current entry
			parent_cw->m_engines.removeOne(h);
			delete h;
			break;
		}
	}

	clear_engine ();

	// set connection titles to listview
	ListView_engines->clear ();

	for (auto h: parent_cw->m_engines)
		new QListWidgetItem(h->title(), ListView_engines);
}

void PreferencesDialog::slot_new_engine()
{
	clear_engine ();
}

void PreferencesDialog::slot_clickedEngines (QListWidgetItem *lvi)
{
	if (!lvi)
		return;

	// fill host info of selected title
	for (auto h: parent_cw->m_engines) {
		if (h->title() == lvi->text ()) {
			engineName->setText(h->title ());
			enginePath->setText(h->path ());
			engineArgs->setText(h->args ());
			engineKomi->setText(h->komi ());
			engineAnalysis->setChecked (h->analysis ());
			break;
		}
	}
}

void PreferencesDialog::slot_engineChanged(const QString &title)
{
	for (auto h: parent_cw->m_engines)
	{
		if (h->title() == title) {
			m_changing_engine = true;
			pb_engine_add->setText(tr("Change"));
			return;
		}
	}

	m_changing_engine = false;
	pb_engine_add->setText(tr("Add"));
}


void PreferencesDialog::clear_host ()
{
	LineEdit_title->clear();
	LineEdit_host->clear();
	LineEdit_port->clear();
	LineEdit_login->clear();
	ComboBox_codec->setCurrentText("");

	m_changing_host = false;
}

void PreferencesDialog::insertStandardHosts()
{
	// standard hosts
	new QListWidgetItem("-- Aurora --", ListView_hosts);
	new QListWidgetItem("-- CTN --", ListView_hosts);
	new QListWidgetItem("-- CWS --", ListView_hosts);
	new QListWidgetItem("-- EGF --", ListView_hosts);
	new QListWidgetItem("-- IGS --", ListView_hosts);
	new QListWidgetItem("-- LGS --", ListView_hosts);
	new QListWidgetItem("-- NNGS --", ListView_hosts);
	new QListWidgetItem("-- WING --", ListView_hosts);
}

void PreferencesDialog::slot_add_server()
{
	// check if at least title and host inserted
	if (!LineEdit_title->text().isEmpty() && !LineEdit_host->text().isEmpty())
	{
		// check if title already exists
		bool found = false;
		bool check;
		unsigned int tmp = LineEdit_port->text().toUInt(&check);
		if (!check)
		{
			tmp = 0;
			qWarning("Failed to convert port to integer!");
		}

		for (auto h: parent_cw->hostlist)
			if (h->title() == LineEdit_title->text())
			{
				found = true;
				// if found, insert at current pos, and remove old item
				parent_cw->hostlist.removeOne (h);
				break;
			}

		// insert host at its sorted position
		parent_cw->hostlist.append(new Host(LineEdit_title->text(),
						    LineEdit_host->text(),
						    tmp,
						    LineEdit_login->text(),
						    LineEdit_pass->text(),
						    ComboBox_codec->currentText()));
		std::sort (parent_cw->hostlist.begin (), parent_cw->hostlist.end (),
			   [] (Host *a, Host *b) { return *a < *b; });

		// create entry in listview
		if (!found)
			new QListWidgetItem(LineEdit_title->text(), ListView_hosts);
		else
		{
			ListView_hosts->currentItem()->setText(LineEdit_title->text());
		}
		ListView_hosts->repaint();
//			cb_title->insertItem(LineEdit_title->text(), 0);
	}

	// init insertion fields
	slot_cbtitle(QString());
}

void PreferencesDialog::slot_delete_server()
{
	for (auto h: parent_cw->hostlist) {
		if (h->title() == LineEdit_title->text())
		{
			// if found, delete current entry
			parent_cw->hostlist.removeOne(h);
			delete h;
			break;
		}
	}

	// set connection titles to listview
	ListView_hosts->clear ();
	for (auto h: parent_cw->hostlist)
		new QListWidgetItem(h->title(), ListView_hosts);
	insertStandardHosts();
	clear_host ();
}

void PreferencesDialog::slot_new_server()
{
	clear_host ();
}

void PreferencesDialog::slot_clickedHostList(QListWidgetItem *lvi)
{
	if (!lvi) {
		return;
	}

	slot_cbtitle (lvi->text());
}

void PreferencesDialog::slot_cbtitle(const QString &txt)
{
	LineEdit_pass->clear();
	if (txt.isEmpty() || txt.isNull())
		clear_host ();
	// fix coding: standard servers
	else if (txt == QString("-- Aurora --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("aurora.go.art.pl");
		LineEdit_port->setText("9696");
		LineEdit_login->setText("guest");
		// What codec does this use?
		ComboBox_codec->setCurrentText("");
	}
	else if (txt == QString("-- CTN --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("weiqi.online.sh.cn");
		LineEdit_port->setText("8888");
		LineEdit_login->setText("guest");
		// What codec does this use?
		ComboBox_codec->setCurrentText("");
	}
	else if (txt == QString("-- CWS --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("cws.weiqi.net");
		LineEdit_port->setText("9696");
		LineEdit_login->setText("qGo" + QString::number(rand() % 10000));
		// What codec does this use?
		ComboBox_codec->setCurrentText("");
	}
	else if (txt == QString("-- LGS --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("lgs.taiwango.net");
		LineEdit_port->setText("9696");
		LineEdit_login->setText("guest");
		// What codec does this use?
		ComboBox_codec->setCurrentText("");
	}
	else if (txt == QString("-- WING --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("wing.gr.jp");
		LineEdit_port->setText("1515");
		LineEdit_login->setText("guest");
		// What codec does this use?
		ComboBox_codec->setCurrentText("");
	}
	else if (txt == QString("-- IGS --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("igs.joyjoy.net");
		LineEdit_port->setText("7777");
		LineEdit_login->setText("guest");
		ComboBox_codec->setCurrentText("SJIS");
	}
	else if (txt == QString("-- NNGS --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("nngs.cosmic.org");
		LineEdit_port->setText("9696");
		LineEdit_login->setText("qGo" + QString::number(rand() % 10000));
		// A guess
		ComboBox_codec->setCurrentText("iso-8859-1");
	}
	else if (txt == QString("-- EGF --"))
	{
		LineEdit_title->clear();
		LineEdit_host->setText("server.european-go.org");
		LineEdit_port->setText("6969");
		LineEdit_login->setText("qGo" + QString::number(rand() % 10000));
		// A guess
		ComboBox_codec->setCurrentText("iso-8859-15");
	}
	else
	{
		// fill host info of selected title
		for (auto h: parent_cw->hostlist) {
			if (h->title() == txt)
			{
				LineEdit_title->setText(h->title());
				LineEdit_host->setText(h->host());
				LineEdit_port->setText(QString::number(h->port()));
				LineEdit_login->setText(h->loginName());
				LineEdit_pass->setText(h->password());
				ComboBox_codec->setCurrentText(h->codec());
				break;
			}
		}
	}
}

void PreferencesDialog::slot_serverChanged(const QString &title)
{
	for (auto h: parent_cw->hostlist)
	{
		if (h->title() == title) {
			m_changing_host = true;
			pb_server_add->setText(tr("Change"));
			return;
		}
	}

	m_changing_host = true;
	pb_server_add->setText(tr("Add"));
}

// play the sound if check box has been clicked
void PreferencesDialog::on_soundButtonGroup_buttonClicked(QAbstractButton *cb)
{
	qDebug() << "button text = " << cb->text();

	if (cb->text() == tr("Stones"))
		setting->qgo->playClick();
	else if (cb->text() == tr("Pass"))
		setting->qgo->playPassSound();
	else if (cb->text() == tr("Autoplay"))
		setting->qgo->playAutoPlayClick();
	else if (cb->text().startsWith(tr("Time"), Qt::CaseInsensitive))
		setting->qgo->playTimeSound();
	else if (cb->text() == tr("Talk"))
		setting->qgo->playTalkSound();
	else if (cb->text() == tr("Say"))
		setting->qgo->playSaySound();
	else if (cb->text() == tr("Match"))
		setting->qgo->playMatchSound();
	else if (cb->text() == tr("Enter"))
		setting->qgo->playEnterSound();
	else if (cb->text() == tr("Game end"))
		setting->qgo->playGameEndSound();
	else if (cb->text() == tr("Leave"))
		setting->qgo->playLeaveSound();
	else if (cb->text() == tr("Disconnect"))
		setting->qgo->playDisConnectSound();
	else if (cb->text() == tr("Connect"))
		setting->qgo->playConnectSound();
}

void PreferencesDialog::slot_getComputerPath()
{
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Choose GTP engine path"), setting->readEntry("LAST_DIR"),
						      tr("All Files (*)")));
	if (fileName.isEmpty())
		return;

  	enginePath->setText(fileName);
}

void PreferencesDialog::slot_getGobanPicturePath()
{
#ifdef Q_OS_MACX
	// On the Mac, we want to default this dialog to the directory in the bundle
	// that contains the wood images we ship with the app
	//get the bundle path and find our resources
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	QString path = (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())
		+ "/Contents/Resources/Backgrounds";
#else
	QString path = setting->readEntry("LAST_DIR");
#endif
	QString old_name = LineEdit_goban->text ();
	if (!old_name.isEmpty ()) {
		QFileInfo info (old_name);
		if (info.exists ()) {
			QDir dir = info.dir ();
			path = dir.absolutePath ();
		}
	}
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Select a goban wood image"), path, tr("Images (*.png *.xpm *.jpg)")));
	if (fileName.isEmpty())
		return;

  	LineEdit_goban->setText(fileName);
}

void PreferencesDialog::slot_getTablePicturePath()
{
#ifdef Q_OS_MACX
	// On the Mac, we want to default this dialog to the directory in the bundle
	// that contains the wood images we ship with the app
	//get the bundle path and find our resources
	CFURLRef bundleRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleRef, kCFURLPOSIXPathStyle);
	QString path = (QString)CFStringGetCStringPtr(bundlePath, CFStringGetSystemEncoding())
		+ "/Contents/Resources/Backgrounds";
#else
	QString path = setting->readEntry("LAST_DIR");
#endif
	QString old_name = LineEdit_Table->text ();
	if (!old_name.isEmpty ()) {
		QFileInfo info (old_name);
		if (info.exists ()) {
			QDir dir = info.dir ();
			path = dir.absolutePath ();
		}
	}
	QString fileName(QFileDialog::getOpenFileName(this, tr ("Select a table background image"), path, tr("Images (*.png *.xpm *.jpg)")));
	if (fileName.isEmpty())
		return;

  	LineEdit_Table->setText(fileName);
}

void PreferencesDialog::slot_main_time_changed(int n)
{
	timeSpin_Nmatch->setMinimum(n);
}


void PreferencesDialog::slot_BY_time_changed(int n)
{
	BYSpin_Nmatch->setMinimum(n);
}

QColor PreferencesDialog::white_color ()
{
	return m_ih->white_color ();
}

QColor PreferencesDialog::black_color ()
{
	return m_ih->black_color ();
}
