#ifndef NORMALTOOLS_H
#define NORMALTOOLS_H
#include "ui_normaltools_gui.h"

class NormalTools : public QWidget, public Ui::NormalTools
{
	Q_OBJECT

public:
	NormalTools( QWidget* parent = 0)
		: QWidget (parent)
	{
		setupUi(this);
	}
};
#endif
