/*
* gametree.cpp
*/

#include "config.h"
#include "setting.h"
#include "gametree.h"
#include "globals.h"
#include "mark.h"
#include "imagehandler.h"
#include "stonehandler.h"
#include "tip.h"
#include "interfacehandler.h"
#include "move.h"
#include "mainwindow.h"
#include "noderesults.h"
#include <qmessagebox.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qgroupbox.h>

GameTree::GameTree(QWidget *parent, const char *name, QCanvas* c)
: QCanvasView(c, parent, name)
{
}

GameTree::~GameTree()
{
}
