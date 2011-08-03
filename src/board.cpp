/*
* board.cpp
*/
#include <vector>

#include <qmessagebox.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <q3groupbox.h>
#include <qlineedit.h>
#include <qcursor.h>

//Added by qt3to4:
#include <Q3PtrList>
#include <QPixmap>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QWheelEvent>

#include "config.h"
#include "setting.h"
#include "qgo.h"
#include "board.h"
#include "globals.h"
#include "mark.h"
#include "imagehandler.h"
#include "stonehandler.h"
#include "tip.h"
#include "interfacehandler.h"
#include "move.h"
#include "scoretools_gui.h"
#include "normaltools_gui.h"
#include "mainwindow.h"
#include "noderesults.h"

Board::Board(QWidget *parent, const char *name, Q3Canvas* c)
: Q3CanvasView(c, parent, name)
{
	viewport()->setMouseTracking(TRUE);
	
	board_size = DEFAULT_BOARD_SIZE;
	showCoords = setting->readBoolEntry("BOARD_COORDS");
	showSGFCoords = setting->readBoolEntry("SGF_BOARD_COORDS");
	antiClicko = setting->readBoolEntry("ANTICLICKO");

	
	// Create a BoardHandler instance.
	boardHandler = new BoardHandler(this);
	CHECK_PTR(boardHandler);
	
	// Create an ImageHandler instance.
	imageHandler = new ImageHandler();
	CHECK_PTR(imageHandler);
	
	// Init the canvas
	canvas = new Q3Canvas(this, "MainCanvas");
	CHECK_PTR(canvas);
	canvas->setDoubleBuffering(TRUE);
	canvas->resize(BOARD_X, BOARD_Y);
	setCanvas(canvas);
	
	gatter = new Gatter(canvas, board_size);

	// Init data storage for marks and ghosts
	marks = new Q3PtrList<Mark>;
	marks->setAutoDelete(TRUE);
	lastMoveMark = NULL;
	
	ghosts = new Q3PtrList<Stone>;
	ghosts->setAutoDelete(TRUE);
	
	// Init the gatter size and the imagehandler pixmaps
	calculateSize();

	imageHandler->init(square_size);
	
	// Initialize some class variables
	nodeResultsDlg = NULL;
	fastLoad = false;
	isModified = false;
	mouseState = Qt::NoButton;
	for (int i=0; i<400; i++)
	{
		if (i < 52)
			letterPool[i] = false;
		numberPool[i] = false;
	}
	
	//coordsTip = new Tip(this);
#ifdef Q_WS_WIN
	resizeDelayFlag = false;
#endif
	curX = curY = -1;
	showCursor = setting->readBoolEntry("CURSOR");
	
	isLocalGame = true;
	
	// Init the ghost cursor stone
	curStone = new Stone(imageHandler->getGhostPixmaps(), canvas, stoneBlack, 0, 0);
	curStone->setZ(4);
	curStone->hide();                       

	lockResize = false;
	navIntersectionStatus = false;
  
	updateCaption();
	gatter_created = false;
	
	isHidingStones = false; // QQQ
}

Board::~Board()
{
//    delete coordsTip;
    delete curStone;
    delete boardHandler;
    marks->clear();
    delete marks;
    ghosts->clear();
    delete ghosts;
    delete lastMoveMark;
    delete canvas;
    delete nodeResultsDlg;
    delete imageHandler;
}

 void Board::calculateSize()
{
    // Calculate the size values
    const int margin = 1,              // distance from table edge to wooden board edge
		w = canvas->width() - margin * 2,  
		h = canvas->height() - margin * 2;

    int table_size = (w < h ? w : h );
    
    offset = table_size * 2/100 ;  // distance from edge of wooden board to playing area (grids + space for stones on 1st & last line)


    Q3CanvasText *coordV = new Q3CanvasText(QString::number(board_size), canvas);
    Q3CanvasText *coordH = new Q3CanvasText("A", canvas);
    int coord_width = coordV->boundingRect().width();
    int coord_height = coordH->boundingRect().height();

    // space for coodinates if shown
    int coord_offset =  (coord_width < coord_height ? coord_height : coord_width);
    
    if (showCoords)
		  offset = coord_offset + 2 ;

    //we need 1 more virtual 'square' for the stones on 1st and last line getting off the grid
    square_size = (table_size - 2*offset) / (board_size);  
    //square_size = (w < h ? (w-2*offset) / (board_size-1) : (h-2*offset) / (board_size-1));
    // Should not happen, but safe is safe.
    if (square_size == 0)
		  square_size = 1;
    
    
	  board_pixel_size = square_size * (board_size-1);    // grid size
    offset =  (table_size - board_pixel_size)/2;   
    
    // Center the board in canvas

    offsetX = margin + (w - board_pixel_size) / 2;
    offsetY = margin + (h - board_pixel_size) / 2;
}

void Board::resizeBoard(int w, int h)
{
    if (w < 30 || h < 30)
		return;

	Move *m_save = boardHandler->getTree()->getCurrent();
	boardHandler->gotoFirstMove();

    // Clear background before canvas is resized
    canvas->setBackgroundPixmap(*(ImageHandler::getTablePixmap(setting->readEntry("SKIN_TABLE"))));

    // Resize canvas
    canvas->resize(w, h);

    // Recalculate the size values
    calculateSize();

    // Rescale the pixmaps in the ImageHandler
    imageHandler->rescale(square_size);//, setting->readBoolEntry("SMALL_STONES"));

    // Delete gatter lines and update stones positions
    Q3CanvasItemList list = canvas->allItems();
    Q3CanvasItem *item;
    Q3CanvasItemList::Iterator it;
    for(it = list.begin(); it != list.end(); ++it)
    {
		item = *it;
		if (item->rtti() == 3)// || item->rtti() == 6)// || item->rtti() == 7)
		{
			item->hide();
			delete item;
		}
		else if (item->rtti() == RTTI_STONE)
		{
			Stone *s = (Stone*)item;
			s->setX(offsetX + square_size * (s->posX() - 1));
			s->setY(offsetY + square_size * (s->posY() - 1));
		}
		else if (item->rtti() >= RTTI_MARK_SQUARE &&
			item->rtti() <= RTTI_MARK_TERR)
		{
			Mark *m;
			switch(item->rtti())
			{
			case RTTI_MARK_SQUARE: m = (MarkSquare*)item; break;
			case RTTI_MARK_CIRCLE: m = (MarkCircle*)item; m->setSmall(setting->readBoolEntry("SMALL_MARKS")); break;
			case RTTI_MARK_TRIANGLE: m = (MarkTriangle*)item; break;
			case RTTI_MARK_CROSS: m = (MarkCross*)item; break;
			case RTTI_MARK_TEXT: m = (MarkText*)item; break;
			case RTTI_MARK_NUMBER: m = (MarkNumber*)item; break;
			case RTTI_MARK_TERR: m = (MarkTerr*)item; break;
			default: continue;
			}
			m->setSize((double)square_size, (double)square_size);
			m->setX((double)offsetX + (double)square_size * ((double)(m->posX()) - 1.0) -
				(double)m->getSizeX()/2.0);
			m->setY((double)offsetY + (double)square_size * ((double)(m->posY()) - 1.0) -
				(double)m->getSizeY()/2.0);
		 }
    }

	boardHandler->gotoMove(m_save);

	// Redraw the board
	drawBackground();
	drawGatter();
	
	if (showCoords)
		drawCoordinates();

  // Redraw the mark on the last played stone                             
  updateLastMove(m_save->getColor(), m_save->getX(), m_save->getY());     //SL added eb 7
  
//	canvas->update();
}

