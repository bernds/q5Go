####### kdevelop will overwrite this part!!! (begin)##########

INCLUDES = $(all_includes)

q5go_METASOURCES = AUTO

KDE_OPTIONS = qtonly

DISTHEADERS_MOC = audio.h bitarray.h board.h clockview.h gamedialog.h gamestable.h gametree.h helpviewer.h icons.h \
	igsconnection.h mainwidget.h mainwindow.h clientwin.h miscdialogs.h \
	misctools.h normaltools.h scoretools.h parser.h playertable.h preferences.h qgo.h \
	qgo_interface.h qgtp.h newaigamedlg.h sizegraphicsview.h tables.h telnet.h textview.h tip.h

DISTHEADERS_NOMOC = goboard.h config.h defines.h gatter.h goboard.h gogame.h gs_globals.h \
	igsinterface.h imagehandler.h komispinbox.h mark.h \
	misc.h msg_handler.h setting.h sgf.h sgfparser.h stone.h \
	svgbuilder.h ui_helpers.h

DISTSOURCES = audio.cpp board.cpp goboard.cc gogame.cc gamedialog.cpp \
		gamestable.cpp gametree.cpp gatter.cpp helpviewer.cpp \
		igsconnection.cpp imagehandler.cpp main.cpp \
		mainwidget.cpp  clientwin.cpp mainwindow.cpp \
		misc.cpp msg_handler.cpp parser.cpp \
		playertable.cpp preferences.cpp qgo.cpp qgo_interface.cpp qgtp.cpp \
		newaigamedlg.cpp setting.cpp  sgfload.cc sgf2board.cc \
		stone.cpp svgbuilder.cpp tables.cpp telnet.cpp textview.cpp tip.cpp

FORMS = clientwindow_gui.ui gameinfo_gui.ui gui_dialog.ui mainwidget_gui.ui newgame_gui.ui \
	newlocalgame_gui.ui normaltools_gui.ui nthmove_gui.ui preferences_gui.ui \
	newaigamedlg_gui.ui scoretools_gui.ui svgview_gui.ui talk_gui.ui textedit_gui.ui textview_gui.ui

RESOURCES = q4go.qrc


FORMHEADERS = $(FORMS:%.ui=ui_%.h)
MOC_CC = $(FORMS:%.ui=ui_%.moc.cc) $(DISTHEADERS_MOC:.h=.moc.cc)
RCC_CC = $(RESOURCES:.qrc=.rcc.cc)

BUILT_SOURCES = $(FORMHEADERS) $(MOC_CC) $(RCC_CC)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(FORMS)

q5go_SOURCES = $(DISTSOURCES) $(DISTHEADERS_MOC) $(DISTHEADERS_NO_MOC)
nodist_q5go_SOURCES = $(MOC_CC) $(RCC_CC)

####### kdevelop will overwrite this part!!! (end)############
bin_PROGRAMS = q5go
SUBDIRS = pics translations sounds

q5go_LDFLAGS = $(all_libraries) $(USER_LDFLAGS)
q5go_LDADD = $(LIBSOCKET) $(LIB_QPE) $(LIB_QT) $(QT5_LIBS)

METASOURCES = AUTO

ui_%.h: %.ui
	$(UIC) -o $@ $<

%.moc.cc: %.h
	$(MOC) -o $@ $<

.qrc.rcc.cc:
	$(RCC) -o $@ $<

SUFFIXES = .h .ui .moc.cc .uic.cc .qrc .rcc.cc
