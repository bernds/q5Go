#ifndef SCORETOOLS_H
#define SCORETOOLS_H

#include "ui_scoretools_gui.h"

class ScoreTools : public QWidget, public Ui::ScoreTools
{
	Q_OBJECT

public:
	ScoreTools( QWidget* parent = 0)
		: QWidget (parent)
	{
		setupUi(this);
	}
};
#endif
