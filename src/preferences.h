/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "preferences_gui.h"

class QIntValidator;
class MainWindow;
class ClientWindow;

class PreferencesDialog : public QDialog, public Ui::PreferencesDialogGui
{ 
	Q_OBJECT

public:
	PreferencesDialog(QWidget* parent = 0, const char* name = 0, bool modal = false);
	~PreferencesDialog();

//	static QString fontToString(QFont f);

signals:
	void signal_addHost(const QString&, const QString&, unsigned int, const QString&, const QString&);
	void signal_delHost(const QString&);

public slots:
	void on_soundButtonGroup_buttonClicked(QAbstractButton *);
	virtual void slot_cbtitle(const QString&);
	virtual void slot_new_server();
	virtual void slot_add_server();
	virtual void slot_delete_server();
	virtual void slot_apply();
	virtual void startHelpMode();
	virtual void selectFont(int);
	virtual void slot_accept();
	virtual void slot_reject();
	virtual void slot_textChanged (const QString &);
	virtual void slot_getComputerPath();
	virtual void slot_getGobanPicturePath();
	virtual void slot_getTablePicturePath();
	virtual void slot_main_time_changed(int);
	virtual void slot_BY_time_changed(int);
	virtual void slot_clickedHostList(QListWidgetItem *);

private:
	void          saveSizes();
	ClientWindow  *parent_cw;
	QIntValidator *val;
	void          insertStandardHosts();
};

#endif