void Board::resizeEvent(QResizeEvent*)
{
#ifdef _WS_WIN_x
    if (!resizeDelayFlag)
    {
		resizeDelayFlag = true;
		// not necessary?
		QTimer::singleShot(50, this, SLOT(changeSize()));
    }
#else
	if (!lockResize)
		changeSize();
#endif
}

void Board::drawBackground()
{
    int w = canvas->width(),
		h = canvas->height();
	
    // Create pixmap of appropriate size
    QPixmap all(w, h);
	
    // Paint table and board on the pixmap
    QPainter painter;
    //QBrush board;
//    board.setPixmap(*(ImageHandler::getBoardPixmap(static_cast<skinType>(setting->readIntEntry("SKIN")))));
    //board.setPixmap(*(ImageHandler::getBoardPixmap(setting->readEntry("SKIN"))));
    //QBrush table;
    // table.setPixmap(*(imageHandler->getTablePixmap()));
    //table.setPixmap(*(ImageHandler::getTablePixmap()));
    //painter.flush();
    painter.begin(&all);
    painter.setPen(Qt::NoPen);
    //painter.fillRect(0, 0, w, h, table);
    painter.drawTiledPixmap (0, 0, w, h,*(ImageHandler::getTablePixmap(setting->readEntry("SKIN_TABLE"))));
    //painter.fillRect(
    painter.drawTiledPixmap (
		offsetX - offset,
		offsetY - offset,
		board_pixel_size + offset*2,
		board_pixel_size + offset*2,
		*(ImageHandler::getBoardPixmap(setting->readEntry("SKIN"))));

    	painter.end();

	QImage image = all.convertToImage();
	int lighter=20;
	int darker=60;
	int width = 3; 

	int x,y;
	for(x=0;x<width;x++)
		for (y= offsetY - offset +x ; y<offsetY + board_pixel_size + offset-x ;y++)
		{
			image.setPixel(
				offsetX - offset+x , 
				y, 
				QColor(image.pixel(offsetX - offset+x,y)).dark(int(100 + darker*(width-x)*(width-x)/width/width)).rgb());

			image.setPixel(
				offsetX + board_pixel_size + offset-x -1, 
				y,
				QColor(image.pixel(offsetX + board_pixel_size + offset-x-1,y)).light(100+ int(lighter*(width-x)*(width-x)/width/width)).rgb());
		}

	for(y=0;y<width;y++)
		for (x= offsetX - offset +y ; x<offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(
				x,
				offsetY - offset+y , 
				QColor(image.pixel(x,offsetY - offset+y)).light(int(100 + lighter*(width-y)*(width-y)/width/width)).rgb());

			image.setPixel(
				x,
				offsetY + board_pixel_size + offset-y -1, 
				QColor(image.pixel(x,offsetY + board_pixel_size + offset-y-1)).dark(100+ int(darker*(width-y)*(width-y)/width/width)).rgb());
		}


	width = 10;
	darker=50;

	for(x=0;(x<=width)&&(offsetX - offset-x >0) ;x++)
		for (y= offsetY - offset+x ; (y<offsetY + board_pixel_size + offset+x)&&(y<h) ;y++)
		{
			image.setPixel(
				offsetX - offset-1-x  , 
				y, 
				QColor(image.pixel(offsetX - offset-1-x,y)).dark(int(100 + darker*(width-x)/width)).rgb());
		}

	for(y=0;(y<=width)&&(offsetY + board_pixel_size + offset+y+1<h);y++)
		for (x= (offsetX - offset - y > 0 ? offsetX - offset - y:0) ; x<offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(
				x,
				offsetY + board_pixel_size + offset+y +1, 
				QColor(image.pixel(x,offsetY + board_pixel_size + offset+y+1)).dark(100+ int(darker*(width-y)/width)).rgb());
		}


	all.convertFromImage(image);
	// Set pixmap as canvas background
	canvas->setBackgroundPixmap(all);
}

void Board::drawGatter()
{
/*	QCanvasLine *line;
	int i,j;
	
	static QCanvasLine  *VGatter[361];
	static QCanvasLine  *HGatter[361];

	// Draw vertical lines
	for (i=0; i<board_size; i++)
	{
		line = new QCanvasLine(canvas);
		line->setPoints(offsetX + square_size * i, offsetY,
			offsetX + square_size * i, offsetY + board_pixel_size);
		line->show();
	}
	
	// Draw horizontal lines
	for (i=0; i<board_size; i++)
	{
		line = new QCanvasLine(canvas);
		line->setPoints(offsetX, offsetY + square_size * i,
			offsetX + board_pixel_size, offsetY + square_size * i);
		line->show();
	}
	*/
	gatter->resize(offsetX,offsetY,square_size);
/*
	// Draw the little circles on the starpoints
	int edge_dist = (board_size > 12 ? 4 : 3);
	int low = edge_dist;
	int middle = (board_size + 1) / 2;
	int high = board_size + 1 - edge_dist;
	if (board_size % 2 && board_size > 9)
	{
		drawStarPoint(middle, low);
		drawStarPoint(middle, high);
		drawStarPoint(low, middle);
		drawStarPoint(high, middle);
		drawStarPoint(middle, middle);
	}
	
	drawStarPoint(low, low);
	drawStarPoint(low, high);
	drawStarPoint(high, low);
	drawStarPoint(high, high);
*/
	updateCanvas();
}

