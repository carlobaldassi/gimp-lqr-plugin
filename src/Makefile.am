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
	io_functions.c  \
	io_function.h   \
	lqr.h		\
	lqr_base.h      \
	lqr_gradient.c  \
	lqr_gradient.h  \
	lqr_cursor.c    \
	lqr_cursor.h    \
	lqr_raster.c    \
	lqr_raster.h    \
	lqr_seams_buffer.c \
	lqr_seams_buffer.h \
	lqr_seams_buffer_list.c \
	lqr_seams_buffer_list.h \
	lqr_colour.c    \
	lqr_colour.h	\
	lqr_progress.c  \
	lqr_progress.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

