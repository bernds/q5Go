/*
* interfacehandler.cpp
*/

#include <qaction.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <q3textedit.h>
#include <q3buttongroup.h>
#include <qlineedit.h>
#include <qslider.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <QPixmap>

#include "defines.h"
#include "interfacehandler.h"
#include "board.h"
#include "mainwidget.h"
#include "icons.h"
#include "move.h"
#include "miscdialogs.h"

//#ifdef USE_XPM
#include ICON_NODE_BLACK
#include ICON_NODE_WHITE
//#endif

struct ButtonState
{
    bool navPrevVar, navNextVar, navBackward, navForward, navFirst, navStartVar, navMainBranch,
		navLast, navNextBranch, navPrevComment, navNextComment, navIntersection; // SL added eb 11
};

InterfaceHandler::InterfaceHandler()
{
    buttonState = new ButtonState;
    scored_flag = false;
}

InterfaceHandler::~InterfaceHandler()
{
    delete buttonState;
}

void InterfaceHandler::setMoveData(int n, bool black, int brothers, int sons, bool hasParent,
								   bool hasPrev, bool hasNext, int lastX, int lastY)
{
    QString s(QObject::tr("Move") + " ");
    s.append(QString::number(n));
    if (lastX >= 1 && lastX <= board->getBoardSize() && lastY >= 1 && lastY <= board->getBoardSize())
    {
		s.append(" (");
		s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
		s.append(QString(QChar(static_cast<const char>('A' + (lastX<9?lastX:lastX+1) - 1))) +
			QString::number(board->getBoardSize()-lastY+1) + ")");
    }
    else if (lastX == 20 && lastY == 20)  // Pass
    {
		s.append(" (");
		s.append(black ? QObject::tr("W")+" " : QObject::tr("B")+" ");
		s.append(" " + QObject::tr("Pass") + ")");
    }
    s += "\n";
    s.append(QString::number(brothers));
    if (brothers == 1)
		s.append(" " + QObject::tr("brother") + "\n");
    else
		s.append(" " + QObject::tr("brothers") + "\n");
    s.append(QString::number(sons));
    if (sons == 1)
		s.append(" " + QObject::tr("son"));
    else
		s.append(" " + QObject::tr("sons"));

    moveNumLabel->setText(s);
    statusTurn->setText(" " + s.right(s.length() - 5) + " ");  // Without 'Move '
	
    statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
    
    s = black ? QObject::tr("Black to play") : QObject::tr("White to play");
    turnLabel->setText(s);

	GameMode mode = board->getGameMode ();
	if (mode == modeNormal || mode == modeEdit)
	{
		// Update the toolbar buttons
		navPrevVar->setEnabled(hasPrev);
		navNextVar->setEnabled(hasNext);
		navBackward->setEnabled(hasParent);
		navForward->setEnabled(sons);
		navFirst->setEnabled(hasParent);
		navStartVar->setEnabled(hasParent);
		navMainBranch->setEnabled(hasParent);
		navLast->setEnabled(sons);
		mainWidget->goPrevButton->setEnabled(hasParent);
		mainWidget->goNextButton->setEnabled(sons);
		mainWidget->goFirstButton->setEnabled(hasParent);
		mainWidget->goLastButton->setEnabled(sons);
		navNextBranch->setEnabled(sons);
		navSwapVariations->setEnabled(hasPrev);
		navPrevComment->setEnabled(hasParent);
		navNextComment->setEnabled(sons);
    		navIntersection->setEnabled(true); //SL added eb 11
		
		slider->setEnabled(true);
	}
	else  if (mode == modeObserve)  // add eb 8
	{
		// Update the toolbar buttons
		navBackward->setEnabled(hasParent);
		navForward->setEnabled(sons);
		navFirst->setEnabled(hasParent);
		navLast->setEnabled(sons);
		mainWidget->goPrevButton->setEnabled(hasParent);
		mainWidget->goNextButton->setEnabled(sons);
		mainWidget->goFirstButton->setEnabled(hasParent);
		mainWidget->goLastButton->setEnabled(sons);
		navPrevComment->setEnabled(hasParent);
		navNextComment->setEnabled(sons);
		navIntersection->setEnabled(true);  //SL added eb 11

		slider->setEnabled(true);

		board->getBoardHandler()->display_incoming_move = !bool(sons);            //SL added eb 9 - This is used to know whether we are browsing through a game or at the last move

	}
	else                                               //end add eb 8
		slider->setDisabled(true);
	
    // Update slider
  mainWidget->toggleSliderSignal(false);

  int mv = slider->maxValue();                            // add eb 8
  int v = slider->value();
    
  if (slider->maxValue() < n)
		  setSliderMax(n);
                                                                  // we need to be carefull with the slider
  if (mode != modeObserve ||                    // normal case, slider is moved
    (mode == modeObserve && mv >= n) ||       // observing, but browsing (no incoming move)
    (mode == modeObserve && mv < n && v==n-1))// observing, but at the last move, and an incoming move occurs 
          slider->setValue(n);
          
                                                             // end add eb 8
      
    mainWidget->toggleSliderSignal(true);
}