void Board::drawStarPoint(int x, int y)
{
    int size = square_size / 5;
    // Round size top be even
    if (size % 2 > 0)
		size--;
    if (size < 6)
		size = 6;

    Q3CanvasEllipse *circle;
    
    circle = new Q3CanvasEllipse(canvas);
    circle->setBrush(Qt::black);
    circle->setSize(size, size);
    circle->setX(offsetX + square_size * (x-1));
    circle->setY(offsetY + square_size * (y-1));
    
    circle->show();
}

 void Board::drawCoordinates()
{
    Q3CanvasText *coord;
    int i;
    //const int off = 2,
    const int coord_centre = (offset - square_size/2 )/2; // centres the coordinates text within the remaining space at table edge
    QString txt;

    // Draw vertical coordinates. Numbers
    for (i=0; i<board_size; i++)
    {
		// Left side
		if(showSGFCoords)
			txt = QString(QChar(static_cast<const char>('a' + i)));
		else
			txt = QString::number(board_size - i);
		coord = new Q3CanvasText(txt, canvas);
		coord->setX(offsetX - offset + coord_centre - coord->boundingRect().width()/2 );
		coord->setY(offsetY + square_size * i - coord->boundingRect().height()/2);
		coord->show();
		// Right side
		coord = new Q3CanvasText(txt, canvas);
    		coord->setX(offsetX + board_pixel_size + offset - coord_centre - coord->boundingRect().width()/2 );
		coord->setY(offsetY + square_size * i - coord->boundingRect().height()/2);
		coord->show();
    }
	
    // Draw horizontal coordinates. Letters (Note: Skip 'i')
    for (i=0; i<board_size; i++)
    {
		if(showSGFCoords)
			txt = QString(QChar(static_cast<const char>('a' + i)));
		else
			txt = QString(QChar(static_cast<const char>('A' + (i<8?i:i+1))));
		// Top
		coord = new Q3CanvasText(txt, canvas);
		coord->setX(offsetX + square_size * i - coord->boundingRect().width()/2);
		coord->setY(offsetY - offset + coord_centre - coord->boundingRect().height()/2 );
		coord->show();
		// Bottom
		coord = new Q3CanvasText(txt, canvas);
		coord->setX(offsetX + square_size * i - coord->boundingRect().width()/2);
		coord->setY(offsetY + offset + board_pixel_size - coord_centre - coord->boundingRect().height()/2  );
		coord->show();
    }
}

void Board::hideStones()  // QQQ
{
    isHidingStones ^= true;
    Q3IntDict<Stone>* stones = boardHandler->getStoneHandler()->getAllStones();
    if (stones->isEmpty())
        return;
    
    Q3IntDictIterator<Stone> it(*stones);
    Stone *s;
    while (s = it.current()) {
        if (isHidingStones) {
	    s->setFrame(1+WHITE_STONES_NB+1);
	    s->shadow->setFrame(1+WHITE_STONES_NB+1);
        }
        else {
	    if (boardHandler->getGameData()->oneColorGo)
	        s->setFrame((rand() % 8) + 1);
	    else
	        s->setFrame(s->getColor() == stoneBlack ? 0 : (rand() % 8) + 1);
	    s->shadow->setFrame(1+WHITE_STONES_NB);
        }
        ++it;
    }
    updateCanvas();
}

Stone* Board::addStoneSprite(StoneColor c, int x, int y, bool &shown)
{
    if (x < 1 || x > board_size || y < 1 || y > board_size)
    {
		qWarning("Board::addStoneSprite() - Invalid stone: %d %d", x, y);
		return NULL;
    }
    
    switch (boardHandler->hasStone(x, y))
    {
    case 1:  // Stone exists and is visible
		// qDebug("*** Already a stone at %d, %d.", x, y);
    if (boardHandler->display_incoming_move)
    	return NULL;
    else      // we are observig a game, and we are just observing a sone that is
              // taken later. A new incoming stone is played there.
              // Ok, this is BAD.
     {
     	Stone *s = boardHandler->getStoneHandler()->getStoneAt(x, y);
	CHECK_PTR(s);
	s->setColor(c);
	s->setPos(x, y);
	return s;
     }
      
    case 0:  // No stone existing. Create a new one
		{
			// qDebug("*** Did not find any stone at %d, %d.", x, y);
			
 			Stone *s = new Stone(imageHandler->getStonePixmaps(), canvas, c, x, y,WHITE_STONES_NB,true);
			
			if (isHidingStones) { // QQQ
				s->setFrame(1+WHITE_STONES_NB+1);
				s->shadow->setFrame(1+WHITE_STONES_NB+1);
			}
			else {
				if (boardHandler->getGameData()->oneColorGo)
				    s->toggleOneColorGo(true);
				else
				    s->setFrame(c == stoneBlack ? 0 : (rand() % 8) + 1);
				s->shadow->setFrame(1+WHITE_STONES_NB);
			}
              
			CHECK_PTR(s);
			
			s->setX(offsetX + square_size * (x-1));
			s->setY(offsetY + square_size * (y-1));

			
			// Change color of a mark on this spot to white, if we have a black stone
			if (c == stoneBlack)
				updateMarkColor(stoneBlack, x, y);
			
			return s;
		}
		break;
		
    case -1:  // Stone exists, but is hidden. Show it and check correct color
		{
			Stone *s = boardHandler->getStoneHandler()->getStoneAt(x, y);
			CHECK_PTR(s);
			
			// qDebug("*** Found a hidden stone at %d, %d (%s).", x, y,
			
			// Check if the color is correct
			if (s->getColor() != c)
				s->setColor(c);
			s->setPos(x, y);
			s->show();
			shown = true;
			
			// Change color of a mark on this spot to white, if we have a black stone
			if (c == stoneBlack)
				updateMarkColor(stoneBlack, x, y);
			
			return s;
		}
    }
    
    return NULL;  // Oops
}

#ifndef NO_DEBUG
void Board::debug()
{
    qDebug("Board::debug()");
	
#if 1
    Mark *m = NULL;
    for (m=marks->first(); m != NULL; m=marks->next())
    {
		qDebug("posX:%d posY:%d  rtti:%d", m->posX(), m->posY(), m->rtti());
    }
#endif
	
#if 0
    Q3CanvasItemList list = canvas->allItems();
    int numC = list.count() - 42;  // 19 + 19 + 4
	
    int numS = boardHandler->getStoneHandler()->numStones();
    
    qDebug("We currently have %d stones in the canvas, and %d stones in the stonehandler.",
		numC, numS);
#endif
	
#if 1
    boardHandler->debug();
#endif
}
#endif

