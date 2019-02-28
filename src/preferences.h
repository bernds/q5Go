/*
 * preferences.h
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "ui_preferences_gui.h"

#include <QStandardItemModel>

class QIntValidator;
class MainWindow;
class ClientWindow;
class ImageHandler;
class QGraphicsScene;
class QGraphicsPixmapItem;

class PreferencesDialog : public QDialog, public Ui::PreferencesDialogGui
{
	Q_OBJECT

	bool m_changing_engine = false;
	bool m_changing_host = false;

	ImageHandler *m_ih;
	QGraphicsPixmapItem *m_w_stone, *m_b_stone;
	QGraphicsScene *m_stone_canvas;
	int m_stone_size;

	QStandardItemModel m_dbpath_model;
	QStringList m_dbpaths;
	bool m_dbpaths_changed = false;
	QString m_last_added_dbdir;

	void init_from_settings ();

	bool avoid_losing_data ();
	void clear_engine ();
	void clear_host ();
	void update_board_image ();
	void update_w_stones ();
	void update_b_stones ();
	void update_stone_params ();

	void update_db_selection ();
	void update_dbpaths (const QStringList &);
public:
	PreferencesDialog(QWidget* parent = 0);
	~PreferencesDialog();

//	static QString fontToString(QFont f);
	QColor white_color ();
	QColor black_color ();

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
	void select_white_color (bool);
	void select_black_color (bool);
	void select_stone_look (bool);

	void slot_dbdir (bool);
	void slot_dbcfg (bool);
	void slot_dbrem (bool);
private:
	void          saveSizes();
	ClientWindow  *parent_cw;
	void          insertStandardHosts();
};

#endif
