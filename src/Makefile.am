## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gimp-lqr-plugin

bindir = $(GIMP_LIBDIR)/plug-ins

gimp_lqr_plugin_SOURCES = \
	plugin-intl.h    \
	main_common.h    \
	main.c	         \
	main.h	         \
	interface.c      \
	interface.h      \
	interface_I.c    \
	interface_I.h    \
	preview.c        \
	preview.h        \
	layers_combo.c   \
	layers_combo.h   \
	render.c         \
	render.h         \
	io_functions.c   \
	io_functions.h   \
	altcoordinates.c \
	altcoordinates.h \
	altsizeentry.c   \
	altsizeentry.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DPLUGIN_DATADIR=\""$(PLUGIN_DATADIR)"\"\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	@LQR_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS) $(LQR_LIBS)

