/*
 * misctools.h
 */

#ifndef MISCTOOLS_H
#define MISCTOOLS_H
#include "scoretools_gui.h"

template<class type> class Q3PtrStack;
class Move;

class ScoreTools : public QWidget, public Ui::ScoreTools
{ 
	Q_OBJECT
		
public:
	ScoreTools( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 )
		: QWidget (parent)
	{
		setupUi(this);
	}
};

#include "normaltools_gui.h"

class NormalTools : public QWidget, public Ui::NormalTools
{ 
	Q_OBJECT
		
public:
	NormalTools( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 )
		: QWidget (parent, name)
	{
		setupUi(this);
	}
};

#endif // MISCTOOLS_H
