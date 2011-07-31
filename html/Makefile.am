## Process this file with automake to produce Makefile.in

EXTRA_DIST = *.xml *.xsl *.html
SUBDIRS = images

htmldir = $(datadir)/doc/$(PACKAGE)/html
html_DATA = *.html

*.xml:
*.xsl:
*.html:

help:
	java -cp /usr/share/java/saxon.jar com.icl.saxon.StyleSheet help.xml help.xsl

htmlhelp:
	java -cp /usr/share/java/saxon.jar com.icl.saxon.StyleSheet help.xml htmlhelp.xsl

htmlclean:
	rm -f *.html *.chm *.hhc *.hhp
