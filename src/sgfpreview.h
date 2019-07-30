#ifndef SGFPREVIEW_H
#define SGFPREVIEW_H

#include <QStringList>

#include "ui_sgfpreview.h"

class QFileDialog;

class SGFPreview : public QDialog, public Ui::SGFPreview
{
	Q_OBJECT

	QFileDialog *fileDialog;
	go_game_ptr m_empty_game;
	go_game_ptr m_game;

	void setPath (QString path);
	void reloadPreview ();
	void clear ();

public:
	SGFPreview (QWidget * parent, const QString &dir);
	~SGFPreview ();
	virtual void accept () override;
	QStringList selected ();

	go_game_ptr selected_record () { return m_game; }
};

#endif
