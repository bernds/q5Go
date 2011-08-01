/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "preferences_gui.h"
#include <q3ptrlist.h>
#include <Q3ListView>

class QIntValidator;
class MainWindow;
class ClientWindow;

class PreferencesDialog : public PreferencesDialogGui
{ 
	Q_OBJECT

public:
	PreferencesDialog(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0);
	~PreferencesDialog();

//	static QString fontToString(QFont f);

signals:
	void signal_addHost(const QString&, const QString&, unsigned int, const QString&, const QString&);
	void signal_delHost(const QString&);

public slots:
	virtual void slot_cbtitle(const QString&);
	virtual void slot_new();
	virtual void slot_add();
	virtual void slot_delete();
	virtual void slot_new_button();
	virtual void slot_add_button();
	virtual void slot_delete_button();
	virtual void slot_textChanged(const QString&);
	virtual void slot_text_buttonChanged(const QString&);
	virtual void slot_clickedListView(Q3ListViewItem*, const QPoint&, int);
	virtual void slot_clicked_buttonListView(Q3ListViewItem*, const QPoint&, int);
	virtual void slot_clickedSoundCheckBox(int boxID);
	virtual void slot_apply();
	virtual void startHelpMode();
	virtual void selectFont(int);
	virtual void selectColor();
	virtual void selectAltColor();
	virtual void slot_accept();
	virtual void slot_reject();
	virtual void slot_getComputerPath();
	virtual void slot_getPixmapPath();
	virtual void slot_getGobanPicturePath();
	virtual void slot_getTablePicturePath();
	virtual void slot_main_time_changed(int);
	virtual void slot_BY_time_changed(int);

private:
	void          saveSizes();
	ClientWindow  *parent_cw;
	QIntValidator *val;
	void          insertStandardHosts();
};

#endif
