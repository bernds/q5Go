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
#include "mainwindow.h"
#include "noderesults.h"

Board::Board(QWidget *parent, QGraphicsScene *c)
: QGraphicsView(c, parent)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	viewport()->setMouseTracking(TRUE);
	setUpdatesEnabled(TRUE);

	board_size = DEFAULT_BOARD_SIZE;
	showCoords = setting->readBoolEntry("BOARD_COORDS");
	showSGFCoords = setting->readBoolEntry("SGF_BOARD_COORDS");
	antiClicko = setting->readBoolEntry("ANTICLICKO");

	setStyleSheet( "QGraphicsView { border-style: none; }" );

	// Create a BoardHandler instance.
	boardHandler = new BoardHandler(this);
	CHECK_PTR(boardHandler);

	// Create an ImageHandler instance.
	imageHandler = new ImageHandler();
	CHECK_PTR(imageHandler);

	// Init the canvas
	canvas = new QGraphicsScene(0,0, BOARD_X, BOARD_Y,this);
	setScene(canvas);
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
	curStone->setZValue(4);
	curStone->hide();

	lockResize = false;
	navIntersectionStatus = false;

	updateCaption();
	gatter_created = false;

	isHidingStones = false; // QQQ
	setupCoords();
	setFocusPolicy(Qt::NoFocus);
}


void Board::setupCoords()
{
	QString hTxt,vTxt;

	// Init the coordinates
	for (int i=0; i<board_size; i++)
	{
		if (showSGFCoords)
		{
			vTxt = QString(QChar(static_cast<const char>('a' + i)));
			hTxt = QString(QChar(static_cast<const char>('a' + i)));
		} else {
			int real_i = i < 8 ? i : i + 1;
			vTxt = QString::number(i + 1);
			hTxt = QString(QChar(static_cast<const char>('A' + real_i)));
		}

		vCoords1.append(new QGraphicsSimpleTextItem(vTxt, 0, canvas));
		hCoords1.append(new QGraphicsSimpleTextItem(hTxt, 0, canvas));
		vCoords2.append(new QGraphicsSimpleTextItem(vTxt, 0, canvas));
		hCoords2.append(new QGraphicsSimpleTextItem(hTxt, 0, canvas));
	}
}

void Board::clearCoords()
{
	QList<QGraphicsSimpleTextItem*>::const_iterator i;
#define FREE_ARRAY_OF_POINTERS(a)							\
	for (i = a.begin(); i != a.end(); ++i)					\
		delete *i;											\
	a.clear();												\

	FREE_ARRAY_OF_POINTERS(vCoords1);
	FREE_ARRAY_OF_POINTERS(hCoords1);
	FREE_ARRAY_OF_POINTERS(vCoords2);
	FREE_ARRAY_OF_POINTERS(hCoords2);
}

Board::~Board()
{
	clearData();
    delete curStone;
    delete boardHandler;
    delete marks;
    delete ghosts;
    delete lastMoveMark;
    delete canvas;
    delete nodeResultsDlg;
    delete imageHandler;
}

// distance from table edge to wooden board edge
const int Board::margin = 2;

// distance from coords to surrounding elements
const int Board::coord_margin = 4;

void Board::calculateSize()
{
	// Calculate the size values

	int w = (int)canvas->width() - margin * 2;
	int h = (int)canvas->height() - margin * 2;

	table_size = w < h ? w : h;

	QGraphicsSimpleTextItem coordV (QString::number(board_size), 0, canvas);
	QGraphicsSimpleTextItem coordH ("A", 0, canvas);
	int coord_width = (int)coordV.boundingRect().width();
	int coord_height = (int)coordH.boundingRect().height();

	// space for coodinates if shown
	coord_offset = coord_width < coord_height ? coord_height : coord_width;

	//we need 1 more virtual 'square' for the stones on 1st and last line getting off the grid
	if (showCoords)
		square_size = (table_size - 2 * (coord_offset + coord_margin * 2));
	else
		square_size = table_size;
	square_size /= (float)board_size;
	// Should not happen, but safe is safe.
	if (square_size == 0)
		  square_size = 1;

	// grid size
	board_pixel_size = square_size * (board_size - 1);
	offset = (table_size - board_pixel_size)/2;

	// Center the board in canvas

	offsetX = margin + (w - board_pixel_size) / 2;
	offsetY = margin + (h - board_pixel_size) / 2;
}

