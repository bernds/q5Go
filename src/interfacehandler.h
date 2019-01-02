/*
* interfacehandler.h
*/

#ifndef INTERFACEHANDLER_H
#define INTERFACEHANDLER_H

#include "defines.h"
#include "board.h"
//Added by qt3to4:
#include <QLabel>
class QLabel;
class QLineEdit;
class Board;
class QWidget;
class QString;
class QPushButton;
class QSlider;
class NormalTools;
class ScoreTools;
class MainWidget;
struct ButtonState;


class InterfaceHandler
{
public:
	InterfaceHandler();
	~InterfaceHandler();
	void clearData();
	void setMarkType(int m);
	void setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
		bool hasPrev, bool hasNext, int lastX=-1, int lastY=-1);
	void setCaptures(float black, float white, bool scored=false);
	void setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones);
	void setTimes(bool, float, int);
	void clearComment();
	void displayComment(const QString &c);
	QString getComment();
	QString getComment2();
	void toggleSidebar(bool toggle);
	QString getTextLabelInput(QWidget *parent, const QString &oldText);
	void showEditGroup();
	void toggleMarks();
	void disableToolbarButtons();
	void restoreToolbarButtons();
	void setScore(int terrB, int capB, int terrW, int capW, float komi=0);
	void setClipboard(bool b);
	void setSliderMax(int n);

	QLabel *moveNumLabel, *turnLabel, *capturesBlack, *capturesWhite;
	QAction *navBackward,  *navForward, *navFirst, *navLast, *navNextVar, *navIntersection, //SL added eb 11
		*navPrevVar, *navStartVar, *navNextBranch, *navMainBranch, *navNthMove, *navAutoplay,
		*editDelete,
	        *navSwapVariations, *navPrevComment, *navNextComment,
 		*fileImportSgfClipB, *fileNew, *fileNewBoard, *fileOpen;
	QTextEdit *commentEdit;
	QLineEdit *commentEdit2;
//	EditTools *editTools;
	NormalTools *normalTools;
//	TeachTools *teachTools;
	ScoreTools *scoreTools;
	QFrame *toolsFrame;
	Board *board;
	QLabel *statusTurn, *statusNav;
	QPushButton /**modeButton,*/ *scoreButton, *passButton, *undoButton, *resignButton, 
		*adjournButton, *refreshButton;
	ButtonState *buttonState;
	QSlider *slider;
	MainWidget *mainWidget;
	bool scored_flag;
//  bool display_incoming_move;     //SL added eb 9

//  bool getDisplay_incoming_move() { return display_incoming_move;} ;  //SL added eb 9
};

#endif