// clear the big field (offline)
void InterfaceHandler::clearComment()
{
	commentEdit->clear();
}

// display text
void InterfaceHandler::displayComment(const QString &c)
{
	if (board->get_isLocalGame())
	{
		if (c.isEmpty())
			commentEdit->clear();
		else
			commentEdit->setText(c);
	}
	else if (!c.isEmpty())
			commentEdit->append(c);
}

// get the comment of commentEdit - the multiline field
QString InterfaceHandler::getComment()
{
	return commentEdit->text();
}

// get the comment of commentEdit2 - the single line
QString InterfaceHandler::getComment2()
{
	QString text = commentEdit2->text();
	
	// clear entry
	commentEdit2->setText("");
	
	// don't show short text
	if (text.length() < 1)
		return 0;
	
	return text;
}

void InterfaceHandler::setMarkType(int m)
{
    MarkType t;
    QString txt;
	
    switch(m)
    {
    case 0:
		t = markNone;
		break;
		
    case 1:
		t = markSquare;
		break;
		
    case 2:
		t = markCircle;
		break;
		
    case 3:
		t = markTriangle;
		break;
		
    case 4:
		t = markCross;
		break;
		
    case 5:
		t = markText;
		break;
		
    case 6:
		t = markNumber;
		break;

	case 7:
	{
		Move *current = board->getBoardHandler()->getTree()->getCurrent();
		// set next move's color
		if (board->getBoardHandler()->getBlackTurn())
		{
			current->setPLinfo(stoneWhite);
			mainWidget->colorButton->setChecked (true);
		}
		else
		{
			current->setPLinfo(stoneBlack);
			mainWidget->colorButton->setChecked (false);
		}

		// check if set color is natural color:
		if (current->getMoveNumber() == 0 && current->getPLnextMove() == stoneBlack ||
			current->getMoveNumber() > 0 && current->getColor() != current->getPLnextMove())
			current->clearPLinfo();

		board->setCurStoneColor();
		return;
	}

    default:
		return;
    }

    board->setMarkType(t);
}

void InterfaceHandler::clearData()
{
	clearComment();
	setMoveData(0, true, 0, 0, false, false, false);
//    modeButton->setOn(false);
	mainWidget->setToolsTabWidget(tabNormalScore);
	mainWidget->editButtonGroup->setButton(0);
//    editTools->hide();
	normalTools->capturesBlack->setText("0");
	normalTools->capturesWhite->setText("0");

    if (board->getGameMode() != modeObserve && 
		board->getGameMode() != modeMatch &&
		board->getGameMode() != modeTeach)
    {
		normalTools->pb_timeBlack->setText("00:00");
		normalTools->pb_timeWhite->setText("00:00");
    }
    normalTools->show();
    scoreButton->setOn(false);
    slider->setValue(0);
    setSliderMax(SLIDER_INIT);
    scored_flag = false;
}

void InterfaceHandler::toggleSidebar(bool toggle)
{
    if (!toggle)
		toolsFrame->hide();
    else
		toolsFrame->show();
}

QString InterfaceHandler::getTextLabelInput(QWidget *parent, const QString &oldText)
{
    TextEditDialog dlg(parent, QObject::tr("textedit"), true);
    dlg.textLineEdit->setFocus();
    if (!oldText.isNull() && !oldText.isEmpty())
		dlg.textLineEdit->setText(oldText);
    
    if (dlg.exec() == QDialog::Accepted)
		return dlg.textLineEdit->text();
    return NULL;
}

void InterfaceHandler::showEditGroup()
{
//    editTools->editButtonGroup->show();
//	mainWidget->setToolsTabWidget(tabEdit);
}

void InterfaceHandler::toggleMarks()
{
	if (board->getGameMode() == modeEdit)
		return;
//    if (!modeButton->isChecked())
//		return;

    int cur = board->getMarkType();
    cur ++;
    if (cur > 6)
		cur = 0;
    mainWidget->editButtonGroup->setButton(cur);
    setMarkType(cur);
}

void InterfaceHandler::setCaptures(float black, float white, bool /*scored*/)
{
#if 0
	if (scored && !scored_flag)
	{
		normalTools->capturesFrame->setTitle(QObject::tr("Points"));
		scored_flag = true;
	}
	else if (!scored && scored_flag)
	{
		normalTools->capturesFrame->setTitle(QObject::tr("Captures"));
		scored_flag = false;
	}
#endif
	capturesBlack->setText(QString::number(black));
	capturesWhite->setText(QString::number(white));
}

void InterfaceHandler::setTimes(const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	if (!btime.isEmpty())
	{
		if (bstones != QString("-1"))
			normalTools->pb_timeBlack->setText(btime + " / " + bstones);
		else
			normalTools->pb_timeBlack->setText(btime);
	}

	if (!wtime.isEmpty())
	{
		if (wstones != QString("-1"))
			normalTools->pb_timeWhite->setText(wtime + " / " + wstones);
		else
			normalTools->pb_timeWhite->setText(wtime);
	}
}

