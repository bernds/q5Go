#ifndef SGFPREVIEW_H
#define SGFPREVIEW_H

#include <QStringList>
#include <QDialog>

#include <gogame.h>

class QFileDialog;

namespace Ui
{
	class SGFPreview;
};

class SGFPreview : public QDialog
{
	Q_OBJECT

	std::unique_ptr<Ui::SGFPreview> ui;

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
