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
#include <q3groupbox.h>

GameTree::GameTree(QWidget *parent, const char *name, Q3Canvas* c)
: Q3CanvasView(c, parent, name)
{
}

GameTree::~GameTree()
{
}
