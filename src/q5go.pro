TEMPLATE	      = app
CONFIG		     += qt warn_on release thread
FORMS	      = gui_dialog.ui \
		clientwindow_gui.ui \
		gameinfo_gui.ui \
		newgame_gui.ui \
		newlocalgame_gui.ui \
		scoretools_gui.ui \
		normaltools_gui.ui \
		mainwidget_gui.ui \
		preferences_gui.ui \
		newaigamedlg_gui.ui \
		talk_gui.ui \
		textedit_gui.ui \
		textview_gui.ui \
		svgview_gui.ui \
		nthmove_gui.ui

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
			defines.h \
			globals.h \
			goboard.h \
			helpviewer.h \
			icons.h \
			imagehandler.h \
			komispinbox.h \
			mainwidget.h \
                        mainwindow.h \
                        normaltools.h \
			parserdefs.h \
			preferences.h \
			qgo.h \
			qgtp.h \
			newaigamedlg.h \
                        sgf.h \
                        scoretools.h \
                        stone.h \
                        svgbuilder.h \
			textview.h \
			tip.h \
                        gatter.h \
                        misctools.h \
                        miscdialogs.h \
                        audio.h

SOURCES		      = gamedialog.cpp \
			gamestable.cpp \
			gametree.cpp \
			goboard.cc \
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
			helpviewer.cpp \
			imagehandler.cpp \
			mainwidget.cpp \
			mainwindow.cpp \
			preferences.cpp \
			qgo.cpp \
			qgtp.cpp \
			newaigamedlg.cpp \
			sgf2board.cc \
			sgfload.cc \
                        stone.cpp \
                        svgbuilder.cpp \
			textview.cpp \
			tip.cpp \
                        gatter.cpp \
                        audio.cpp

isEmpty(PREFIX) {
PREFIX = /usr/local
}

TARGET                = q5go
DATADIR               = $$PREFIX/share/q5go
DOCDIR                = $$PREFIX/share/doc/q5go

unix:INCLUDEPATH      += .
win32:INCLUDEPATH     += .
#win32:QMAKE_CFLAGS   += -GX -Gf
#win32:QMAKE_CXXFLAGS += -GX -Gf

DISTFILES            += *.dsw \
			pics/* \
			sounds/*.wav \
			wav*.c \
			wav*.h \
			*.rc \
			*.ts \
                        qgo.pro
!win32:DEFINES       += "DATADIR=\\\"$$DATADIR\\\""
!win32:DEFINES       += "DOCDIR=\\\"$$DOCDIR\\\""
release:DEFINES      += NO_CHECK
win32:DEFINES        += QT_DLL QT_THREAD_SUPPORT HAVE_CONFIG_H _USE_MATH_DEFINES
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
translation.path      = $$DATADIR/translations
translation.files     = translations/*
INSTALLS += translation

sounds.path           = $$DATADIR/sounds
sounds.files          = sounds/*
INSTALLS += sounds

documentation.path    = $$DOCDIR/
documentation.files   = ../AUTHORS ../COPYING ../NEWS ../README ../TODO ../ChangeLog
INSTALLS += documentation

html.path             = $$DOCDIR/html/
html.files            = ../html/*.html ../html/images/*.png
INSTALLS += html

target.path           = $$PREFIX/bin
INSTALLS += target
}

QT += widgets gui xml network multimedia svg

RESOURCES += \
    q4go.qrc
