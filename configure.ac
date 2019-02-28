# Original Author was Kalle@kde.org
# I lifted it in some mater. (Stephan Kulow)
# I used much code from Janos Farkas

dnl Process this file with autoconf to produce a configure script.

AC_INIT(src/goboard.h)

dnl AC_INIT(acinclude.m4) dnl a source file from your sub dir

dnl This is so we can use kde-common
AC_CONFIG_AUX_DIR(admin)

AC_PROG_CXX
AC_OBJEXT
AC_EXEEXT

AC_CANONICAL_SYSTEM 

dnl Automake doc recommends to do this only here. (Janos)
AM_INIT_AUTOMAKE(q5go,0.3)

AC_PREFIX_DEFAULT(/usr/local)
if test "x$prefix" = "xNONE"; then
  prefix=$ac_default_prefix
  ac_configure_args="$ac_configure_args --prefix $prefix"
fi

dnl generate the config header
AM_CONFIG_HEADER(config.h) dnl at the distribution this done

dnl Checks for programs.
AC_PROG_CXX

CXXFLAGS="$NOOPT_CXXFLAGS" dnl __kdevelop[noopt]__
CFLAGS="$NOOPT_CFLAGS" dnl __kdevelop[noopt]__
dnl CXXFLAGS="$CXXFLAGS $USE_EXCEPTIONS" dnl __kdevelop[exc]__

all_libraries="$all_libraries $USER_LDFLAGS"
all_includes="$all_includes $USER_INCLUDES"
AC_SUBST(all_includes)
AC_SUBST(all_libraries)

dnl Look for Qt4 libs
PKG_CHECK_MODULES([QT5], ["Qt5Core Qt5Widgets Qt5Gui Qt5Xml Qt5Multimedia Qt5Svg Qt5Sql"], [], [
	AC_MSG_ERROR([Qt5 libs not found])])
dnl Look for moc, either on $PATH or in $QTDIR/bin
AC_PATH_PROG(MOC, moc, , $PATH:$QTDIR/bin)
if test -z "$MOC" ; then
    AC_MSG_ERROR([Cannot find 'moc'])
fi
AC_PATH_PROG(UIC, uic, , $PATH:$QTDIR/bin)
if test -z "$UIC" ; then
    AC_MSG_ERROR([Cannot find 'uic'])
fi
AC_PATH_PROG(RCC, rcc, , $PATH:$QTDIR/bin)
if test -z "$RCC" ; then
    AC_MSG_ERROR([Cannot find 'rcc'])
fi

AC_PATH_PROG(qtchooser, qtchooser, , $PATH:$QTDIR/bin)
if test -z "QTCHOOSER" ; then
    AC_MSG_ERROR([Cannot find 'qtchooser'])
else
    MOC='qtchooser -run-tool=moc -qt=qt5'
    UIC='qtchooser -run-tool=uic -qt=qt5'
    RCC='qtchooser -run-tool=rcc -qt=qt5'
fi

CXXFLAGS="$CXXFLAGS $QT5_CFLAGS -fPIC"

AC_SUBST(QT5_LIBS)

dnl add here all your Makefiles. This are created by configure
AC_CONFIG_FILES([ Makefile ])

dnl AC_CONFIG_FILES([ po/Makefile ])
AC_CONFIG_FILES([ src/Makefile ])
AC_CONFIG_FILES([ src/pics/Makefile ])
AC_CONFIG_FILES([ src/translations/Makefile ])
AC_CONFIG_FILES([ templates/Makefile ])

dnl CPPFLAGS="$CPPFLAGS -DDATADIR=\"$(datadir)\""
CXXFLAGS="-DDATADIR='\"${prefix}/share/${PACKAGE}\"' $CXXFLAGS"
CXXFLAGS="-DDOCDIR='\"${prefix}/share/doc/${PACKAGE}\"' $CXXFLAGS"

AC_OUTPUT()
