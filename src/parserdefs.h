/*
 * parserdefs.h
 */

#ifndef PARSERDEFS_H
#define PARSERDEFS_H

enum State { stateVarBegin, stateNode, stateVarEnd };
enum Property { moveBlack, moveWhite, editBlack, editWhite, editErase, comment, editMark, unknownProp, nodeName, timeLeft, openMoves, nextMove};
enum TimeSystem { time_none, absolute, byoyomi, canadian };
struct Position { int x, y; };
struct MoveNum {int n; };

#endif
