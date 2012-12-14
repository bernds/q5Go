/*
* preferences.cpp
*/

#include "preferences.h"
#include "mainwindow.h"
#include "qgo.h"
#include <QPixmap>
#include <Q3FileDialog>
#include <Q3WhatsThis>
#include "mainwin.h"

#ifdef Q_OS_MACX
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFBundle.h>
#endif //Q_OS_MACX

/* 
*  Constructs a PreferencesDialog which is a child of 'parent', with the 
*  name 'name' and widget flags set to 'f' 
*
*  The dialog will by default be modeless, unless you set 'modal' to
*  TRUE to construct a modal dialog.
*/
PreferencesDialog::PreferencesDialog(QWidget* parent,  const char* name, bool modal, Qt::WFlags fl)
	: QDialog ( parent, name, modal, fl )
{
	setupUi(this);
	ListView_buttons->setColumnWidth(4,0);

	// pointer to ClientWindow
	parent_cw = setting->cw;
	CHECK_PTR(parent_cw);

	if (parent_cw->getPrefSize().width() > 0)
	{
		resize(parent_cw->getPrefSize());
		move(parent_cw->getPrefPos());
	}

	// set hostlist
	//parent_cw->hostlist;

	// set connection titles to listview
	for (Host *h = parent_cw->hostlist.first(); h != 0; h = parent_cw->hostlist.next())
		new Q3ListViewItem(ListView_hosts,
			h->title(),
			h->host(),
			QString::number(h->port()),
			h->loginName(),
			(h->password().length() ? "***" : ""));
		//cb_title->insertItem(h->title());

	// set the user toolbar list 
	QPixmap p;
	Q3ListViewItem *lvi;
	QObjectList bl = parent_cw->UserToolbar->queryList( "QToolButton" ,NULL,true,false);
    
	QListIterator<QObject *> bli(bl);
	while (bli.hasNext ())
	{
		QToolButton *b0 = (QToolButton*)bli.next();
		lvi = new  Q3ListViewItem(ListView_buttons,
					  "", //first column is for pixmap
					  b0->textLabel(),
					  b0->caption(),
					  b0->toolTip(),
					  b0->iconText() );
		
		if (p.load(b0->iconText()))
			lvi->setPixmap(0,p);
	}
	// init random-number generator
	srand( (unsigned)time( NULL ) );

	insertStandardHosts();

	// set valid port range
	val = new QIntValidator(0, 9999, this);
	LineEdit_port->setValidator(val);

	// clear edit field
	LineEdit_title->clear();

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
  
	QPalette pal = colorBackgroundButton->palette();
	pal.setColor(QColorGroup::Button, setting->colorBackground);
	colorBackgroundButton->setPalette(pal);

	pal = colorAltBackgroundButton->palette();
	pal.setColor(QColorGroup::Button, setting->colorAltBackground);
	colorAltBackgroundButton->setPalette(pal);

#ifdef QGO_NOSTYLES
	styleListBox->setDisabled(true);
#endif

	// Default codec
	ComboBox_codec->insertItem("");
	// The rest by name
	QTextCodec *codec;
	for(int i = 0; (codec = QTextCodec::codecForIndex(i)); ++i)
		ComboBox_codec->insertItem(codec->name());
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
	((MainWindow*)parentWidget())->preferencesSave(this);
	((MainWindow*)parentWidget())->preferencesAccept();
}

