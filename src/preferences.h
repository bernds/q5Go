/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "ui_preferences_gui.h"

class QIntValidator;
class MainWindow;
class ClientWindow;

class PreferencesDialog : public QDialog, public Ui::PreferencesDialogGui
{
	Q_OBJECT

	bool m_changing_engine = false;
	bool m_changing_host = false;

	bool avoid_losing_data ();
	void clear_engine ();
	void clear_host ();
public:
	PreferencesDialog(QWidget* parent = 0);
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
	virtual void slot_new_engine();
	virtual void slot_add_engine();
	virtual void slot_delete_engine();
	virtual void slot_apply();
	virtual void startHelpMode();
	virtual void selectFont(int);
	virtual void selectStandardFont() { selectFont (0); }
	virtual void selectMarksFont() { selectFont (1); }
	virtual void selectCommentsFont() { selectFont (2); }
	virtual void selectListsFont() { selectFont (3); }
	virtual void selectClocksFont() { selectFont (4); }
	virtual void selectConsoleFont() { selectFont (5); }
	virtual void slot_accept();
	virtual void slot_reject();
	virtual void slot_serverChanged (const QString &);
	virtual void slot_engineChanged (const QString &);
	virtual void slot_getComputerPath();
	virtual void slot_getGobanPicturePath();
	virtual void slot_getTablePicturePath();
	virtual void slot_main_time_changed(int);
	virtual void slot_BY_time_changed(int);
	virtual void slot_clickedHostList(QListWidgetItem *);
	virtual void slot_clickedEngines(QListWidgetItem *);

private:
	void          saveSizes();
	ClientWindow  *parent_cw;
	QIntValidator *val;
	void          insertStandardHosts();
};

#endif
