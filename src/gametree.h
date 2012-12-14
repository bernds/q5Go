/*
 * gametree.h
 */

#ifndef GAMETREE_H
#define GAMETREE_H

#include "defines.h"
#include "setting.h"
#include "boardhandler.h"
#include "stone.h"
#include <Q3Canvas>

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class GameData;
class NodeResults;

class GameTree : public Q3CanvasView
{
	Q_OBJECT

public:
	GameTree(QWidget *parent=0, const char *name=0, Q3Canvas* c=0);
	~GameTree();
};

#endif
