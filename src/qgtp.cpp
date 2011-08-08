/***************************************************************************
qgtp.cpp  -  description
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


//#include <unistd.h>
#include <stdlib.h>
#include <q3process.h>
#include "defines.h"
#include "qgtp.h"

#ifdef Q_WS_WIN
#include <Windows.h>
#endif

/* Function:  open a session
* Arguments: name and path of go program
* Fails:     never
* Returns:   nothing
*/
QGtp::QGtp()
{
	/*openGtpSession(filename);*/
	programProcess = NULL; // This to permit proper ending
}


QGtp::~QGtp()
{
	if (programProcess)
		programProcess->kill();     // not very clean (should use tryTerminate)
}

/* Code */

/* Function:  get the last message from gnugo
* Arguments: none
* Fails:     never
* Returns:   last message from GO
*/


QString QGtp::getLastMessage()
{
	return _response;
}

int QGtp::openGtpSession(QString filename, int size, float komi, int handicap, int level)
{
	_cpt = 1;
	
    programProcess = new Q3Process();
    
    programProcess->clearArguments();
    programProcess->addArgument(filename);
    programProcess->addArgument("--mode");
    programProcess->addArgument("gtp");
    programProcess->addArgument("--quiet");  
	
    connect(programProcess, SIGNAL(readyReadStdout()),
		this, SLOT(slot_readFromStdout()) );
    connect(programProcess, SIGNAL(processExited()),
		this, SLOT(slot_processExited()) );
	
	if (!programProcess->start())
	{
		  _response="Could not start "+filename;
		  return FAIL ;
	}
	
    //?? never know ... otherwise, I get a segfault with sprint ...
	if ((outFile = (char *)malloc (100)) == NULL)
	{
		  _response="Yuck ! Could not allocate 100 bytes !!!"  ;
		  return FAIL ;
	}   
	
	if (protocolVersion()==OK)
	{
		if(getLastMessage().toInt() !=2)
		{
			qDebug("Protocol version problem???");
			//				quit();
			_response="Protocol version not supported";
			//				return FAIL;
		}
		if(setBoardsize(size)==FAIL)
		{
			return FAIL;
		}
		if(clearBoard()==FAIL)
		{
			// removed by frosla -> protocol changes...
			// return FAIL;
		}


		if(knownCommand("level")==FAIL)
    		{
			  return FAIL;
		}
	
		else if (getLastMessage().contains("true"))
        	{
        		if (setLevel(level)==FAIL)
			{
				return FAIL;
			}
        	}
 
      
		if(setKomi(komi)==FAIL)
		{
			return FAIL;
		}
		if(fixedHandicap(handicap)==FAIL)
		{
			return FAIL;
		}
	}
	else
	{
		quit();
		_response="Protocol version error";
		return FAIL;
	}
	//}
	return OK;
}

// Read from stdout
void QGtp::slot_readFromStdout()
{
	QString buff;
	
	while (programProcess->canReadLineStdout())
	{
		buff=programProcess->readLineStdout();
		buff=buff.stripWhiteSpace();
		if (buff.length() != 0)
		{
			_response = buff;
//			qDebug(QString("** QGtp::slot_readFromStdout(): [%1]").arg(_response));
		}
	}
}

// exit
void QGtp::slot_processExited()
{
	qDebug() << _cpt << " quit";
	sprintf (outFile, "%d quit\n", _cpt);
	fflush(outFile);
	//	return waitResponse();
}


/* Function:  wait for a go response
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::waitResponse()
{
	QString buff = _response;
	//	QTextStream * inFile;
	//	char symbole;
	int number;
	int pos;
	
	do
	{
		qApp->processEvents();
/*#ifdef Q_WS_WIN
		Sleep(100000);
#else
		usleep(100000);
#endif
*/
	} while (_response.isEmpty() || _response == buff);
	
	/*
	inFile=new QTextStream(programProcess->readStdout(),IO_ReadOnly);
	do
	{
	* inFile>>symbole;
	* inFile>>number;
	buff=inFile->readLine();
	} while(number !=_cpt);
	*/
	
	//	_response=buff.stripWhiteSpace();