void Board::leaveEvent(QEvent*)
{
    curStone->hide();
    canvas->update();
}

int Board::convertCoordsToPoint(int c, int o)
{
    int p = c - o + square_size/2;
    if (p >= 0)
		return p / square_size + 1;
    else
		return -1;
}

void Board::contentsMouseMoveEvent(QMouseEvent *e)
{
    int x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);
    
    // Outside the valid board?
    if (x < 1 || x > board_size || y < 1 || y > board_size)
    {
		curStone->hide();
		canvas->update();
		curX = curY = -1;
		return;
    }
	
    // Nothing changed
    if (curX == (short)x && curY == (short)y)
		return;
	
    // Update the statusbar coords tip
    emit coordsChanged(x, y, board_size,showSGFCoords);
	
    // Remember if the cursor was hidden meanwhile.
    // If yes, we need to repaint it at the old position.
    bool flag = curX == -1;
	
    curX = (short)x;
    curY = (short)y;
    
    if (!showCursor || //setting->readBoolEntry("CURSOR") ||
		(boardHandler->getGameMode() == modeEdit &&
		boardHandler->getMarkType() != markNone) ||
		boardHandler->getGameMode() == modeScore ||
		(curStone->posX() == x &&
		curStone->posY() == y && !flag))
		return;
    
    curStone->setX(offsetX + square_size * (x-1));
    curStone->setY(offsetY + square_size * (y-1));
    curStone->setPos(x, y);

    bool notMyTurn = 	(curStone->getColor() == stoneBlack && !myColorIsBlack ||
			 curStone->getColor() == stoneWhite && myColorIsBlack);
    
    if (navIntersectionStatus ||              
        boardHandler->getGameMode() == modeObserve ||
	( boardHandler->getGameMode() == modeMatch && notMyTurn) ||
	( boardHandler->getGameMode() == modeComputer && notMyTurn))
	
		curStone->hide();
    else
		curStone->show();
    
    canvas->update();
}

void Board::contentsWheelEvent(QWheelEvent *e)
{
    // leave if observing or playing
    if (//boardHandler->getGameMode() == modeObserve ||
		boardHandler->getGameMode() == modeMatch ||
		boardHandler->getGameMode() == modeTeach)
		return;
    
    // Check delay
    if (QTime::currentTime() < wheelTime)
		return;
	
    // Needs an extra check on variable mouseState as state() does not work on Windows.
    if (e->delta() != 120) // QQQ weel down to next
    {
		if (e->state() == Qt::RightButton || mouseState == Qt::RightButton)
			nextVariation();
		else
			nextMove();
    }
    else
    {
		if (e->state() == Qt::RightButton || mouseState == Qt::RightButton)
			previousVariation();
		else
			previousMove();
    }
    
    // Delay of 100 msecs to avoid too fast scrolling
    wheelTime = QTime::currentTime();
    wheelTime = wheelTime.addMSecs(50);
    
    e->accept();
}

void Board::contentsMouseReleaseEvent(QMouseEvent* e)
{
	mouseState = Qt::NoButton;
    
	int 	x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);
	
	//qDebug("Mouse should be released after %d,%03d", wheelTime.second(),wheelTime.msec());
	//qDebug("Mouse released at time         %d,%03d", QTime::currentTime().second(),QTime::currentTime().msec());
	
	if ( 	(boardHandler->getGameMode()==modeMatch) && 
    		(QTime::currentTime() > wheelTime))
	{	
		
		if (boardHandler->getBlackTurn())
		{
			if (myColorIsBlack)
			{
				boardHandler->addStone(stoneBlack, x, y);
				emit signal_addStone(stoneBlack, x, y);
			}
		}
		else
		{
			if (!myColorIsBlack)
			{
				boardHandler->addStone(stoneWhite, x, y);
				emit signal_addStone(stoneWhite, x, y);
			}
		}  
	
	}
	
	
	 
}