void InterfaceHandler::setTimes(bool isBlacksTurn, float time, int stones)
{
	QString strTime;
	int seconds = (int)time;
	bool neg = seconds < 0;
	if (neg)
		seconds = -seconds;

	int h = seconds / 3600;
	seconds -= h*3600;
	int m = seconds / 60;
	int s = seconds - m*60;

	QString sec;

	// prevailling 0 for seconds
	if ((h || m) && s < 10)
		sec = "0" + QString::number(s);
	else
		sec = QString::number(s);

	if (h)
	{
		QString min;

		// prevailling 0 for minutes
		if (h && m < 10)
			min = "0" + QString::number(m);
		else
			min = QString::number(m);

		strTime = (neg ? "-" : "") + QString::number(h) + ":" + min + ":" + sec;
	}
	else
		strTime = (neg ? "-" : "") + QString::number(m) + ":" + sec;

	if (isBlacksTurn)
		setTimes(strTime, QString::number(stones), 0, 0);
	else
		setTimes(0, 0, strTime, QString::number(stones));
}

void InterfaceHandler::disableToolbarButtons()
{
    CHECK_PTR(buttonState);
	
    buttonState->navPrevVar = navPrevVar->isEnabled();
    navPrevVar->setEnabled(false);
	
    buttonState->navNextVar = navNextVar->isEnabled();
    navNextVar->setEnabled(false);
	
    buttonState->navBackward = navBackward->isEnabled();
    navBackward->setEnabled(false);
    
    buttonState->navForward = navForward->isEnabled();
    navForward->setEnabled(false);
	
    buttonState->navFirst = navFirst->isEnabled();
    navFirst->setEnabled(false);
	
    buttonState->navStartVar = navStartVar->isEnabled();
    navStartVar->setEnabled(false);
	
    buttonState->navMainBranch = navMainBranch->isEnabled();
    navMainBranch->setEnabled(false);
	
    buttonState->navLast = navLast->isEnabled();
    navLast->setEnabled(false);
	
    buttonState->navNextBranch = navNextBranch->isEnabled();
    navNextBranch->setEnabled(false);
	
    buttonState->navPrevComment = navPrevComment->isEnabled();
	  navPrevComment->setEnabled(false);

    buttonState->navNextComment = navNextComment->isEnabled();
	  navNextComment->setEnabled(false);

    buttonState->navIntersection = navIntersection->isEnabled(); // added eb 111
	  navIntersection->setEnabled(false);                          // end add eb 11

    navNthMove->setEnabled(false);
    navAutoplay->setEnabled(false);
    editDelete->setEnabled(false);
    navSwapVariations->setEnabled(false);
    fileImportSgfClipB->setEnabled(false);
}

void InterfaceHandler::restoreToolbarButtons()
{
    CHECK_PTR(buttonState);
	
	navPrevVar->setEnabled(buttonState->navPrevVar);
	navNextVar->setEnabled(buttonState->navNextVar);
	navBackward->setEnabled(buttonState->navBackward);
	navForward->setEnabled(buttonState->navForward);
	navFirst->setEnabled(buttonState->navFirst);
	navStartVar->setEnabled(buttonState->navStartVar);
	navMainBranch->setEnabled(buttonState->navMainBranch);
	navLast->setEnabled(buttonState->navLast);
	navNextBranch->setEnabled(buttonState->navNextBranch);
	navPrevComment->setEnabled(buttonState->navPrevComment);
	navNextComment->setEnabled(buttonState->navNextComment);
	navIntersection->setEnabled(buttonState->navNextComment);  // SL added eb 11

	navNthMove->setEnabled(true);
	navAutoplay->setEnabled(true);
	editDelete->setEnabled(true);
	navSwapVariations->setEnabled(true);
	fileImportSgfClipB->setEnabled(true);
}

void InterfaceHandler::setScore(int terrB, int capB, int terrW, int capW, float komi)
{
	qDebug() << "setScore " << capW << " " << capB;
	scoreTools->komi->setText(QString::number(komi));
	scoreTools->terrWhite->setText(QString::number(terrW));
	scoreTools->capturesWhite->setText(QString::number(capW));
	scoreTools->totalWhite->setText(QString::number((float)terrW + (float)capW + komi));
	scoreTools->terrBlack->setText(QString::number(terrB));
	scoreTools->capturesBlack->setText(QString::number(capB));
	scoreTools->totalBlack->setText(QString::number(terrB + capB));

	double res = (float)terrW + (float)capW + komi - terrB - capB;
	if (res < 0)
		scoreTools->result->setText ("B+" + QString::number (-res));
	else if (res == 0)
		scoreTools->result->setText ("Jigo");
	else
		scoreTools->result->setText ("W+" + QString::number (res));
}

void InterfaceHandler::setSliderMax(int n)
{
    if (n < 0)
		n = 0;
    
    slider->setMaxValue(n);
    mainWidget->sliderRightLabel->setText(QString::number(n));
}

