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
#include "textedit_gui.h"
#include "normaltools_gui.h"
#include "scoretools_gui.h"
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
		navLast, navNextBranch, navPrevComment, navNextComment, navIntersection, editPaste, editPasteBrother; // SL added eb 11
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

GameMode InterfaceHandler::toggleMode()
{
	GameMode mode = board->getGameMode();
	
	switch (mode)
	{
	case modeEdit:
		board->setMode(modeNormal);
//		modeButton->setEnabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabEnable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Score", "button label"));
		passButton->setEnabled(true);
		undoButton->setDisabled(true); // for later: undo button -> one step back
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setDisabled(true);
		commentEdit->setReadOnly(false);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("N", "Board status line: normal mode") + " ");
		statusMark->setText(" - ");
		return modeNormal;
		
	case modeNormal:
		board->setMode(modeEdit);
//		modeButton->setEnabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabEnable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setDisabled(true);
		scoreButton->setText(QObject::tr("Score", "button label"));
		passButton->setDisabled(true);
		undoButton->setDisabled(true);
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setDisabled(true);
		commentEdit->setReadOnly(false);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("E", "Board status line: edit mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeEdit;
		
	case modeObserve:
		board->setMode(modeObserve);
//		modeButton->setDisabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabDisable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Edit", "button label"));
		passButton->setDisabled(true);
		undoButton->setDisabled(true);
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setEnabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
    		editCut->setEnabled(false);
    		editDelete->setEnabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("O", "Board status line: observe mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeObserve;
		
	case modeMatch : 
		board->setMode(modeMatch);
//		modeButton->setDisabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabDisable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Edit", "button label"));
		passButton->setEnabled(true);
		passButton->setText(QObject::tr("Pass", "button label"));
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeMatch;

	case   modeComputer :           // added eb 12
		board->setMode(modeComputer);
//		modeButton->setDisabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabDisable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Edit", "button label"));
		passButton->setEnabled(true);
		passButton->setText(QObject::tr("Pass", "button label"));
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(false);
		refreshButton->setEnabled(false);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("P", "Board status line: play mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeComputer;               //end add eb 12
      		
	case modeTeach:
		board->setMode(modeTeach);
//		modeButton->setDisabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabDisable);
		mainWidget->setToolsTabWidget(tabTeachGameTree, tabEnable);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Edit", "button label"));
		passButton->setEnabled(true);
		passButton->setText(QObject::tr("Pass", "button label"));
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		commentEdit->setReadOnly(true);
		commentEdit2->setReadOnly(false);
		commentEdit2->setDisabled(false);
		fileNew->setEnabled(false);
		fileNewBoard->setEnabled(false);
		fileOpen->setEnabled(false);
		statusMode->setText(" " + QObject::tr("T", "Board status line: teach mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeTeach;
		
	case modeScore:
//		modeButton->setDisabled(true);
		mainWidget->setToolsTabWidget(tabEdit, tabDisable);
//		mainWidget->setToolsTabWidget(tabNormalScore);
		scoreButton->setEnabled(true);
		scoreButton->setText(QObject::tr("Score", "button label"));
		passButton->setDisabled(true);
		undoButton->setEnabled(true);
		resignButton->setDisabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		commentEdit->setReadOnly(true);
		//commentEdit2->setReadOnly(true);
		commentEdit2->setDisabled(true);
		statusMode->setText(" " + QObject::tr("S", "Board status line: score mode") + " ");
		statusMark->setText(getStatusMarkText(board->getMarkType()));
		return modeScore;
		
	default:
		return modeNormal;
	}
}

void InterfaceHandler::setEditMode()
{
//    modeButton->setOn(true);
	mainWidget->setToolsTabWidget(tabEdit);
//    normalTools->hide();
//    editTools->show();
    board->setMode(modeEdit);
    statusMode->setText(" " + QObject::tr("E", "Board status line: edit mode") + " ");
    statusMark->setText(getStatusMarkText(board->getMarkType()));
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
    
    moveNumLabel->setText(s);
    statusTurn->setText(" " + s.right(s.length() - 5) + " ");  // Without 'Move '
	
    statusNav->setText(" " + QString::number(brothers) + "/" + QString::number(sons));
    
    s = black ? QObject::tr("Black to play") : QObject::tr("White to play");
    turnLabel->setText(s);
	
    s = "";
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
    varLabel->setText(s);
	
	if (board->getGameMode() == modeNormal || board->getGameMode() == modeEdit)
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
		navNextBranch->setEnabled(sons);
		navSwapVariations->setEnabled(hasPrev);
		navPrevComment->setEnabled(hasParent);
		navNextComment->setEnabled(sons);
    		navIntersection->setEnabled(true); //SL added eb 11
		
		slider->setEnabled(true);
	}
	else  if (board->getGameMode() == modeObserve)  // add eb 8
	{
		// Update the toolbar buttons
		navBackward->setEnabled(hasParent);
		navForward->setEnabled(sons);
		navFirst->setEnabled(hasParent);
		navLast->setEnabled(sons);
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
  if (board->getGameMode() != modeObserve ||                    // normal case, slider is moved
    (board->getGameMode() == modeObserve && mv >= n) ||       // observing, but browsing (no incoming move)
    (board->getGameMode() == modeObserve && mv < n && v==n-1))// observing, but at the last move, and an incoming move occurs 
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
	
    statusMark->setText(getStatusMarkText(t));
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
    editPaste->setEnabled(false);
    editPasteBrother->setEnabled(false);
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
//    if (!modeButton->isOn())
//		return;
	
    int cur = board->getMarkType();
    cur ++;
    if (cur > 6)
		cur = 0;
    mainWidget->editButtonGroup->setButton(cur);
    setMarkType(cur);
}

const QString InterfaceHandler::getStatusMarkText(MarkType t)
{
    QString txt;
    
    switch(t)
    {
    case markNone:
		txt = " S ";
		break;
		
    case markSquare:
		txt = " Q ";
		break;
		
    case markCircle:
		txt = " C ";
		break;
		
    case markTriangle:
		txt = " T ";
		break;
		
    case markCross:
		txt = " X ";
		break;
		
    case markText:
		txt = " A ";
		break;
		
    case markNumber:
		txt = " 1 ";
		break;
		
    default:
		txt = " ? ";
    }
    
    return txt;
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

    buttonState->editPaste = editPaste->isEnabled();
    editPaste->setEnabled(false);
	
    buttonState->editPasteBrother = editPasteBrother->isEnabled();
    editPasteBrother->setEnabled(false);
    
    navNthMove->setEnabled(false);
    navAutoplay->setEnabled(false);
    editCut->setEnabled(false);
    editDelete->setEnabled(false);
    navEmptyBranch->setEnabled(false);
    navCloneNode->setEnabled(false);
    navSwapVariations->setEnabled(false);
    fileImportASCII->setEnabled(false);
    fileImportASCIIClipB->setEnabled(false);
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
	editPaste->setEnabled(buttonState->editPaste);
	editPasteBrother->setEnabled(buttonState->editPasteBrother);
	
	navNthMove->setEnabled(true);
	navAutoplay->setEnabled(true);
	editCut->setEnabled(true);
	editDelete->setEnabled(true);
	navEmptyBranch->setEnabled(true);
	navCloneNode->setEnabled(true);
	navSwapVariations->setEnabled(true);
	fileImportASCII->setEnabled(true);
	fileImportASCIIClipB->setEnabled(true);
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
}

void InterfaceHandler::setClipboard(bool b)
{
    if (b)  // Clipboard filled
    {
		editPaste->setEnabled(true);
		editPasteBrother->setEnabled(true);
    }
    else    // Clipboard filled
    {
		editPaste->setEnabled(false);
		editPasteBrother->setEnabled(false);
    }
}

void InterfaceHandler::setSliderMax(int n)
{
    if (n < 0)
		n = 0;
    
    slider->setMaxValue(n);
    mainWidget->sliderRightLabel->setText(QString::number(n));
}

