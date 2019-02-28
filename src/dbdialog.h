#ifndef DBDIALOG_H
#define DBDIALOG_H

#include <QStringList>
#include <QAbstractItemModel>

#include "ui_dbdialog_gui.h"

class DBDialog : public QDialog, public Ui::DBDialog
{
	Q_OBJECT

	std::shared_ptr<game_record> m_empty_game;
	std::shared_ptr<game_record> m_game;
	game_state *m_last_move;

	class db_model;
	struct entry
	{
		friend class db_model;
		QString filename;
		QString pw, pb;
		QString date, result, event;

		entry (const QString &f, const QString &w, const QString &b,
		       const QString &d, const QString &r, const QString &e)
			: filename (f), pw (w), pb (b), date (d), result (r), event (e)
		{
		}
		entry (entry &&other) = default;
		entry &operator =(entry &&other) = default;
	};
	class db_model : public QAbstractItemModel {
		std::vector<entry> m_all_entries;
		std::vector<size_t> m_entries;
	public:
		db_model ()
		{
		}
		void populate_list ();
		void apply_filter (const QString &p1, const QString &p2, const QString &event,
				   const QString &dtfrom, const QString &dtto);
		void reset_filters ();
		const entry &find (size_t) const;
		QString status_string () const;

		virtual QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
		QModelIndex index (int row, int col, const QModelIndex &parent = QModelIndex()) const override;
		QModelIndex parent (const QModelIndex &index ) const override;
		int rowCount (const QModelIndex &parent = QModelIndex()) const override;
		int columnCount (const QModelIndex &parent = QModelIndex()) const override;
		QVariant headerData (int section, Qt::Orientation orientation,
				     int role = Qt::DisplayRole) const override;

		// Qt::ItemFlags flags(const QModelIndex &index) const override;
	};
	db_model m_model;

	bool setPath (QString path);
	void clear_preview ();
	bool update_selection ();
	void handle_doubleclick ();
	void update_buttons ();

public slots:
	void clear_filters (bool);
	void apply_filters (bool);

public:
	DBDialog (QWidget *parent);
	~DBDialog ();
	void update_prefs ();

	virtual void accept () override;

	std::shared_ptr<game_record> selected_record () { return m_game; }
};

extern DBDialog *db_dialog;

#endif
