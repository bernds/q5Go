#ifndef SGFPREVIEW_H
#define SGFPREVIEW_H

#include <QStringList>

#include "ui_sgfpreview.h"

class QFileDialog;

class SGFPreview : public QDialog, public Ui::SGFPreview
{
	Q_OBJECT

	QFileDialog *fileDialog;
	game_state m_empty_board;
	std::shared_ptr<game_record> m_empty_game;
	std::shared_ptr<game_record> m_game;

	void setPath (QString path);
	void reloadPreview ();
	void clear ();

public:
	SGFPreview (QWidget * parent, const QString &dir);
	~SGFPreview ();
	virtual void accept () override;
	QStringList selected ();

	std::shared_ptr<game_record> selected_record () { return m_game; }
};

#endif
