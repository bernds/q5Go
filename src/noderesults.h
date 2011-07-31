/*
* noderesults.h
*/

#ifndef NODERESULTS_H
#define NODERESULTS_H
#include "noderesults_gui.h"

template<class type> class QPtrStack;
class Move;

class NodeResults : public NodeResultsGUI
{ 
	Q_OBJECT
		
public:
	NodeResults( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
	~NodeResults();
	
	void setNodes(QPtrStack<Move> *nodes);
	
public slots:
	void fump(QIconViewItem*);
	void doHide();
	
signals:
	void doFump(Move*);
};

#endif // NODERESULTS_H