void Board::resizeBoard(int w, int h)
{
	if (w < 30 || h < 30)
		return;

	Move *m_save = boardHandler->getTree()->getCurrent();

	// Resize canvas
	canvas->setSceneRect(0,0,w,h);

	// Recalculate the size values
	calculateSize();

	// Rescale the pixmaps in the ImageHandler
	imageHandler->rescale(square_size);

	// Delete gatter lines and update stones positions
	QList<QGraphicsItem *> list = canvas->items();
	QGraphicsItem *item;

	QListIterator<QGraphicsItem *> it( list );


	for (; it.hasNext();)
	{
		item = it.next();
		/*
		 * Coordinates : type = 9
		 */
//		if (item->type() == 9)// || item->type() == 3)// || item->rtti() == 7)
//		{
//			item->hide();
//			delete item;
//		}
		/*else*/ if (item->type() == RTTI_STONE)
		{
			Stone *s = (Stone*)item;
			s->setColor(s->getColor());
			s->setPos(offsetX + square_size * (s->posX() - 1) - s->pixmap().width()/2,
				offsetY + square_size * (s->posY() - 1) - s->pixmap().height()/2 );

			//TODO introduce a ghost list in the stone class so that this becomes redundant code
			if (s->isDead())
				s->togglePixmap(imageHandler->getGhostPixmaps(), FALSE);



		}
		else if (item->type() >= RTTI_MARK_SQUARE &&
			item->type() <= RTTI_MARK_TERR)
		{
			Mark *m;
			switch(item->type())
			{
			case RTTI_MARK_SQUARE: m = (MarkSquare*)item; break;
			case RTTI_MARK_CIRCLE: m = (MarkCircle*)item;/* m->setSmall(setting->readBoolEntry("SMALL_MARKS")); */break;
			case RTTI_MARK_TRIANGLE: m = (MarkTriangle*)item; break;
			case RTTI_MARK_CROSS: m = (MarkCross*)item; break;
			case RTTI_MARK_TEXT: m = (MarkText*)item; break;
			case RTTI_MARK_NUMBER: m = (MarkNumber*)item; break;
			case RTTI_MARK_TERR: m = (MarkTerr*)item; break;
			default: continue;
			}
			m->setSize(square_size, square_size);
			m->setPos(offsetX + square_size * (m->posX() - 1) - m->getSizeX()/2.0,
				offsetY + square_size * (m->posY() - 1) - m->getSizeY()/2.0);
		 }
	}

//	boardHandler->gotoMove(m_save);

	/* FIXME sometimes this draws the lines after/on top of the marks.
	 * moving it earlier doesn't fix anything */

	// Redraw the board
	drawBackground();
	drawGatter();
//	if (showCoords && !isDisplayBoard)
	drawCoordinates();

	// Redraw the mark on the last played stone
	updateLastMove(m_save->getColor(), m_save->getX(), m_save->getY());     //SL added eb 7
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
	QSettings settings;

	int w = (int)canvas->width();
	int h = (int)canvas->height();

	// Create pixmap of appropriate size
	//QPixmap all(w, h);
	QImage image(w, h, QImage::Format_RGB32);

	// Paint table and board on the pixmap
	QPainter painter;

	painter.begin(&image);
	painter.setPen(Qt::NoPen);


	painter.drawTiledPixmap (0, 0, w, h,*(ImageHandler::getTablePixmap(  settings.value("SKIN_TABLE").toString())));

	painter.drawTiledPixmap (offsetX - offset, offsetY - offset, table_size, table_size,
				 * (ImageHandler::getBoardPixmap(settings.value("SKIN").toString())));

	painter.end();

	// Modify the edges of the board so they appear slightly three-dimensional.
	//QImage image = all.toImage();
	int lighter=20;
	int darker=60;
	int width = 3;

	for(int x = 0; x < width; x++)
		for (int yi = x; yi < table_size - x; yi++)
		{
			int y = yi + offsetY - offset;
			int x1 = offsetX - offset + x;
			image.setPixel(x1, y,
				       QColor(image.pixel(x1,y)).dark(int(100 + darker*(width-x)*(width-x)/width/width)).rgb());

			int x2 = offsetX - offset + table_size - x - 1;
			image.setPixel(x2, y,
				       QColor(image.pixel(x2,y)).light(100+ int(lighter*(width-x)*(width-x)/width/width)).rgb());
		}
	for(int y = 0; y < width; y++)
		for (int xi = y; xi < table_size - y; xi++)
		{
			int x = xi + offsetX - offset;
			int y1 = offsetY - offset + y;
			image.setPixel(x, y1,
				       QColor(image.pixel(x, y1)).light(int(100 + lighter*(width-y)*(width-y)/width/width)).rgb());
			int y2 = offsetY - offset + table_size - y - 1;
			image.setPixel(x, y2,
				       QColor(image.pixel(x, y2)).dark(100+ int(darker*(width-y)*(width-y)/width/width)).rgb());
		}

	// Draw a shadow below the board
	width = 10;
	darker=50;

	for (int x = 0; x <= width && offsetX - offset - x > 0; x++)
		for (int yi = x; yi < table_size - x; yi++)
		{
			int y = yi + offsetY - offset;
			image.setPixel(offsetX - offset - 1 - x, y,
				       QColor(image.pixel(offsetX - offset - 1 - x,y)).dark(int(100 + darker*(width-x)/width)).rgb());
		}

	for (int y = 0; y <= width && offsetY + board_pixel_size + offset + y + 1 < h;y++)
		for (int x = (offsetX - offset - y > 0 ? offsetX - offset - y : 0); x < offsetX + board_pixel_size + offset-y ;x++)
		{
			image.setPixel(x, offsetY + board_pixel_size + offset + y +1,
				QColor(image.pixel(x,offsetY + board_pixel_size + offset+y+1)).dark(100+ int(darker*(width-y)/width)).rgb());
		}

	canvas->setBackgroundBrush(QBrush(image));
}

