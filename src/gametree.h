/*
 * gametree.h
 */

#ifndef GAMETREE_H
#define GAMETREE_H

#include "defines.h"
#include "setting.h"
#include "boardhandler.h"
#include "stone.h"
#include <qcanvas.h>

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class GameData;
class NodeResults;

class GameTree : public QCanvasView
{
	Q_OBJECT

public:
	GameTree(QWidget *parent=0, const char *name=0, QCanvas* c=0);
	~GameTree();
};

#endif