void Board::contentsMousePressEvent(QMouseEvent *e)
{
    setFocus();
    
    mouseState = e->button();
    
    int x = convertCoordsToPoint(e->x(), offsetX),
		y = convertCoordsToPoint(e->y(), offsetY);
	
    // Button gesture outside the board?
    if (x < 1 || x > board_size || y < 1 || y > board_size)
    {
		if (e->button() == Qt::LeftButton &&
			e->state() == Qt::RightButton)
			previousMove();
		else if (e->button() == Qt::RightButton &&
			e->state() == Qt::LeftButton)
			nextMove();
		else if (e->button() == Qt::LeftButton &&
			e->state() == Qt::MidButton)
			gotoVarStart();
		else if (e->button() == Qt::RightButton &&
			e->state() == Qt::MidButton)
			gotoNextBranch();
		
		return;
    }
	
    // Lock accidental gesture over board
    if ((e->button() == Qt::LeftButton && e->state() == Qt::RightButton) ||
		(e->button() == Qt::RightButton && e->state() == Qt::LeftButton) ||
		(e->button() == Qt::LeftButton && e->state() == Qt::MidButton) ||
		(e->button() == Qt::RightButton && e->state() == Qt::MidButton))
		return;
  
    
    // Ok, we are inside the board, and it was no gesture.

    // We just handle the case of getting where the mouse was clicked
    if (navIntersectionStatus) // added eb 11
    {
        navIntersectionStatus = false;

//   *** Several unsuccessfull tries with clean method ***        
//      unsetCursor();
//      setCursor(ArrowCursor);
//      this->topLevelWidget()->setCursor(ArrowCursor);
//   *** Therefore we apply thick method :

        QApplication::restoreOverrideCursor();  
        boardHandler->findMoveByPos(x, y);                                 //SL added eb 11
	return;
	}                         // end add eb 11


    // resume normal proceeding
    switch (boardHandler->getGameMode())
    {
    case modeNormal:
		switch (e->button())
		{
		case Qt::LeftButton:
			if (e->state() == Qt::ShiftModifier)   // Shift: Find move in main branch
			{
				navIntersectionStatus = false;
				boardHandler->findMoveByPos(x, y);                                 //SL added eb 11
				return;
			}
			else if (e->state() == Qt::ControlModifier)  // Control: Find move in all following variations
			{
				if (boardHandler->findMoveInVar(x, y))  // Results found?
				{
					// Init dialog if not yet done
					if (nodeResultsDlg == NULL)
					{
						nodeResultsDlg = new NodeResults(this, "noderesult", Qt::WType_TopLevel);
						connect(nodeResultsDlg, SIGNAL(doFump(Move*)), this, SLOT(gotoMove(Move*)));
					}
					nodeResultsDlg->setNodes(boardHandler->nodeResults);
					nodeResultsDlg->show();
					nodeResultsDlg->raise();
				}
				return;
			}
			if (boardHandler->getBlackTurn())
				boardHandler->addStone(stoneBlack, x, y);
			else
				boardHandler->addStone(stoneWhite, x, y);
			
			break;
			
		case Qt::RightButton:
			if (e->state() == Qt::ShiftModifier)  // Shift: Find move in this branch
			{
				boardHandler->findMoveByPosInVar(x, y);
				return;
			}
			break;
			
		default:
			break;
		}
		break;
		
	case modeEdit:
		switch (e->button())
		{
		case Qt::LeftButton:
			if (boardHandler->getMarkType() == markNone)
				boardHandler->addStone(stoneBlack, x, y);
			else
			{
				// Shift-click setting a text mark
				if (boardHandler->getMarkType() == markText &&
					e->state() == Qt::ShiftModifier)
				{
					// Dont open dialog if a text mark already exists
					Mark *m;
					QString oldTxt = NULL;
					// If its a text mark, get the old text and put it in the dialog
					if ((m = hasMark(x, y)) != NULL &&
						m->getType() == markText)
						oldTxt = boardHandler->getTree()->getCurrent()->getMatrix()->getMarkText(x, y);
					// Get the label string from the dialog. Moved to inferface handler
					// to keep the dialog stuff out of this class.
					QString txt = getInterfaceHandler()->getTextLabelInput(this, oldTxt);
					if (txt.isNull() || txt.isEmpty())  // Aborted dialog
						break;
					setMark(x, y, markText, true, txt);
				}
				else
					setMark(x, y, boardHandler->getMarkType());
				canvas->update();
			}
			break;
		case Qt::RightButton:
			if (boardHandler->getMarkType() == markNone)
				boardHandler->addStone(stoneWhite, x, y);
			else
			{
				removeMark(x, y);
				canvas->update();
			}
			break;
		default:
			break;
		}
		break;
		
	case modeScore:
		switch (e->button())
		{
		case Qt::LeftButton:
			if (get_isLocalGame())
				boardHandler->markDeadStone(x, y);  // Mark or unmark as dead
			emit signal_addStone(stoneBlack, x, y); // the client accepts a coordinate in scoring mode
			break;
		case Qt::RightButton:
			if (get_isLocalGame())
				boardHandler->markSeki(x, y);  // Mark group as alive in seki
			emit signal_addStone(stoneBlack, x, y); // the client accepts a coordinate in scoring mode
			break;
		default:
			break;
		}
		break;
		
	case modeObserve:
		// do nothing but observe
		break;
		
	case modeMatch:
		// Delay of 250 msecs to avoid clickos
    		wheelTime = QTime::currentTime();
    		//qDebug("Mouse pressed at time %d,%03d", wheelTime.second(),wheelTime.msec());
		if (antiClicko)
			wheelTime = wheelTime.addMSecs(250);
		
		
		/*if (boardHandler->getBlackTurn())
		{
			if (myColorIsBlack)
			{
				boardHandler->addStone(stoneBlack, x, y);
				emit signal_addStone(stoneBlack, x, y);
			}
		}
		else
		{
			if (!myColorIsBlack)
			{
				boardHandler->addStone(stoneWhite, x, y);
				emit signal_addStone(stoneWhite, x, y);
			}
		}*/
		break;

	case modeComputer:                          //added eb 12
	

		if (boardHandler->getBlackTurn())
		{
			if (myColorIsBlack)
			{
				boardHandler->addStone(stoneBlack, x, y);
				emit signal_Stone_Computer(stoneBlack, x, y);
			}
		}
		else
		{
			if (!myColorIsBlack)
			{
				boardHandler->addStone(stoneWhite, x, y);
				emit signal_Stone_Computer(stoneWhite, x, y);
			}
		}
		break;                                   // end add eb 12

        		
	case modeTeach:
		if (boardHandler->getBlackTurn())
		{
			boardHandler->addStone(stoneBlack, x, y);
			emit signal_addStone(stoneBlack, x, y);
		}
		else
		{
			boardHandler->addStone(stoneWhite, x, y);
			emit signal_addStone(stoneWhite, x, y);
		}
		break;
		
	default:
		qWarning("   *** Invalid game mode! ***");
    }
}

void Board::increaseSize()
{
    resizeBoard(canvas->width() + 20, canvas->height() + 20);
}

void Board::decreaseSize()
{
    QSize s = viewportSize(width()-5, height()-5);
    if (canvas->width() - 20 < s.width() ||
		canvas->height() - 20 < s.height())
		return;
    resizeBoard(canvas->width() - 20, canvas->height() - 20);
}

void Board::changeSize()
{
#ifdef Q_WS_WIN
    resizeDelayFlag = false;
#endif
    QSize s = viewportSize(width()-5, height()-5);
    resizeBoard(s.width(), s.height());
}

void Board::hideAllStones()
{
    Q3CanvasItemList list = canvas->allItems();
    Q3CanvasItem *item;
    
    Q3CanvasItemList::Iterator it;
    for(it = list.begin(); it != list.end(); ++it)
    {
		item = *it;
		if (item->rtti() == RTTI_STONE)
			item->hide();
    }
}

void Board::hideAllMarks()
{
    MarkText::maxLength = 1;
    marks->clear();
		
    gatter->showAll();

    for (int i=0; i<400; i++)
    {
		if (i < 52)
			letterPool[i] = false;
		numberPool[i] = false;
    }
}

bool Board::openSGF(const QString &fileName, const QString &filter)
{
	
    // Load the sgf
    if (!boardHandler->loadSGF(fileName, filter, fastLoad))
		return false;
	
    canvas->update();
    setModified(false);
    return true;
}

bool Board::startComputerPlay(QNewGameDlg * dlg,const QString &fileName, const QString &filter,const QString &computer_path)
{

     // Clean up everything and get to last move
    //clearData();
    
    // Initiate the dialog with computer
    if (!boardHandler->openComputerSession(dlg,fileName,filter,computer_path))
    		return false;
   
    canvas->update();
    setModified(false);
    return true;
}

