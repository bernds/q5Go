####### kdevelop will overwrite this part!!! (begin)##########

INCLUDES = $(all_includes)

q5go_METASOURCES = AUTO

KDE_OPTIONS = qtonly

DISTHEADERS_MOC = analyzedlg.h autodiagsdlg.h \
	bitarray.h board.h \
	clientwin.h clickableviews.h clockview.h \
	dbdialog.h \
	edit_analysis.h evalgraph.h \
	figuredlg.h \
	gamedb.h gamedialog.h gamestable.h gametree.h greeterwindow.h \
	helpviewer.h igsconnection.h \
	mainwindow.h miscdialogs.h msg_handler.h multisave.h \
	normaltools.h \
	parser.h patternsearch.h playertable.h preferences.h \
	qgo.h qgo_interface.h qgtp.h \
	scoretools.h sizegraphicsview.h slideview.h sgfpreview.h \
	tables.h telnet.h textview.h tutorial.h \
	variantgamedlg.h

DISTHEADERS_NOMOC = goboard.h config.h defines.h grid.h goboard.h gogame.h gotools.h \
	gs_globals.h imagehandler.h komispinbox.h misc.h newaigamedlg.h pattern.h \
	setting.h sgf.h \
	sgfparser.h svgbuilder.h timing.h ui_helpers.h

DISTSOURCES = analyzedlg.cpp autodiagsdlg.cpp \
	board.cpp \
	clientwin.cpp clockview.cpp \
	dbdialog.cpp \
	edit_analysis.cpp evalgraph.cpp \
	figuredlg.cpp \
	gamedb.cpp gamedialog.cpp gamestable.cpp gametree.cpp goboard.cc gogame.cc gotools.cc \
	grid.cpp greeterwindow.cpp \
	helpviewer.cpp \
	igsconnection.cpp imagehandler.cpp \
	main.cpp mainwindow.cpp misc.cpp msg_handler.cpp multisave.cpp \
	newaigamedlg.cpp \
	parser.cpp pattern.cpp patternsearch.cpp playertable.cpp preferences.cpp \
	qgo.cpp qgo_interface.cpp qgtp.cpp \
	setting.cpp sgfload.cc sgf2board.cc sgfpreview.cpp slideview.cpp svgbuilder.cpp \
	tables.cpp telnet.cpp textview.cpp timing.cpp tips.cpp tutorial.cpp \
	variantgamedlg.cpp

FORMS = analyze_gui.ui autodiags_gui.ui \
	boardwindow_gui.ui \
	clientwindow_gui.ui dbdialog_gui.ui \
	edit_analysis_gui.ui enginedlg_gui.ui \
	figuredlg_gui.ui \
	gameinfo_gui.ui gui_dialog.ui greeterwindow_gui.ui \
	multisave_gui.ui \
	newaigamedlg_gui.ui newgame_gui.ui newlocalgame_gui.ui newvariantgame_gui.ui \
	normaltools_gui.ui nthmove_gui.ui \
	patternsearch_gui.ui preferences_gui.ui \
	scoretools_gui.ui sgfpreview.ui slideshow_gui.ui slideview_gui.ui slideshow_gui.ui svgview_gui.ui \
	talk_gui.ui textedit_gui.ui textview_gui.ui tips_gui.ui twoaigamedlg_gui.ui

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
SUBDIRS = translations

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