//	qDebug(QString("** QGtp::waitResponse(): [%1]").arg(_response));
	/*	
	buff=programProcess->readLineStdout();
	while(!buff.isEmpty())
	{
	_response=_response+"\n"+buff;
	buff=programProcess->readLineStdout();
	}
	*/

	QChar r0 = _response[0];
	if ((pos = _response.find(' ')) < 1)
		pos = 1;
	number = _response.mid(1,pos).toInt();
	_response = _response.right(_response.length() - pos - 1);

	qDebug() << QString("** QGtp::waitResponse(): [%1]").arg(_response);

	if (r0 == '?') //symbole=='?')
	{
		return FAIL;
	}
	else
	{
		return OK;
	}
}

/****************************
* Program identity.        *
****************************/

/* Function:  Report the name of the program.
* Arguments: none
* Fails:     never
* Returns:   program name
*/
int
QGtp::name ()
{
	qDebug() << _cpt << " name";
	sprintf (outFile, "%d name\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Report protocol version.
* Arguments: none
* Fails:     never
* Returns:   protocol version number
*/
int
QGtp::protocolVersion ()
{
	qDebug() << _cpt << " protocol_version";
	sprintf (outFile, "%d protocol_version\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

void QGtp::fflush(char * s)
{
/*
int msglen = strlen(s);

  QByteArray dat(msglen);
  for (int j = 0; j < msglen; j++) {
  dat[j] = s[j];
  }
  programProcess->writeToStdin(dat);
	*/
	_cpt++;

	qDebug() << "flush -> " << s;
	programProcess->writeToStdin(QString(s));
	
	
}

/****************************
* Administrative commands. *
****************************/

/* Function:  Quit
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::quit ()
{
	sprintf (outFile, "%d quit\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Report the version number of the program.
* Arguments: none
* Fails:     never
* Returns:   version number
*/
int
QGtp::version ()
{
	sprintf (outFile, "%d version\n", _cpt);
	fflush(outFile);
	return waitResponse();
}


/***************************
* Setting the board size. *
***************************/

/* Function:  Set the board size to NxN.
* Arguments: integer
* Fails:     board size outside engine's limits
* Returns:   nothing
*/
int
QGtp::setBoardsize (int size)
{
	sprintf (outFile, "%d boardsize %d\n", _cpt, size);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Find the current boardsize
* Arguments: none
* Fails:     never
* Returns:   board_size
*/
int
QGtp::queryBoardsize()
{
	sprintf (outFile, "%d query_boardsize\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/***********************
* Clearing the board. *
***********************/

/* Function:  Clear the board.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/

int
QGtp::clearBoard ()
{
    sprintf (outFile, "%d clear_board\n", _cpt);
    fflush(outFile);
    return waitResponse();
}

/***************************
* Setting.           *
***************************/

/* Function:  Set the komi.
* Arguments: float
* Fails:     incorrect argument
* Returns:   nothing
*/
int
QGtp::setKomi(float f)
{
	sprintf (outFile, "%d komi %.2f\n", _cpt,f);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Set the playing level.
* Arguments: int
* Fails:     incorrect argument
* Returns:   nothing
*/
int
QGtp::setLevel (int level)
{
	sprintf (outFile, "%d level %d\n", _cpt,level);
	fflush(outFile);
	return waitResponse();
}
/******************
* Playing moves. *
******************/

/* Function:  Play a black stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::playblack (char c , int i)
{
	//  sprintf (outFile, "%d play black %c%d\n", _cpt,c,i);
	sprintf (outFile, "%d play black %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Black pass.
* Arguments: None
* Fails:     never
* Returns:   nothing
*/
int
QGtp::playblackPass ()
{
	//  sprintf (outFile, "%d play black pass\n", _cpt);
	sprintf (outFile, "%d play black pass\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Play a white stone at the given vertex.
* Arguments: vertex
* Fails:     invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::playwhite (char c, int i)
{
	//  sprintf (outFile, "%d play white %c%d\n", _cpt,c,i);
	sprintf (outFile, "%d play white %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  White pass.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::playwhitePass ()
{
	//  sprintf (outFile, "%d play white pass\n", _cpt);
	sprintf (outFile, "%d play white pass\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Set up fixed placement handicap stones.
* Arguments: number of handicap stones
* Fails:     invalid number of stones for the current boardsize
* Returns:   list of vertices with handicap stones
*/
int
QGtp::fixedHandicap (int handicap)
{
	if (handicap < 2) 
		return OK;
	
	sprintf (outFile, "%d fixed_handicap %d\n", _cpt,handicap);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Load an sgf file, possibly up to a move number or the first
*            occurence of a move.
* Arguments: filename + move number, vertex, or nothing
* Fails:     missing filename or failure to open or parse file
* Returns:   color to play
*/
int
QGtp::loadsgf (QString filename,int movNumber,char c,int i)
{
	//sprintf (outFile, "%d loadsgf %s %d %c%d\n", _cpt,(const char *) filename,movNumber,c,i);
	sprintf (outFile, "%d loadsgf %s\n", _cpt,(const char *) filename);
	fflush(outFile);
	return waitResponse();
}

/*****************
* Board status. *
*****************/

/* Function:  Return the color at a vertex.
* Arguments: vertex
* Fails:     invalid vertex
* Returns:   "black", "white", or "empty"
*/
int
QGtp::whatColor (char c, int i)
{
	sprintf (outFile, "%d color %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Count number of liberties for the string at a vertex.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   Number of liberties.
*/
int
QGtp::countlib (char c, int i)
{
	sprintf (outFile, "%d countlib %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return the positions of the liberties for the string at a vertex.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   Sorted space separated list of vertices.
*/
int
QGtp::findlib (char c, int i)
{
	sprintf (outFile, "%d findlib %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Tell whether a move is legal.
* Arguments: move
* Fails:     invalid move
* Returns:   1 if the move is legal, 0 if it is not.
*/
int
QGtp::isLegal (QString color, char c, int i)
{
	sprintf (outFile, "%d is_legal %s %c%d\n", _cpt,(const char *)color,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  List all legal moves for either color.
* Arguments: color
* Fails:     invalid color
* Returns:   Sorted space separated list of vertices.
*/
int
QGtp::allLegal (QString color)
{
	sprintf (outFile, "%d all_legal %s\n", _cpt,(const char *)color);
	fflush(outFile);
	return waitResponse();
}

/* Function:  List the number of captures taken by either color.
* Arguments: color
* Fails:     invalid color
* Returns:   Number of captures.
*/
int
QGtp::captures (QString color)
{
	sprintf (outFile, "%d captures %s\n", _cpt,(const char *)color);
	fflush(outFile);
	return waitResponse();
}

/**********************
* Retractable moves. *
**********************/

/* Function:  Play a stone of the given color at the given vertex.
* Arguments: move (color + vertex)
* Fails:     invalid color, invalid vertex, illegal move
* Returns:   nothing
*/
int
QGtp::trymove (QString color, char c, int i)
{
	sprintf (outFile, "%d trymove %s %c%d\n", _cpt,(const char *)color,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Undo a trymove.
* Arguments: none
* Fails:     stack empty
* Returns:   nothing
*/
int
QGtp::popgo ()
{
	sprintf (outFile, "%d popgo\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/*********************
* Tactical reading. *
*********************/

/* Function:  Try to attack a string.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   attack code followed by attack point if attack code nonzero.
*/
int
QGtp::attack (char c, int i)
{
	sprintf (outFile, "%d attack %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Try to defend a string.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   defense code followed by defense point if defense code nonzero.
*/
int
QGtp::defend (char c, int i)
{
	sprintf (outFile, "%d defend %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Increase depth values by one.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::increaseDepths ()
{
	sprintf (outFile, "%d increase_depths\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Decrease depth values by one.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::decreaseDepths ()
{
	sprintf (outFile, "%d decrease_depths\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/******************
* owl reading. *
******************/

/* Function:  Try to attack a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   attack code followed by attack point if attack code nonzero.
*/
int
QGtp::owlAttack (char c, int i)
{
	sprintf (outFile, "%d owl_attack %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Try to defend a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   defense code followed by defense point if defense code nonzero.
*/
int
QGtp::owlDefend (char c, int i)
{
	sprintf (outFile, "%d owl_defend %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/********
* eyes *
********/

/* Function:  Evaluate an eye space
* Arguments: vertex
* Fails:     invalid vertex
* Returns:   Minimum and maximum number of eyes. If these differ an
*            attack and a defense point are additionally returned.
*            If the vertex is not an eye space or not of unique color,
*            a single -1 is returned.
*/

int
QGtp::evalEye (char c, int i)
{
	sprintf (outFile, "%d eval_eye %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/*****************
* dragon status *
*****************/

/* Function:  Determine status of a dragon.
* Arguments: vertex
* Fails:     invalid vertex, empty vertex
* Returns:   status ("alive", "critical", "dead", or "unknown"),
*            attack point, defense point. Points of attack and
*            defense are only given if the status is critical and the
*            owl code is enabled.
*
* FIXME: Should be able to distinguish between life in seki
*        and life with territory. Should also be able to identify ko.
*/

int
QGtp::dragonStatus (char c, int i)
{
	sprintf (outFile, "%d dragon_status %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Determine whether two stones belong to the same dragon.
* Arguments: vertex, vertex
* Fails:     invalid vertex, empty vertex
* Returns:   1 if the vertices belong to the same dragon, 0 otherwise
*/

int
QGtp::sameDragon (char c1, int i1, char c2, int i2)
{
	sprintf (outFile, "%d same_dragon %c%d %c%d\n", _cpt,c1,i1,c2,i2);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return the information in the dragon data structure.
* Arguments: nothing
* Fails:     never
* Returns:   Dragon data formatted in the corresponding way to gtp_worm__
*/
int
QGtp::dragonData ()
{
	sprintf (outFile, "%d dragon_data \n", _cpt);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Return the information in the dragon data structure.
* Arguments: intersection
* Fails:     invalid coordinate
* Returns:   Dragon data formatted in the corresponding way to gtp_worm__
*/
int
QGtp::dragonData (char c,int i)
{
	sprintf (outFile, "%d dragon_data %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}


/***********************
* combination attacks *
***********************/

/* Function:  Find a move by color capturing something through a
*            combination attack.
* Arguments: color
* Fails:     invalid color
* Returns:   Recommended move, PASS if no move found
*/

int
QGtp::combinationAttack (QString color)
{
	sprintf (outFile, "%d combination_attack %s\n", _cpt,(const char *)color);
	fflush(outFile);
	return waitResponse();
}

/********************
* generating moves *
********************/

/* Function:  Generate and play the supposedly best black move.
* Arguments: none
* Fails:     never
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmoveBlack ()
{
	sprintf (outFile, "%d genmove black\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Generate and play the supposedly best white move.
* Arguments: none
* Fails:     never
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmoveWhite ()
{
	sprintf (outFile, "%d genmove white\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Generate the supposedly best move for either color.
* Arguments: color to move, optionally a random seed
* Fails:     invalid color
* Returns:   a move coordinate (or "PASS")
*/
int
QGtp::genmove (QString color,int seed)
{
	sprintf (outFile, "%d gg_genmove %s %d\n", _cpt,(const char *)color,seed);
	fflush(outFile);
	return waitResponse();
}

/* Function : Generate a list of the best moves for White with weights
* Arguments: none
* Fails:   : never
* Returns  : list of moves with weights
*/

int
QGtp::topMovesWhite ()
{
	sprintf (outFile, "%d top_moves_white\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function : Generate a list of the best moves for Black with weights
* Arguments: none
* Fails:   : never
* Returns  : list of moves with weights
*/

int
QGtp::topMovesBlack ()
{
	sprintf (outFile, "%d top_moves_black\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Undo last move
* Arguments: int
* Fails:     If move pointer is 0
* Returns:   nothing
*/
int
QGtp::undo (int i)
{
	sprintf (outFile, "%d undo %d\n", _cpt,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Report the final status of a vertex in a finished game.
* Arguments: Vertex, optional random seed
* Fails:     invalid vertex
* Returns:   Status in the form of one of the strings "alive", "dead",
*            "seki", "white_territory", "black_territory", or "dame".
*/
int
QGtp::finalStatus (char c, int i, int seed)	
{
	sprintf (outFile, "%d final_status %c%d %d\n", _cpt,c,i,seed);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Report vertices with a specific final status in a finished game.
* Arguments: Status in the form of one of the strings "alive", "dead",
*            "seki", "white_territory", "black_territory", or "dame".
*            An optional random seed can be added.
* Fails:     missing or invalid status string
* Returns:   Vertices having the specified status. These are split with
*            one string on each line if the vertices are nonempty (i.e.
*            for "alive", "dead", and "seki").
*/
int
QGtp::finalStatusList (QString status, int seed)
{
	sprintf (outFile, "%d final_status_list %s %d\n", _cpt,(const char *)status,seed);
	fflush(outFile);
	return waitResponse();
}
/**************
* score *
**************/

/* Function:  Compute the score of a finished game.
* Arguments: Optional random seed
* Fails:     never
* Returns:   Score in SGF format (RE property).
*/
int
QGtp::finalScore (int seed)
{
	sprintf (outFile, "%d final_score %d\n", _cpt,seed);
	fflush(outFile);
	return waitResponse();
}

int
QGtp::estimateScore ()
{
	sprintf (outFile, "%d estimate_score\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

int
QGtp::newScore ()
{
	sprintf (outFile, "%d new_score \n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/**************
* statistics *
**************/

/* Function:  Reset the count of life nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetLifeNodeCounter ()
{
	sprintf (outFile, "%d reset_life_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Retrieve the count of life nodes.
* Arguments: none
* Fails:     never
* Returns:   number of life nodes
*/
int
QGtp::getLifeNodeCounter ()
{
	sprintf (outFile, "%d get_life_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Reset the count of owl nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetOwlNodeCounter ()
{
	sprintf (outFile, "%d reset_owl_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Retrieve the count of owl nodes.
* Arguments: none
* Fails:     never
* Returns:   number of owl nodes
*/
int
QGtp::getOwlNodeCounter ()
{
	sprintf (outFile, "%d get_owl_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Reset the count of reading nodes.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetReadingNodeCounter ()
{
	sprintf (outFile, "%d reset_reading_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Retrieve the count of reading nodes.
* Arguments: none
* Fails:     never
* Returns:   number of reading nodes
*/
int
QGtp::getReadingNodeCounter ()
{
	sprintf (outFile, "%d get_reading_node_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Reset the count of trymoves/trykos.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::resetTrymoveCounter ()
{
	sprintf (outFile, "%d reset_trymove_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Retrieve the count of trymoves/trykos.
* Arguments: none
* Fails:     never
* Returns:   number of trymoves/trykos
*/
int
QGtp::getTrymoveCounter ()
{
	sprintf (outFile, "%d get_trymove_counter\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/*********
* debug *
*********/

/* Function:  Write the position to stderr.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::showboard ()
{
	sprintf (outFile, "%d showboard\n", _cpt);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Dump stack to stderr.
* Arguments: none
* Fails:     never
* Returns:   nothing
*/
int
QGtp::dumpStack ()
{
	sprintf (outFile, "%d dump_stack\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Write information about the influence function to stderr.
* Arguments: color to move, optionally a list of what to show
* Fails:     invalid color
* Returns:   nothing
*/
int
QGtp::debugInfluence (QString color,QString list)
{
	sprintf (outFile, "%d debug_influence %s %s\n", _cpt,(const char *)color,(const char *)list);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Write information about the influence function after making
*            a move to stderr.
* Arguments: move, optionally a list of what to show
* Fails:     never
* Returns:   nothing
*/
int
QGtp::debugMoveInfluence (QString color, char c, int i,QString list)
{
	sprintf (outFile, "%d debug_move_influence %s %c%d %s\n", _cpt,(const char *)color,c,i,(const char *)list);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return information about the influence function.
* Arguments: color to move
* Fails:     never
* Returns:   Influence data formatted
*/
int
QGtp::influence (QString color)
{
	sprintf (outFile, "%d influence %s\n", _cpt,(const char *)color);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return information about the influence function after a move
* Arguments: move
* Fails:     never
* Returns:   Influence data formatted in the same way as for gtp_influence.
*/
int
QGtp::moveInfluence (QString color, char c, int i)
{
	sprintf (outFile, "%d move_influence %s %c%d\n", _cpt,(const char *)color,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return the information in the worm data structure.
* Arguments: none
* Fails:     never
* Returns:   Worm data formatted
*/
int
QGtp::wormData ()
{
	sprintf (outFile, "%d worm_data\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Return the information in the worm data structure.
* Arguments: vertex
* Fails:     never
* Returns:   Worm data formatted
*/
int
QGtp::wormData (char c, int i)
{
	sprintf (outFile, "%d worm_data %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Return the cutstone field in the worm data structure.
* Arguments: non-empty vertex
* Fails:     never
* Returns:   cutstone
*/
int
QGtp::wormCutstone (char c, int i)
{
	sprintf (outFile, "%d worm_cutstone %c%d\n", _cpt,c,i);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Tune the parameters for the move ordering in the tactical
*            reading.
* Arguments: MOVE_ORDERING_PARAMETERS integers
* Fails:     incorrect arguments
* Returns:   nothing
*/
int
QGtp::tuneMoveOrdering (int MOVE_ORDERING_PARAMETERS)
{
	sprintf (outFile, "%d tune_move_ordering %d\n", _cpt,MOVE_ORDERING_PARAMETERS);
	fflush(outFile);
	return waitResponse();
}

/* Function:  Echo the parameter
* Arguments: string
* Fails:     never
* Returns:   nothing
*/
int
QGtp::echo (QString param)
{
	sprintf (outFile, "%d echo %s\n", _cpt,(const char *)param);
	fflush(outFile);
	return waitResponse();
}

/* Function:  List all known commands
* Arguments: none
* Fails:     never
* Returns:   list of known commands, one per line
*/
int
QGtp::help ()
{
	sprintf (outFile, "%d help\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

/* Function:  evaluate wether a command is known
* Arguments: command word
* Fails:     never
* Returns:   true or false
*/
int
QGtp::knownCommand (QString s)
{
	sprintf (outFile, "%d known_command %s\n", _cpt,(const char *)s);
	fflush(outFile);
	return waitResponse();
}


/* Function:  Turn uncertainty reports from owl_attack
*            and owl_defend on or off.
* Arguments: "on" or "off"
* Fails:     invalid argument
* Returns:   nothing
*/
int
QGtp::reportUncertainty (QString s)
{
	sprintf (outFile, "%d report_uncertainty %s\n", _cpt,(const char *)s);
	fflush(outFile);
	return waitResponse();
}

/* Function:  List the stones of a worm
* Arguments: the location
* Fails:     if called on an empty or off-board location
* Returns:   list of stones
*/
int
QGtp::wormStones()
{
	sprintf (outFile, "%d worm_stones\n", _cpt);
	fflush(outFile);
	return waitResponse();
}

int
QGtp::shell(QString s)
{
	sprintf (outFile, "%d %s\n", _cpt, (const char *)s);
	fflush(outFile);
	return waitResponse();
}

