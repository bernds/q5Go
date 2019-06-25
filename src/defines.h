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

#define SLIDER_INIT 0

#define WHITE_STONES_NB 8

/*
* Set available languages here. The Codes have to be in the same order as the language names
*/
#define NUMBER_OF_AVAILABLE_LANGUAGES 15
#define AVAILABLE_LANGUAGES { \
	"Chinese", \
	"Chinese (simplified)", \
	"Czech", \
	"Danish", \
	"Deutsch", \
	"English", \
	"Espanol",\
	u8"Fran\u00e7ais", \
	"Italiano", \
	"Latin (!)", \
	"Nederlands", \
	"Polish", \
	"Portuges", \
	"Russian", \
	u8"T\u00fcrk\u00e7e"}

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


enum GameMode { modeNormal, modeEdit, modeScore, modeScoreRemote, modeObserve, modeObserveGTP,
		modeMatch, modeTeach, modeComputer, modeBatch };
enum assessType { noREQ, FREE, RATED, TEACHING };
enum tabType {tabNormalScore=0, tabTeachGameTree };
enum tabState {tabSet, tabEnable, tabDisable };

#define CHECK_PTR(x) do {} while (0)
#define ASSERT(x)  do {} while (0)

#include <QDebug>

#endif
