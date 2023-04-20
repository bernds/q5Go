#include "common.h"
#include <QAction>

namespace Ui
{
	class TipsDialog;
};

class TipsDialog : public QDialog
{
	Ui::TipsDialog *ui;
	int m_idx = 0;
	QAction *leftAction {};
	QAction *rightAction {};

	void set_tip ();
public:
	TipsDialog (QWidget *);
	~TipsDialog ();
};
