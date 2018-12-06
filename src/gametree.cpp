/*
* gametree.cpp
*/

#include "config.h"
#include "setting.h"
#include "gametree.h"
#include "globals.h"
#include "imagehandler.h"

GameTree::GameTree(QWidget *parent, const char *name, QGraphicsScene* c)
  : QGraphicsView(c, parent)
{
}

GameTree::~GameTree()
{
}
