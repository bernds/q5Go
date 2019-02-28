####### kdevelop will overwrite this part!!! (begin)##########

INCLUDES = $(all_includes)

q5go_METASOURCES = AUTO

KDE_OPTIONS = qtonly

DISTHEADERS_MOC = analyzedlg.h audio.h autodiagsdlg.h bitarray.h board.h clickableviews.h \
	clockview.h dbdialog.h evalgraph.h figuredlg.h gamedialog.h gamestable.h gametree.h helpviewer.h icons.h \
	igsconnection.h mainwindow.h clientwin.h miscdialogs.h msg_handler.h normaltools.h scoretools.h \
	parser.h playertable.h preferences.h qgo.h qgo_interface.h qgtp.h newaigamedlg.h sizegraphicsview.h \
	sgfpreview.h tables.h telnet.h textview.h variantgamedlg.h

DISTHEADERS_NOMOC = goboard.h config.h defines.h grid.h goboard.h gogame.h gs_globals.h \
	igsinterface.h imagehandler.h komispinbox.h mark.h \
	misc.h setting.h sgf.h sgfparser.h \
	svgbuilder.h ui_helpers.h

DISTSOURCES = analyzedlg.cpp audio.cpp autodiagsdlg.cpp board.cpp clockview.cpp dbdialog.cpp evalgraph.cpp \
		figuredlg.cpp goboard.cc gogame.cc gamedialog.cpp gamestable.cpp gametree.cpp grid.cpp \
		helpviewer.cpp igsconnection.cpp imagehandler.cpp main.cpp \
		clientwin.cpp mainwindow.cpp \
		misc.cpp msg_handler.cpp parser.cpp \
		playertable.cpp preferences.cpp qgo.cpp qgo_interface.cpp qgtp.cpp \
		newaigamedlg.cpp setting.cpp  sgfload.cc sgf2board.cc sgfpreview.cpp \
		svgbuilder.cpp tables.cpp telnet.cpp textview.cpp variantgamedlg.cpp

FORMS = analyze_gui.ui autodiags_gui.ui boardwindow_gui.ui clientwindow_gui.ui dbdialog_gui.ui figuredlg_gui.ui \
	gameinfo_gui.ui gui_dialog.ui newgame_gui.ui newlocalgame_gui.ui newvariantgame_gui.ui \
	normaltools_gui.ui nthmove_gui.ui preferences_gui.ui newaigamedlg_gui.ui scoretools_gui.ui \
	svgview_gui.ui sgfpreview.ui talk_gui.ui textedit_gui.ui textview_gui.ui

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
SUBDIRS = pics translations

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
