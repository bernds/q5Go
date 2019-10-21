TEMPLATE	      = app
CONFIG		     += qt warn_on release thread
FORMS	      = gui_dialog.ui \
		analyze_gui.ui \
		autodiags_gui.ui \
		boardwindow_gui.ui \
                clientwindow_gui.ui \
                dbdialog_gui.ui \
                enginedlg_gui.ui \
		figuredlg_gui.ui \
                gameinfo_gui.ui \
                greeterwindow_gui.ui \
		newgame_gui.ui \
		newlocalgame_gui.ui \
		newvariantgame_gui.ui \
		scoretools_gui.ui \
		normaltools_gui.ui \
		preferences_gui.ui \
		newaigamedlg_gui.ui \
		twoaigamedlg_gui.ui \
		sgfpreview.ui \
		talk_gui.ui \
		textedit_gui.ui \
		textview_gui.ui \
		slideview_gui.ui \
		svgview_gui.ui \
                nthmove_gui.ui

HEADERS		      = analyzedlg.h \
		        autodiagsdlg.h \
                        config.h \
                        clickableviews.h \
                        clockview.h \
                        dbdialog.h \
			evalgraph.h \
                        figuredlg.h \
                        gamedialog.h \
			gamestable.h \
                        gametree.h \
                        greeterwindow.h \
			gs_globals.h \
			igsconnection.h \
			clientwin.h \
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
			goboard.h \
			gogame.h \
			helpviewer.h \
			imagehandler.h \
			komispinbox.h \
                        mainwindow.h \
                        normaltools.h \
			preferences.h \
			qgo.h \
			qgtp.h \
			newaigamedlg.h \
                        sgf.h \
                        sgfpreview.h \
                        scoretools.h \
                        sizegraphicsview.h \
                        slideview.h \
                        svgbuilder.h \
                        textview.h \
                        timing.h \
                        ui_helpers.h \
                        grid.h \
                        miscdialogs.h \
                        variantgamedlg.h \
                        audio.h

SOURCES		      = analyzedlg.cpp \
			autodiagsdlg.cpp \
			clientwin.cpp \
                        clockview.cpp \
                        dbdialog.cpp \
			evalgraph.cpp \
			figuredlg.cpp \
			gamedialog.cpp \
			gamestable.cpp \
			gametree.cpp \
                        goboard.cc \
                        gogame.cc \
                        greeterwindow.cpp \
			igsconnection.cpp \
			main.cpp \
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
			mainwindow.cpp \
			preferences.cpp \
			qgo.cpp \
			qgtp.cpp \
			newaigamedlg.cpp \
			sgf2board.cc \
			sgfload.cc \
                        sgfpreview.cpp \
                        slideview.cpp \
			svgbuilder.cpp \
                        textview.cpp \
                        timing.cpp \
                        grid.cpp \
                        variantgamedlg.cpp \
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

macx :{
    LIBS += -framework CoreFoundation
}

!win32 {
translation.path      = $$DATADIR/translations
translation.files     = translations/*
INSTALLS += translation

documentation.path    = $$DOCDIR/
documentation.files   = ../AUTHORS ../COPYING ../NEWS ../README.md ../TODO ../ChangeLog
INSTALLS += documentation

system("pandoc --help >/dev/null 2>&1"): HAS_PANDOC = TRUE

equals (HAS_PANDOC, TRUE) {
readme.path            = $$DOCDIR/html/
readme.files		= readme.html
readme.commands		= pandoc -f markdown_github -o readme.html ../README.md
readme_images.path	= $$DOCDIR/html/screens/
readme_images.files	= ../screens/*.png ../screens/*.jpg
INSTALLS += readme readme_images
}

html.path             = $$DOCDIR/html/
html.files            = ../html/*.html ../html/images/*.png
INSTALLS += html

target.path           = $$PREFIX/bin
INSTALLS += target
}

QT += widgets gui xml network multimedia svg sql

RESOURCES += \
    q4go.qrc
