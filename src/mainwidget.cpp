/*
* mainwidget.cpp
*/

#include "qgo.h"
//Added by qt3to4:
#include <QPixmap>
#include "mainwidget.h"
#include "interfacehandler.h"
#include "defines.h"
#include "icons.h"
#include <q3buttongroup.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtabwidget.h>

//#ifdef USE_XPM
#include ICON_NODE_BLACK
#include ICON_NODE_WHITE
//#endif

/* 
*  Constructs a MainWidget which is a child of 'parent', with the 
*  name 'name' and widget flags set to 'f' 
*/
MainWidget::MainWidget(MainWindow *win, QWidget* parent,  const char* name, Qt::WFlags fl )
	: QWidget( parent, name, fl ), m_mainwin (win)
{
	setupUi(this);
	connect(toolsTabWidget,
		SIGNAL(currentChanged(QWidget*)),
		SLOT(slot_toolsTabChanged(QWidget*)));

	normalTools->show();

//	scoreTools = new ScoreTools(tab_ns, "scoreTools");
//	scoreTools->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
//	scoreTools->sizePolicy().hasHeightForWidth()));
//	scoreTools->setMinimumSize(QSize(80, 230));
	scoreTools->hide();
	
	interfaceHandler = new InterfaceHandler();
	
	interfaceHandler->moveNumLabel = moveNumLabel;
	interfaceHandler->turnLabel = turnLabel;
	interfaceHandler->varLabel = varLabel;
//	interfaceHandler->editTools = editTools;
//	interfaceHandler->modeButton = modeButton;
	interfaceHandler->passButton = passButton;
	interfaceHandler->scoreButton = scoreButton;
	interfaceHandler->undoButton = undoButton;
	interfaceHandler->adjournButton = adjournButton;
	interfaceHandler->resignButton = resignButton;
	interfaceHandler->refreshButton = refreshButton;
	interfaceHandler->capturesBlack = normalTools->capturesBlack;
	interfaceHandler->capturesWhite = normalTools->capturesWhite;
	interfaceHandler->normalTools = normalTools;
//	interfaceHandler->teachTools = teachTools;
	interfaceHandler->scoreTools = scoreTools;
	interfaceHandler->toolsFrame = toolsFrame;

	interfaceHandler->board = board;
	
//	connect(editTools->editButtonGroup, SIGNAL(clicked(int)), this, SLOT(setMarkType(int)));
	
	showSlider = true;
	toggleSlider(setting->readBoolEntry("SLIDER"));
	slider->setMaximum(SLIDER_INIT);
	sliderRightLabel->setText(QString::number(SLIDER_INIT));
	sliderSignalToggle = true;

	setFont(setting->fontStandard);
	normalTools->pb_timeWhite->setFont(setting->fontClocks);
	normalTools->pb_timeBlack->setFont(setting->fontClocks);
}

/*  
*  Destroys the object and frees any allocated resources
*/
MainWidget::~MainWidget()
{
	// no need to delete child widgets, Qt does it all for us
	delete interfaceHandler;
	delete scoreTools;
	delete normalTools;
}

// a tab has been clicked
void MainWidget::slot_toolsTabChanged(QWidget * /*w*/)
{
	switch (toolsTabWidget->currentPageIndex())
	{
		// normal/score tools
	case tabNormalScore:
		interfaceHandler->board->setMode (modeNormal);
		m_mainwin->setGameMode (modeNormal);
		break;

		// edit tools
	case tabEdit:
		// set color of next move
		if (interfaceHandler->board->getBoardHandler()->getBlackTurn())
		{
			colorButton->setChecked (false);
		}
		else
		{
			colorButton->setChecked (true);
		}

		interfaceHandler->board->setMode (modeEdit);
		m_mainwin->setGameMode (modeEdit);
		break;

		// teach tools + game tree
	case tabTeachGameTree:
		break;

	default:
		break;
	}
}

// set a tab on toolsTabWidget
void MainWidget::setToolsTabWidget(enum tabType p, enum tabState s)
{
	QWidget *w = NULL;

	switch (p)
	{
		case tabNormalScore:
			w = tab_ns;
			break;

		case tabEdit:
			w = tab_e;
			break;

		case tabTeachGameTree:
			w = tab_tg;
			break;

		default:
			return;
			break;
	}

	if (s == tabSet)
	{
		// check whether the page to switch to is enabled
		if (!toolsTabWidget->isEnabled())
			toolsTabWidget->setTabEnabled(w, true);

		toolsTabWidget->setCurrentPage(p);
	}
	else
	{
		// check whether the current page is to disable; then set to 'normal'
		if (s == tabDisable && toolsTabWidget->currentPageIndex() == p)
			toolsTabWidget->setCurrentPage(tabNormalScore);

		toolsTabWidget->setTabEnabled(w, s == tabEnable);
	}
}

/*
void MainWidget::toggleGameMode()
{
	if (toggleMode() != modeEdit)
	{
//		editTools->hide();
		normalTools->show();
//		if (modeButton->isChecked())
//			modeButton->setOn(false);
	}
	else
	{
		normalTools->hide();
//		editTools->show();
	}
}
*/
void MainWidget::setMarkType(int m)
{
	interfaceHandler->setMarkType(m);
}

void MainWidget::on_colorButton_clicked(bool checked)
{
	interfaceHandler->setMarkType(7);
}

void MainWidget::doPass()
{
	interfaceHandler->board->doPass();
}

void MainWidget::doCountDone()
{
	interfaceHandler->board->doCountDone();
}

void MainWidget::doResign()
{ 
	interfaceHandler->board->doResign(); 
}

void MainWidget::doRefresh()
{ 
	interfaceHandler->board->doRefresh(); 
}

