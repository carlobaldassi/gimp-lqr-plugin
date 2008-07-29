## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gimp-lqr-plugin

bindir = $(GIMP_LIBDIR)/plug-ins

gimp_lqr_plugin_SOURCES = \
	plugin-intl.h	\
	interface.c	\
	interface.h	\
	main.c		\
	main.h		\
	main_common.h	\
	render.c	\
	render.h	\
	io_functions.c  \
	io_functions.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	@LQR_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS) $(LQR_LIBS)

