/***************************************************************************
                          qgtp.h  -  description
                             -------------------
    begin                : Thu Nov 29 2001
    copyright            : (C) 2001 by PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGTP_H
#define QGTP_H

#include <qapplication.h>
#include <q3process.h>
//#include "global.h"

#define IGTP_BUFSIZE 2048    /* Size of the response buffer */
#define OK 0
#define FAIL -1


/**
  *@author PALM Thomas , DINTILHAC Florian, HIVERT Anthony, PIOC Sebastien
  */

class QGtp : public QObject{
	Q_OBJECT

public slots:
	void slot_readFromStdout();
	void slot_processExited();

public:
	QGtp();
	~QGtp();

	/****************************
 	*                          *
 	****************************/
	QString getLastMessage();
	int openGtpSession(QString filename, int size, float komi, int handicap, int level);
	Q3Process  * programProcess ;
	void fflush(char * s);

	/****************************
 	* Administrative commands. *
 	****************************/
	int quit ();

	/****************************
 	* Program identity.        *
 	****************************/
	int name ();
	int protocolVersion ();
	int version ();

	/***************************
 	* Setting the board size. *
 	***************************/
	int setBoardsize (int size);
	int queryBoardsize();

	/***************************
	* Clearing the board.     *
 	***************************/
	int clearBoard ();

	/************
 	* Setting. *
 	************/
	int setKomi (float k);
	int setLevel (int level);

	/******************
 	* Playing moves. *
 	******************/
	int playblack (char c, int j);
	int playblackPass ();
	int playwhite (char c, int j);
	int playwhitePass ();
	int fixedHandicap (int handicap);
	int loadsgf (QString filename,int movNumber=0,char c='A',int i=0);

	/*****************
	* Board status. *
 	*****************/
	int whatColor (char c, int j);
	int countlib (char c, int j);
	int findlib (char c, int j);
	int isLegal (QString color, char c, int j);
	int allLegal (QString color);
	int captures (QString color);

	/**********************
	* Retractable moves. *
	**********************/
	int trymove (QString color, char c, int j);
	int popgo ();

	/*********************
	 * Tactical reading. *
 	*********************/
	int attack (char c, int j);
	int defend (char c, int j);
	int increaseDepths ();
	int decreaseDepths ();

	/******************
	 * owl reading. *
	 ******************/
	int owlAttack (char c, int j);
	int owlDefend (char c, int j);

	/********
 	* eyes *
	 ********/
	int evalEye (char c, int j);

	/*****************
 	* dragon status *
	 *****************/
	int dragonStatus (char c, int j);
	int sameDragon (char c1, int i1, char c2, int i2);
	int dragonData ();
	int dragonData (char c, int i);

	/***********************
 	* combination attacks *
 	***********************/
	int combinationAttack (QString color);

	/********************
	 * generating moves *
	 ********************/
	int genmoveBlack ();
	int genmoveWhite ();
	int genmove (QString color,int seed=0);
	int topMovesBlack ();
	int topMovesWhite ();
	int undo (int i=1);
	int finalStatus (char c, int i, int seed=0);
	int finalStatusList (QString status, int seed=0);

	/**************
	 * score *
	 **************/
	int finalScore (int seed=0);
	int estimateScore ();
	int newScore ();

	/**************
	 * statistics *
 	**************/
	int getLifeNodeCounter ();
	int getOwlNodeCounter ();
	int getReadingNodeCounter ();
	int getTrymoveCounter ();
	int resetLifeNodeCounter ();
	int resetOwlNodeCounter ();
	int resetReadingNodeCounter ();
	int resetTrymoveCounter ();

	/*********
 	* debug *
	*********/
	int showboard ();
	int dumpStack ();
	int debugInfluence (QString color, QString list);
	int debugMoveInfluence (QString color, char c, int i, QString list);
	int influence (QString color);
	int moveInfluence (QString color, char c, int i);
	int wormCutstone (char c, int i);
	int wormData ();
	int wormData (char c, int i);
	int wormStones ();
	int tuneMoveOrdering (int MOVE_ORDERING_PARAMETERS);
	int echo (QString param);
	int help ();
	int reportUncertainty (QString s);
  int shell(QString s);
  int knownCommand(QString s);

	private :

	int  _cpt;	/* Number of the request */
	char *_outFile;
	char *outFile;
	FILE *_inFile;
	QString _response;

	int waitResponse();
	int waitResponseOld();
};




#endif