void MainWidget::doUndo() 
{ 	
	interfaceHandler->board->doUndo(); 
}

void MainWidget::doAdjourn() 
{ 
	interfaceHandler->board->doAdjourn(); 
}

void MainWidget::doRealScore(bool toggle)
{
	static GameMode rememberMode;
	static int      rememberTab;

	qDebug("MainWidget::doRealScore()");
	if (toggle)
	{
		rememberMode = interfaceHandler->board->getGameMode();
		rememberTab = toolsTabWidget->currentPageIndex();
//		modeButton->setEnabled(false);
		setToolsTabWidget(tabEdit, tabDisable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		if (scoreButton->text() == QString(tr("Edit")))
		{
			passButton->hide ();
			doneButton->show ();
			scoreButton->setEnabled(false);

      			if (rememberMode==modeComputer)
      			{
        			adjournButton->setEnabled(false);
        			resignButton->setEnabled(false);
        			undoButton->setEnabled(false);
      			}
		} else
			passButton->setEnabled (false);

		interfaceHandler->disableToolbarButtons();
//		editTools->hide();
		normalTools->hide();
		scoreTools->show();
//		interfaceHandler->board->setMode(modeScore);
		interfaceHandler->board->getBoardHandler()->countScore();
	}
	else
	{
//		modeButton->setEnabled(true);
		setToolsTabWidget(tabEdit, tabEnable);
		setToolsTabWidget(tabTeachGameTree, tabEnable);
		if (scoreButton->text() == QString(tr("Edit")))
		{
			passButton->show ();
			doneButton->hide ();
			scoreButton->setEnabled(true);
		}
		else
			passButton->setEnabled(true);

		interfaceHandler->restoreToolbarButtons();
		scoreTools->hide();
		normalTools->show();
//		if (modeButton->isChecked())
//			editTools->show();
//		else
//			normalTools->show();
		interfaceHandler->board->setMode(rememberMode);
		setToolsTabWidget(static_cast<tabType>(rememberTab));
	}
}
	
void MainWidget::doEdit()
{
	qDebug("MainWidget::doEdit()");
	// online mode -> don't score, open new Window instead
	interfaceHandler->board->doEditBoardInNewWindow();
}

void MainWidget::sliderChanged(int n)
{
	if (sliderSignalToggle)
		interfaceHandler->board->gotoNthMoveInVar(n);
}

void MainWidget::toggleSlider(bool b)
{
	if (showSlider == b)
		return;
	
	showSlider = b;
	
	if (b)
	{
		slider->show();
		sliderLeftLabel->show();
		sliderRightLabel->show();
	}
	else
	{
		slider->hide();
		sliderLeftLabel->hide();
		sliderRightLabel->hide();
	}
}

// Overwritten from QWidget
void MainWidget::setFont(const QFont &font)
{
	QFont f(font);
	f.setBold(true);
	scoreTools->totalBlack->setFont(f);
	scoreTools->totalWhite->setFont(f);
	
	QWidget::setFont(font);
}

void MainWidget::setGameMode(GameMode mode)
{
	switch (mode) {
	case modeEdit:
//		modeButton->setEnabled(true);
		setToolsTabWidget(tabEdit, tabEnable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->show ();
		editButton->hide ();
		doneButton->hide ();
		passButton->setEnabled(true);
		undoButton->setDisabled(true); // for later: undo button -> one step back
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setDisabled(true);
		break;

	case modeNormal:
//		modeButton->setEnabled(true);
		setToolsTabWidget(tabEdit, tabEnable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(true);
		scoreButton->show ();
		editButton->hide ();
		doneButton->hide ();
		passButton->setEnabled(true);
		undoButton->setDisabled(true);
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setDisabled(true);
		break;

	case modeObserve:
//		modeButton->setDisabled(true);
		setToolsTabWidget(tabEdit, tabDisable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(false);
		scoreButton->hide ();
		editButton->show ();
		doneButton->hide ();
		passButton->setDisabled(true);
		undoButton->setDisabled(true);
		resignButton->setDisabled(true);
		adjournButton->setDisabled(true);
		refreshButton->setEnabled(true);
		break;

	case modeMatch:
//		modeButton->setDisabled(true);
		setToolsTabWidget(tabEdit, tabDisable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(false);
		scoreButton->hide ();
		editButton->show ();
		doneButton->hide ();
		passButton->setEnabled(true);
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		break;

	case modeComputer:           // added eb 12
//		modeButton->setDisabled(true);
		setToolsTabWidget(tabEdit, tabDisable);
		setToolsTabWidget(tabTeachGameTree, tabDisable);
		scoreButton->setEnabled(false);
		scoreButton->hide ();
		editButton->show ();
		doneButton->hide ();
		passButton->setEnabled(true);
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(false);
		refreshButton->setEnabled(false);
		break;

	case modeTeach:
//		modeButton->setDisabled(true);
		setToolsTabWidget(tabEdit, tabDisable);
		setToolsTabWidget(tabTeachGameTree, tabEnable);
		scoreButton->setEnabled(false);
		scoreButton->hide ();
		editButton->show ();
		doneButton->hide ();
		passButton->setEnabled(true);
		undoButton->setEnabled(true);
		resignButton->setEnabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		break;

	case modeScore:
//		modeButton->setDisabled(true);
		setToolsTabWidget(tabEdit, tabDisable);
//		setToolsTabWidget(tabNormalScore);
		scoreButton->setEnabled(true);
		scoreButton->hide ();
		editButton->hide ();
		doneButton->show ();
		passButton->setDisabled(true);
		undoButton->setEnabled(true);
		resignButton->setDisabled(true);
		adjournButton->setEnabled(true);
		refreshButton->setEnabled(true);
		break;
	}
}

