/*
* defines.h
*/

#ifndef DEFINES_H
#define DEFINES_H

/*
* Global defines
*/

#define DEFAULT_BOARD_SIZE 19
#define BOARD_X 500
#define BOARD_Y 500

// space
//#define SP QString(" ")
// new line
//#define NL "\n"
#define CONSOLECMDPREFIX "--->"

#define RTTI_STONE 1001
#define RTTI_MARK_SQUARE 1002
#define RTTI_MARK_CIRCLE 1003
#define RTTI_MARK_TRIANGLE 1004
#define RTTI_MARK_CROSS 1005
#define RTTI_MARK_TEXT 1006
#define RTTI_MARK_NUMBER 1007
#define RTTI_MARK_TERR 1008
#define RTTI_MARK_OTHERLINE 1009

#define SLIDER_INIT 0

#define WHITE_STONES_NB 8

/*
* Set available languages here. The Codes have to be in the same order as the language names
*/
#define NUMBER_OF_AVAILABLE_LANGUAGES 14
#define AVAILABLE_LANGUAGES { \
	"Chinese", \
	"Chinese (simplified)", \
	"Czech", \
	"Danish", \
	"Deutsch", \
	"English", \
	"Espanol",\
	"Français", \
	"Italiano", \
	"Latin (!)", \
	"Nederlands", \
	"Polish", \
	"Portuges", \
	"Russian", \
	"Türkçe"}
	
#define LANGUAGE_CODES { \
	"zh", \
	"zh_cn", \
	"cz", \
	"dk", \
	"de", \
	"en", \
	"es",\
	"fr", \
	"it", \
	"la", \
	"nl", \
	"pl", \
	"pt", \
	"ru", \
	"tr"}


/*
* Enum definitions
*/
enum StoneColor { stoneNone, stoneWhite, stoneBlack, stoneErase };
enum GameMode { modeNormal, modeEdit, modeScore, modeObserve, modeMatch, modeTeach, modeComputer };
enum MarkType { markNone, markSquare, markCircle, markTriangle, markCross, markText, markNumber, markTerrBlack, markTerrWhite };
enum skinType { skinLight, skinDark, skin3, skin4, skin5 };
enum VariationDisplay { vardisplayNone, vardisplayGhost, vardisplaySmallStone };
enum Codec { codecNone, codecBig5, codecEucJP, codecJIS, codecSJIS, codecEucKr, codecGBK, codecTscii };
enum assessType { noREQ, FREE, RATED, TEACHING };
enum tabType {tabNormalScore=0, tabEdit, tabTeachGameTree };
enum tabState {tabSet, tabEnable, tabDisable };
enum player_type {HUMAN=0,COMPUTER} ;

/*
* Global structs
*/
struct ASCII_Import
{
	char blackStone, whiteStone, starPoint, emptyPoint, hBorder, vBorder;
};

#include <iostream>
#include <fstream>

#ifdef DODEBUG
inline std::ostream &qDebug() { return std::cout; }
#else
inline std::ofstream &qDebug() { static std::ofstream fish("/dev/null"); return fish; }
#endif

#endif