void PreferencesDialog::startHelpMode()
{
	Q3WhatsThis::enterWhatsThisMode();
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

void PreferencesDialog::selectColor()
{
	// Open a font dialog to select a new font
	QColor c = QColorDialog::getColor(setting->colorBackground, this);
	if (c.isValid())  // Accepted
	{
		setting->colorBackground = c;

		// set button color
		QPalette pal = colorBackgroundButton->palette();
		pal.setColor(QColorGroup::Button, setting->colorBackground);
		colorBackgroundButton->setPalette(pal);
	}
}

void PreferencesDialog::selectAltColor()
{
	// Open a font dialog to select a new font
	QColor c = QColorDialog::getColor(setting->colorAltBackground, this);
	if (c.isValid())  // Accepted
	{
		setting->colorAltBackground = c;

		// set button color
		QPalette pal = colorAltBackgroundButton->palette();
		pal.setColor(QColorGroup::Button, setting->colorAltBackground);
		colorAltBackgroundButton->setPalette(pal);
	}
}

void PreferencesDialog::slot_accept()
{
	saveSizes();

	// save settings
	parent_cw->saveSettings();

	accept();
}

void PreferencesDialog::slot_reject()
{
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

void PreferencesDialog::insertStandardHosts()
{
	// standard hosts
	new Q3ListViewItem(ListView_hosts, "-- Aurora --");
	new Q3ListViewItem(ListView_hosts, "-- CTN --");
	new Q3ListViewItem(ListView_hosts, "-- CWS --");
	new Q3ListViewItem(ListView_hosts, "-- EGF --");
	new Q3ListViewItem(ListView_hosts, "-- IGS --");
	new Q3ListViewItem(ListView_hosts, "-- LGS --");
	new Q3ListViewItem(ListView_hosts, "-- NNGS --");
	new Q3ListViewItem(ListView_hosts, "-- WING --");
}

// button "add" clicked or "ok" pressed
void PreferencesDialog::slot_add()
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

		for (Host *h = parent_cw->hostlist.first(); !found && h != 0; h = parent_cw->hostlist.next())
		{
			if (h->title() == LineEdit_title->text())
			{
				found = true;
				// if found, insert at current pos, and remove old item
				parent_cw->hostlist.remove();
			}
		}
		
		// insert host at it's sorted position
		parent_cw->hostlist.inSort(new Host(LineEdit_title->text(),
				LineEdit_host->text(),
				tmp,
				LineEdit_login->text(),
				LineEdit_pass->text(),
				ComboBox_codec->currentText()));
		// create entry in listview
		if (!found)
			new Q3ListViewItem(ListView_hosts,
				LineEdit_title->text(),
				LineEdit_host->text(),
				QString::number(tmp),
				LineEdit_login->text(),
				(LineEdit_pass->text().length() ? "***" : ""));
		else
		{
			ListView_hosts->currentItem()->setText(0, LineEdit_title->text());
			ListView_hosts->currentItem()->setText(1, LineEdit_host->text());
			ListView_hosts->currentItem()->setText(2, QString::number(tmp));
			ListView_hosts->currentItem()->setText(3, LineEdit_login->text());
			ListView_hosts->currentItem()->setText(4, (LineEdit_pass->text().length() ? "***" : ""));
		}
		ListView_hosts->repaint();
//			cb_title->insertItem(LineEdit_title->text(), 0);

		// add to ClientWindow hostlist       !!! does not seem to be used !
		emit signal_addHost(LineEdit_title->text(),
		                    LineEdit_host->text(),
		                    tmp,
		                    LineEdit_login->text(),
		                    LineEdit_pass->text());

	}

	// init insertion fields
	slot_cbtitle(QString());
}

// button "delete" clicked
void PreferencesDialog::slot_delete()
{
	bool found = false;
	Host *h;
	for (h = parent_cw->hostlist.first(); !found && h != 0; h = parent_cw->hostlist.next())
	{
		if (h->title() == LineEdit_title->text())
		{
			found = true;
			// if found, delete current entry
			parent_cw->hostlist.remove();
			emit signal_delHost(LineEdit_title->text());
		}
	}

	// set connection titles to listview
	Q3ListViewItemIterator lv(ListView_hosts);
	for (Q3ListViewItem *lvi; (lvi = lv.current());)
	{
		lv++;
		delete lvi;
	}

	// clear entries
	slot_cbtitle(QString());

	for (h = parent_cw->hostlist.first(); h != 0; h = parent_cw->hostlist.next())
		new Q3ListViewItem(ListView_hosts,
			h->title(),
			h->host(),
			QString::number(h->port()),
			h->loginName(),
			(h->password().length() ? "***" : ""));

	insertStandardHosts();
}

void PreferencesDialog::slot_new()
{
	slot_cbtitle(QString());
}

void PreferencesDialog::slot_clickedListView(Q3ListViewItem *lvi, const QPoint&, int)
{
	if (!lvi)
		return;

	slot_cbtitle(lvi->text(0));
}

void PreferencesDialog::slot_cbtitle(const QString &txt)
{
	LineEdit_pass->clear();
	if (txt.isEmpty() || txt.isNull())
	{
		LineEdit_title->clear();
		LineEdit_host->clear();
		LineEdit_port->clear();
		LineEdit_login->clear();
		ComboBox_codec->setCurrentText("");
	}
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
	else if (!txt.isEmpty())
	{
		// fill host info of selected title
		Host *h = parent_cw->hostlist.first();
		bool found = false;
		while (h != 0)
		{
			if (h->title() == txt)
			{
				LineEdit_title->setText(h->title());
				LineEdit_host->setText(h->host());
				LineEdit_port->setText(QString::number(h->port()));
				LineEdit_login->setText(h->loginName());
				LineEdit_pass->setText(h->password());
				ComboBox_codec->setCurrentText(h->codec());
				found = true;
			}

			h = parent_cw->hostlist.next();
		}
	}
}

void PreferencesDialog::slot_textChanged(const QString &title)
{
	bool found = false;
	Host *h;
	for (h = parent_cw->hostlist.first(); !found && h != 0; h = parent_cw->hostlist.next())
	{
		if (h->title() == title)
			found = true;
	}

	if (found)
		pb_add->setText(tr("Change"));
	else
		pb_add->setText(tr("Add"));
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
	else if (cb->text().startsWith(tr("Time"),false))
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
	QString fileName(Q3FileDialog::getOpenFileName(setting->readEntry("LAST_DIR"),
		tr("All Files (*)"), this));
	if (fileName.isEmpty())
		return;

  	LineEdit_computer->setText(fileName);
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
	QString fileName(Q3FileDialog::getOpenFileName(path,
		tr("Images (*.png *.xpm *.jpg)"), this));
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
	QString fileName(Q3FileDialog::getOpenFileName(path,
		tr("Images (*.png *.xpm *.jpg)"), this));
	if (fileName.isEmpty())
		return;

  	LineEdit_Table->setText(fileName);
}

