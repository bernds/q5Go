/*
* mainwidget.cpp
*/

#include "qgo.h"
//Added by qt3to4:
#include <QPixmap>
#include "mainwidget.h"
#include "interfacehandler.h"
#include "normaltools_gui.h"
#include "scoretools_gui.h"
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
MainWidget::MainWidget(QWidget* parent,  const char* name, Qt::WFlags fl )
: QWidget( parent, name, fl )
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
	slider->setMaxValue(SLIDER_INIT);
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
	static bool rememberEditTab = false;

	if (rememberEditTab)
	{
		// edit tab has been released
		rememberEditTab = false;
		interfaceHandler->toggleMode();
	}

	switch (toolsTabWidget->currentPageIndex())
	{
		// normal/score tools
		case tabNormalScore:
			if (interfaceHandler->board->getGameMode() == modeEdit)
			{
				// sholdn't be, but however...
				rememberEditTab = false;
				interfaceHandler->toggleMode();
			}
			break;

		// edit tools
		case tabEdit:
			// set color of next move
			if (interfaceHandler->board->getBoardHandler()->getBlackTurn())
			{
//#ifndef USE_XPM
//				colorButton->setPixmap(QPixmap(ICON_NODE_BLACK));
//#else
				colorButton->setPixmap(QPixmap(const_cast<const char**>(node_black_xpm)));
//#endif
			}
			else
			{
//#ifndef USE_XPM
//				colorButton->setPixmap(QPixmap(ICON_NODE_WHITE));
//#else
				colorButton->setPixmap(QPixmap(const_cast<const char**>(node_white_xpm)));
//#endif
			}

			interfaceHandler->toggleMode();
			rememberEditTab = true;
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
	if (interfaceHandler->toggleMode() != modeEdit)
	{
//		editTools->hide();
		normalTools->show();
//		if (modeButton->isOn())
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

void MainWidget::doPass()
{
	if (interfaceHandler->board->getGameMode() == modeScore)
	{
		interfaceHandler->board->doCountDone();
/*		if (scoreButton->text() != QString(tr("Edit")))
		{
			scoreButton->toggle();
		}*/
	}
	else
		interfaceHandler->board->doPass();
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
			passButton->setText(tr("Done"));
			scoreButton->setEnabled(false);

      			if (rememberMode==modeComputer)
      			{
        			adjournButton->setEnabled(false);
        			resignButton->setEnabled(false);
        			undoButton->setEnabled(false);
      			}      
		}
    
		else
			passButton->setEnabled(false);
			
		interfaceHandler->disableToolbarButtons();
//		editTools->hide();
		normalTools->hide();
		scoreTools->show();
//		interfaceHandler->board->setMode(modeScore);
		interfaceHandler->board->countScore();
	}
	else
	{
//		modeButton->setEnabled(true);
		setToolsTabWidget(tabEdit, tabEnable);
		setToolsTabWidget(tabTeachGameTree, tabEnable);
		if (scoreButton->text() == QString(tr("Edit")))
		{
			passButton->setText(tr("Pass"));
			scoreButton->setEnabled(true);
		}
		else
			passButton->setEnabled(true);
			
		interfaceHandler->restoreToolbarButtons();
		scoreTools->hide();
		normalTools->show();
//		if (modeButton->isOn())
//			editTools->show();
//		else
//			normalTools->show();
		interfaceHandler->board->setMode(rememberMode);
		setToolsTabWidget(static_cast<tabType>(rememberTab));
	}
}
	
void MainWidget::doScore(bool toggle)
{
	static bool     skipNextSignal;

qDebug("MainWidget::doScore()");
	if (scoreButton->text() == QString(tr("Edit")))
	{
		if (scoreButton->isOn())
		{
			// online mode -> don't score, open new Window instead
			interfaceHandler->board->doEditBoardInNewWindow();
			// setOn() causes a signal which has to be skipped over
			skipNextSignal = true;
			
			scoreButton->setOn(false);
			return;
		}
		
		if (skipNextSignal)
		{
			// skip over this one incoming signal
			skipNextSignal = false;
			return;
		}
	}

	// offline mode -> scoring
	doRealScore(toggle);
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

