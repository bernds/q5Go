TEMPLATE	      = app
CONFIG		     += qt warn_on release thread
FORMS =			

#The following line was changed from INTERFACES to FORMS3 by qt3to4
FORMS3	      = gui_dialog.ui \
 clientwindow_gui.ui \
			gameinfo_gui.ui \
			newgame_gui.ui \
			newlocalgame_gui.ui \
			scoretools_gui.ui \
			normaltools_gui.ui \
			mainwidget_gui.ui \
			preferences_gui.ui \
			qnewgamedlg_gui.ui \
			talk_gui.ui \
			textedit_gui.ui \
			textview_gui.ui \
			nthmove_gui.ui \
			noderesults_gui.ui
HEADERS		      = config.h \
			gamedialog.h \
			gamestable.h \
			gametree.h \
			gs_globals.h \
			igsconnection.h \
			igsinterface.h \
			mainwin.h \
			misc.h \
			msg_handler.h \
			parser.h \
			playertable.h \
			qgo_interface.h \
			setting.h \
			tables.h \
			telnet.h \
			board.h \
			boardhandler.h \
			defines.h \
			globals.h \
			group.h \
			helpviewer.h \
			icons.h \
			imagehandler.h \
			interfacehandler.h \
			komispinbox.h \
			maintable.h \
			mainwidget.h \
			mainwindow.h \
			mark.h \
			matrix.h \
			move.h \
			noderesults.h \
			parserdefs.h \
			preferences.h \
			qgo.h \
			qgtp.h \
			qnewgamedlg.h \
			sgfparser.h \
			xmlparser.h \
			searchpath.h \
			stone.h \
			stonehandler.h \
			textview.h \
			tip.h \
			tree.h
unix:HEADERS	     +=	wavplay.h \
			wavfile.h
SOURCES		      = gamedialog.cpp \
			gamestable.cpp \
			gametree.cpp \
			igsconnection.cpp \
			main.cpp \
			mainwin.cpp \
			misc.cpp \
			msg_handler.cpp \
			parser.cpp \
			playertable.cpp \
			qgo_interface.cpp \
			setting.cpp \
			tables.cpp \
			telnet.cpp \
			board.cpp \
			boardhandler.cpp \
			group.cpp \
			helpviewer.cpp \
			imagehandler.cpp \
			interfacehandler.cpp \
			maintable.cpp \
			mainwidget.cpp \
			mainwindow.cpp \
			mark.cpp \
			matrix.cpp \
			move.cpp \
			noderesults.cpp \
			preferences.cpp \
			qgo.cpp \
			qgtp.cpp \
			qnewgamedlg.cpp \
			sgfparser.cpp \
			xmlparser.cpp \
			searchpath.cpp \
			stone.cpp \
			stonehandler.cpp \
			textview.cpp \
			tip.cpp \
			tree.cpp
unix:SOURCES	     +=	wavplay.c \
			wavfile.c
TARGET                = qGo
unix:INCLUDEPATH      += .
win32:INCLUDEPATH     += .
win32:QMAKE_CFLAGS   += -GX -Gf
win32:QMAKE_CXXFLAGS += -GX -Gf
DISTFILES            += *.dsw \
			pics/* \
			sounds/*.wav \
			wav*.c \
			wav*.h \
			*.rc \
			*.ts \
			qgo.pro
release:DEFINES      += NO_CHECK
win32:DEFINES        += QT_DLL QT_THREAD_SUPPORT HAVE_CONFIG_H
win32:RC_FILE	      = qgo.rc
OBJECTS_DIR	      = temp_
MOC_DIR		      = temp_
UI_DIR		    = temp_
TRANSLATIONS	      = qgo_cz.ts \
			qgo_de.ts \
			qgo_dk.ts \
			qgo_es.ts \
			qgo_fr.ts \
			qgo_it.ts \
			qgo_nl.ts \
			qgo_pl.ts\
			qgo_pt.ts\
			qgo_ru.ts\
			qgo_tr.ts\
			qgo_zh.ts \
			qgo_zh_cn.ts 
!win32 {
translation.path      = /usr/local/share/qgo/translations
translation.files     = translations/*
INSTALLS += translation

sounds.path           = /usr/local/share/qgo/sounds
sounds.files          = sounds/*
INSTALLS += sounds

documentation.path    = /usr/local/share/doc/qgo
documentation.files   = ../AUTHORS ../COPYING ../NEWS ../README ../TODO ../ChangeLog
INSTALLS += documentation

html.path             = /usr/local/share/doc/qgo/html
html.files            = ../html/*
INSTALLS += html

target.path = /usr/local/bin
INSTALLS += target
}
#The following line was inserted by qt3to4
QT += xml network  qt3support 
#The following line was inserted by qt3to4
CONFIG += uic3