void Board::clearData()
{
    hideAllStones();
    hideAllMarks();
    ghosts->clear();
    removeLastMoveMark();
    boardHandler->clearData();
    if (curStone != NULL)
		curStone->setColor(stoneBlack);
    canvas->update();
    isModified = false;
    if (nodeResultsDlg != NULL)
    {
		nodeResultsDlg->hide();
		delete nodeResultsDlg;
		nodeResultsDlg = NULL;
    }
}

void Board::updateComment()
{
    boardHandler->updateComment();
}

void Board::updateComment2()
{
	// emit signal to opponent in online game
	sendcomment(getInterfaceHandler()->getComment2());
}

void Board::modifiedComment()
{
    setModified();
}

void Board::setMark(int x, int y, MarkType t, bool update, QString txt, bool overlay)
{
    if (x == -1 || y == -1)
		return;
	
    Mark *m;

    // We already have a mark on this spot? If it is of the same type,
    // do nothing, else overwrite with the new mark.
    if ((m = hasMark(x, y)) != NULL)
    {
		if (m->getType() == t && m->getType() != markText)  // Text labels are overwritten
			return;
		
		removeMark(x, y, update);
    }
    
    if (lastMoveMark != NULL &&
		lastMoveMark->posX() == x &&
		lastMoveMark->posY() == y)
		removeLastMoveMark();
	
    QColor col = Qt::black;
	
    // Black stone or black ghost underlying? Then we need a white mark.
    if ((boardHandler->hasStone(x, y) == 1 &&
		boardHandler->getStoneHandler()->getStoneAt(x, y)->getColor() == stoneBlack) ||
		(setting->readIntEntry("VAR_GHOSTS") && hasVarGhost(stoneBlack, x, y)))
		col = Qt::white;
    
    short n = -1;
	
    switch(t)
    {
    case markSquare:
		m = new MarkSquare(x, y, square_size, canvas, col);
		break;
		
    case markCircle:
		m = new MarkCircle(x, y, square_size, canvas, col, true);//setting->readBoolEntry("SMALL_STONES"));
        break;
		
    case markTriangle:
		m = new MarkTriangle(x, y, square_size, canvas, col);
		break;
		
    case markCross:
		m = new MarkCross(x, y, square_size, canvas, col);
		break;
		
    case markText:
		if (txt.isNull())
		{
			n = 0;
			while (letterPool[n] && n < 51)
				n++;
			letterPool[n] = true;
			
			txt = QString(QChar(static_cast<const char>('A' + (n>=26 ? n+6 : n))));
			
			// Update matrix with this letter
			boardHandler->getTree()->getCurrent()->getMatrix()->setMarkText(x, y, txt);
		}
		else if (txt.length() == 1)
		{
			// Text was given as argument, check if it can converted to a single letter
			n = -1;
			if (txt[0] >= 'A' && txt[0] <= 'Z')
				n = txt[0].latin1() - 'A';
			else if (txt[0] >= 'a' && txt[0] <= 'a')
				n = txt[0].latin1() - 'a' + 26;
			
			if (n > -1)
				letterPool[n] = true;
		}
		m = new MarkText(imageHandler, x, y, square_size, txt, canvas, col, n, false, overlay);
		gatter->hide(x,y);//setMarkText(x, y, txt);
		break;
		
    case markNumber:
		if (txt.isNull())
		{
			n = 0;
			while (numberPool[n] && n < 399)
				n++;
			
			txt = QString::number(n+1);
			
			// Update matrix with this letter
			boardHandler->getTree()->getCurrent()->getMatrix()->setMarkText(x, y, txt);	    
		}
		else
			n = txt.toInt() - 1;
		numberPool[n] = true;
		m = new MarkNumber(imageHandler, x, y, square_size, n, canvas, col, false);
		setMarkText(x, y, txt);
		gatter->hide(x,y);
		break;
		
    case markTerrBlack:
		m = new MarkTerr(x, y, square_size, stoneBlack, canvas);
		if (boardHandler->hasStone(x, y) == 1)
		{
			boardHandler->getStoneHandler()->getStoneAt(x, y)->setDead(true);
			boardHandler->getStoneHandler()->getStoneAt(x, y)->setSequence(imageHandler->getGhostPixmaps());
			boardHandler->getStoneHandler()->getStoneAt(x, y)->shadow->hide();
			boardHandler->markedDead = true;
		}
		boardHandler->getTree()->getCurrent()->setScored(true);
		break;
		
    case markTerrWhite:
		m = new MarkTerr(x, y, square_size, stoneWhite, canvas);
		if (boardHandler->hasStone(x, y) == 1)
		{
			boardHandler->getStoneHandler()->getStoneAt(x, y)->setDead(true);
			boardHandler->getStoneHandler()->getStoneAt(x, y)->setSequence(imageHandler->getGhostPixmaps());
			boardHandler->getStoneHandler()->getStoneAt(x, y)->shadow->hide();
			boardHandler->markedDead = true;
		}
		boardHandler->getTree()->getCurrent()->setScored(true);
		break;
		
    default:
		qWarning("   *** Board::setMark() - Bad mark type! ***");
		return;
    }
	
    CHECK_PTR(m);
    m->setX(offsetX + square_size * (x-1) - m->getSizeX()/2);
    m->setY(offsetY + square_size * (y-1) - m->getSizeY()/2);
    m->show();
	
    marks->append(m);
	
    if (update)
		boardHandler->editMark(x, y, t, txt);
}

void Board::removeMark(int x, int y, bool update)
{
    Mark *m = NULL;
	
    if (lastMoveMark != NULL &&
		lastMoveMark->posX() == x &&
		lastMoveMark->posY() == y)
		removeLastMoveMark();
	
    for (m=marks->first(); m != NULL; m=marks->next())
    {
		if (m->posX() == x && m->posY() == y)
		{
			if (m->getCounter() != -1)
			{
				if (m->getType() == markText)
					letterPool[m->getCounter()] = false;
				else if (m->getType() == markNumber)
					numberPool[m->getCounter()] = false;
			}
			
			marks->remove(m);
			gatter->show(x,y);
			if (update)
				boardHandler->editMark(x, y, markNone);
			return;
		}
    }
}

void Board::setMarkText(int x, int y, const QString &txt)
{
    Mark *m;
    
    // Oops, no mark here, or no text mark
    if (txt.isNull() || txt.isEmpty() ||
		(m = hasMark(x, y)) == NULL || m->getType() != markText)
		return;
	
    m->setText(txt);
    // Adjust the position on the board, if the text size has changed.
    m->setSize((double)square_size, (double)square_size);
    m->setX(offsetX + square_size * (x-1) - m->getSizeX()/2);
    m->setY(offsetY + square_size * (y-1) - m->getSizeY()/2);
	
}

