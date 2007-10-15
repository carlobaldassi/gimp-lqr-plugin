## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gimp-lqr-plugin

bindir = $(GIMP_LIBDIR)/plug-ins

gimp_lqr_plugin_SOURCES = \
	plugin-intl.h	\
	interface.c	\
	interface.h	\
	main.c		\
	main.h		\
	render.c	\
	render.h	\
	lqr.h		\
	lqr_gradient.c  \
	lqr_gradient.h  \
	lqr_data.c      \
	lqr_data.h      \
	lqr_cursor.c    \
	lqr_cursor.h    \
	lqr_raster.c    \
	lqr_raster.h    \
	lqr_external.c  \
	lqr_external.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

