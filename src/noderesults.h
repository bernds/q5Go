/*
* noderesults.h
*/

#ifndef NODERESULTS_H
#define NODERESULTS_H
#include "noderesults_gui.h"

template<class type> class Q3PtrStack;
class Move;

class NodeResults : public QWidget, public Ui::NodeResultsGUI
{ 
	Q_OBJECT
		
public:
	NodeResults( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
	~NodeResults();
	
	void setNodes(Q3PtrStack<Move> *nodes);
	
public slots:
	void fump(Q3IconViewItem*);
	void doHide();
	
signals:
	void doFump(Move*);
};

#endif // NODERESULTS_H