void PreferencesDialog::slot_getPixmapPath()
{
	QString fileName(Q3FileDialog::getOpenFileName("",tr("Images (*.png *.jpg *.xpm *.ico)"), this));
	if (fileName.isEmpty())
		return;

	LineEdit_pixfile->setText(fileName);

	QPixmap p;
	p.load(fileName);
  
	pixmapLabel->setPixmap(p);
}


void PreferencesDialog::slot_text_buttonChanged(const QString &title)
{
	if (ListView_buttons->findItem(title,1))
		pb_add_2->setText(tr("Change"));
	else
		pb_add_2->setText(tr("Add"));
}

void PreferencesDialog::slot_new_button()
{
		LineEdit_label->clear();
		LineEdit_help->clear();
		LineEdit_pixfile->clear();
		LineEdit_command->clear();
    pixmapLabel->setPixmap(NULL);
}

// button "add" clicked or "ok" pressed
void PreferencesDialog::slot_add_button()
{
	QObjectList bl = parent_cw->UserToolbar->queryList( "QToolButton" ,NULL,true,false);
	bool found = false;
	// check if at least title and host inserted
	if (!LineEdit_label->text().isEmpty() && !LineEdit_command->text().isEmpty())
	{
	// check if label already exists

		QListIterator<QObject *> bli(bl);
		while (bli.hasNext ())
		{
			QToolButton *b0 = (QToolButton*)bli.next();
      			if (b0->textLabel() == LineEdit_label->text())
			{
				found = true;
				// if found, remove old item
				parent_cw->userButtonGroup->remove(b0);
				delete b0;
			}  
		}

		QPixmap p;

		QToolButton *b = new QToolButton(parent_cw->UserToolbar) ;
		b->setTextLabel(LineEdit_label->text());   //label of the button
		b->setText(LineEdit_label->text());
		b->setCaption(LineEdit_command->text()); //dirty but handy
		b->setIconText(LineEdit_pixfile->text()); //dirty also but handy
		QToolTip::add(b,LineEdit_help->text());
		b->setMinimumWidth(25);

		if ( p.load(LineEdit_pixfile->text()))
			b->setPixmap(p);
  
		parent_cw->userButtonGroup->insert(b,-1);

		if (!found)
		{
			Q3ListViewItem *buttonItem = new Q3ListViewItem(ListView_buttons,
					"",
					LineEdit_label->text(),
					LineEdit_command->text(),
					LineEdit_help->text(),
			LineEdit_pixfile->text());   
			buttonItem->setPixmap(0,p);
		}
		else
		{
			ListView_buttons->currentItem()->setText(1, LineEdit_label->text());
			ListView_buttons->currentItem()->setText(2, LineEdit_command->text());
			ListView_buttons->currentItem()->setText(3, LineEdit_help->text());
			ListView_buttons->currentItem()->setText(4, LineEdit_pixfile->text());
			ListView_buttons->currentItem()->setPixmap(0,p);
		}
	}
  	slot_new_button();    
}


void PreferencesDialog::slot_clicked_buttonListView(Q3ListViewItem *lvi, const QPoint&, int)
{
	if (!lvi)
		return;
	LineEdit_label->setText(lvi->text(1));
	LineEdit_command->setText(lvi->text(2));
	LineEdit_help->setText(lvi->text(3));
	LineEdit_pixfile->setText(lvi->text(4));
  

	QPixmap p;
	p.load(LineEdit_pixfile->text());

	pixmapLabel->setPixmap(p);
}


void PreferencesDialog::slot_delete_button()
{
	QObjectList bl = parent_cw->UserToolbar->queryList( "QToolButton" ,NULL,true,false);
	bool found=false;
	
	// check if at least title and host inserted
	if (!LineEdit_label->text().isEmpty() )//&& !LineEdit_command->text().isEmpty())
	{
		// check if label already exists
		
		Q3ListViewItem *lvi = ListView_buttons->findItem(LineEdit_label->text(),1);
		
		// if found, remove old item and clear fields       
		if(lvi)
		{
			QListIterator<QObject *> bli(bl);
			while (bli.hasNext ())
			{
				QToolButton *b0 = (QToolButton*)bli.next();
				//QString sb= b0->text();
				//QString sl = LineEdit_label->text();
				if (b0->textLabel() == LineEdit_label->text())
				{
					found = true;
					// if found, remove old item
					parent_cw->userButtonGroup->remove(b0);
					delete b0;
				}
			}
			
			ListView_buttons->takeItem(lvi);
			slot_new_button();
		}
	}
	
}


void PreferencesDialog::slot_main_time_changed(int n)
{
	timeSpin_Nmatch->setMinValue(n);
}


void PreferencesDialog::slot_BY_time_changed(int n)
{
	BYSpin_Nmatch->setMinValue(n);
}
