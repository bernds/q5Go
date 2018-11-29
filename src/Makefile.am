####### kdevelop will overwrite this part!!! (begin)##########

INCLUDES = $(all_includes)

q4go_METASOURCES = AUTO

KDE_OPTIONS = qtonly

DISTHEADERS_MOC = audio.h board.h gamedialog.h gamestable.h gametree.h helpviewer.h icons.h \
	igsconnection.h mainwidget.h mainwindow.h mainwin.h miscdialogs.h \
	misctools.h noderesults.h parser.h playertable.h preferences.h qgo.h \
	qgo_interface.h qgtp.h qnewgamedlg.h searchpath.h tables.h telnet.h textview.h tip.h

DISTHEADERS_NOMOC = boardhandler.h config.h defines.h gatter.h globals.h group.h gs_globals.h \
	igsinterface.h imagehandler.h interfacehandler.h komispinbox.h mark.h \
	matrix.h misc.h move.h msg_handler.h parserdefs.h setting.h sgfparser.h stone.h \
	stonehandler.h tree.h

DISTSOURCES = audio.cpp alsa.cpp board.cpp boardhandler.cpp gamedialog.cpp \
		gamestable.cpp gametree.cpp gatter.cpp group.cpp helpviewer.cpp \
		igsconnection.cpp imagehandler.cpp interfacehandler.cpp main.cpp \
		mainwidget.cpp  mainwin.cpp mainwindow.cpp mark.cpp matrix.cpp \
		misc.cpp move.cpp msg_handler.cpp noderesults.cpp parser.cpp \
		playertable.cpp preferences.cpp qgo.cpp qgo_interface.cpp qgtp.cpp \
		qnewgamedlg.cpp setting.cpp sgfparser.cpp stone.cpp stonehandler.cpp tables.cpp \
		telnet.cpp textview.cpp tip.cpp tree.cpp searchpath.cpp

FORMS = clientwindow_gui.ui gameinfo_gui.ui gui_dialog.ui mainwidget_gui.ui newgame_gui.ui \
	newlocalgame_gui.ui noderesults_gui.ui normaltools_gui.ui nthmove_gui.ui preferences_gui.ui \
	qnewgamedlg_gui.ui scoretools_gui.ui talk_gui.ui textedit_gui.ui textview_gui.ui 

RESOURCES = q4go.qrc


FORMHEADERS = $(FORMS:.ui=.h)
MOC_CC = $(FORMS:.ui=.moc.cc) $(DISTHEADERS_MOC:.h=.moc.cc)
RCC_CC = $(RESOURCES:.qrc=.rcc.cc)

BUILT_SOURCES = $(FORMHEADERS) $(MOC_CC) $(RCC_CC)
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(FORMS)

q4go_SOURCES = $(DISTSOURCES) $(DISTHEADERS_MOC) $(DISTHEADERS_NO_MOC)
nodist_q4go_SOURCES = $(MOC_CC) $(RCC_CC)

####### kdevelop will overwrite this part!!! (end)############
bin_PROGRAMS = q4go
SUBDIRS = pics translations sounds

q4go_LDFLAGS = $(all_libraries) $(USER_LDFLAGS)
q4go_LDADD = $(LIBSOCKET) $(LIB_QPE) $(LIB_QT) $(QT4_LIBS)

METASOURCES = AUTO

.ui.h:
	$(UIC) -o $@ $<

.h.moc.cc:
	$(MOC) -o $@ $<

.qrc.rcc.cc:
	$(RCC) -o $@ $<

SUFFIXES = .h .ui .moc.cc .uic.cc .qrc .rcc.cc
