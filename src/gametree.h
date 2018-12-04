/*
 * gametree.h
 */

#ifndef GAMETREE_H
#define GAMETREE_H

#include <QGraphicsView>
#include "defines.h"
#include "setting.h"
#include "boardhandler.h"
#include "stone.h"

class ImageHandler;
class Mark;
class Tip;
class InterfaceHandler;
class GameData;
class NodeResults;

class GameTree : public QGraphicsView
{
	Q_OBJECT

public:
	GameTree(QWidget *parent=0, const char *name=0, QGraphicsScene* c=0);
	~GameTree();
};

#endif
