#ifndef GREETERWINDOW_H
#define GREETERWINDOW_H

#include <QMainWindow>

namespace Ui
{
	class GreeterWindow;
};

class GreeterWindow : public QMainWindow {
	Q_OBJECT
	Ui::GreeterWindow *ui;

	void enable_buttons ();
public:
	GreeterWindow (QWidget *);
	~GreeterWindow ();
};
#endif
