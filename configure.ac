dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

dnl Name your plug-in here
m4_define([plugin_name], [gimp-lqr-plugin])

dnl These three define the plug-in version number
m4_define([plugin_major_version], [0])
m4_define([plugin_minor_version], [7])
m4_define([plugin_micro_version], [2])

m4_define([plugin_version],
          [plugin_major_version.plugin_minor_version.plugin_micro_version])

AC_INIT([plugin_name], [plugin_version])

AC_DEFINE(PLUGIN_NAME, PACKAGE_NAME, [Plug-In name])
AC_DEFINE(PLUGIN_VERSION, PACKAGE_VERSION, [Plug-In version])

AC_DEFINE(PLUGIN_MAJOR_VERSION, plugin_major_version, [Plug-In major version])
AC_DEFINE(PLUGIN_MINOR_VERSION, plugin_minor_version, [Plug-In minor version])
AC_DEFINE(PLUGIN_MICRO_VERSION, plugin_micro_version, [Plug-In micro version])

AC_CONFIG_SRCDIR([src/main.c])
dnl AC_CONFIG_SRCDIR([gap/plug_in_lqr_iter.c])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(no-define)

dnl AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC


ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AM_MAINTAINER_MODE


dnl Use -Wall if we have gcc.
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl


GIMP_REQUIRED_VERSION=2.8.0

PKG_CHECK_MODULES(GIMP,
  gimp-2.0 >= $GIMP_REQUIRED_VERSION gimpui-2.0 >= $GIMP_REQUIRED_VERSION)

AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)

GIMP_LIBDIR=`$PKG_CONFIG --variable=gimplibdir gimp-2.0`
AC_SUBST(GIMP_LIBDIR)

GIMP_DATADIR=`$PKG_CONFIG --variable=gimpdatadir gimp-2.0`
AC_SUBST(GIMP_DATADIR)


LQR_REQUIRED_VERSION=0.4.0

PKG_CHECK_MODULES(LQR,
		  lqr-1 >= $LQR_REQUIRED_VERSION)

dnl LQR_CFLAGS=`$PKG_CONFIG --cflags lqr-1`
AC_SUBST(LQR_CFLAGS)

dnl LQR_LIBS=`$PKG_CONFIG --libs lqr-1`
AC_SUBST(LQR_LIBS)

LQR_LIBDIR=`$PKG_CONFIG --variable=libdir lqr-1`
AC_SUBST(LQR_LIBDIR)



dnl i18n stuff

GETTEXT_PACKAGE=gimp20-lqr-plugin
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
				    [The gettext translation domain.])

dnl Add the languages which your application supports here.
ALL_LINGUAS="it fr ru pt_BR de eu_ES zh_CN es_AR hu ro_RO sr es_ES nb_NO zh_TW"

AC_PROG_INTLTOOL
AM_GLIB_GNU_GETTEXT

AC_CHECK_FUNCS(bind_textdomain_codeset)

LOCALEDIR="${datadir}/locale"
if test "$DATADIRNAME" = "lib"; then
   LOCALEDIR="$libdir/locale"
fi
AC_SUBST(LOCALEDIR)

PLUGIN_DATADIR="${datadir}/plugin_name"
AC_SUBST(PLUGIN_DATADIR)

SCRIPTSDIR="${GIMP_DATADIR}/scripts"
AC_SUBST(SCRIPTSDIR)

AC_MSG_CHECKING([if GTK+ is version 2.12.0 or newer])
if $PKG_CONFIG --atleast-version=2.12.0 gtk+-2.0; then
  have_gtk_2_12=yes
else
  have_gtk_2_12=no
fi
AC_MSG_RESULT($have_gtk_2_12)

if test "x$have_gtk_2_12" != "xyes"; then
  CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi


AC_MSG_CHECKING([if GIMP is version 2.8.0 or newer])
if $PKG_CONFIG --atleast-version=2.8.0 gimp-2.0; then
  have_gimp_2_8=yes
else
  have_gimp_2_8=no
fi
AC_MSG_RESULT($have_gimp_2_8)

if test "x$have_gimp_2_8" != "xyes"; then
  CPPFLAGS="$CPPFLAGS -DGIMP_DISABLE_DEPRECATED"
fi

AC_MSG_CHECKING([if we are on Win32])
AC_LANG([C])
AC_PREPROC_IFELSE([[
		   #ifndef WIN32
		   #error
		   #endif
		   ]], is_win32=yes, is_win32=no)
if test "x$is_win32" = "xyes"; then
  AC_MSG_RESULT([yes])
  CFLAGS="$CFLAGS -s"
  LDFLAGS="$LDFLAGS -mwindows"
else
  AC_MSG_RESULT([no])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
gap/Makefile
batch/Makefile
po/Makefile.in
help/Makefile
help/en/Makefile
help/en/images/Makefile
help/it/Makefile
help/it/images/Makefile
help/ru/Makefile
help/ru/images/Makefile
help/css/Makefile
])

AC_OUTPUT
