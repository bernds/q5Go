####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = sgf.desktop qgo.desktop 

install-data-local:
	$(mkinstalldirs) $(datadir)/mimelnk/text/
	$(INSTALL_DATA) $(srcdir)/sgf.desktop $(datadir)/mimelnk/text/
	$(mkinstalldirs) $(datadir)/applnk/Games/
	$(INSTALL_DATA) $(srcdir)/qgo.desktop $(datadir)/applnk/Games/

uninstall-local:
	-rm -f $(datadir)/mimelnk/text/
	-rm -f $(datadir)/applnk/Games/

####### kdevelop will overwrite this part!!! (end)############
Applinkdir = $(datadir)/applnk/Games/
Mimelinkdir = $(datadir)/mimelnk/text/
Mimelink_DATA = sgf.desktop
Applink_DATA = qgo.desktop
