####### kdevelop will overwrite this part!!! (begin)##########

INCLUDES = $(all_includes)

qgo_METASOURCES = AUTO

KDE_OPTIONS = qtonly



EXTRA_DIST = board.cpp board.h boardhandler.cpp boardhandler.h compiling.txt config.h defines.h gamedialog.cpp gamedialog.h gameinfo_gui.ui gamestable.cpp gamestable.h gametree.cpp gametree.h globals.h group.cpp group.h gs_globals.h gui_dialog.ui helpviewer.cpp helpviewer.h icons.h igsconnection.cpp igsconnection.h igsinterface.h imagehandler.cpp imagehandler.h interfacehandler.cpp interfacehandler.h komispinbox.h main.cpp main.cpp.bak main.pro maintable.cpp maintable.h main_trans.pro mainwidget.cpp mainwidget_gui.ui mainwidget.h mainwin.cpp mainwindow.cpp mainwindow.h mainwin.h mark.cpp mark.h matrix.cpp matrix.h misc.cpp misc.h move.cpp move.h msg_handler.cpp msg_handler.h newgame_gui.ui newlocalgame_gui.ui noderesults.cpp noderesults_gui.ui noderesults.h normaltools_gui.ui nthmove_gui.ui parser.cpp parserdefs.h parser.h playertable.cpp playertable.h preferences.cpp preferences_gui.ui preferences.h qgo23.dsw qgo.cpp qgo.dsw qgo.h qgo_interface.cpp qgo_interface.h qgo.pro qgo.rc scoretools_gui.ui setting.cpp setting.h sgfparser.cpp sgfparser.h startupframe.cpp startupframe_gui.ui startupframe.h stone.cpp stone.h stonehandler.cpp stonehandler.h tables.cpp tables.h telnet.cpp telnet.h textedit_gui.ui textview.cpp textview_gui.ui textview.h tip.cpp tip.h tree.cpp tree.h wavfile.c wavfile.h wavplay.c wavplay.h xmlparser.cpp xmlparser.h qnewgamedlg.cpp qnewgamedlg.h qgtp.cpp qgtp.h qnewgamedlg_gui.ui clientwindow_gui.ui talk_gui.ui 

####### kdevelop will overwrite this part!!! (end)############
bin_PROGRAMS = qgo
qgo_SOURCES = board.cpp boardhandler.cpp clientwindow_gui.ui gamedialog.cpp \
		gameinfo_gui.ui gamestable.cpp gametree.cpp group.cpp gui_dialog.ui helpviewer.cpp \
		igsconnection.cpp imagehandler.cpp interfacehandler.cpp main.cpp maintable.cpp \
		mainwidget.cpp mainwidget_gui.ui mainwin.cpp mainwindow.cpp mark.cpp matrix.cpp \
		misc.cpp move.cpp msg_handler.cpp newgame_gui.ui newlocalgame_gui.ui \
		noderesults.cpp noderesults_gui.ui normaltools_gui.ui nthmove_gui.ui parser.cpp \
		playertable.cpp preferences.cpp preferences_gui.ui qgo.cpp qgo_interface.cpp qgtp.cpp \
		qnewgamedlg.cpp qnewgamedlg_gui.ui scoretools_gui.ui setting.cpp sgfparser.cpp \
		stone.cpp stonehandler.cpp tables.cpp talk_gui.ui telnet.cpp textedit_gui.ui \
		textview.cpp textview_gui.ui tip.cpp tree.cpp wavfile.c wavplay.c xmlparser.cpp \
	searchpath.cpp
SUBDIRS = pics translations sounds


qgo_LDFLAGS = $(all_libraries) $(USER_LDFLAGS)
qgo_LDADD = $(LIBSOCKET) $(LIB_QPE) $(LIB_QT)

METASOURCES = AUTO
noinst_HEADERS = board.h boardhandler.h config.h defines.h gamedialog.h \
	gamestable.h gametree.h globals.h group.h gs_globals.h helpviewer.h icons.h \
	igsconnection.h igsinterface.h imagehandler.h interfacehandler.h komispinbox.h \
	maintable.h mainwidget.h mainwindow.h mainwin.h mark.h matrix.h misc.h move.h \
	msg_handler.h noderesults.h parserdefs.h parser.h playertable.h preferences.h qgo.h \
	qgo_interface.h qgtp.h qnewgamedlg.h setting.h sgfparser.h stone.h stonehandler.h \
	tables.h telnet.h textview.h tip.h tree.h wavfile.h wavplay.h xmlparser.h