Mark* Board::hasMark(int x, int y)
{
    Mark *m = NULL;
	
    for (m=marks->first(); m != NULL; m=marks->next())
		if (m->posX() == x && m->posY() == y)
			return m;
		
		return NULL;
}

void Board::updateLastMove(StoneColor c, int x, int y)
{

  delete lastMoveMark;
	lastMoveMark = NULL;

	if (x == 20 && y == 20)  // Passing
		removeLastMoveMark();
	else if (c != stoneNone && x != -1 && y != -1 && x <= board_size && y <= board_size)
	{
		if (isHidingStones)
			lastMoveMark = new MarkRedCircle(x, y, square_size, canvas,
				c == stoneBlack ? Qt::white : Qt::black, true); // QQQ
		else
			lastMoveMark = new MarkCross(x, y, square_size, canvas,
				c == stoneBlack ? Qt::white : Qt::black, true);


		ASSERT(lastMoveMark);

		lastMoveMark->setX(offsetX + square_size * (x-1) - lastMoveMark->getSizeX()/2);
		lastMoveMark->setY(offsetY + square_size * (y-1) - lastMoveMark->getSizeY()/2);
		lastMoveMark->show();
	}

	setCurStoneColor();
}

void Board::setCurStoneColor()
{
    // Switch the color of the ghost stone cursor
    if (curStone != NULL)
		curStone->setColor(boardHandler->getBlackTurn() ? stoneBlack : stoneWhite);
}

void Board::removeLastMoveMark()
{
    if (lastMoveMark != NULL)
    {
		lastMoveMark->hide();
		delete lastMoveMark;
		lastMoveMark = NULL;
    }
}

void Board::checkLastMoveMark(int x, int y)
{
    Mark *m = NULL;
    
    for (m=marks->first(); m != NULL; m=marks->next())
    {
		if (m->posX() == x && m->posY() == y &&
			m->rtti() != RTTI_MARK_TERR &&
			m->getColor() == Qt::white)
		{
			m->setColor(Qt::black);
			break;
		}
    }
	
    if (lastMoveMark == NULL ||
		lastMoveMark->posX() != x ||
		lastMoveMark->posY() != y)
		return;
	
    removeLastMoveMark();
}

void Board::updateMarkColor(StoneColor c, int x, int y)
{
    Mark *m = NULL;
	
    for (m=marks->first(); m != NULL; m=marks->next())
    {
		if (m->posX() == x && m->posY() == y && m->rtti() != RTTI_MARK_TERR)
		{
			m->setColor(c == stoneBlack ? Qt::white : Qt::black);
			break;
		}
    }
}

void Board::setVarGhost(StoneColor c, int x, int y)
{
    Stone *s = NULL;
	
    if (setting->readIntEntry("VAR_GHOSTS") == vardisplayGhost)
		s = new Stone(imageHandler->getGhostPixmaps(), canvas, c, x, y);
    else if (setting->readIntEntry("VAR_GHOSTS") == vardisplaySmallStone)
		s = new Stone(imageHandler->getAlternateGhostPixmaps(), canvas, c, x, y, 1);
    else
		return;
	
    ghosts->append(s);
    
    if (x == 20 && y == 20)  // Pass
    {
		s->setX(offsetX + square_size * (board_size+1));
		s->setY(offsetY + square_size * board_size);
		setMark(board_size+2, board_size+1, markText, false, tr("Pass"), false);
    }
    else
    {
		s->setX(offsetX + square_size * (x-1));
		s->setY(offsetY + square_size * (y-1));
    }
}

bool Board::hasVarGhost(StoneColor c, int x, int y)
{
    Stone *s;
    for (s=ghosts->first(); s != NULL; s=ghosts->next())
		if (s->posX() == x && s->posY() == y &&
			s->getColor() == c)
			return true;
		return false;
}

void Board::setVariationDisplay(VariationDisplay d)
{
    if (d == vardisplayNone)
    {
		ghosts->clear();
		canvas->update();
    }
}

void Board::setShowCursor(bool b)
{
    if (!b && curStone != NULL)
		curStone->hide();
}

void Board::removeGhosts()
{
    // Remove all variation ghosts
    if (!ghosts->isEmpty())
		ghosts->clear();
}

void Board::setShowCoords(bool b)
{
    bool old = showCoords;
    showCoords = b;
    if (old != showCoords)
		changeSize();  // Redraw the board if the value changed.
}

void Board::setShowSGFCoords(bool b)
{
	bool old = showSGFCoords;
	showSGFCoords = b;
	if(old != showSGFCoords)
		changeSize();  // Redraw the board if the value changed.
}

void Board::initGame(GameData *d, bool sgf)
{
	CHECK_PTR(d);
	
	int oldsize = board_size;
	board_size = d->size;
	
	// Clear up everything
	clearData();
	
	// Different board size? Redraw the canvas.
	if (board_size != oldsize)
	{
		delete gatter;
		gatter = new Gatter(canvas, board_size);	
		changeSize();
	}

	boardHandler->initGame(d, sgf);
	updateCaption();
}

void Board::setModified(bool m)
{
    if (m == isModified || boardHandler->getGameMode() == modeObserve)
		return;
 
    isModified = m;
    updateCaption();
}

void Board::updateCaption()
{
    // Print caption
    // example: qGo 0.0.5 - Zotan 8k vs. tgmouse 10k
    // or if game name is given: qGo 0.0.5 - Kogo's Joseki Dictionary
    topLevelWidget()->setCaption(QString(isModified ? "* " : "") +
		(boardHandler->getGameData()->gameNumber != 0 ?
		"(" + QString::number(boardHandler->getGameData()->gameNumber) + ") " : QString()) +
		(boardHandler->getGameData()->gameName.isEmpty() ?
		boardHandler->getGameData()->playerWhite +
		(!boardHandler->getGameData()->rankWhite.isEmpty() ?
		" " + boardHandler->getGameData()->rankWhite : QString())
		+ " " + tr("vs.") + " "+
		boardHandler->getGameData()->playerBlack +
		(!boardHandler->getGameData()->rankBlack.isEmpty() ?
		" " + boardHandler->getGameData()->rankBlack : QString()) :
		boardHandler->getGameData()->gameName) +
		"   " + QString(PACKAGE " " VERSION));

	if (getInterfaceHandler())
	{
		bool simple = boardHandler->getGameData()->rankWhite.length() == 0 && boardHandler->getGameData()->rankBlack.length() == 0;
		Q3GroupBox *gb = getInterfaceHandler()->normalTools->whiteFrame;
		QString player = boardHandler->getGameData()->playerWhite;
		if (simple && player == tr("White"))
			gb->setTitle(tr("White"));	
		else
		{
			// truncate to 12 characters max
			player.truncate(12);

			if (boardHandler->getGameData()->rankWhite.length() != 0)
				player = tr("W") + ": " + player + " " + boardHandler->getGameData()->rankWhite;
			else
				player = tr("W") + ": " + player;
			
			gb->setTitle(player);
		}

		gb = getInterfaceHandler()->normalTools->blackFrame;
		player = boardHandler->getGameData()->playerBlack;
		if (simple && player == tr("Black"))
			gb->setTitle(tr("Black"));	
		else
		{
			// truncate to 12 characters max
			player.truncate(12);

			if (boardHandler->getGameData()->rankBlack.length() != 0)
				player = tr("B") + ": " + player + " " + boardHandler->getGameData()->rankBlack;
			else
				player = tr("B") + ": " + player;
			
			gb->setTitle(player);
		}
	}
}

