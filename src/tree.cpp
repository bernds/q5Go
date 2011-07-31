/*
* tree.cpp
*/

#include "tree.h"
#include "move.h"
#include "qgo.h"
#include <iostream.h>
#include <qptrstack.h>

Tree::Tree(int board_size)
{
	root = new Move(board_size);
	current = root;
}

void Tree::init(int board_size)
{
	clear();
	root = new Move(board_size);
	current = root;
}

Tree::~Tree()
{
	clear();
}

bool Tree::addBrother(Move *node)
{
	if (root == NULL)
	{
		qFatal("Error: No root!");
	}
	else
	{
		if (current == NULL)
		{
			qFatal("Error: No current node!");
			return false;
		}
		
		Move *tmp = current;
		
		// Find brother farest right
		while (tmp->brother != NULL)
			tmp = tmp->brother;
		
		tmp->brother = node;
		node->parent = current->parent;
		node->setTimeinfo(false);
	}
	
	current = node;
	
	return true;
}

// Returns: True  - A son found, added as brother
//          False - No son found, added as first son
bool Tree::addSon(Move *node)
{
	if (root == NULL)
	{
		qFatal("Error: No root!");
		return false;
	}
	else
	{
		if (current == NULL)
		{
			qFatal("Error: No current node!");
			return false;
		}
		
		// current node has no son?
		if (current->son == NULL)
		{
			current->son = node;
			node->parent = current;
			node->setTimeinfo(false);
			current = node;
			return false;
		}
		// A son found. Add the new node as farest right brother of that son
		else
		{
			current = current->son;
			if (!addBrother(node))
			{
				qFatal("Failed to add a brother.");
				return false;
			}
			current = node;
			return true;
		}
	}
}

int Tree::getNumBrothers()
{
	if (current == NULL || current->parent == NULL)
		return 0;
	
	Move *tmp = current->parent->son;
	int counter = 0;
	
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}

bool Tree::hasNextBrother()
{
	if (current == NULL || current->brother == NULL)
		return false;
	
	return current->brother != NULL;
}

bool Tree::hasPrevBrother(Move *m)
{
	if (m == NULL)
		m = current;
	
	if (root == NULL || m == NULL)
		return false;
	
	Move *tmp;
	
	if (m->parent == NULL)
	{
		if (m == root)
			return false;
		else
			tmp = root;
	}
	else
		tmp = m->parent->son;
	
	return tmp != m;
}

int Tree::getNumSons(Move *m)
{
	if (m == NULL)
	{
		if (current == NULL)
			return 0;
		
		m = current;
	}
	
	Move *tmp = m->son;
	
	if (tmp == NULL)
		return 0;
	
	int counter = 1;
	while ((tmp = tmp->brother) != NULL)
		counter ++;
	
	return counter;
}

int Tree::getBranchLength(Move *node)
{
	Move *tmp;
	
	if (node == NULL)
	{
		if (current == NULL)
			return -1;
		tmp = current;
	}
	else
		tmp = node;
	
	CHECK_PTR(tmp);
	
	int counter=0;
	// Go down the current branch, use the marker if possible to remember a previously used path
	while ((tmp = (tmp->marker != NULL ? tmp->marker : tmp->son)) != NULL)
		counter ++;
	
	return counter;
}

Move* Tree::nextMove()
{
	if (root == NULL || current == NULL || current->son == NULL)
		return NULL;
	
	if (current->marker == NULL)  // No marker, simply take the main son
		current = current->son;
	else
		current = current->marker;  // Marker set, use this to go the remembered path in the tree
	
	current->parent->marker = current;  // Parents remembers this move we went to
	return current;
}

Move* Tree::previousMove()
{
	if (root == NULL || current == NULL || current->parent == NULL)
		return NULL;
	
	current->parent->marker = current;  // Remember the son we came from
	current = current->parent;          // Move up in the tree
	return current;
}

Move* Tree::nextVariation()
{
	if (root == NULL || current == NULL || current->brother == NULL)
		return NULL;
	
	current = current->brother;
	return current;
}

Move* Tree::previousVariation()
{
	if (root == NULL || current == NULL) // || current->parent == NULL)
		return NULL;
	
	Move *tmp, *old;
	
	if (current->parent == NULL)
	{
		if (current == root)
			return NULL;
		else
			tmp = root;
	}
	else
		tmp = current->parent->son;
	
	old = tmp;
	
	while ((tmp = tmp->brother) != NULL)
	{
		if (tmp == current)
			return current = old;
		old = tmp;
	}
	
	return NULL;
}

bool Tree::hasSon(Move *m)
{
	if (root == NULL || m == NULL || current == NULL || current->son == NULL)
		return false;
	
	Move *tmp = current->son;
	
	do {
		if (m->equals(tmp))
		{
			current = tmp;
			return true;
		}
	} while ((tmp = tmp->brother) != NULL);
	
	return false;
}

void Tree::clear()
{
#ifndef NO_DEBUG
	qDebug("Tree had %d nodes.", count());
#endif
	
	if (root == NULL)
		return;
	
	traverseClear(root);
	
	root = NULL;
	current = NULL;
}

void Tree::traverseClear(Move *m)
{
	CHECK_PTR(m);
	QPtrStack<Move> stack;
	QPtrStack<Move> trash;
	trash.setAutoDelete(TRUE);
	Move *t = NULL;
	
	// Traverse the tree and drop every node into stack trash
	stack.push(m);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			trash.push(t);
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
	
	// Clearing this stack deletes all moves. Smart, eh?
	trash.clear();
}

// Find a move starting from the given in the argument in the all following branches
// Results are saved into the reference variable result.
void Tree::traverseFind(Move *m, int x, int y, QPtrStack<Move> &result)
{
	CHECK_PTR(m);
	QPtrStack<Move> stack;
	Move *t = NULL;
	
	// Traverse the tree and drop every node into stack result
	stack.push(m);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			if (t->getX() == x && t->getY() == y)
				result.push(t);
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
}

Move* Tree::findMove(Move *start, int x, int y, bool checkmarker)
{
	if (start == NULL)
		return NULL;
	
	Move *t = start;
	
	do {
		if (t->getX() == x && t->getY() == y)
			return t;
		if (checkmarker && t->marker != NULL)
			t = t->marker;
		else
			t = t->son;
	} while (t != NULL);
	
	return NULL;
}

int Tree::count()
{
	if (root == NULL)
		return 0;
	
	QPtrStack<Move> stack;
	int counter = 0;
	Move *t = NULL;
	
	// Traverse the tree and count all moves
	stack.push(root);
	
	while (!stack.isEmpty())
	{
		t = stack.pop();
		if (t != NULL)
		{
			counter ++;
			stack.push(t->brother);
			stack.push(t->son);
		}
	}
	
	return counter;
}

void Tree::setToFirstMove()
{
	if (root == NULL)
		qFatal("Error: No root!");
	
	current = root;
}

int Tree::mainBranchSize()
{
	if (root == NULL || current == NULL )//|| current->son == NULL)
		return 0;
	
	Move *tmp = root;
	
	int counter = 1;
	while ((tmp = tmp->son) != NULL)
		counter ++;
	
	return counter;    
}


Move *Tree::findLastMoveInMainBranch()       // added eb 9
{
  Move *m = root;
  CHECK_PTR(m);

  if (m==NULL)
    return NULL;
  
	// Descend tree until root reached
	while (m->son != NULL)
		m = m->son;

  return m;
}                                           // end add eb 9