void Board::drawGatter()
{
	gatter->resize(offsetX,offsetY,square_size);
}

void Board::drawCoordinates()
{
	QGraphicsSimpleTextItem *coord;
	int i;

	// centres the coordinates text within the remaining space at table edge
	const int coord_centre = coord_offset/2 + coord_margin;
	QString txt;

	for (i=0; i<board_size; i++)
	{
		// Left side
		coord = vCoords1.at(i);
		coord->setPos(offsetX - offset + coord_centre - coord->boundingRect().width()/2,
			      offsetY + square_size * (board_size - i - 1) - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Right side
		coord = vCoords2.at(i);
    		coord->setPos(offsetX + board_pixel_size + offset - coord_centre - coord->boundingRect().width()/2,
			      offsetY + square_size * (board_size - i - 1) - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Top
		coord = hCoords1.at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2,
			      offsetY - offset + coord_centre - coord->boundingRect().height()/2 );

		if (showCoords)
			coord->show();
		else
			coord->hide();

		// Bottom
		coord = hCoords2.at(i);
		coord->setPos(offsetX + square_size * i - coord->boundingRect().width()/2,
			      offsetY + offset + board_pixel_size - coord_centre - coord->boundingRect().height()/2);

		if (showCoords)
			coord->show();
		else
			coord->hide();
	}
}

void Board::hideStones()  // QQQ
{
	isHidingStones ^= true;
	if (isHidingStones)
		hideAllStones();
	else
		showAllStones();
}

Stone* Board::addStoneSprite(StoneColor c, int x, int y, bool &shown)
{
	if (x < 1 || x > board_size || y < 1 || y > board_size)
	{
		qWarning("Board::addStoneSprite() - Invalid stone: %d %d", x, y);
		return NULL;
	}

	int t = boardHandler->hasStone(x, y);
	if (t == 1) {
		// Stone exists and is visible
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
	}
	if (t == 0)
	{
		// qDebug("*** Did not find any stone at %d, %d.", x, y);

		Stone *s = new Stone(imageHandler->getStonePixmaps(), canvas, c, x, y,WHITE_STONES_NB,true);

		if (isHidingStones) { // QQQ
			s->hide();
		}
		else {
			if (boardHandler->getGameData()->oneColorGo)
				s->toggleOneColorGo(true);
		}

		CHECK_PTR(s);

		s->setPos(offsetX + square_size * (x-1.5), offsetY + square_size * (y-1.5));


		// Change color of a mark on this spot to white, if we have a black stone
		if (c == stoneBlack)
			updateMarkColor(stoneBlack, x, y);

		return s;
	}
	else if (t == -1)
	{
		// Stone exists, but is hidden. Show it and check correct color

		Stone *s = boardHandler->getStoneHandler()->getStoneAt(x, y);
		CHECK_PTR(s);

		// qDebug("*** Found a hidden stone at %d, %d (%s).", x, y,

		// Check if the color is correct
		if (s->getColor() != c)
			s->setColor(c);
		s->show();
		shown = true;

		// Change color of a mark on this spot to white, if we have a black stone
		if (c == stoneBlack)
			updateMarkColor(stoneBlack, x, y);

		return s;
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
		qDebug("posX:%d posY:%d  rtti:%d", m->posX(), m->posY(), m->type());
    }
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

void Board::mouseMoveEvent(QMouseEvent *e)
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

    curStone->setPos(offsetX + square_size * (x-1.5), offsetY + square_size * (y-1.5));

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

void Board::mouseWheelEvent(QWheelEvent *e)
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

void Board::mouseReleaseEvent(QMouseEvent* e)
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

void Board::mousePressEvent(QMouseEvent *e)
{
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
#if 0 // Later.  Maybe.
    resizeBoard(canvas->width() + 20, canvas->height() + 20);
#endif
}

void Board::decreaseSize()
{
#if 0
    QSize s (width()-5, height()-5);
    if (canvas->width() - 20 < s.width() ||
		canvas->height() - 20 < s.height())
		return;
    resizeBoard(canvas->width() - 20, canvas->height() - 20);
#endif
}

void Board::changeSize()
{
#ifdef Q_WS_WIN
    resizeDelayFlag = false;
#endif
    resizeBoard(width(), height());
}

void Board::hideAllStones()
{
	QList<QGraphicsItem *> list = canvas->items();
	QGraphicsItem *item;

	QListIterator<QGraphicsItem *> it( list );


	for (; it.hasNext();)
	{
		item = it.next();
		if (item->type() == RTTI_STONE)
			item->hide();
	}
}

void Board::showAllStones()
{
#if 0 // Later.
    Q3IntDict<Stone>* stones = boardHandler->getStoneHandler()->getAllStones();
    if (stones.isEmpty())
        return;

    Q3IntDictIterator<Stone> it(*stones);
    Stone *s;
    while (s = it.current()) {
        if (isHidingStones)
		s->hide();
        else
		s->show();
        ++it;
    }
#endif
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

bool Board::openSGF(const QString &fileName)
{

    // Load the sgf
    if (!boardHandler->loadSGF(fileName))
		return false;

    canvas->update();
    setModified(false);
    return true;
}

bool Board::startComputerPlay(QNewGameDlg * dlg,const QString &fileName, const QString &computer_path)
{

     // Clean up everything and get to last move
    //clearData();

    // Initiate the dialog with computer
    if (!boardHandler->openComputerSession(dlg,fileName,computer_path))
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
	clearCoords();
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
		m = new MarkText(x, y, square_size, txt, canvas, col, n, false, overlay);
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
		m = new MarkNumber(x, y, square_size, n, canvas, col, false);
		setMarkText(x, y, txt);
		gatter->hide(x,y);
		break;

    case markTerrBlack:
		m = new MarkTerr(x, y, square_size, stoneBlack, canvas);
		if (boardHandler->hasStone(x, y) == 1)
		{
			Stone *s = boardHandler->getStoneHandler()->getStoneAt(x, y);
			s->setDead(true);
			s->togglePixmap(boardHandler->board->getImageHandler()->getGhostPixmaps(),
					false);
			boardHandler->markedDead = true;
		}
		boardHandler->getTree()->getCurrent()->setScored(true);
		break;

    case markTerrWhite:
		m = new MarkTerr(x, y, square_size, stoneWhite, canvas);
		if (boardHandler->hasStone(x, y) == 1)
		{
			Stone *s = boardHandler->getStoneHandler()->getStoneAt(x, y);
			s->setDead(true);
			s->togglePixmap(boardHandler->board->getImageHandler()->getGhostPixmaps(),
					false);
			boardHandler->markedDead = true;
		}
		boardHandler->getTree()->getCurrent()->setScored(true);
		break;

    default:
		qWarning("   *** Board::setMark() - Bad mark type! ***");
		return;
    }

    CHECK_PTR(m);
    m->setPos(offsetX + square_size * (x-1) - m->getSizeX()/2, offsetY + square_size * (y-1) - m->getSizeY()/2);
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
    m->setPos(offsetX + square_size * (x-1) - m->getSizeX()/2, offsetY + square_size * (y-1) - m->getSizeY()/2);

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
			lastMoveMark = new MarkRedCircle(x, y, square_size, canvas); // QQQ
		else
			lastMoveMark = new MarkCross(x, y, square_size, canvas,
				c == stoneBlack ? Qt::white : Qt::black, true);


		ASSERT(lastMoveMark);

		lastMoveMark->setPos(offsetX + square_size * (x-1) - lastMoveMark->getSizeX()/2,
				     offsetY + square_size * (y-1) - lastMoveMark->getSizeY()/2);
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
			m->type() != RTTI_MARK_TERR &&
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
		if (m->posX() == x && m->posY() == y && m->type() != RTTI_MARK_TERR)
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
		s->setPos(offsetX + square_size * (board_size+1),
			  offsetY + square_size * board_size);
		setMark(board_size+2, board_size+1, markText, false, tr("Pass"), false);
	}
	else
	{
		s->setPos(offsetX + square_size * (x-1) - s->boundingRect().width()/2,
			  offsetY + square_size * (y-1) - s->boundingRect().height()/2);
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

	// and setup back
	delete gatter;
	gatter = new Gatter(canvas, board_size);
	setupCoords();
	changeSize();

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