void Board::exportPicture(const QString &fileName, const QString &filter, bool toClipboard)
{
    QPixmap pix = QPixmap::grabWidget(this,
		offsetX - offset + 2,
		offsetY - offset + 2 ,
		board_pixel_size + offset*2,
		board_pixel_size + offset*2);
    
    if (toClipboard)
    {
		QApplication::clipboard()->setPixmap(pix);
		return;
    }
    
    if (!pix.save(fileName, filter))
		QMessageBox::warning(this, PACKAGE, tr("Failed to save image!"));
}

void Board::countScore()
{
    // Switch to score mode
    boardHandler->setMode(modeScore);
	
#if 0
    // Don't clean the board from existing territory marks and dead stones.
    boardHandler->getStoneHandler()->removeDeadMarks();
    boardHandler->getTree()->getCurrent()->getMatrix()->clearTerritoryMarks();
	
    boardHandler->countScore();
#else
    // Instead count the dead stones and add them to the captures. This way we keep
    // existing scoring (Cgoban2) and don't need to mark the dead stones again.
    int caps_black=0, caps_white=0;
    boardHandler->getStoneHandler()->updateDeadMarks(caps_black, caps_white);
	
    boardHandler->enterScoreMode(caps_black, caps_white);
    boardHandler->countScore();
#endif
	
    setModified();
}

void Board::doCountDone()
{ 
    float komi = getGameData()->komi;
    int capW = getInterfaceHandler()->scoreTools->capturesWhite->text().toInt(),
		capB = getInterfaceHandler()->scoreTools->capturesBlack->text().toInt(),
		terrW = getInterfaceHandler()->scoreTools->terrWhite->text().toInt(),
		terrB = getInterfaceHandler()->scoreTools->terrBlack->text().toInt();
	
    float totalWhite = capW + terrW + komi;
    int totalBlack = capB + terrB;
    float result = 0;
    QString rs;
	
    QString s;
    s.sprintf(tr("White") + "\n%d + %d + %.1f = %.1f\n\n" + tr("Black") + "\n%d + %d = %d\n\n",
		terrW, capW, komi, totalWhite,
		terrB, capB, totalBlack);
    
    if (totalBlack > totalWhite)
    {
		result = totalBlack - totalWhite;
		s.append(tr("Black wins with %1").arg(result));
		rs = "B+" + QString::number(result);
    }
    else if (totalWhite > totalBlack)
    {
		result = totalWhite - totalBlack;
		s.append(tr("White wins with %1").arg(result));
		rs = "W+" + QString::number(result);
    }
    else
    {
		rs = tr("Jigo");
		s.append(rs);
    }
	
    //if (QMessageBox::information(this, PACKAGE " - " + tr("Game Over"), s, tr("Ok"), tr("Update gameinfo")) == 1)
		boardHandler->getGameData()->result = rs;
  
    boardHandler->getTree()->getCurrent()->setTerritoryMarked(false);
    boardHandler->getTree()->getCurrent()->setScore(totalBlack, totalWhite);

    emit signal_done();        
}

int Board::getCurrentMoveNumber() const
{
    return boardHandler->getTree()->getCurrent()->getMoveNumber();
}

InterfaceHandler* Board::getInterfaceHandler()
{
    return ((MainWindow*)topLevelWidget())->getInterfaceHandler();
}

// button "Pass" clicked
void Board::doPass()
{
	// wait for server message if online
	if (isLocalGame)
		// pass move is ok
		boardHandler->doPass();

  // emit in every case
	emit signal_pass();

}

void Board::doResign()
{
	emit signal_resign();
}

void Board::set_isLocalGame(bool isLocal)
{
	isLocalGame = isLocal;
	getInterfaceHandler()->commentEdit2->setDisabled(isLocalGame);
	if (isLocalGame)
		((MainWindow*)topLevelWidget())->getListView_observers()->hide();
	else
		((MainWindow*)topLevelWidget())->getListView_observers()->show();
}

void Board::navIntersection()          
{
   
 /***** several unsuccessful tries with clean method
 //   unsetCursor();
 //   this->topLevelWidget()->unsetCursor();

 // this is debug code to check if we can catch the corrrect cursor
    if (this->topLevelWidget()->ownCursor())
      qDebug("cursor = top");
      
    bool b;
    int i= 0;
    QWidget *w = this;
    do {
      
      if (w->ownCursor())
          qDebug("cursor = %d",i);
       i++  ;
      }
    while (w=w->parentWidget(true)) ;

    qDebug("stack %d ",i-1);
      
    setCursor(pointingHandCursor);

  *** Therefore we apply thick method  :           */    
    QApplication::setOverrideCursor( QCursor(Qt::pointingHandCursor) );
      
    navIntersectionStatus = true;

}

 /**
  * Generate a candidate for the filename for this game
  **/
QString Board::getCandidateFileName()
{
	GameData data = getGameData();
	QString base = QDate::currentDate().toString("yyyy-MM-dd") + "-" + data.playerWhite + "-" + data.playerBlack    ;
	QString result = base ;
	QString dir= "" ;

	if (setting->readBoolEntry("REM_DIR"))
			dir = setting->readEntry("LAST_DIR");
	int i = 1;
	while (QFile(dir + result+".sgf").exists())
	{
		//number = Q.number(i++);
		result = base + "-"+ QString::number(i++);
		//fileName = fileName + ".sgf";
	} 
	return dir + result + ".sgf";
}

